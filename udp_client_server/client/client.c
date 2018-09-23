#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
 
#define IP_PROTOCOL 0
#define PKT_SIZE 1024


void put(FILE *fp,char* data_buf,struct sockaddr_in server_addr, int sockfd) {
     int server_addrlen = sizeof(server_addr);
     int nb =0; int num_pkts = 0;
     fseek(fp,0,SEEK_END);
     int file_size = ftell(fp);
     fseek(fp,0,SEEK_SET);
     printf("file_size = %d \n ",file_size);
     while(file_size > 0) {
         memset(data_buf,0,PKT_SIZE);
         fread(data_buf,1,PKT_SIZE,fp);
         data_buf[PKT_SIZE] = '\0';
         nb = sendto(sockfd, data_buf,PKT_SIZE, 0,
                        (struct sockaddr*)&server_addr, server_addrlen);
         file_size = file_size - PKT_SIZE;
         printf("file_size = %d \n",file_size);
         num_pkts++;
     }

}  

int get(char* data_buf,FILE *fp,struct sockaddr_in server_addr,int sockfd) {
    int addrlen = sizeof(server_addr);  int nb=0;
    while (1) {
	 
	memset(data_buf,(int)'\0',PKT_SIZE);
	nb = recvfrom(sockfd, data_buf, PKT_SIZE,
		    0, (struct sockaddr*)&server_addr, &addrlen);
        printf("file rxed\n ");
	if(strncmp(data_buf,"end",3+1)== 0)
	    break;
	nb  = fwrite(data_buf,1,PKT_SIZE,fp);
        printf("nb = %d \n ",nb);
        printf("data rxed = %s \n ",data_buf);   
    }
    return 0;

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
    char file_name[20];
    char com_buf[20];
    memset(file_name, (int)'\0',20);   
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM,
                    IP_PROTOCOL);
   
    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);
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
        printf("db=%sxx", data_buf); 
        index =0;
        while(data_buf[index] != ' ' && data_buf[index]!='\n') {
            com_buf[index]=data_buf[index];
            index++;
        }
        com_buf[index] = '\0';
        
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
        printf("com_buf=%s--\n",com_buf);
        printf("file_name=%s--\n",file_name);
        
        }
        memset(data_buf,0,PKT_SIZE);
        switch(input) {
            case 103:
	        data_buf[0] = '0';
                data_buf[1] = ',';
		strcpy(data_buf+2,file_name);
		printf("data_buf = %s \n",data_buf);
                 
		sendto(sockfd, data_buf, PKT_SIZE,
		   0, (struct sockaddr*)&server_addr,
				    addrlen);
    	        printf("\n---------Data Received---------\n");
		fp = fopen(file_name,"w");
                get(data_buf,fp,server_addr,sockfd);
	        fclose(fp);     
                break;
            case 112:
                data_buf[0] = '1';
                data_buf[1] = ',';
		strcpy(data_buf+2,file_name);
		printf("data_buf = %s \n",data_buf);
		sendto(sockfd, data_buf, PKT_SIZE,
		   0, (struct sockaddr*)&server_addr, addrlen);
                memset(data_buf,0,PKT_SIZE);
                nBytes = recvfrom(sockfd, data_buf, PKT_SIZE,
				    0, (struct sockaddr*)&server_addr, &addrlen);
		if(strncmp(data_buf,"rxrdy",6)==0) {
             	    fp = fopen(file_name,"r");
                    put(fp,data_buf,server_addr,sockfd);
                    sendto(sockfd, "end", 4, 0,
                            (struct sockaddr*)&server_addr, addrlen);
                    fclose(fp); 
                }
		break;
            case 100:
                data_buf[0] = '2';
                data_buf[1] = ',';
		strcpy(data_buf+2,file_name);
		printf("data_buf = %s \n",data_buf);
		sendto(sockfd, data_buf, PKT_SIZE,
		   0, (struct sockaddr*)&server_addr, addrlen);
                memset(data_buf,0,PKT_SIZE);
                nBytes = recvfrom(sockfd, data_buf, PKT_SIZE,
				    0, (struct sockaddr*)&server_addr, &addrlen);
		if(strncmp(data_buf,"deleted",8)==0) {
                    printf("%s is deleted successfully \n ",file_name);      
                } else {
                    printf("%s cannot deleted. OPeration failed \n",file_name);
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
