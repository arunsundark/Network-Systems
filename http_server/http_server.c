/**
@author: Arunsundar Kannan
@file : HTTP WEBSERVER
*/




#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAXLINE 1024 /*max text line length*/
#define SERV_PORT  8081/*port*/
#define LISTENQ 8 /*maximum number of client connections*/


char root_path[512];
typedef enum {
	FILE_NOT_FOUND = -2,
	SERVER_ERROR = -1
} err_t;
static volatile int keep_running = 1;
char proto_ver_db[2][20];	
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

// Error Handling function
int error_handle(err_t e_type, int connfd ) {
	char err_res[512];
	char temp_str[32];
	memset(err_res,0,512);
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


//FUnction to send data from server to client
void send_to_client(char* buf,int fs,FILE* fp,int connfd) {
	int nbytes = 0;
	while(fs > 0) {
		memset(buf,0,MAXLINE);
		nbytes = fread(buf,1,MAXLINE,fp);
		fs = fs - MAXLINE;
	//	printf("fsin loop =%d\n",fs);
		send(connfd, buf,nbytes,0);
	}
}

//Check the extension of the file
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

//Populates the filetype database
void fill_file_type_db() {
	strcat(file_type_db[0],"text/html");
	strcat(file_type_db[1],"text/plain");
	strcat(file_type_db[2],"image/png");
	strcat(file_type_db[3],"image/gif");
	strcat(file_type_db[4],"image/jpg");
	strcat(file_type_db[5],"text/css");
	strcat(file_type_db[6],"application/javascript");
}
//populates error message database
void fill_err_msg_db(){
	strcat(err_msg_db[0],"404 File Not Found");
	strcat(err_msg_db[1] ,"500 Internal Server Error");
}

//populates the protocol version database
void fill_proto_ver_db() {
	strcat(proto_ver_db[0], "HTTP/1.0 ");
	strcat(proto_ver_db[1], "HTTP/1.1 ");
}
//Initialisation for the TCP socket communication
int tcp_connection_init(int* sockfd, struct sockaddr_in* servaddr, int addrlen) {

	int listen_fd = 0;

        //Create a socket for the soclet
	//If sockfd<0 there was an error in the creation of the socket
	if ((*sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Socket Creation Error\n");
		exit(2);
	}
	int tval = 1;
        //preparation of the socket address

	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr->sin_port = htons(SERV_PORT);
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

	listen_fd = listen (*sockfd, LISTENQ);
        if(listen_fd < 0) {
		printf("Listen Error \n");
		exit(2);
	} 
        return 0;
}


//Check the command issued by the client
int check_command(char* req_param) {
	if(strncmp(req_param,"GET",3) == 0) return 1;
	else return -1;
}

//checks the protocol version 
int check_protocol_version(char* req_param) {
	if(strstr(req_param,"/1.1")) return 1;
	if(strstr(req_param,"/1.0")) return 0;
}		


//FUnction to process client request
int handle_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen,int connfd)  {
	//close listening socket
	int n;int i = 0;int fs =0;int ft;FILE* fp;
	char buf[MAXLINE];char req_param[10][256];char default_res[512];char* tok;
	
	n = recv(connfd, buf, MAXLINE,0);
	printf("%s","String received from client:");
	fputs(buf,stdout);
	tok = strtok(buf,"/");
	printf("\n *******************NEW INCOMING REQUEST*************************\n");
	
	while((tok != NULL)  && (i<3 )) {
		strcpy(req_param[i],tok);
	//	printf("req_type=%s\n",req_param[i]);
		tok = strtok(NULL,"-");
		i++;

	}
	int proto_ver = check_protocol_version(buf);
	if(proto_ver <  0) {
		printf("Bad version\n, closing connection");
		error_handle(SERVER_ERROR,connfd);
		return 0;
	} else printf("\n ret val from proto check is %d\n",proto_ver);
		
	if(check_command(req_param[0]) < 0) {
		printf("Bad command\n, closing connection");
		error_handle(SERVER_ERROR,connfd);
		return 0;
	}
// sends the default page 
	if(strncmp(req_param[1], " HTTP",5)==0) {
		memset(default_res,0,512);
		strcat(default_res,req_param[1]);
		strcat(default_res," ");
		strcat(default_res,"200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:");
		strcat(default_res,"3346\r\n\r\n");
		printf("server response =");
		puts(default_res);     
		send(connfd, default_res, strlen(default_res), 0);
		fp = fopen("www/index.html","r");
		if(fp == NULL) {
			error_handle(FILE_NOT_FOUND, connfd);
			return 0; 
		}
		fs = get_file_size(fp);

	} else  {//sends the requested URL
		int j =0;
		memset(default_res,0,512);
		strcat(req_param[1],req_param[2]);
		printf("req_param[1] =");
		puts(req_param[1]);
		printf("\n");
		char* temp_tok = strtok(req_param[1]," ");
		printf("\nfile_name=%s\n",temp_tok); 
                char file_name[512];
		strcpy(file_name,root_path);
		strcat(file_name, temp_tok);
		fp = fopen(file_name,"r");
		if( fp == NULL) {
			printf("File Not Found\n");
			error_handle(FILE_NOT_FOUND, connfd);
			return 0;
		}
		fs = get_file_size(fp);
		printf("fs=%d\n",fs);	
		puts(req_param[1]);
		strcat(default_res,proto_ver_db[proto_ver]);
		strcat(default_res,"200 Document Follows\r\nContent-Type:");
		ft = find_file_type(temp_tok);
		if(ft < 0) {
			printf("file type found\n");
			error_handle(SERVER_ERROR, connfd);
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
	send_to_client(buf,fs,fp,connfd);
	fclose(fp);
	memset(buf,0,MAXLINE); 
	printf("\n*************************REQUEST COMPLETED*******************************\n");	
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
	strcpy(root_path, "www/");
 	fill_file_type_db();       
	fill_err_msg_db();
	fill_proto_ver_db();
        tcp_connection_init( &sockfd, &servaddr, sizeof(servaddr));   
        pid_t childpid;
	printf("%s\n","Server running...waiting for connections.");
	process_tcp_client_request(&sockfd, &cliaddr, &clilen); 
			

}
