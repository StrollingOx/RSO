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

void send_request_ROOT(int, double);
void send_request_TIME(int);
void notify_server(int, uint32_t);
uint64_t double_to_uint64_t(double);
double uint64_t_to_double(int64_t);
ssize_t writeWrapper(int, void *, size_t);
ssize_t readWrapper(int, void *, size_t);

int main()
{
//////////////////////////////////////////////////////////////////////////////////////////////
//CONNECTION	

	int sockfd;
	socklen_t len;
	struct sockaddr_in address;
	
	len = sizeof(address);

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
        	printf("socket creation failed...\n"); 
        	exit(0); 
    	} 

	bzero(&address, len); 

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(9734);
	
	int result = connect(sockfd, (struct sockaddr *) &address, len);
	
	if(result == -1){ 
        	printf("Connection with the server failed...\n"); 
        	exit(0); 
    	} 

//////////////////////////////////////////////////////////////////////////////////////////////
//COMMUNICATION

	//ignores signal SIGPIPE
	//SIGPIPE is sent to a process if it tried to write to a socket that had been shutdown for writing or isn't connected [anymore].
	signal(SIGPIPE, SIG_IGN);
	
	srand(time(NULL));
	send_request_ROOT(sockfd, 4.84);
	send_request_TIME(sockfd);
	close(sockfd);
}

void send_request_ROOT(int sockfd, double value)
{
	uint32_t request_id = (int)rand()%999;

	struct request ROOT_REQUEST = {htobe32(0x00000001), htobe32(request_id)};
	uint64_t VALUE = htobe64(double_to_uint64_t(value));

	writeWrapper(sockfd, &ROOT_REQUEST, sizeof(ROOT_REQUEST));
	writeWrapper(sockfd, &VALUE, sizeof(uint64_t));

	readWrapper(sockfd, &ROOT_REQUEST, sizeof(ROOT_REQUEST));
	readWrapper(sockfd, &VALUE, sizeof(uint64_t));
	
	ROOT_REQUEST.opc.uint32code = be32toh(ROOT_REQUEST.opc.uint32code);
	ROOT_REQUEST.id = be32toh(ROOT_REQUEST.id);
	VALUE = be64toh(VALUE);

	if(ROOT_REQUEST.id != request_id) {printf("WRONG REQUEST ID (ROOT)\n"); exit(1);}
	else{
	switch(ROOT_REQUEST.opc.uint32code){

		case 0x01000001:
			printf("Answer opcode: 0x%08x\n", ROOT_REQUEST.opc.uint32code);
			printf("Answer ID: %d\n", ROOT_REQUEST.id);
			printf("Received root: %.2f\n", uint64_t_to_double(VALUE));
			
			break;
		default:
			printf("Something went wrong - ROOT_REQUEST, SWITCH(OPCODE) TRIGERRED DEFAULT\n");
	}}
	printf("Request complete.\n\n");
}

void send_request_TIME(int sockfd)
{
	char *time;
	uint32_t date_size;
	uint32_t request_id = (int)rand()%999+1000;

	struct request TIME_REQUEST = {htobe32(0x00000002), htobe32(request_id)};

	writeWrapper(sockfd, &TIME_REQUEST, sizeof(TIME_REQUEST));
	readWrapper(sockfd, &TIME_REQUEST, sizeof(TIME_REQUEST));

	TIME_REQUEST.opc.uint32code = be32toh(TIME_REQUEST.opc.uint32code);
	TIME_REQUEST.id = be32toh(TIME_REQUEST.id);
	
	if(TIME_REQUEST.id != request_id) {printf("WRONG REQUEST ID (TIME)\n"); exit(1);}
	switch(TIME_REQUEST.opc.uint32code){
		case 0x01000002:
			readWrapper(sockfd, &date_size, sizeof(uint32_t));
			date_size = be32toh(date_size);
			time = (char *)malloc(sizeof(char)*date_size+1);
			readWrapper(sockfd, time, sizeof(char)*date_size);

			printf("Answer opcode: 0x%08x\n", TIME_REQUEST.opc.uint32code);
			printf("Answer ID: %d\n", TIME_REQUEST.id);
			printf("Date received: %s\n", time);
			free(time);	
			break;
		default:
			printf("Something went wrong - TIME_REQUEST, SWITCH(OPCODE) TRIGERRED DEFAULT\n");
	}
	printf("Request complete.\n\n");
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
			printf("bytes_read == 0\n");
			close(sockfd);
			exit(1);
		}
	}
	return total_bytes_read;
}

