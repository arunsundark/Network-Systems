#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>


#define MAXLINE 1024 /*max text line length*/
#define SERV_PORT  8080/*port*/
#define LISTENQ 8 /*maximum number of client connections*/


typedef enum {
	FILE_NOT_FOUND = -2,
	SERVER_ERROR = -1
} err_t;

char file_type_db[10][50];
char err_msg_db[2][512];
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

int find_file_type(char* s1) {

	if(strstr(s1,".html")) return 0;
	else if(strstr(s1,".txt")) return 1;
	else if(strstr(s1,".png")) return 2;
	else if(strstr(s1,".gif")) return 3;
	else if(strstr(s1,".jpg")) return 4;
	else if(strstr(s1,".css")) return 5;
	else if(strstr(s1,".js")) return 6;
	else return -1;
}
void fill_file_type_db() {
	strcat(file_type_db[0],"text/html");
	strcat(file_type_db[1],"text/plain");
	strcat(file_type_db[2],"image/png");
	strcat(file_type_db[3],"image/gif");
	strcat(file_type_db[4],"image/jpg");
	strcat(file_type_db[5],"text/css");
	strcat(file_type_db[6],"application/javascript");
}

void fill_err_msg_db(){
	strcat(err_msg_db[0],"404 File Not Found");
	strcat(err_msg_db[1] ,"500 Internal Server Error");
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
int error_handle(err_t e_type, int connfd, char* req_param) {
	char err_res[512];
	char temp_str[32];
	memset(err_res,0,512);
//	strcat(err_res,"HTTP/1.1");
//	strcat(err_res," ");
		
	if(e_type == FILE_NOT_FOUND) {
		strcat(err_res,err_msg_db[0]);
	}
	if(e_type == SERVER_ERROR) {
		strcat(err_res,err_msg_db[1]);
	}
	printf("server response =");
	puts(err_res);     
	send(connfd, err_res, strlen(err_res), 0);
	
	return 0;
}
int handle_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen,int connfd)  {
	//close listening socket
	int n;
	char buf[MAXLINE];
        char req_param[10][256];
        char default_res[512];
        char* tok;
	
	n = recv(connfd, buf, MAXLINE,0);
		printf("%s","String received from client:");
		puts(buf);
                tok = strtok(buf,"/");
		int i = 0;int fs =0;int ft;FILE* fp;
		printf("\n *******************NEW INCOMING REQUEST*************************\n");
			
		while((tok != NULL)  && (i<3 )) {
			strcpy(req_param[i],tok);
		 	printf("req_type=%s\n",req_param[i]);
			tok = strtok(NULL,"-");
			i++;
			
		}
                if(strncmp(req_param[1], " HTTP",5)==0) {
			memset(default_res,0,512);
			strcat(default_res,req_param[1]);
			strcat(default_res," ");
			strcat(default_res,"200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:");
			strcat(default_res,"3346\r\n\r\n");
			printf("server response =");
			puts(default_res);     
			send(connfd, default_res, strlen(default_res), 0);
			fp = fopen("index.html","r");
			if(fp == NULL) {
				error_handle(FILE_NOT_FOUND, connfd, req_param[1]);
				return 0; 
			}
			fs = get_file_size(fp);
		
		} else  {
	        	int j =0;
	                memset(default_res,0,512);
			strcat(req_param[1],req_param[2]);
			printf("req_param[1] =");
			puts(req_param[1]);
			printf("\n");
		        char* temp_tok = strtok(req_param[1]," ");
			printf("\nfile_name=%s\n",temp_tok); 
			fp = fopen(temp_tok,"r");
			if( fp == NULL) {
				printf("File Not Found\n");
				error_handle(FILE_NOT_FOUND, connfd, req_param[1]);
				return 0;
			}
			fs = get_file_size(fp);
			printf("fs=%d\n",fs);	
			puts(req_param[1]);
			strcat(default_res,"HTTP/1.1 200 Document Follows\r\nContent-Type:");
			ft = find_file_type(temp_tok);
			if(ft < 0) {
				printf("file type found\n");
				error_handle(SERVER_ERROR, connfd, req_param[1]);
				return 0;
			}
			else {
				strcat(default_res,file_type_db[ft]);
			}
                        strcat(default_res,"\r\nContent-Length:");
			char fs_str[20];
			strcat(default_res,my_itoa(fs,fs_str));
			strcat(default_res,"\r\n\r\n");
			
			send(connfd, default_res, strlen(default_res), 0);
			printf("\n SERVER RESPONSE\n");
			puts(default_res);
		} 		
		memset(buf,0,MAXLINE);
		int nbytes = 0;
		while(fs > 0) {
			memset(buf,0,MAXLINE);
			nbytes = fread(buf,1,MAXLINE,fp);
			fs = fs - MAXLINE;
			printf("fsin loop =%d\n",fs);
			send(connfd, buf,nbytes,0);
		}	
	printf("not in while \n"); 		
	fclose(fp);
	memset(buf,0,MAXLINE); 
	printf("\n*************************REQUEST COMPLETED*******************************\n");	
	return 0;
}
int process_tcp_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen)  {
	int connfd;
	int n =0;
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
		}close(connfd);

		printf("\n*************************SERVICED********************************\n");
	}

}
int main (int argc, char **argv)  {
	int sockfd, connfd, n;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	memset(&servaddr,0,sizeof(servaddr));
        int addrlen = 0;
 	fill_file_type_db();       
	fill_err_msg_db();
        tcp_connection_init( &sockfd, &servaddr, sizeof(servaddr));   
        pid_t childpid;
	printf("%s\n","Server running...waiting for connections.");
	process_tcp_client_request(&sockfd, &cliaddr, &clilen); 
			

}
