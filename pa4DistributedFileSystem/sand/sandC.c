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



int open_connectfd(char* hostname) 
{
    int sockfd, serverlen, n, status;
    struct hostent *server;
    struct sockaddr_in serveraddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        return -1;
    }
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        return -2;
    }

    /* build the server's Internet address*/
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(8080);
    serverlen = sizeof(serveraddr);
    status = connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    
    if (status == -1) {
        perror("connect");
        return -1;
    }
    return sockfd;
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

int sendFilePiece(int connid, FILE** fp, char* buf, int size)
{
	char buf2[MAXBUF];
	int start = strlen(buf);

	fread(&buf2, 1, size, fp);
	strcat(buf, buf2);

	send(connid, buf, MAXBUF, 0);
	recv(connid, buf, MAXLINE, 0);
	fprintf(stderr, "Recieved %s \n", buf);
	return 0;
}

int handelGet(int connid, char* filename)
{
	int size, dump;
	getFileData(filename, &dump, &size);
	FILE* fp;
    fp = fopen(filename,"r");

    char buf[MAXBUF];
    int sentLen = 0;
    sprintf(buf,"w+ %s", &filename[5]);
    int currentPieceSize = MAXBUF - (strlen(buf) + 6);
    if(currentPieceSize > size)
    {
    	currentPieceSize = size;
    }
    sprintf(buf,"%s %4d\n", buf, currentPieceSize);
    fprintf(stderr, "%s\n", buf);
	sendFilePiece(connid, fp, &buf, currentPieceSize);
    sentLen = currentPieceSize;
    while (sentLen < size)
    {
    	sprintf(buf,"a %s", &filename[5]);
    	currentPieceSize = MAXBUF - (strlen(buf) + 6);
    	if(currentPieceSize > size - sentLen)
	    {
	    	currentPieceSize = size - sentLen;
	    }
	    sprintf(buf,"%s %4d\n", buf, currentPieceSize);
	    fprintf(stderr, "%s\n", buf);
    	sendFilePiece(connid, fp, &buf, currentPieceSize);
    	sentLen += currentPieceSize;
    }

    strcpy(buf, "end");
    send(connid, buf, MAXBUF, 0);
    fclose(fp);
}

int main()
{
	int connid;
	connid = open_connectfd("127.0.0.1");
	char buf[MAXBUF];

	strcpy(buf, "ready?");
	send(connid, buf, MAXBUF, 0);
	
	recv(connid, &buf, MAXBUF, 0);
	fprintf(stderr, "%s\n", buf);

	strcpy(buf, "DFS1/monster");
	handelGet(connid, &buf);


	fprintf(stderr, "ending\n");
	return 0;
}