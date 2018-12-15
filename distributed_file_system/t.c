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

	unsigned char digest[16];
	const char* string = "Hello World";
	struct MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, string, strlen(string));
	MD5Final(digest, &context);
	char md5string[33];
	for(int i = 0; i < 16; ++i)
	    sprintf(&md5string[i*2], "%02x", (unsigned int)digest[i]);
	printf("output =%s\n",md5string);	
	return 0;
}
