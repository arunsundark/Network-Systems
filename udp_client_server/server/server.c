 /* server */
#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  

#define MAX_SIZE 1024 

char commands[20];

static inline void get_commands_from_user(int argc,char** argv) {
    strncpy(commands,argv[1],strlen(argv[1])+1);
    printf("commands=%s \n",commands);
}

static inline int socket_create(int _domain, int _type, int _protocol) {
    int socket_fd = socket(_domain, _type, _protocol);
    if( socket_fd < 0) {
       printf("Error in socket creation \n ");
       exit(1);
 
    } else {
    printf(" socket in function = %d", socket_fd);
    return socket_fd; 
    }
}


static inline void socket_addr_init(struct sockaddr_in* my_addr,uint16_t port_no, char* ip_addr) {
    // Filling server information 
    memset(my_addr, 0, sizeof(*my_addr)); 
    my_addr->sin_family    = AF_INET; // IPv4 
    my_addr->sin_addr.s_addr = htonl(INADDR_ANY); 
    my_addr->sin_port = htons(port_no); 
} 

  
// Driver code 
int main(int argc, char** argv) { 
    int socket_fd; 
    char buffer[MAX_SIZE]; 
    char *hello = "Hello from server"; 
    struct sockaddr_in server_addr, client_addr; 
    get_commands_from_user(argc,argv); 
    socket_fd = socket_create(PF_INET, SOCK_DGRAM, 0);
    printf("socket in main =%d ", socket_fd); 
    int port_no = atoi(commands);
    printf("%d \n",port_no);
    char ip_addr[] ="10.0.2.20";
    socket_addr_init(&server_addr,port_no,ip_addr); 
    memset(&client_addr, 0, sizeof(client_addr)); 
      
     
    // Bind the socket with the server address 
    if ( bind(socket_fd, (const struct sockaddr *)&server_addr,  
            sizeof(server_addr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    uint32_t len; 
    int n; 
   // printf("ip_addr =%s \n", inet_ntoa(server_addr.sin_addr));
    n = recvfrom(socket_fd, (char *)buffer, MAX_SIZE,  
                MSG_WAITALL, ( struct sockaddr *) &client_addr, 
                &len); 
    buffer[n] = '\0'; 
    printf("Client : %s\n", buffer); 
    sendto(socket_fd, (const char *)hello, strlen(hello),  
        MSG_CONFIRM, (const struct sockaddr *) &client_addr, 
            len); 
    printf("Hello message sent.\n");  
      
    return 0; 
} 
