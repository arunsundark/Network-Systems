#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h> 
#include <openssl/md5.h>
#include <errno.h>
#define IP_PROTOCOL 0
#define PKT_SIZE 1024
#define MAX_SERVERS 4
#define SA struct sockaddr
#define MD5_DB "md5_db/"
int sockfd[MAX_SERVERS]; 
struct sockaddr_in dfs_addr[MAX_SERVERS]; 
char conf_details[6][50];
char username[25];
char password[25];
char dfs_ip[MAX_SERVERS][25];
char dfs_port[MAX_SERVERS][10];
int dfs_active[MAX_SERVERS];

int check_file_present_in_cache(char* path_name) {
	if(access(path_name, F_OK) !=-1) return 1;
	else return -1;
}
void get_configuration() {
	FILE* fp = fopen("dfc.conf","r");
	char * line = NULL;
        size_t len = 0;
        ssize_t read;
	int i = 0;int j =0;
	char* tok;
	char buf[50];
        if(fp == NULL) {
		exit(EXIT_FAILURE);
	}
	while ((read = getline(&line, &len, fp)) != -1) {
        	memset(conf_details[i],'\0',50);
		strncpy(conf_details[i],line,read);
    		i++;
	}
	while(j < 4) {
		i = 0;
		tok = strtok(conf_details[j]," ");
		while((tok != NULL)  && (i<3 )) {
			memset(buf,0,50);
			strcpy(buf,tok);
			tok = strtok(NULL," ");
			i++;
		}
		memset(dfs_ip[j],0,25);
		strcpy(dfs_ip[j],strtok(buf,":"));
		memset(dfs_port[j],0,10);
		strncpy(dfs_port[j],strtok(NULL," "),5);
		j++;
	}
	memset(username,0,25);
	strtok(conf_details[4],":");
	tok = strtok(NULL," ");
	strncpy(username,tok,strlen(tok));
	memset(password,0,25);
	strtok(conf_details[5],":");
	tok = strtok(NULL,"\n");
	strncpy(password,tok,strlen(tok));
	fclose(fp);
    	if (line)
        	free(line);
}


int tcp_connection_init() {
	for(int i =0; i <1 ; i++) {
		// socket create and varification 
    		sockfd[i] = socket(AF_INET, SOCK_STREAM, 0); 
    		if (sockfd[i] == -1) { 
        		printf("socket creation failed for Server %d\n", i); 
        		dfs_active[i] = 0; 
    		} else {
        		printf("Socket successfully created for Server %d\n",i); 
			dfs_active[i] = 1;
		}    
		bzero(&dfs_addr[i], sizeof(dfs_addr[i])); 
  
    		// assign IP, PORT 
    		dfs_addr[i].sin_family = AF_INET; 
    		dfs_addr[i].sin_addr.s_addr = inet_addr(dfs_ip[i]); 
    		dfs_addr[i].sin_port = htons(atoi(dfs_port[i])); 
  
    		// connect the client socket to server socket 
    		if (connect(sockfd[i], (SA*)&dfs_addr[i], sizeof(dfs_addr[i])) != 0) { 
        		printf("connection with the Server %d failed...\n",i);
			printf("Error is:%s\n",strerror(errno));
			dfs_active[i] = 0; 
        		return 0; 
    		} else {
        		printf("connected to the server %d..\n",i); 
			dfs_active[i] = 1;
		}
	}
}
void calculate_md5sum(char* filename, unsigned char* md5sum) {
	int n;
	MD5_CTX c;
	char buf[512];
	ssize_t bytes;
	unsigned char out[MD5_DIGEST_LENGTH];
	FILE* fp = fopen(filename,"r");
	MD5_Init(&c);
	bytes=fread(buf,1,512,fp);
	while(bytes > 0)
	{
		MD5_Update(&c, buf, bytes);
		bytes=fread(buf,1,512,fp);
	}
	MD5_Final(out, &c);
	fclose(fp);
	for(n=0; n<MD5_DIGEST_LENGTH; n++)
		sprintf(&md5sum[2*n],"%02x", out[n]);
	printf("\n");
}	
int get(char* filename) {
	char* msg_type = "GET";
	char* comma = ",";
	char* msg_header = (char*) malloc(1024);
	char* buf = (char*) malloc(1024);
	memset(msg_header,0,1024);
	strncpy(msg_header,msg_type,strlen(msg_type));
	strncat(msg_header,comma,1);	
	strncat(msg_header,username,strlen(username));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,password,strlen(password));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,filename,strlen(filename));	
	send(sockfd[0],msg_header,strlen(msg_header),0);
	memset(msg_header,0,1024);
	memset(buf,0,1024);
	recv(sockfd[0], buf, 100,0);
	return 0;
}       
int put(char* filename) {
	char* msg_type = "PUT";
	char* comma = ",";
	char msg_header[PKT_SIZE];
	char buf[PKT_SIZE];
	char md5sum[PKT_SIZE];
	FILE* fp; 
	char filepath[PKT_SIZE];
	int md5_mod =0;
	memset(msg_header,0,PKT_SIZE);
	strncpy(msg_header,msg_type,strlen(msg_type));
	strncat(msg_header,comma,1);	
	strncat(msg_header,username,strlen(username));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,password,strlen(password));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,filename,strlen(filename));	
	calculate_md5sum(filename,md5sum);
	memset(filepath,0,PKT_SIZE);
	strncpy(filepath,MD5_DB,strlen(MD5_DB));
	strncat(filepath,md5sum,strlen(md5sum));
	strncat(filepath,".txt",strlen(".txt"));
	if(check_file_present_in_cache(filepath) == -1) {
		fp = fopen(filepath,"a");
		fwrite(filename,1,strlen(filename),fp);
		fclose(fp);
	}
	md5_mod = atoi(&md5sum[strlen(md5sum)-2]);
	printf("md5_mod=%d\n",md5_mod);
	printf("msg_header%s*\n",msg_header);
	send(sockfd[0],msg_header,strlen(msg_header),0);
	memset(msg_header,0,PKT_SIZE);
	memset(buf,0,PKT_SIZE);
	printf("no seg\n");
	recv(sockfd[0], buf, PKT_SIZE,0);
	printf("server message:%s\n",buf);
	
	return 0;


}
int main(int argc, char** argv)
{	char user_input[100];
	get_configuration();
	tcp_connection_init();
	int input_type = 0;
	char* tok = NULL;
	char filename[50];
	while(1) {
		memset(user_input,0, 100);
		if(fgets(user_input,100,stdin) == NULL) {
            		printf("Error in getting input \n");
            		continue;
        	} else {
			tok = strtok(user_input," ");
			if(strncmp("get",tok,3)==0) {
				input_type = 1;
				strcpy(filename,strtok(NULL,"\n"));
				printf("filename *%s*\n",filename);
				get(filename);
			} else if(strncmp("put",tok,3)==0) {
				input_type = 2;
				strcpy(filename,strtok(NULL,"\n"));
				printf("filename *%s*\n",filename);
				put(filename);

			} else if(strncmp("ls",tok,2)==0) {
				input_type = 3;
			} else {
				printf("Invalid command, please try again\n");
				continue;
			}
		}
	}
	return 0;
}
