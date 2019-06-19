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

		} else { printf("Client accepted\n"); }
//////////////////////////////////////////////////////////////////////////////////////////////
//COMMUNICATION
		/*if(fork() ==0)
		{	
			handle_request(client_sockfd); 
		}
		*/
		 
		handle_request(client_sockfd); 
		handle_request(client_sockfd);  

		close(client_sockfd);
	}
}

void handle_request(int sockfd)
{
	uint64_t value;
	char *current_time;
	uint32_t date_sizeof;
	uint32_t date_length;

	void *buffer = malloc(sizeof(struct request));
	int bytes_received = readWrapper(sockfd, buffer, sizeof(struct request));
	if(bytes_received == -1)
	{
		free(buffer);
		printf("Could not receive the message\n");
	} else if (bytes_received == sizeof(struct request)) {
		struct request REQUEST;
		memcpy(&REQUEST, buffer, sizeof(struct request));
		REQUEST.opc.uint32code = be32toh(REQUEST.opc.uint32code);
		REQUEST.id = be32toh(REQUEST.id);
		printf("Request received: OPCODE: 0x%08x; ID: %d\n", REQUEST.opc.uint32code, REQUEST.id);
		switch(REQUEST.opc.uint32code){
			case 0x00000001:
				//////////////////////////////////////////ROOT request
				buffer = (uint64_t *)calloc(1, sizeof(uint64_t));
				readWrapper(sockfd, buffer, sizeof(uint64_t));
				memcpy(&value, buffer, sizeof(uint64_t));
				free(buffer);
				
				REQUEST.opc.uint32code = htobe32(0x01000001);
				REQUEST.id = htobe32(REQUEST.id);
				value = htobe64(double_to_uint64_t(sqrt(uint64_t_to_double(be64toh(value)))));

				buffer = malloc(sizeof(REQUEST) + sizeof(uint64_t));
				memcpy(buffer			, &REQUEST	, sizeof(REQUEST));
				memcpy(buffer + sizeof(REQUEST)	, &value	, sizeof(uint64_t));
				if(writeWrapper(sockfd, buffer, sizeof(REQUEST) + sizeof(uint64_t)) <= 0)
				{
					printf("Error while answering request(root)\n");
				}
				//////////////////////////////////////////////////////
				break;
				
			case 0x00000002:
				//////////////////////////////////////////TIME request
				free(buffer);
				
				REQUEST.opc.uint32code = htobe32(0x01000002);
				REQUEST.id = htobe32(REQUEST.id);
				current_time = get_time();
				date_length = strlen(current_time);
				date_sizeof = date_length;
				
				//8 bytes for request structure, 4 bytes for respond length info, respond_size-bytes of response
				buffer = malloc(sizeof(REQUEST) + sizeof(uint32_t) + (sizeof(char)*date_length));
				//toBE
				date_sizeof = htobe32(date_sizeof);
				/*Convert time to BigEndian? Is it needed?*/

				memcpy(buffer			, &REQUEST		, sizeof(REQUEST));
				memcpy(buffer + sizeof(REQUEST)	, &date_sizeof	, sizeof(uint32_t));
				memcpy(buffer + (sizeof(REQUEST) + sizeof(uint32_t)), current_time, (sizeof(char)*date_length));
				
				if(writeWrapper(sockfd, buffer, sizeof(REQUEST) + sizeof(uint32_t) + (sizeof(char)*date_length)) <= 0)
				{
					printf("Error while answering request(time)\n");
				}
				//////////////////////////////////////////////////////
				break;
				
			default:
				printf("ERROR-HANDLE_REQUEST-SWITCH-DEFAULT\n");
				free(buffer);
				exit(1);
		}
	}
	free(buffer);
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
	ssize_t BYTES_TO_WRITE = 0;
	
	size_t BYTES_NOT_WRITTEN = estimated_size;
	ssize_t BYTES_WRITTEN = 0;
	char * REQUEST_BUFFER = (char *)buffer;
	while(BYTES_NOT_WRITTEN > 0){

		BYTES_WRITTEN = write(sockfd, REQUEST_BUFFER, BYTES_NOT_WRITTEN);

		if(BYTES_WRITTEN > 0){

			BYTES_TO_WRITE += BYTES_WRITTEN;	//how many bytes are already written
			BYTES_NOT_WRITTEN -= BYTES_WRITTEN;	//how many bytes left to write
			REQUEST_BUFFER += BYTES_WRITTEN;	//move pointer
		} else if (BYTES_WRITTEN == -1) {
			printf("ERROR while writing bytes! Im closing socket...\n");
			close(sockfd);
			return -1;
		} else if (BYTES_WRITTEN == 0 ) {
			printf("Nothing to write (maybe buffer is empty?)\n");
			return 0;
		}
	}
	return BYTES_TO_WRITE;
}

ssize_t readWrapper(int sockfd, void *buffer, size_t estimated_size)
{
	ssize_t BYTES_TO_READ = 0;
	
	size_t BYTES_NOT_READ = estimated_size;
	ssize_t BYTES_READ = 0;
	char * REQUEST_BUFFER = (char *)buffer;
	while(BYTES_NOT_READ > 0){

		BYTES_READ = read(sockfd, REQUEST_BUFFER, BYTES_NOT_READ);

		if(BYTES_READ> 0){

			BYTES_TO_READ += BYTES_READ;	//how many bytes are already written
			BYTES_NOT_READ -= BYTES_READ;	//how many bytes left to write
			REQUEST_BUFFER += BYTES_READ;	//move pointer
		} else if (BYTES_READ == -1) {
			printf("ERROR while reading bytes! Im closing socket...\n");
			close(sockfd);
			return -1;
		} else if (BYTES_READ == 0 ) {
			printf("Nothing to read (maybe buffer is empty?)\n");
			return 0;
		}
	}
	return BYTES_TO_READ;
}
