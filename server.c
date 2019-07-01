//REMEMBER TO USE -lm WHILE COMPILING!!!

#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <endian.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <time.h> 
#include <signal.h> 
#include <errno.h> 
#include <arpa/inet.h>
#include <stdbool.h>
#include <math.h>

union uint64_t_double {
    uint64_t u;
    double d;
};

struct __attribute__((__packed__)) opcode_struct{
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
};

union opcode{
	uint32_t uint32code;
	struct opcode_struct code;
};

struct __attribute__((__packed__)) request{
	//4
	union opcode opc;
	//4
	uint32_t id;
};

void handle_request(int);
char *get_time();
uint64_t double_to_uint64_t(double);
double uint64_t_to_double(int64_t);
ssize_t writeWrapper(int, void *, size_t);
ssize_t readWrapper(int, void *, size_t);

int main() 
{ 
//////////////////////////////////////////////////////////////////////////////////////////////
//CONNECTION

	//uint8_t request_id = 0;
	//sockfd - socket file descriptor
	bool server_online = true;
	int server_sockfd;
	int client_sockfd;
	socklen_t server_len;
	socklen_t client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	
	server_len = sizeof(server_address);
	client_len = sizeof(client_address);

	server_sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (server_sockfd == -1) 
	{ 
        	printf("socket creation failed...\n"); 
        	exit(0); 
    	} 

	bzero(&server_address, server_len); //erases data in server_address

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(9734);
	
  	if ((bind(server_sockfd, (struct sockaddr *) &server_address, server_len)) != 0) 
	{ 
        	printf("socket bind failed...\n"); 
        	exit(0); 
    	} 
	
	if ((listen(server_sockfd, 5)) != 0) 
	{ 
        	printf("Listen failed...\n"); 
        	exit(0); 
    	} 
    	else
        	printf("Server listening..\n"); 


	//child processes of the calling processes shall not be transformed into zombie processes when they terminate
//	signal (SIGCHLD, SIG_IGN); 

	while(server_online)
	{
		client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
		if(client_sockfd<0){
			printf("Server accept failed...\n");
			exit(0);

		} else { printf("<-----------Client accepted----------->\n\n"); }
//////////////////////////////////////////////////////////////////////////////////////////////
//COMMUNICATION
		/*if(fork() ==0)
		{	
			handle_request(client_sockfd); 
		}
		*/
		
		//ignores signal SIGPIPE
		//SIGPIPE is sent to a process if it tried to write to a socket that had been shutdown for writing or isn't connected [anymore].
		signal(SIGPIPE, SIG_IGN);
		handle_request(client_sockfd); 
		handle_request(client_sockfd);  

		close(client_sockfd);
	}
}

void handle_request(int sockfd)
{
	void *buffer;
	uint64_t value;
	char *current_time;
	uint32_t date_sizeof;
	uint32_t date_length;
	struct request REQUEST;
		
	readWrapper(sockfd, &REQUEST, sizeof(struct request));
	REQUEST.opc.uint32code = be32toh(REQUEST.opc.uint32code);
	REQUEST.id = be32toh(REQUEST.id);
	/*debug*/printf("Request received: OPCODE: 0x%08x; ID: %d\n", REQUEST.opc.uint32code, REQUEST.id);
		
	switch(REQUEST.opc.uint32code){
		case 0x00000001:
		//////////////////////////////////////////ROOT request
			readWrapper(sockfd, &value, sizeof(uint64_t));
			REQUEST.opc.uint32code = htobe32(0x01000001);
			REQUEST.id = htobe32(REQUEST.id);
			value = htobe64(double_to_uint64_t(sqrt(uint64_t_to_double(be64toh(value)))));

			writeWrapper(sockfd, &REQUEST, sizeof(REQUEST));
			writeWrapper(sockfd, &value, sizeof(uint64_t));
		//////////////////////////////////////////////////////
			break;
				
		case 0x00000002:
		//////////////////////////////////////////TIME request
			REQUEST.opc.uint32code = htobe32(0x01000002);
			REQUEST.id = htobe32(REQUEST.id);
			current_time = get_time();
			date_length = strlen(current_time);
			date_sizeof = htobe32(date_length);

			writeWrapper(sockfd, &REQUEST, sizeof(REQUEST));
			writeWrapper(sockfd, &date_sizeof, sizeof(uint32_t));
			writeWrapper(sockfd, current_time, sizeof(char)*date_length);
			/*--------*/
		//////////////////////////////////////////////////////
			break;
				
		default:
			printf("ERROR-HANDLE_REQUEST-SWITCH-DEFAULT\n");
			exit(1);
		}
	printf("\n\n");
}

char *get_time()
{
	char *buff;
	time_t curtime;
	time(&curtime);
	return ctime(&curtime);

}

uint64_t double_to_uint64_t(double d) {
    union uint64_t_double ud;
    ud.d = d;
    return ud.u;
}

double uint64_t_to_double(int64_t u) {
    union uint64_t_double ud;
    ud.u = u;
    return ud.d;
}

ssize_t writeWrapper(int sockfd, void *buffer, size_t estimated_size)
{
	ssize_t total_bytes_write = 0;
	
	size_t amount_of_bytes_yet_to_write = estimated_size;
	ssize_t bytes_written = 0;
	char * request_buffer = (char *)buffer;
	while(amount_of_bytes_yet_to_write > 0){

		bytes_written = write(sockfd, request_buffer, amount_of_bytes_yet_to_write);

		if(bytes_written > 0){

			total_bytes_write += bytes_written;		//how many bytes are already written
			amount_of_bytes_yet_to_write -= bytes_written;	//how many bytes left to write
			request_buffer += bytes_written;		//move pointer
		} else if (bytes_written == -1 && errno == EINTR) {
			perror(strerror(errno));
			close(sockfd);
			exit(1);
		} else if (bytes_written == 0 ) {
			close(sockfd);
			exit(1);
		}
	}
	return total_bytes_write;
}

ssize_t readWrapper(int sockfd, void *buffer, size_t estimated_size)
{
	ssize_t total_bytes_read = 0;
	
	size_t amount_of_bytes_yet_to_read = estimated_size;
	ssize_t bytes_read = 0;
	char * request_buffer = (char *)buffer;

	while(amount_of_bytes_yet_to_read > 0){

		bytes_read = read(sockfd, request_buffer, amount_of_bytes_yet_to_read);

		if(bytes_read> 0){

			total_bytes_read += bytes_read;			//how many bytes are already written
			amount_of_bytes_yet_to_read -= bytes_read;	//how many bytes left to write
			request_buffer += bytes_read;			//move pointer
		} else if (bytes_read == -1 && errno == EINTR) {
			perror(strerror(errno));
			close(sockfd);
			exit(1);
		} else if (bytes_read == 0 ) {
			close(sockfd);
			exit(1);
		}
	}
	return total_bytes_read;
}
