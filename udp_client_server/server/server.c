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

typedef struct {

int pkt_num;
char data_buf[PKT_SIZE];

} udp_packet_t;




int list_all_files() {
 
        int i=0;
    	DIR *dr = opendir(".");
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


 
void get(FILE *fp,udp_packet_t* packet, struct sockaddr_in server_addr, int sockfd )
{   int server_addrlen = sizeof(server_addr);
    int udp_packet_size = sizeof(packet->pkt_num) + sizeof(packet->data_buf);
    int nb=0;int num_pkts=0;
    packet->pkt_num = 0;
    fseek(fp,0,SEEK_END);
    int file_size = ftell(fp);
    fseek(fp,0,SEEK_SET);
    printf("file_size = %d \n ",file_size);
    while(file_size > 0) {
        memset(packet->data_buf,0,PKT_SIZE);    
        fread(packet->data_buf,1,PKT_SIZE,fp);
        packet->pkt_num++;

        packet->data_buf[PKT_SIZE] = '\0';
        printf("data= %s\n ",packet->data_buf);
        nb= sendto(sockfd, packet, udp_packet_size,
               0, (struct sockaddr*)&server_addr, server_addrlen);
        nb = recvfrom(sockfd, &num_pkts, sizeof(num_pkts), 0,
                          (struct sockaddr*)&server_addr, &server_addrlen);
        while(num_pkts != packet->pkt_num) {
            nb= sendto(sockfd, packet, udp_packet_size,
               0, (struct sockaddr*)&server_addr, server_addrlen);
            nb = recvfrom(sockfd, &num_pkts, sizeof(num_pkts), 0,
                          (struct sockaddr*)&server_addr, &server_addrlen);
        }
        file_size =file_size - PKT_SIZE;  
        printf("file_size = %d \n ",file_size);
        
        printf("nb in if is %d \n",nb); 
        printf(" perror = %d \n",errno);
               
     }
     
         
}

void put(FILE *fp,char* data_buf, struct sockaddr_in server_addr, int sockfd )
{   int server_addrlen = sizeof(server_addr);
    int nb=0;int num_pkts=0;
    while(1) {

        memset(data_buf,0,PKT_SIZE);    
        nb = recvfrom(sockfd, data_buf, PKT_SIZE, 0,
                          (struct sockaddr*)&server_addr, &server_addrlen);
        if(strncmp(data_buf,"end",4)==0) break;
        fwrite(data_buf,1,PKT_SIZE,fp);
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
    udp_packet_t packet;    
    int udp_packet_size = sizeof(packet.pkt_num) + sizeof(packet.data_buf);
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
		get(fp,&packet,server_addr, sockfd);
		char end_buf[5];
                memset(packet.data_buf,0,PKT_SIZE);
		strncpy(packet.data_buf,"end",3);
                packet.pkt_num++;        
	//	do {
                    sendto(sockfd, &packet,udp_packet_size,
		        0, (struct sockaddr*)&server_addr, server_addrlen);
          //          memset(packet.data_buf,0,PKT_SIZE);
//		    recvfrom(sockfd, &packet,udp_packet_size,
//		        0, (struct sockaddr*)&server_addr, &server_addrlen);
  //              while(!(strncmp(packet.data_buf,"endok",6)==0))     
		if(fp != NULL)
		     fclose(fp);
                break;
            case 1:
                memset(file_name,0,strlen(file_name));              
		strcpy(file_name,data_buf +2);
		printf("file_name=%s\n",file_name); 
		fp = fopen(file_name, "w");
		printf("\nFile Name Received: %s\n", file_name);
		if (fp == NULL)
		    printf("\nFile open failed!\n");
		else
		    printf("\nFile Successfully opened!\n");
		sendto(sockfd, "rxrdy",6,
		        0, (struct sockaddr*)&server_addr, server_addrlen);
		put(fp,data_buf,server_addr, sockfd);
                fclose(fp);
                break;
            case 2:
                memset(file_name,0,strlen(file_name));              
		strcpy(file_name,data_buf +2);
		printf("file_name=%s\n",file_name); 
	
		printf("\nFile Name Received: %s\n", file_name);
		
		if(remove(file_name) == 0) {
                        printf(" %s is deleted from the server \n",file_name);
                        sendto(sockfd, "deleted",8, 0,
                                 (struct sockaddr*)&server_addr, server_addrlen);
		} else {
                        sendto(sockfd, "failed",8, 0,
                                 (struct sockaddr*)&server_addr, server_addrlen);
                 }
                break;
            case 3:
                memset(data_buf,0,PKT_SIZE);
                int total_files = list_all_files();
                for(int i=0;i < total_files; i++) {
                    printf("file -%d %s\n ",i+1,file_list[i]);
                    strcat(data_buf,file_list[i]);
                    strcat(data_buf,",");
                }
                printf("ls \n  %s \n ",data_buf);
                sendto(sockfd, data_buf,PKT_SIZE, 0,
                           (struct sockaddr*)&server_addr, server_addrlen);
		      
                break; 
            case 4:
                printf("server exiting .... \n ");
                return 0;
                break;
          default :
                break;
        }
    }
    return 0;
}
