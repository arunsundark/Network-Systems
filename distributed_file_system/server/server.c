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
int check_directory_present(char* dirname) {

	struct stat st = {0};
	if (stat(dirname, &st) == -1) {
		mkdir(dirname, 0700);
	}
}
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
int check_user_credentials(char* username, char* password) {
	FILE* fp = fopen("DFS1/dfs.conf","r");
	char * line = NULL;
        size_t len = 0;
        ssize_t read;
	int i = 0;int j =0;
	char* tok;int valid = 0;
	char user_details[50];
	
        if(fp == NULL) {
		exit(EXIT_FAILURE);
	}
	while ((read = getline(&line, &len, fp)) != -1) {
        	memset(user_details,'\0',50);
		strncpy(user_details,line,read);
		tok = strtok(user_details,":");
		printf("tok1=%s\n",tok);
		if(strncmp(username,tok,strlen(username))==0) {
			tok = strtok(NULL," ");
			if(strncmp(password,tok,strlen(password))==0) {
				printf("tok2=%s\n",tok);
				valid = 1;
				break;
			}
		}	
			
    	
	}
	return valid;
}
	
int put(int connfd, char params[][64], int fs, int serverno) {

	int readlen = fs/4 + 3;
	char* buf = (char*) malloc(readlen);
	memset(buf,0, readlen);
	char pn[10];memset(pn,0,10);
	char sm_buf[10];int n;char name_buf[512];
	memset(sm_buf,0,10);
        // receiving part number 
	n = recv(connfd,sm_buf,10,0);
	printf("part no is=%s\n",sm_buf);
	
	// receiving file content for part number 
	n = recv(connfd,buf,readlen,0);
	printf("rx data=%s\n",buf);
	strncpy(pn,sm_buf,strlen(sm_buf));
	memset(sm_buf,0,10);
	my_itoa(serverno,sm_buf);
	memset(name_buf, 0,512);
	strncpy(name_buf,"DFS",3);
	strncat(name_buf,sm_buf,1);
	strncat(name_buf,"/",1);
	strncat(name_buf,params[1], strlen(params[1]));
	printf("dirname=%s*\n",name_buf);
	check_directory_present(name_buf);
	strncat(name_buf,"/.",2);
	strncat(name_buf,params[3],strlen(params[3]));
	memset(sm_buf,0,10);
	
	strncat(name_buf,".",1);
	strncat(name_buf,pn,strlen(pn));
	printf("pathname=%s*\n",name_buf);
	FILE* fp = fopen(name_buf, "w");
	n = fwrite(buf,1,n,fp);
	memset(sm_buf,0,10);
	strncpy(sm_buf,"ACK",3);
	n = send(connfd,sm_buf,3,0);
	memset(sm_buf,0,10);
	printf("Done storing\n");
	fclose(fp);
	return 0;
}





//process client request
int handle_client_request(struct sockaddr_in* cliaddr, socklen_t* clilen,int connfd, char* portno)  {
	int n,i=0;
	char buf[MAXLINE];
	char* tok;
	while(1) {
		char request[6][64];
		memset(buf, 0, MAXLINE);
		n = recv(connfd, buf, MAXLINE,0);
		printf("%s*\n",buf);
		tok = strtok(buf,",");

		while((tok != NULL)  && (i<5)) {
			strcpy(request[i],tok);
			printf("req_type=%s\n",request[i]);
			tok = strtok(NULL,",");
			i++;
		}
		memset(buf, 0, MAXLINE);

		if(check_user_credentials(request[1], request[2]) == 0) {
			strncpy(buf,"INV",3 );
			n = send(connfd, buf, strlen(buf),0);
			return 0;
		}
		if(strncmp(request[0],"PUT",3)==0) {
			strncpy(buf,"RDY",3 );
			printf("valid user\n");
			n = send(connfd, buf, strlen(buf),0);
			put(connfd,request,atoi(request[4]),atoi(portno)%10);
			put(connfd,request,atoi(request[4]),atoi(portno)%10);
		}
	}
	return 0;
}
void intHandler(int dummy) {
	keep_running = 0;
	fflush(stdout);
	printf("Server Exiting\n");
	exit(0);	
}

int process_tcp_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen, char* portno)  {
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
				handle_client_request(cliaddr, clilen, connfd, portno);
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
	process_tcp_client_request(&sockfd, &cliaddr, &clilen,argv[1]); 
}
