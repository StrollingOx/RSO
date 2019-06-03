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

void chat(int/*, uint8_t**/);
double reverse(double);
bool check_order(double);

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
	signal (SIGCHLD, SIG_IGN); 

	while(server_online)
	{
		client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
		if(client_sockfd<0)
		{
			printf("Server accept failed...\n");
			exit(0);
		} else { printf("Client accepted\n"); }

		if(fork() ==0)
		{	
			chat(client_sockfd/*, &request_id*/); //can't use it beacuse forks work on the same resources...right?
		}
		close(client_sockfd);
	}
}
/* https://linux.die.net/man/3/htobe64 */
bool check_order(double a)
{
	if(__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__)
		return true; 
	else
		return false; 
}

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

void chat(int sockfd/*, uint8_t * id*/)
{
	//htobe64(uint64_t) tylko trzeba zrobić unie, w unie odwrócić i zwrócić jako double
	//be64toh(uint64_t)
	struct request req;
	bool client_online = true;
	bool isBigEndian;
	double temp;
	while(client_online)
	{
		read(sockfd, &req, sizeof(struct request));
		//TODO: sprawdzić czy wszystko przyszło
		if(req.b3 == 1)
		{	
			//*id = *id + 1;
			//req.RQ_ID = *id;
			//printf("Request id: %d. Processing...\n", *id);

			isBigEndian = check_order(req.value);
			if(isBigEndian)
				req.value = sqrt(req.value);
			else
				req.value = reverse(sqrt(reverse(req.value)));
			req.b0 = 1;
		}
		else if (req.b3 == 2)
		{
			//*id = *id + 1;
			//req.RQ_ID = *id;
			//printf("Request id: %d. Processing...\n", *id);

			time_t curtime;
			time(&curtime);
			strcpy(req.tab, ctime(&curtime));
			req.b0 = 1;
		}
		send(sockfd, &req, sizeof(struct request), 0);
		//printf("Request id: %d -> Done!\n", *id);
	}
}
