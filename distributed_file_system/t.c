#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <openssl/md5.h>

int main() {

    int n;
    MD5_CTX c;
    char buf[512];
    ssize_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];
    FILE* fp = fopen("server.c","r");
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
        printf("%02x", out[n]);
    printf("\n");
    return 0;
}
