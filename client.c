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

void chat(int);
//double reverse(double);
//double check_order(double);

union uint64_t_double {
    uint64_t u;
    double d;
};

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

/* https://stackoverflow.com/questions/4306186/structure-padding-and-packing */
struct __attribute__((__packed__)) request
{	
	//4 bytes
	uint8_t b0;
	uint8_t b1;
	uint8_t b2;
	uint8_t b3;
	uint32_t RQ_ID;
	//8 bytes
	double value;
	//19 bytes
	//TODO: zmienny rozmiar
	char tab[19];
	
};

int main()
{
	int sockfd;
	socklen_t len;
	struct sockaddr_in address;
	
	len = sizeof(address);

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
	{ 
        	printf("socket creation failed...\n"); 
        	exit(0); 
    	} 

	bzero(&address, len); 

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(9734);
	
	int result = connect(sockfd, (struct sockaddr *) &address, len);
	
	if(result == -1)
	{ 
        	printf("Connection with the server failed...\n"); 
        	exit(0); 
    	} 
	
	double myDouble = 4.84;
	uint64_t lol = double_to_uint64_t(myDouble);
	lol = htobe64(lol);
	lol = be64toh(lol);
	myDouble = uint64_t_to_double(lol);
	printf("myDouble: %f\n", myDouble);
	
	chat(sockfd);
	close(sockfd);
}

/*
double check_order(double a)
{
	if(__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__)
		return reverse(a);
	else
		return a;
}

*/


double reverse(double value)
{	
	//printf("Original value: %f\n", value);
	int i;
	double result;
	char *primal = (char *) &value;
	char *reversed = (char *) &result;
	
	

	for(i = 0; i<sizeof(value); i++)
		reversed[i] = primal[sizeof(value) - i - 1];
	
	//printf("Reversed value: %f\n", result);
	return result;
}


void chat(int sockfd)
{
	struct request req_root;
	struct request req_date;
	double value_to_root = 4.84;
	//root test
	req_root.b0 = 0;
	req_root.b1 = 0;
	req_root.b2 = 0;
	req_root.b3 = 1;
	req_root.value = reverse(value_to_root); // to BigEndian

	write(sockfd, &req_root, sizeof(struct request));
	read(sockfd, &req_root, sizeof(struct request));

	if(req_root.b0 == 1)
	{
		printf("Value sent to server: %f\nRooted value from server: %f\n", value_to_root, reverse(req_root.value)); //to LittleEndian
		req_root.b0 = 0;
	}
	else
		printf("Server did not deliver correct response\n");
	
	//date test
	req_date.b0 = 0;
	req_date.b1 = 0;
	req_date.b2 = 0;
	req_date.b3 = 2;

	write(sockfd, &req_date, sizeof(struct request));
	read(sockfd, &req_date, sizeof(struct request));
	//TODO: sprawdzić czy wszystko przyszło
	
	if(req_date.b0 == 1)
	{
		printf("Respense from server: %s\n", req_date.tab);
		req_date.b0 = 0;
	}
	else
		printf("Server did not deliver correct response\n");

	close(sockfd);
	
}

