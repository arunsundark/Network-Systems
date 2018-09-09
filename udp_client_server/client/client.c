/* client.c */
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
 
#define MAX_SIZE 1024 
char commands [2][20]; 
static inline void get_commands_from_user(int argc,char** argv) {
    strncpy(commands[0],argv[1],strlen(argv[1])+1);
    printf("commands=%s \n",commands[0]);
    strncpy(commands[1],argv[2],strlen(argv[2])+1);
    printf("commands=%s \n",commands[1]);
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
    my_addr->sin_addr.s_addr = inet_addr(ip_addr); 
    my_addr->sin_port = htons(port_no); 
} 

 
// Driver code 
int main(int argc, char** argv) { 
    int socket_fd; 
    char buffer[MAX_SIZE]; 
    char requests[5][10] = { "Get","Put","Delete","List","Exit"};
    int index;
    for( index=0; index < 5; index++)
        printf("press %d to %s operation \n ",index,requests[index]);
    struct sockaddr_in   server_addr;
    get_commands_from_user(argc,argv);
    socket_fd = socket_create(PF_INET,SOCK_DGRAM,0);
    int port_no = atoi(commands[1]); 
    socket_addr_init(&server_addr,port_no,commands[0]);
  
    int n, len; 
      
    sendto(socket_fd, (const char *)requests[0], strlen(requests[0]), 
        MSG_CONFIRM, (const struct sockaddr *) &server_addr,  
            sizeof(server_addr)); 
    printf("Hello message sent.\n"); 
          
    n = recvfrom(socket_fd, (char *)buffer, MAX_SIZE,  
                MSG_WAITALL, (struct sockaddr *) &server_addr, 
                &len); 
    buffer[n] = '\0'; 
    printf("Server : %s\n", buffer); 
  
    close(socket_fd); 
    return 0; 
} 
