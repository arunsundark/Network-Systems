#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>


 
#define IP_PROTOCOL 0
#define PORT_NO 5050
#define PKT_SIZE 1024
#define cipherKey 'S'
#define sendrecvflag 0
#define nofile "File Not Found!"
struct dirent *home_dirent;   
char commands[20];
char file_list [10][10]; 





/*
int list_all_files() {
 
        int i=0;
    	DIR *dr = opendir("database");
        char check_buf [4];
        if(dr==NULL) {
        printf("error in opening directory \n");
        return -1;
        }
        
        while((home_dirent = readdir(dr)) != NULL) {
               // strncpy(check_buf, home_dirent->d_name, strlen(home_dirent->d_name)+1);
                if(home_dirent->d_name[0] !='.') {
                    strncpy(file_list[i], home_dirent->d_name, strlen(home_dirent->d_name)+1);
                    i++;
                    printf("%s \n",home_dirent->d_name);
                }
        }

        return i;
}
*/
/*   int total_files = list_all_files();
    for(int i=0;i < total_files; i++)
    printf("file -%d %s\n ",i+1,file_list[i]);
           req_buf[0] = buffer[0];
         //   j++;
       // } 
       printf(" buffer +2 = %s",(buffer + 2));
       printf(" strlen(buffer) = %d",strlen(buffer));
       switch(atoi(req_buf)) {
            case 0:
                memset(file_name,0,strlen(file_name));              
                for(j=2;j< strlen(buffer);j++) {
                file_name[j-2] = buffer[j];
                } 
               // strcpy((buffer + 2),file_name);
                printf("file_name=%s\n",file_name); 
  */

// funtion to encrypt
char Cipher(char ch)
{
    return ch ^ cipherKey;
}
 
// funtion sending file
/*int sendFile(FILE* fp, char* buf, int s)
{
    int i, len;
    if (fp == NULL) {
        strcpy(buf, nofile);
        len = strlen(nofile);
        buf[len] = EOF;
    //    for (i = 0; i <= len; i++)
    //        buf[i] = Cipher(buf[i]);
        return 1;
    }
 
    char ch, ch2;
    for (i = 0; i < s; i++) {
        ch = fgetc(fp);
      //  ch2 = Cipher(ch);
        buf[i] = ch;
        if (ch == EOF)   {
            printf("found EOF \n");
            printf("data= %s\n ",buf);
            return 1;
        }
    }
             printf("data= %s\n ",buf);
    return 0;
}*/
void get(FILE *fp,char* data_buf, struct sockaddr_in server_addr, int sockfd )
{   int server_addrlen = sizeof(server_addr);
    int nb=0;int num_pkts=0;
    fseek(fp,0,SEEK_END);
    int file_size = ftell(fp);
    fseek(fp,0,SEEK_SET);
    printf("file_size = %d \n ",file_size);
    while(file_size > 0) {
        memset(data_buf,0,PKT_SIZE);    
        fread(data_buf,1,PKT_SIZE,fp);
        data_buf[PKT_SIZE] = '\0';
        printf("data= %s\n ",data_buf);
        nb= sendto(sockfd, data_buf, PKT_SIZE,
               0, (struct sockaddr*)&server_addr, server_addrlen);
        file_size =file_size - PKT_SIZE;  
        printf("file_size = %d \n ",file_size);
    num_pkts++;    
             printf("nb in if is %d \n",nb); 
             printf(" perror = %d \n",errno);
               
     }
     
         
}
        
// driver code
int main(int argc, char** argv)
{
    int sockfd, nBytes;
    struct sockaddr_in server_addr;
    int server_addrlen = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    int port_no = atoi(argv[1]);
    server_addr.sin_port = htons(port_no);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    char* data_buf= (char*)malloc(PKT_SIZE * sizeof(char));
    FILE* fp;
    char file_name[20];
        
    // socket()
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
 
    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);
 
    // bind()
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0)
        printf("\nSuccessfully binded!\n");
    else
        printf("\nBinding Failed!\n");
 
    while (1) {
        printf("\nWaiting for file name...\n");
 
        // receive file name
        
        memset(data_buf,(int)'\0',PKT_SIZE);
        nBytes = recvfrom(sockfd, data_buf,
                          PKT_SIZE, 0,
                          (struct sockaddr*)&server_addr, &server_addrlen);
        printf(" rxed data = %s \n ",data_buf);
        char req_buf[3];
        req_buf[0] = data_buf[0];
        switch(atoi(req_buf)) {
            case 0:
		memset(file_name,0,strlen(file_name));              
		strcpy(file_name,data_buf +2);
		printf("file_name=%s\n",file_name); 
		fp = fopen(file_name, "r");
		printf("\nFile Name Received: %s\n", file_name);
		if (fp == NULL)
		    printf("\nFile open failed!\n");
		else
		    printf("\nFile Successfully opened!\n");
		get(fp,data_buf,server_addr, sockfd);
		char end_buf[5];
		strncpy(end_buf,"end",3);
		sendto(sockfd, end_buf,4,
		        0, (struct sockaddr*)&server_addr, server_addrlen);
		if(fp != NULL)
		     fclose(fp);
                break;
           default :
                break;
        }
    }
    return 0;
}
