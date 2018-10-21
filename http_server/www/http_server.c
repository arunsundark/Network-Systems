#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>


#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT  8081/*port*/
#define LISTENQ 8 /*maximum number of client connections*/

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

int tcp_connection_init(int* sockfd, struct sockaddr_in* servaddr, int addrlen) {

	int listen_fd = 0;

        //Create a socket for the soclet
	//If sockfd<0 there was an error in the creation of the socket
	if ((*sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Socket Creation Error\n");
		exit(2);
	}

        //preparation of the socket address

	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr->sin_port = htons(SERV_PORT);

	//bind the socket
	if(bind (*sockfd, (struct sockaddr *) servaddr, addrlen) < 0) {
		printf("Binding Error \n");
		exit(2);
	}

	//listen to the socket by creating a connection queue, then wait for clients

	listen_fd = listen (*sockfd, LISTENQ);
        if(listen_fd < 0) {
		printf("Listen Error \n");
		exit(2);
	} 
        return 0;
}

int process_tcp_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen,int connfd)  {
        int n;
	char buf[MAXLINE];
        char req_param[10][25];
        char default_res[512];
        char* tok;	pid_t childpid;
        *clilen = sizeof(*cliaddr);
	connfd = accept (*sockfd, (struct sockaddr *) cliaddr, clilen);
	if( connfd > 0) {
		if((childpid = fork ()) == 0 ) {//if it’s 0, it’s child process
			printf ("%s\n","Child created for dealing with client requests");
		}
	}

	//close listening socket
	close (*sockfd);

//	while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
		n = recv(connfd, buf, MAXLINE,0);
		printf("%s","String received from and resent to the client:");
		puts(buf);

		tok = strtok(buf,"/");
		int i = 0;
		while(tok != NULL) {
			strcpy(req_param[i],tok);
		 //	printf("req_type=%s\n",req_param[i]);
			tok = strtok(NULL,"-");
			i++;
			
		}  
		memset(buf,0,MAXLINE);
		memset(default_res,0,512);
		strcat(default_res,req_param[1]);
		strcat(default_res," ");
		strcat(default_res,"200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:");
		strcat(default_res,"3346\r\n\r\n");
		// strcat(buf,"\0");
                printf("server response =");
                puts(default_res);     
                printf("\n***************************\n");
		send(connfd, default_res, strlen(default_res), 0);
		FILE* fp = fopen("index.html","r");
		int fs = get_file_size(fp);
		int nbytes = 0;
		while(fs > 0) {
			memset(buf,0,MAXLINE);
			nbytes = fread(buf,1,MAXLINE,fp);
			fs = fs - MAXLINE;
			send(connfd, buf,nbytes,0);
		}	

//	}
	close(connfd);
	return 0;
	}
int main (int argc, char **argv)  {
	int sockfd, connfd, n;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	memset(&servaddr,0,sizeof(servaddr));
        int addrlen = 0;
        tcp_connection_init( &sockfd, &servaddr, sizeof(servaddr));   
        pid_t childpid;
	


	printf("%s\n","Server running...waiting for connections.");
	process_tcp_client_request(&sockfd, &cliaddr, &clilen,connfd); 
			
/*	for ( ; ; ) {

		clilen = sizeof(cliaddr);
		//accept a connection
				printf("%s\n","Received request...");

		if ( (childpid = fork ()) == 0 ) {//if it’s 0, it’s child process

			printf ("%s\n","Child created for dealing with client requests");

			//close listening socket
			close (sockfd);

			while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
				printf("%s","String received from and resent to the client:");
				puts(buf);
				send(connfd, buf, n, 0);
			}

			if (n < 0)
				printf("%s\n", "Read error");
			exit(0);
		}
		//close socket of the server
		close(connfd);
	} */
       /* while(1) {
		connfd = accept (sockfd, (struct sockaddr *) &cliaddr, &clilen);
                if( connfd > 0) {
			if((childpid = fork ()) == 0 ) {//if it’s 0, it’s child process
				printf ("%s\n","Child created for dealing with client requests");
	          		process_tcp_client_request(&sockfd, &cliaddr, &clilen,connfd); 
			}
		}
	}
*/
}
