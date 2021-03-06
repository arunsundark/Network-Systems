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
char* my_itoa(int num, char* str) {
	sprintf(str,"%d",num);
	return str;
}

int get_file_size(FILE* fp) {
	fseek(fp,0,SEEK_END);
	int file_size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	return file_size;
}

void encdec(char* str) {
	char key = 'P';
	int len = strlen(str);
	for(int i =0; i < len; i++) {
		str[i] = str[i] ^ key;
	}
}

void send_with_pattern(char* filename,int* valid, int pattern[][2]) {
	FILE* fp = fopen(filename,"r");
	int fs = get_file_size(fp);
	int rem = fs%4;
	int read_len = fs/4;
	int nbytes = 0;int i =0;
	char* buf = (char*)malloc(read_len + rem);
	char recv_buf[10];
	memset(buf,0,read_len + rem);
	while(i < 4) {
		if(i < 3) 
			nbytes = fread(buf,1,read_len,fp);
		if(i == 3)
			nbytes = fread(buf,1,read_len + rem,fp);
		if(valid[pattern[i][0]-1] == 1) {
			//sending file content to server 'x'	
			my_itoa(i+1,recv_buf);
			send(sockfd[pattern[i][0]-1], recv_buf,strlen(recv_buf),0);
			printf("part no is=%s\n",recv_buf);
			memset(recv_buf,0,10);
			//encdec(buf);
			send(sockfd[pattern[i][0]-1], buf,nbytes,0);
			recv(sockfd[pattern[i][0]-1], recv_buf,10,0);
			printf("server ack=%s\n",recv_buf);
			memset(recv_buf,0,10);
		} else {
			printf("Invalid user. Try again\n");
		}
		if(valid[pattern[i][1]-1] == 1) {
			//sending file content to server 'y'
			my_itoa(i+1,recv_buf);
			send(sockfd[pattern[i][1]-1], recv_buf,strlen(recv_buf),0);
			memset(recv_buf,0,10);
			//encdec(buf);
			send(sockfd[pattern[i][1]-1], buf,nbytes,0);
			recv(sockfd[pattern[i][1]-1], recv_buf,10,0);
			memset(buf,0,read_len + rem);
			memset(recv_buf,0,10);

		} else {
			printf("Invalid user. Try again\n");
		}
		i++;
	}
}
		
void send_to_server(char* filename,int md5_mod, int* valid) {
	int pattern_0[4][2] = {{1,4},{1,2},{2,3},{3,4}};
	int pattern_1[4][2] = {{1,2},{2,3},{3,4},{1,4}};
	int pattern_2[4][2] = {{2,3},{3,4},{1,4},{1,2}};
	int pattern_3[4][2] = {{3,4},{1,4},{1,2},{2,3}};
	
	if( md5_mod == 0) { 
		send_with_pattern(filename, valid, pattern_0); 
	} else if (md5_mod == 1) { 
		send_with_pattern(filename, valid, pattern_1); 
	} else if (md5_mod == 2) {
		send_with_pattern(filename, valid, pattern_2);
	} else  { 
		send_with_pattern(filename, valid, pattern_3);
	}
}

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
	for(int i =0; i <MAX_SERVERS ; i++) {
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
int check_file_availability(char* filename, int* valid, char* msg_header) {
	char buf[10];int retval = 0;
	int n = 0;int active_count =0;
	for(int i = 0; i < MAX_SERVERS; i++) {
		if(dfs_active[i] == 1) {
			memset(buf,0,10);
			n = send(sockfd[i],msg_header,strlen(msg_header),0);
			n = recv(sockfd[i], buf, 10,0);
			if(strncmp(buf,"RDY",3)==0) {
				valid[i] = 1;
				active_count++;
			}
		}
	}
	retval = valid[0] * valid[2];
	if( retval == 1) {
		return (retval -1);
	} else {
		retval = valid[1] * valid[3];
		if (retval == 1) return retval; 
	}
	return -1;		
		
}
void recv_by_part(char* filename, int* part, int avail) {
	char sm_buf[32]; char tm_buf[32];
	int n =0;int fs = 0;int readlen = 0;
	strncpy(sm_buf,"FS",2);
	FILE* fp = fopen(filename,"w");
	for( int j =0; j < 3;j= j + 2) {
		n = send(sockfd[j + avail],sm_buf,strlen(sm_buf),0);
		n = recv(sockfd[j + avail],tm_buf,32,0);
	}
	fs = atoi(tm_buf);
	memset(tm_buf,0,32);
	readlen = fs + 3;
	char* buf = (char*) malloc(readlen);
	memset(buf,0,readlen);
	memset(sm_buf,0,32);
	memset(tm_buf,0,32);
	strncpy(sm_buf,"TX",2);
	for( int i =0; i < MAX_SERVERS; i++) {
		n = send(sockfd[part[i]],sm_buf,strlen(sm_buf),0);
		n = recv(sockfd[part[i]],buf,readlen,0);
		//encdec(buf);
		fwrite(buf,1,n,fp);
		memset(buf,0,readlen);
	}
	fclose(fp);
}
			
	

void recv_from_server(int avail, char* filename, int* valid) {
	char sm_buf[32];int n =0;char* tok;
	char temp_buf[32];int part[4];
	printf("in recv from server\n");
	if(avail == 0) {
		printf("in avl 0\n");
		memset(sm_buf,0,32);
		strncpy(sm_buf,"PRT",3);
		n = send(sockfd[0],sm_buf,strlen(sm_buf),0);
		n = send(sockfd[2],sm_buf,strlen(sm_buf),0);
		memset(sm_buf,0,32);
		n = recv(sockfd[0], sm_buf, 32,0);
		memset(temp_buf,0,32);
		n = recv(sockfd[2], temp_buf, 32,0);
		tok = strtok(sm_buf,",");
		part[atoi(tok) - 1] = 0;
		tok = strtok(NULL," ");
		part[atoi(tok) - 1] = 0;
		tok = NULL;
		tok = strtok(temp_buf,",");
		part[atoi(tok) - 1] = 2;
		tok = strtok(NULL,",");
		part[atoi(tok) - 1] = 2;
	}
 	if(avail == 1) {
		memset(sm_buf,0,32);
		printf("in avl 1\n");
		n = send(sockfd[1],sm_buf,strlen(sm_buf),0);
		n = send(sockfd[3],sm_buf,strlen(sm_buf),0);
		memset(sm_buf,0,32);
		n = recv(sockfd[1], sm_buf, 32,0);
		memset(temp_buf,0,32);
		n = recv(sockfd[3], temp_buf, 32,0);
		tok = strtok(sm_buf,",");
		part[atoi(tok) - 1] = 1;
		tok = strtok(NULL," ");
		part[atoi(tok) - 1] = 1;
		tok = NULL;
		tok = strtok(temp_buf,",");
		part[atoi(tok) - 1] = 3;
		tok = strtok(NULL,",");
		part[atoi(tok) - 1] = 3;
	}
	printf("part:%d,%d,%d,%d\n",part[0],part[1],part[2],part[3]);
	recv_by_part(filename, part,avail);
}
	
int get(char* filename) {
	char* msg_type = "GET";
	char* comma = ",";
	char* msg_header = (char*) malloc(PKT_SIZE);
	char* buf = (char*) malloc(PKT_SIZE);
	int valid[4];int file_avail =0;
	memset(msg_header,0,PKT_SIZE);
	strncpy(msg_header,msg_type,strlen(msg_type));
	strncat(msg_header,comma,1);	
	strncat(msg_header,username,strlen(username));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,password,strlen(password));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,filename,strlen(filename));	
	printf("Get msg:%s\n",msg_header);
	file_avail = check_file_availability(filename, valid, msg_header);
	if(file_avail < 0) {
		printf("Servers are down. Please try later\n");
		return 0;
	} else { 
		recv_from_server(file_avail, filename,valid);
		return 0;
	}
}       
int put(char* filename) {
	char* msg_type = "PUT";
	char* comma = ",";
	char msg_header[PKT_SIZE];
	char buf[PKT_SIZE];
	char md5sum[PKT_SIZE];
	FILE* fp; 
	char filepath[PKT_SIZE];int valid[4];
	int md5_mod =0;int j =0;char str[32];
	memset(msg_header,0,PKT_SIZE);
	strncpy(msg_header,msg_type,strlen(msg_type));
	strncat(msg_header,comma,1);	
	strncat(msg_header,username,strlen(username));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,password,strlen(password));	
	strncat(msg_header,comma,1);	
	strncat(msg_header,filename,strlen(filename));	
	memset(str,0,32);
	fp = fopen(filename,"r");
	int fs = get_file_size(fp);
	my_itoa(fs,str);
	fclose(fp);
	strncat(msg_header,comma,1);	
	strncat(msg_header,str,strlen(str));	
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
	md5_mod = atoi(&md5sum[strlen(md5sum)-2])%4;
	printf("md5_mod=%d\n",md5_mod);
	printf("msg_header%s*\n",msg_header);
	while(j < 4) {
		if(dfs_active[j] == 1) {
			send(sockfd[j],msg_header,strlen(msg_header),0);
			memset(buf,0,PKT_SIZE);
			recv(sockfd[j], buf, PKT_SIZE,0);
			if(strncmp(buf,"RDY",3)==0) {
				valid[j] = 1;
			}
			else if(strncmp(buf,"INV",3)==0) {
				valid[j] = 0;
			}
			else {
				valid[j] = 0;
			}
		
			printf("server message:%s\n",buf);
		}
		j++;
	}
	send_to_server(filename, 0,valid);
	memset(msg_header,0,PKT_SIZE);
	return 0;


}
int ls_util(char* msg_header) {
	char buf[10];int retval = 0;
	int n = 0;int active_count =0;
	int valid[4];
	for(int i = 0; i < MAX_SERVERS; i++) {
		if(dfs_active[i] == 1) {
			memset(buf,0,10);
			n = send(sockfd[i],msg_header,strlen(msg_header),0);
			n = recv(sockfd[i], buf, 10,0);
			if(strncmp(buf,"RDY",3)==0) {
				valid[i] = 1;
				active_count++;
			}
			else {
				valid[i] = 0;
			}
		}
	}
	retval = valid[0] * valid[2];
	if( retval == 1) {
		return (retval -1);
	} else {
		retval = valid[1] * valid[3];
		if (retval == 1) return retval; 
	}
	return -1;
}

void gen_getline(char* fn) {

	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen("list.txt", "r");
	if (fp == NULL)
		return;
	int exist = 0;
	while ((read = getline(&line, &len, fp)) != -1) {
		if(strncmp(fn,line,strlen(fn))==0) {
			exist = 1;
		}

	}
	char buf[100];
	memset(buf,0,100);
	strncpy(buf,fn,strlen(fn));
	strncat(buf,"\n",1);
	fclose(fp);
	fp = fopen("list.txt","a");
	fwrite(buf,1,strlen(buf),fp);
	fclose(fp);
	if (line)
		free(line);
	


}

void ls(){
	FILE * fp;
    	char * line = NULL;
    	size_t len = 0;
   	ssize_t read;
	char list[100][512];
	char msg_type[512];
	memset(msg_type,0,512);
	strncpy(msg_type,"LS,",3);
	strncat(msg_type,username,strlen(username));
	strncat(msg_type,",",1);
	strncat(msg_type,password,strlen(password));
	strncat(msg_type,",",1);
	char* msg_header = (char*) malloc(1024);
    	fp = fopen("list.txt", "r");
    	if (fp == NULL)
        	return ;
	int dec_var;int ct = 0;
    	while ((read = getline(&line, &len, fp)) != -1) {
        	
		strncpy(msg_header,msg_type,strlen(msg_type));
		strncat(msg_header,line,strlen(line));
		dec_var = ls_util(msg_header);
		if (dec_var >=0) {
			memset(list[ct],0,512);
			strncpy(list[ct],line,strlen(line));
			ct++;printf("%s\n",list[ct]);

		}	
        	else {
			memset(list[ct],0,512);
			strncpy(list[ct],line,strlen(line));
			strncat(list[ct],"  [Incomplete]",strlen("  [Incomplete]"));
			printf("%s\n",list[ct]);
			ct++;
 	   	}
	}
	
    	fclose(fp);
    	if (line)
        	free(line);
   
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
		printf("Enter the command\n");
		if(fgets(user_input,100,stdin) == NULL) {
            		printf("Error in getting input \n");
            		continue;
        	} else {
			tok = strtok(user_input," ");
			if(strncmp("get",tok,3)==0) {
				input_type = 1;
				strcpy(filename,strtok(NULL,"\n"));
				printf("filename *%s*\n",filename);
				gen_getline(filename);
				get(filename);
			} else if(strncmp("put",tok,3)==0) {
				input_type = 2;
				strcpy(filename,strtok(NULL,"\n"));
				printf("filename *%s*\n",filename);
				gen_getline(filename);
				put(filename);

			} else if(strncmp("ls",tok,2)==0) {
				input_type = 3;
				ls();
			} else {
				printf("Invalid command, please try again\n");
				continue;
			}
		}
	}
	return 0;
}
