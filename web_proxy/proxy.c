/**
@author: Arunsundar Kannan
@file : HTTP WEBSERVER
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
#define MAXLINE 1024 /*max text line length*/
#define SERV_PORT  8082/*port*/
#define LISTENQ 8 /*maximum number of client connections*/

int cache_timeout;
typedef enum {
	FILE_NOT_FOUND = -2,
	SERVER_ERROR = -1
} err_t;
static volatile int keep_running = 1;
char proto_ver_db[2][20];	

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

char* calculate_md5sum_for_url(char* buf) {
	int n;
	MD5_CTX c;
	char* md5result = (char*) malloc(33);
	ssize_t bytes = strlen(buf);
	unsigned char digest[16];
	MD5_Init(&c);
	while(bytes > 0) {
		if(bytes > 512) {
			MD5_Update(&c, buf, 512);
			} else {
				MD5_Update(&c, buf,bytes);
		}
		bytes = bytes - 512;
		buf = buf + 512;
	}
	MD5_Final(digest, &c);
	for (n =0;n < 16; ++n) {
		snprintf(&(md5result[n*2]), 16*2,"%02x", (unsigned int)digest[n]);
	}
	return md5result;
}

	




//FUnction to send data from server to client
void send_to_client(char* buf,int fs,FILE* fp,int connfd) {
	int nbytes = 0;
	while(fs > 0) {
		memset(buf,0,MAXLINE);
		nbytes = fread(buf,1,MAXLINE,fp);
		fs = fs - MAXLINE;
		send(connfd, buf,nbytes,0);
	}
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
	int tval = 1;
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
	else if(strstr(req_param,"/1.0")) return 0;
	else return -1;
}		

int check_file_present_in_cache(char* path_name) {
	if(access(path_name, F_OK) !=-1) return 1;
	else return -1;
}


//process client request
int handle_client_request(int* sockfd, struct sockaddr_in* cliaddr, socklen_t* clilen,int connfd)  {
	//close listening socket
	int i = 0;//int fs =0;int ft;
	FILE* fp;
	char buf[MAXLINE];char req_param[10][256];unsigned char md5sum[512];char* tok;
	char root_path[512];
        char message[MAXLINE];
	char client_message[MAXLINE];
	struct sockaddr_in main_server;int host_sockfd;
	int n = recv(connfd, buf, MAXLINE,0);
	printf("%s","String received from client:");
	fputs(buf,stdout); strncpy(client_message,buf,strlen(buf));
	tok = strtok(buf," ");
	printf("\n *******************NEW INCOMING REQUEST*************************\n");
	
	while((tok != NULL)  && (i<4 )) {
		strcpy(req_param[i],tok);
		printf("req_type=%s\n",req_param[i]);
		tok = strtok(NULL," ");
		i++;

	}
        /* req_param[0] = request type. Ony get requests are serviced 
	   req_param[1] = URL 
	   req_param[2] = HTTP protocol version. Only HTTP1.0/1.1 are serviced
	   req_param[3] = hostname without http://
	*/
	int proto_ver = check_protocol_version(req_param[2]);
	if(proto_ver <  0) {
		printf("Bad version\n, closing connection");
		error_handle(SERVER_ERROR,connfd);
		return 0;
	} 
	if(check_command(req_param[0]) < 0) {
		printf("Bad command\n, closing connection");
		error_handle(SERVER_ERROR,connfd);
		return 0;
	} 
	MD5_CTX url_md5;
	MD5_Init(&url_md5);
	MD5_Update(&url_md5, req_param[1], strlen(req_param[1]));
	MD5_Final(md5sum, &url_md5);
	memset(buf,0,MAXLINE);
	memset(message,0,MAXLINE);
	for (i =0; i < MD5_DIGEST_LENGTH;i++) 
		sprintf(&buf[2*i],"%02x",md5sum[i]);
	int timeout_flag = 0;
	printf("md5sum = %s\n",buf);
	strncat(buf,".html",5);
	strcpy(root_path,"cache/");
	strncat(root_path,buf,strlen(buf));
	if(check_file_present_in_cache(root_path) > 0) {
		printf("\nCACHE HIT\n");
		struct stat cached_file;
		time_t curr_time;
		stat(root_path,&cached_file);
		time(&curr_time);
		int time_diff = (int)difftime(curr_time,cached_file.st_mtime);
		printf("Last creation time of file is%d\n ",time_diff);
		if(time_diff <= cache_timeout) {
			fp = fopen(root_path,"r");	
			memset(buf,0,MAXLINE); 
			send_to_client(buf,get_file_size(fp), fp, connfd);
			fclose(fp);
			timeout_flag = 0;
		} else {
			timeout_flag = 1;
		}
	}	
	if(timeout_flag == 1) {
		fp = fopen(root_path,"a");	 
		struct hostent* host_entry;
		i = 7;
		while(req_param[1][i]!='/') {
			message[i-7] = req_param[1][i];
			i++;
		} 
		printf("\n hostname2==%s\n",message);
	
		host_entry = gethostbyname(message);
		(&main_server)->sin_family = AF_INET;
		memcpy(&(main_server.sin_addr),host_entry->h_addr,host_entry->h_length);
		(&main_server)->sin_port = htons(80);
		if ((host_sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("Socket Creation Error\n");
			exit(2);
		}
		int tval = 1;
        	if(setsockopt(host_sockfd,SOL_SOCKET, SO_REUSEADDR,&tval,sizeof(int)) < 0) {
			printf("sectsock error \n");
			exit(2);
		}
		if(connect(host_sockfd,(struct sockaddr*)&main_server,sizeof(main_server)) < 0) {
			printf("connection error \n");
			exit(2);
		}
		memset(message,0,MAXLINE);
		printf("client message to host %s\n",client_message);
		n = send(host_sockfd,client_message,sizeof(client_message),0);
		if ( n < 0) printf("error in sending\n");
		else {
			printf("getting message from server\n");
			do {
				memset(message,0,MAXLINE);
				n = recv(host_sockfd,message,sizeof(message),0);
				if(n > 0) {
					send(connfd,message,n,0);
					printf("...sending\n");
					fwrite(message,1,n,fp);
				}
			} while(n > 0);
		}
		fclose(fp);
		close(host_sockfd);

	}	
	

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
	cache_timeout = atoi(argv[1]);
	fill_err_msg_db();
	fill_proto_ver_db();
        tcp_connection_init( &sockfd, &servaddr, sizeof(servaddr));   
        pid_t childpid;
	printf("%s\n","Server running...waiting for connections.");
	process_tcp_client_request(&sockfd, &cliaddr, &clilen); 
			

}
