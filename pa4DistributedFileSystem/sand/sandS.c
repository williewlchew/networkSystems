#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/types.h>
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <poll.h>

#define MAXBUF   8192  /* max buffer size */
#define MAXLINE  1024  /* max I/O buffer size */



int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, MAXLINE) < 0)
        return -1;
    return listenfd;
} 


int nextWord(char* buffer, char* dest, int start, int limit)
{
    char charDump = buffer[start];
    char strDump[limit];
    int i = 0;
    int strI = 0;
    while((charDump == ' ' || charDump == '\r' || charDump == '\n') && start + i <= limit - 1)
    {
    	i++;
    	charDump = buffer[i + start];
    }

    while(!(charDump == ' ' || charDump == '\r' || charDump == '\n'||start + i >= limit - 1 ))
    {
        strDump[strI] = charDump;
        strI++;
        i++;
        charDump = buffer[i + start];
    }
    strDump[strI] = '\0';
    strcpy(dest, strDump);

    return i+1;
}

//// File handling
int getFileData(char* filename, int* breaks, int* size)
{
    FILE* fp;
    fp = fopen(filename, "r"); 
    if(fp == NULL){
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    *size = ftell(fp); 
    fseek(fp, 0L, SEEK_SET);
    *breaks = (int)(*size/4);
    fclose(fp);
    return 0;
}


int bufferToFile(char* filename, char* buffer, int fileSize, char* mode)
{
	FILE* fp;

    fp = fopen(filename, mode);
    if(fp == NULL)
        return 1;

    fwrite(buffer, 1, fileSize, fp);
    fclose(fp);
    return 0;
}

int getFileParts(char* dir, char* filename, char* buffer)
{
	FILE* fp;
	char tempPath[MAXLINE];
	strcpy(buffer, "");
	for(int i = 0; i < 4; i++)
	{
		sprintf(tempPath, "%s/%s.%d", dir, filename, i);
		fp = fopen(tempPath, "r");
		if(fp != NULL)
		{
			sprintf(buffer, "%s%s\n", buffer, tempPath);
			fclose(fp);
		}
	}
}

//// Handle requests

int recieveFilePiece(int connid, char buf, int size)
{
	char extract1[MAXLINE];
	char extract2[MAXLINE];
	char extract4[MAXLINE];
	char extract3[MAXBUF];
	int bufPtr = 0;
	int len = 0;

	len = recv(connid, extract3, MAXBUF, 0);
	bufPtr = nextWord(extract3, extract1, 0, MAXLINE);
	while(strcmp(extract1, "end") != 0)
	{
		bufPtr += nextWord(extract3, extract2, bufPtr, MAXLINE);
		bufPtr += nextWord(extract3, extract4, bufPtr, MAXLINE);
		bufferToFile(extract2, &extract3[bufPtr], atoi(extract4), extract1);

		sprintf(extract3, "ack!\n");
		send(connid, extract3, MAXBUF, 0);
		len = recv(connid, extract3, MAXBUF, 0);
		fprintf(stderr, "Processed %s bytes\n", extract4);
		bufPtr = nextWord(extract3, extract1, 0, MAXLINE);
	}
	return 0;
}



int main()
{
	int listenfd, connid, *connfdp, port, clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    struct threadArgs* args;
    pthread_t tid; 
    char buf[MAXBUF];

    port = 8080;
    listenfd = open_listenfd(port);
    connid = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);

	/////////////////////////

    recv(connid, &buf, MAXBUF, 0);
    fprintf(stderr, "%s\n", buf);

    strcpy(buf, "yea");
    send(connid, buf, MAXBUF, 0);


//	recv(connid, &buf, MAXLINE, 0);
//    fprintf(stderr, "%s\n", buf);

    recieveFilePiece(connid, buf, 3427);
    
    fprintf(stderr, "ending\n");
    return 0;
}