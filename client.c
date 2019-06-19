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
//TESTS	

/*
	double myDouble = 4.84;
	uint64_t lol = double_to_uint64_t(myDouble);
	lol = htobe64(lol);
	lol = be64toh(lol);
	myDouble = uint64_t_to_double(lol);
	printf("myDouble: %f\n", myDouble);
*/	

//////////////////////////////////////////////////////////////////////////////////////////////
//COMMUNICATION
	send_request_ROOT(sockfd, 4.84);
	send_request_TIME(sockfd);
	close(sockfd);
}

void send_request_ROOT(int sockfd, double value)
{
	//buffer with all data (16 bytes)
	//4+4 bytes of opcode and id
	//8 bytes for the value
	void *buffer;

	//data that will be sent
	struct request ROOT_REQUEST = {htobe32(0x00000001), htobe32(125)};
	uint64_t VALUE = htobe64(double_to_uint64_t(value));
	uint32_t REQUEST_SIZE = sizeof(ROOT_REQUEST) + sizeof(VALUE);

	buffer = malloc(REQUEST_SIZE);
	
	memcpy(buffer				, &ROOT_REQUEST	, sizeof(ROOT_REQUEST));
	memcpy(buffer + sizeof(ROOT_REQUEST)	, &VALUE	, sizeof(uint64_t));
	
	//send data
	int result = writeWrapper(sockfd, buffer, REQUEST_SIZE);
	if(result == -1)
	{
		printf("ERROR-CLIENT-SEND_REQUEST_ROOT-WRITEWRAPPER\n");
		free(buffer);
		exit(1);
	} else if (result == REQUEST_SIZE) {
		printf("Request was successfully sent(ROOT)!\n");
	}

	free(buffer); buffer = malloc(sizeof(REQUEST_SIZE)); //calloc instead?

	//answer from server
	result = readWrapper(sockfd, buffer, REQUEST_SIZE);
	if(result == -1)
	{
		printf("ERROR-CLIENT-SEND_REQUEST_ROOT-READWRAPPER\n");
		free(buffer);
		exit(1);
	} else if (result == REQUEST_SIZE) {
		printf("Answer received from server!\n");
	}
	
	memcpy(&ROOT_REQUEST, buffer, sizeof(ROOT_REQUEST));
	ROOT_REQUEST.opc.uint32code = be32toh(ROOT_REQUEST.opc.uint32code);
	ROOT_REQUEST.id = be32toh(ROOT_REQUEST.id);
	switch(ROOT_REQUEST.opc.uint32code){

		case 0x01000001:
			memcpy(&VALUE, buffer + sizeof(ROOT_REQUEST), sizeof(uint64_t));
			VALUE = be64toh(VALUE);
			printf("Answer opcode: 0x%08x\n", ROOT_REQUEST.opc.uint32code);
			printf("Answer ID: %d\n", ROOT_REQUEST.id);
			printf("Received root: %.2f\n", uint64_t_to_double(VALUE));
			
			break;
		default:
			printf("Something went wrong :/ (ROOT_REQUEST, OPCODE FROM SERVER)\n");
	}

	free(buffer);
	//close(sockfd);
}

void send_request_TIME(int sockfd)
{
	void *buffer;

	//data that will be sent
	struct request TIME_REQUEST = {htobe32(0x00000002), htobe32(744)};

	buffer = malloc(sizeof(struct request));
	
	memcpy(buffer, &TIME_REQUEST, sizeof(TIME_REQUEST));
	
	//send data
	int result = writeWrapper(sockfd, buffer, sizeof(TIME_REQUEST));
	if(result == -1)
	{
		printf("ERROR-CLIENT-SEND_REQUEST_TIME-WRITEWRAPPER\n");
		free(buffer);
		exit(1);
	} else if (result == sizeof(TIME_REQUEST)) {
		printf("Request was successfully sent!(TIME)\n");
	}

	free(buffer); buffer = malloc(sizeof(TIME_REQUEST) + sizeof(uint32_t)); //calloc instead?

	//answer from server
	result = readWrapper(sockfd, buffer, sizeof(TIME_REQUEST) + sizeof(uint32_t));
	if(result == -1)
	{
		printf("ERROR-CLIENT-SEND_REQUEST_TIME-READWRAPPER\n");
		free(buffer);
		exit(1);
	} else if (result == (sizeof(TIME_REQUEST) + sizeof(uint32_t))) {
		printf("Answer received from server!\n");
	}
	
	memcpy(&TIME_REQUEST, buffer, sizeof(TIME_REQUEST));
	TIME_REQUEST.opc.uint32code = be32toh(TIME_REQUEST.opc.uint32code);
	TIME_REQUEST.id = be32toh(TIME_REQUEST.id);
	uint32_t date_size;
	char *time;
	memcpy(&date_size, buffer + sizeof(TIME_REQUEST), sizeof(uint32_t));
	date_size = be32toh(date_size);	


	switch(TIME_REQUEST.opc.uint32code){

		case 0x01000002:
			free(buffer); if(date_size<100) buffer=malloc(sizeof(char)*date_size);
			readWrapper(sockfd, buffer, sizeof(char)*date_size);
			time = (char *)malloc(sizeof(char)*date_size);
			memcpy(time, (char *)buffer, sizeof(char)*date_size);
			/* betoh time*/
			printf("Answer opcode: 0x%08x\n", TIME_REQUEST.opc.uint32code);
			printf("Answer ID: %d\n", TIME_REQUEST.id);
			printf("Date received: %s\n", time);
			
			break;
		default:
			printf("Something went wrong :/ (TIME_REQUEST, OPCODE FROM SERVER)\n");
	}

	free(buffer);
	//close(sockfd);


}

/*void notify_server(int sockfd, uint32_t request_size)
{
	void * buffer = malloc(sizeof(uint32_t));
	uint32_t size = request_size;
	memcpy(buffer, &size, sizeof(uint32_t));
	printf("Sending notification to server: bytesToRead: %d\n", size);
	int result = writeWrapper(sockfd, buffer, sizeof(uint32_t));
	if(result == -1)
	{
		printf("ERROR-NOTIFY_SERVER\n");
		free(buffer);
		exit(1);
	} else if (result == sizeof(uint32_t)) {
		printf("Notification was successfully sent!\n");
	}

	free(buffer);
}*/

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

