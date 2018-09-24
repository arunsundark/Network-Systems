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
#define IP_PROTOCOL 0
#define PKT_SIZE 1024

struct timeval timeout;
typedef struct {

int pkt_num;
char data_buf[PKT_SIZE];

} udp_packet_t;




void put(FILE *fp,udp_packet_t* packet,struct sockaddr_in server_addr, int sockfd) {
     int server_addrlen = sizeof(server_addr);
     int nb =0; int num_pkts = 0;
     int udp_packet_size= sizeof(packet->pkt_num) + sizeof(packet->data_buf);
     packet->pkt_num = 0;
     fseek(fp,0,SEEK_END);
     int file_size = ftell(fp);
     fseek(fp,0,SEEK_SET);
     printf("file_size = %d \n ",file_size);
     timeout.tv_sec =1;
     timeout.tv_usec =0;
     setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)); 
     while(file_size > 0) {
         memset(packet->data_buf,0,PKT_SIZE);
         fread(packet ->data_buf,1,PKT_SIZE,fp);
         packet->pkt_num++;
         packet->data_buf[PKT_SIZE] = '\0';
         nb = sendto(sockfd, packet,udp_packet_size, 0,
                        (struct sockaddr*)&server_addr, server_addrlen);
         nb = recvfrom(sockfd, &num_pkts,sizeof(num_pkts), 0,
                        (struct sockaddr*)&server_addr,&server_addrlen);
         while(num_pkts != packet->pkt_num) {
             nb = sendto(sockfd, packet,udp_packet_size, 0,
                        (struct sockaddr*)&server_addr, server_addrlen);
             nb = recvfrom(sockfd, &num_pkts,sizeof(num_pkts), 0,
                        (struct sockaddr*)&server_addr, &server_addrlen);
         }

         file_size = file_size - PKT_SIZE;
         printf("file_size = %d \n",file_size);
         num_pkts++;
     }
    timeout.tv_sec =1;
    timeout.tv_usec =0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)); 
    
}  

int get(udp_packet_t* packet,FILE *fp,struct sockaddr_in server_addr,int sockfd) {
    int addrlen = sizeof(server_addr);  int nb=0;int num_pkts=0;
    int exp_pkt_num=0;
    int udp_packet_size= sizeof(packet->pkt_num) + sizeof(packet->data_buf);
    timeout.tv_sec =1;
    timeout.tv_usec =0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)); 
    while (1) {
	 
	memset(packet->data_buf,(int)'\0',PKT_SIZE);
        exp_pkt_num++;
	nb = recvfrom(sockfd,packet, udp_packet_size,
		    0, (struct sockaddr*)&server_addr, &addrlen);
        if(strncmp(packet->data_buf,"nofile",7)==0){
            printf("File not found\n");
            break;
        }
         
        num_pkts = packet->pkt_num;
        if(strncmp(packet->data_buf,"end",3+1)== 0)
	    break;
	

        nb = sendto(sockfd,&num_pkts ,sizeof(num_pkts), 0,
                        (struct sockaddr*)&server_addr, addrlen);
         
        while(exp_pkt_num != num_pkts) {
            nb = recvfrom(sockfd,packet, udp_packet_size,
		    0, (struct sockaddr*)&server_addr, &addrlen);
            num_pkts = packet->pkt_num;
            printf("rxed packet->pkt_num=%d\n",packet->pkt_num);
            printf("expt exp_pkt_num=%d\n",exp_pkt_num);

            nb = sendto(sockfd,&num_pkts ,sizeof(num_pkts), 0,
                        (struct sockaddr*)&server_addr, addrlen);
        } 
        nb  = fwrite(packet->data_buf,1,PKT_SIZE,fp);
        printf("nb = %d \n ",nb);
       // printf("data rxed = %s \n ",packet->data_buf);   
    }
    return 0;
    timeout.tv_sec =1;
    timeout.tv_usec =0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)); 
    
}
 
 
int main(int argc, char** argv)
{
    int sockfd, nBytes;
    struct sockaddr_in server_addr;
    int addrlen = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    char data_buf[PKT_SIZE];
    FILE* fp;
    char requests[5][25] = { "get [file name]","put [file name]","delete [file name]","ls","exit"};
    int index,j;
    udp_packet_t packet;
    char file_name[20];
    char com_buf[20];
    memset(file_name, (int)'\0',20); 
    int udp_packet_size= sizeof(packet.pkt_num) + sizeof(packet.data_buf);
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM,
                    IP_PROTOCOL);
   
    if (sockfd < 0)
        printf("Socket cannot be created\n");
    else
        printf("Socket %d is created\n", sockfd);
    printf("Server is Online \n");
   
    while (1) {
        printf("Enter any of the following commands\n");        
        for(index =0; index < 5;index++)
            printf("%s\n",requests[index]);
        memset(data_buf,0,PKT_SIZE);
        if(fgets(data_buf,25,stdin) == NULL) {
            printf("Error in getting input \n");
            continue;
        }
        index =0;
        while(data_buf[index] != ' ' && data_buf[index]!='\n') {
            com_buf[index]=data_buf[index];
            index++;
        }
        com_buf[index] = '\0';
        int flag =(strncmp(com_buf,"get",4) ==0) || (strncmp(com_buf,"put",4) ==0)
                       || (strncmp(com_buf,"delete",7) ==0) || (strncmp(com_buf,"ls",3) ==0)
                       || (strncmp(com_buf,"exit",5) ==0);
        if(flag !=1) {
        printf("command  %s is not found\n",data_buf);
        continue;
        }
        index++;
        j=0;
        int input = (int)com_buf[0];
        if(input != 108 && input != 101) {
            while(data_buf[index] != '\n') {
                file_name[j]=data_buf[index];
                j++;
                index++;
            }
        file_name[j]='\0';
        }
        memset(data_buf,0,PKT_SIZE);
        switch(input) {
            case 103:
	        data_buf[0] = '0';
                data_buf[1] = ',';
		strcpy(data_buf+2,file_name);
		sendto(sockfd, data_buf, PKT_SIZE,
		   0, (struct sockaddr*)&server_addr,
				    addrlen);
    	        printf("\n---------Data Received---------\n");
		fp = fopen(file_name,"w");
                get(&packet,fp,server_addr,sockfd);
	        fclose(fp);     
                break;
            case 112:
                data_buf[0] = '1';
                data_buf[1] = ',';
		strcpy(data_buf+2,file_name);
		sendto(sockfd, data_buf, PKT_SIZE,
		   0, (struct sockaddr*)&server_addr, addrlen);
                memset(data_buf,0,PKT_SIZE);
                
                nBytes = recvfrom(sockfd, data_buf, PKT_SIZE,
				    0, (struct sockaddr*)&server_addr, &addrlen);
		if(strncmp(data_buf,"rxrdy",6)==0) {
             	    fp = fopen(file_name,"r");
                    if(fp ==NULL) {
                    memset(packet.data_buf,0,PKT_SIZE);
                    strncpy(packet.data_buf,"nofile",7);
                    sendto(sockfd, &packet,udp_packet_size, 0,
                            (struct sockaddr*)&server_addr, addrlen);
                    printf("file not found\n");
                    break;
                    }
                    memset(packet.data_buf,0,PKT_SIZE);
                    put(fp,&packet,server_addr,sockfd);
                    memset(packet.data_buf,0,PKT_SIZE);
                    strncpy(packet.data_buf,"end",4);
                    sendto(sockfd, &packet,udp_packet_size, 0,
                            (struct sockaddr*)&server_addr, addrlen);
                    fclose(fp); 
                }
		break;
            case 100:
                data_buf[0] = '2';
                data_buf[1] = ',';
		strcpy(data_buf+2,file_name);
		sendto(sockfd, data_buf, PKT_SIZE,
		   0, (struct sockaddr*)&server_addr, addrlen);
                memset(data_buf,0,PKT_SIZE);
                nBytes = recvfrom(sockfd, data_buf, PKT_SIZE,
				    0, (struct sockaddr*)&server_addr, &addrlen);
		if(strncmp(data_buf,"deleted",8)==0) {
                    printf("%s is deleted successfully \n ",file_name);      
                } else {
                    printf("%s cannot deleted. Operation failed \n",file_name);
                }
                break;
            case 108:
       	        data_buf[0] = '3';
                data_buf[1] = ',';
		sendto(sockfd, data_buf, PKT_SIZE,
		        0, (struct sockaddr*)&server_addr, addrlen);
                memset(data_buf,0,PKT_SIZE);
                nBytes = recvfrom(sockfd, data_buf, PKT_SIZE,
				    0, (struct sockaddr*)&server_addr, &addrlen);
	         int index =0;
                 for(index = 0;data_buf[index] != '\0';index ++) {
                     if(data_buf[index] != ',') 
                         printf("%c",data_buf[index]);  	
                     else printf("\n");
                 }
                 break; 
             case 101:
                 data_buf[0] = '4';
                 data_buf[1] = ',';
		 sendto(sockfd, data_buf, PKT_SIZE,
		        0, (struct sockaddr*)&server_addr, addrlen);
                 memset(data_buf,0,PKT_SIZE);
                 nBytes = recvfrom(sockfd, data_buf, PKT_SIZE,
				    0, (struct sockaddr*)&server_addr, &addrlen);
	         break; 
            default :
                break;
        }                         

		    printf("\n-------------------------------\n");
    }
    return 0;
}
