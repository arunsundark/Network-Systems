/**
@author: Arunsundar Kannan
@file : Distributed File Server
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <openssl/md5.h>
#include <time.h>
#include <arpa/inet.h>

#define MAXLINE 1024 /*max text line length*/


static volatile int keep_running = 1;
int get_file_size(FILE* fp) {
	fseek(fp,0,SEEK_END);
	int file_size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	return file_size;
}

char* my_itoa(int num, char* str) {
	sprintf(str,"%d",num);
	return str;
}

//Function to send data from server to client
void send_to_client(char* buf,int fs,FILE* fp,int connfd) {
	int nbytes = 0;
	while(fs > 0) {
		memset(buf,0,MAXLINE);
		nbytes = fread(buf,1,MAXLINE,fp);
		fs = fs - MAXLINE;
		send(connfd, buf,nbytes,0);
	}
}

//Initialisation for the TCP socket communication
int tcp_connection_init(int* sockfd, struct sockaddr_in* servaddr, int addrlen, char* portno) {

	int listen_fd = 0;
	int tval = 1;
        //Create a socket for the soclet
	//If sockfd<0 there was an error in the creation of the socket
	if ((*sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Socket Creation Error\n");
		exit(2);
	}

        //preparation of the socket address

	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr->sin_port = htons(atoi(portno));
	if(setsockopt(*sockfd,SOL_SOCKET, SO_REUSEADDR,&tval,sizeof(int)) < 0) {
		printf("sectsock error \n");
		exit(2);
	}
	//bind the socket
	if(bind (*sockfd, (struct sockaddr *) servaddr, addrlen) < 0) {
		printf("Binding Error \n");
		exit(2);
	}
	//listen to the socket by creating a connection queue, then wait for clients

	listen_fd = listen (*sockfd, 8);
        if(listen_fd < 0) {
		printf("Listen Error \n");
		exit(2);
	} 
        return 0;
}

//process client request
int handle_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen,int connfd)  {
	int n;
	char buf[MAXLINE];
//	while(1) {
			
		n = recv(connfd, buf, MAXLINE,0);
		printf("%s\n",buf);
		memset(buf, 0, MAXLINE);
		strcpy(buf, "hello client dfc");
		n = send(connfd, buf, strlen(buf),0);
//	}		
	

	return 0;
}
void intHandler(int dummy) {
	keep_running = 0;
	fflush(stdout);
	printf("Server Exiting\n");
	exit(0);	
}

int process_tcp_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen)  {
	int connfd;
	int n =0;
	signal(SIGINT, intHandler);
	while(1) {	
		pid_t childpid;

		*clilen = sizeof(*cliaddr);
		memset(cliaddr,0,sizeof(*cliaddr));
		printf("\n*************************WAITING*********************************\n");
		connfd = accept (*sockfd, (struct sockaddr *) cliaddr, clilen);
		if( connfd > 0) {
			if((childpid = fork ()) == 0 ) {//if it’s 0, it’s child process
				printf ("%s\n","Child created for dealing with client requests");
				handle_client_request( sockfd,  cliaddr, clilen, connfd);
				exit(0);
			}
			n++;
		}
	
		close(connfd);
		
		printf("\n*************************SERVICED********************************\n");
	}

}
int main (int argc, char **argv)  {
	int sockfd, connfd, n;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	memset(&servaddr,0,sizeof(servaddr));
        int addrlen = 0;
	tcp_connection_init( &sockfd, &servaddr, sizeof(servaddr), argv[1]);   
        pid_t childpid;
	printf("%s\n","Server running...waiting for connections.");
	process_tcp_client_request(&sockfd, &cliaddr, &clilen); 
}
