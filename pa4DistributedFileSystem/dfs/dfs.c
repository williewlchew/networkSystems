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

void *thread(void* vargp);

struct threadArgs {
    int* connid;
    char* serverDir;
    int* authorizedUsers;
};

///// GENERAL
unsigned int md5Hash(MD5_CTX* md5, char* word) {
    char hold[MAXLINE];
    strcpy(hold, word);

    MD5_Update(&md5,hold,strlen(hold));
    MD5_Final(&hold, &md5);

    unsigned int ret = (unsigned int)hold;
    printf("%u\n", hold);
    return ret;
}

int hashDest(char* hashInput){
    int m = 1013;
    long long hash_value = 1;
    for (int i = 0; i < strlen(hashInput); i++)
    {
        hash_value *= hashInput[i] + (i % 3);
    }
    hash_value = abs(hash_value % m);
    return hash_value;
}

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

void loadAuthorized(int users[1013])
{
	FILE* fp = fopen("dfs.config", "r");
    char * readLine = NULL;
    char line[MAXLINE];
    int read;
    int len;

    while((read = getline(&readLine, &len, fp)) != -1){
    	nextWord(readLine, line, 0, strlen(readLine));
    	fprintf(stderr, "Authorized: %s\n", line);
    	users[hashDest(line)] = 1;
    }

    return 0;
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

int recieveFilePiece(int connid, char* buf, int size)
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
		fprintf(stderr, "%s %s %s\n", extract1, extract2, extract4);
		bufferToFile(buf, &extract3[bufPtr], atoi(extract4), extract1);

		sprintf(extract3, "ack!\n");
		send(connid, extract3, size, 0);
		len = recv(connid, extract3, MAXBUF, 0);
		fprintf(stderr, "Processed %s bytes\n", extract4);
		bufPtr = nextWord(extract3, extract1, 0, MAXLINE);
	}
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
    fclose(fp);
}

void* thread(void* vargp)
{   
    struct threadArgs* args = (struct threadArgs*) vargp;
    int connid = *args->connid;
    int* authorizedUsers = args->authorizedUsers;
    char serverDir[MAXLINE];
    strcpy(serverDir, args->serverDir);
    pthread_detach(pthread_self()); 
    free(vargp);

    char buf[MAXBUF];
    char extract1[MAXLINE];
    char extract2[MAXLINE];
    int len;

    /// authorization
    len = recv(connid, &buf, MAXLINE, 0);
    char creds[MAXLINE];
    nextWord(buf, &creds, 0, MAXLINE);
    fprintf(stderr, "Credentials %s trying to connect. \n", creds, hashDest(creds));

    if(authorizedUsers[hashDest(creds)] == 0){
    	strcpy(&buf, "Unauthorized User\000");
    	fprintf(stderr, "Unauthorized User\n");
    	len = send(connid, &buf, strlen(buf), 00);
	    close(connid);
    }
    else
    {
    	strcpy(&buf, "Authorized User\000");
    	fprintf(stderr, "Authorized User\n");
    	len = send(connid, &buf, strlen(buf), 00);
    }
    bzero(buf, MAXBUF);
    
    len = recv(connid, &buf, MAXLINE, 0);

	int linePtr = 0;
	linePtr = nextWord(&buf, &extract1, linePtr, strlen(buf));
	
	if(strcmp(extract1, "get") == 0)
    {
    	fprintf(stderr, "Processing get request\n");
    	// get filename
    	linePtr += nextWord(&buf, &extract1, linePtr, strlen(buf));
    	getFileParts(serverDir,extract1, &extract2);
		char extract3[MAXLINE];

    	len = 0;
    	int limit = strlen(extract2)-1;
    	while(len < limit)
	    {
	        len += nextWord(&extract2, &extract3, len, MAXLINE);
	        fprintf(stderr, "Sending %s\n", &extract3);
	        handelGet(connid, &extract3);
	    }

	    strcpy(buf, "end\n");
	    len = send(connid, &buf, strlen(buf), 0);
	    fprintf(stderr, "Sent %s data\n", extract1);
	    bzero(buf, MAXBUF);
    }

    else if (strcmp(extract1, "put") == 0)
    {
    	// get filename
    	linePtr += nextWord(&buf, &extract1, linePtr, strlen(buf));
    	// get size
    	linePtr += nextWord(&buf, &extract2, linePtr, strlen(buf));

    	char filepath[MAXLINE];
    	sprintf(filepath, "%s/%s", serverDir, extract1);
    	fprintf(stderr, "Processing put request for %s\n", extract1);
    	send(connid, filepath, strlen(buf), 0);
    	recieveFilePiece(connid, filepath, MAXLINE);

    	sprintf(&buf, "Successfully saved %s \n", filepath);
    	len = send(connid, &buf, strlen(buf), 0);

    	fprintf(stderr, "%s\n", buf);
    	bzero(buf, MAXBUF);
    }

    else if (strcmp(extract1, "list") == 0)
    {
    	strcpy(&buf, "Unable to interpret request\n");
    	len = send(connid, &buf, strlen(buf), 0);
    }

    else
    {
    	strcpy(&buf, "Finnished or unable to interpret request\n");
    	fprintf(stderr, "Closing connection\n");
    	len = send(connid, &buf, strlen(buf), 0);
    	bzero(buf, MAXBUF);
    }
    bzero(buf, MAXBUF);
    close(connid);
   return NULL;
}


int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    struct threadArgs* args;
    pthread_t tid; 

    if (argc < 2) {
	    fprintf(stderr, "usage: %s <directory> <port>\n", argv[0]);
	    exit(0);
    }

    /* Initialize authorized users */
    int authorizedUsers[1013];
	loadAuthorized(authorizedUsers);
    
    port = atoi(argv[2]);
    listenfd = open_listenfd(port);
    while (1) {
        args = malloc(sizeof(struct threadArgs));
        
        args->connid = malloc(sizeof(int));
        *args->connid = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        
        args->serverDir = malloc(sizeof(char)*MAXLINE);
        strcpy(args->serverDir, argv[1]);
        args->authorizedUsers = malloc(sizeof(int*));
        args->authorizedUsers = &authorizedUsers;
        
        pthread_create(&tid, NULL, thread, args);
    }
}