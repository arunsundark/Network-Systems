#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
 
#define IP_PROTOCOL 0
#define IP_ADDRESS "10.0.2.15" // localhost
#define PORT_NO 5050
#define PKT_SIZE 1024
#define cipherKey 'S'
#define sendrecvflag 0
 
// funtion to clear buffer

// function for decryption

 
// function to receive file
int recvFile(char* buf, int s,FILE *fp)
{
    int i;
    char ch;
    int nb = fwrite(buf,1,s,fp);
    printf("nb = %d \n ",nb);
 //   printf("data rxed = %s \n ",buf);   
    return 0;
}
    // driver code
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
    char requests[5][10] = { "Get","Put","Delete","List","Exit"};
    int index;
    char file_name[20];
    memset(file_name, (int)'\0',20);   
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM,
                    IP_PROTOCOL);
 
    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);
 
    while (1) {
        for(index=0; index < 5; index++)
            printf("press %d to %s operation \n ",index,requests[index]);
        int input;
        scanf("%d",&input);
        while(input < 0 || input > 4) {
            printf("invalid input \n please enter numbers from 0 to 4 \n"); 
            scanf("%d",&input);
        }
        data_buf[0] = (char)(input + 48);
        data_buf[1] = ',';
        switch(input) {
            case 0:
		printf("PLEASE ENTER THE FILE NAME \n");
		scanf("%s",file_name);
		printf("file_name = %s \n",file_name);
		strcpy(data_buf+2,file_name);
		printf("data_buf = %s \n",data_buf);
		sendto(sockfd, data_buf, PKT_SIZE,
		   0, (struct sockaddr*)&server_addr,
				    addrlen);
    	        printf("\n---------Data Received---------\n");
		fp = fopen(file_name,"w");
		while (1) {
	            // receive
		    memset(data_buf,(int)'\0',PKT_SIZE);
		    nBytes = recvfrom(sockfd, data_buf, PKT_SIZE,
				    0, (struct sockaddr*)&server_addr,
				    &addrlen);
		    printf("file rxed\n ");
			    // process
		    if(strncmp(data_buf,"end",3+1)== 0)
				    break;
		     recvFile(data_buf, PKT_SIZE,fp); 

		     if(nBytes <1024) break;
		    }  
		    fclose(fp);     
                break;
            case 1:
                break;
            default :
                break;
        }                         

		    printf("\n-------------------------------\n");
    }
    return 0;
}
