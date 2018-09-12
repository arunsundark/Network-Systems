 /* server */
#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <dirent.h>  
#include <fcntl.h>
#define MAX_SIZE 1024 
FILE *foo1;
FILE *foo2;
FILE *foo3;
struct dirent *home_dirent;   
char commands[20];
char file_list [10][10]; 
    
static inline void get_commands_from_user(int argc,char** argv) {
    strncpy(commands,argv[1],strlen(argv[1])+1);
}

static inline int socket_create(int _domain, int _type, int _protocol) {
    int socket_fd = socket(_domain, _type, _protocol);
    if( socket_fd < 0) {
       printf("Error in socket creation \n ");
       exit(1);
 
    } else {
    return socket_fd; 
    }
}


static inline void socket_addr_init(struct sockaddr_in* my_addr,uint16_t port_no ) {
    // Filling server information 
    memset(my_addr, 0, sizeof(*my_addr)); 
    my_addr->sin_family    = AF_INET; // IPv4 
    my_addr->sin_addr.s_addr = htonl(INADDR_ANY); 
    my_addr->sin_port = htons(port_no); 
} 

static inline void socket_bind(int socket_fd, struct sockaddr_in* my_addr) {
     
    // Bind the socket with the server address 
    if ( bind(socket_fd, (const struct sockaddr *)my_addr,  
            sizeof(*my_addr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
} 
int server_filesystem_init() {

    foo1 = fopen("foo1.txt","w+");
    foo2 = fopen("foo2.txt","w+");
    foo3 = fopen("foo3.txt","w+");

    fprintf(foo1,"%s","I am foo1.txt. I am used for netsys pa-1");
    fprintf(foo2,"%s","I am foo2.txt. I am used for netsys pa-1");
    fprintf(foo3,"%s","I am foo3.txt. I am used for netsys pa-1");
  
    fclose(foo1);
    fclose(foo2);
    fclose(foo3);
 
    return 0;
    
}


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




// Driver code 
int main(int argc, char** argv) { 
    int socket_fd;
    char buffer[32]; 
    char* hello = "fuck my life"; 
    struct sockaddr_in server_addr, client_addr; 
    get_commands_from_user(argc,argv); 
    socket_fd = socket_create(PF_INET, SOCK_DGRAM, 0);
    int port_no = atoi(commands);
    socket_addr_init(&server_addr,port_no); 
    memset(&client_addr, 0, sizeof(client_addr)); 
    printf("working until here \n ");
    int total_files = list_all_files();
    for(int i=0;i < total_files; i++)
    printf("file -%d %s\n ",i+1,file_list[i]);
    socket_bind(socket_fd, &server_addr); 
    uint32_t len; 
    int n;
    n = recvfrom(socket_fd, (char *)buffer, MAX_SIZE,  
                0, ( struct sockaddr *) &client_addr, 
                &len); 
    buffer[n] = '\0'; 
    printf("Client : %s\n", buffer); 
    

    FILE* fp4;   
    fp4 = fopen("database/foo4.txt","r");

    char temp_buf[1024];
    memset(temp_buf, (int)'\0',1024);
    fseek(fp4,0L,SEEK_END);
    int sz = ftell(fp4);
    fseek(fp4,0L,SEEK_SET);  
    printf(" sz = %d \n",sz);
    fread(temp_buf,sz,1,fp4);
    printf("%s \n",temp_buf);
    fclose(fp4);  
    sendto(socket_fd, (const char *)temp_buf, strlen(temp_buf),  
        0, (const struct sockaddr *) &client_addr, 
            len); 
     
    return 0; 
} 
