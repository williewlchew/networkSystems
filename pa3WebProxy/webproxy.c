/* 
 * webproxy.c - A simple web proxy server with threads by Willie Chew
 */

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

#define MAXBUF   8192  /* max I/O buffer size */
#define MAXLINE  1024  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */


void *thread(void* vargp);

struct threadArgs {
    int* connid;
    int* expiration;
    struct CacheObject** cacheList;
    struct CacheObject** blacklist;
};

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen = sizeof(struct sockaddr_in);
    int expiration;
    struct sockaddr_in clientaddr;
    struct threadArgs* args;
    pthread_t tid; 

    if (argc < 2) {
    fprintf(stderr, "usage: %s <port> <cache expiration>\n", argv[0]);
    exit(0);
    }

    if (argc == 3) {
        expiration = atoi(argv[2]);
    }
    else
    {
        expiration = 60;
    }

    /* Initialize cache and blacklist structure */
    struct CacheObject** cacheArr[5000];
    struct CacheObject** blackArr[5000];
    int i = 0;
    while(i < 5000)
    {
        cacheArr[i] = NULL;
        blackArr[i] = NULL;
        i++;
    }
    loadBlacklist(&blackArr);
    
    port = atoi(argv[1]);
    listenfd = open_listenfd(port);
    while (1) {
        args = malloc(sizeof(struct threadArgs));
        
        args->connid = malloc(sizeof(int));
        *args->connid = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        args->expiration = malloc(sizeof(int));
        *args->expiration = expiration;
        args->cacheList = malloc(sizeof(struct CacheObject**));
        args->cacheList = &cacheArr;
        args->blacklist = malloc(sizeof(struct CacheObject**));
        args->blacklist = &blackArr;
        
        pthread_create(&tid, NULL, thread, args);
    }
}


/* 
* start a new thread to handel a new request 
*/
void* thread(void* vargp)
{   
    struct threadArgs* args = (struct threadArgs*) vargp;
    int connfd = *args->connid;
    int expiration = *args->expiration;
    struct CacheObject** cacheList = args->cacheList;
    struct CacheObject** blacklist = args->blacklist;
    pthread_detach(pthread_self()); 
    free(vargp);

    char buf[MAXBUF];
    char hostname[MAXLINE];
    char pathname[MAXLINE];
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(connfd, (struct sockaddr *)&addr, &addr_size);
    strcpy(&hostname, inet_ntoa(addr.sin_addr));

    fprintf(stderr, "%s connected\n", hostname);

    if(inBlackList(hostname, blacklist) == 1)
    {
        strcpy(&buf, "403 Forbidden");
        forwardServerResponse(connfd, &buf);
    }

    else
    {
        processClientRequest(connfd, &buf, &hostname, &pathname); 
        getServerResponse(&buf, hostname, pathname, cacheList, blacklist, expiration);    
        forwardServerResponse(connfd, &buf);
    }

    close(connfd);
    return NULL;
}


/*************************
* Opening socket connections
*************************/

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
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
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} 

int open_connectfd(char* hostname, int portno) 
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
    serveraddr.sin_port = htons(portno);
    serverlen = sizeof(serveraddr);
    status = connect(sockfd, (struct sockaddr *)&serveraddr,sizeof(serveraddr));
    if (status == -1) {
        perror("connect");
        return -1;
    }
    return sockfd;
}



/*************************
* web request processing methods 
*************************/
void processClientRequest(int connid, char* buffer, char* hostname, char* pathname)
{
	int n;
	n = read(connid, buffer, MAXLINE);
	if(n < 0)
	{
		fprintf(stderr,"error sending request to proxy %s\n", hostname);
	}

	int charPos = 0;
	char action[MAXLINE];
    char path[MAXLINE];
 	char protocol[MAXLINE];
 	char body[MAXBUF];

 	charPos += nextWord(buffer, action, charPos, n);
	charPos += nextWord(buffer, path, charPos, n);
	charPos += nextWord(buffer, protocol, charPos, n);
	getBody(buffer, body, charPos, n);

	getHostAndPath(&path, hostname, n);
    strcpy(pathname, path);

	sprintf(buffer, "%s %s %s\r\nHost: %s\r\n%s\n\n", 
		action, path, protocol, hostname, body);

	fprintf(stderr,"%s", buffer);

	return;
}

void getServerResponse(char* buffer, char* hostname, char* path, struct CacheObject** cacheList, struct CacheObject** blacklist, int expiration)
{

    if(!(buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T'))
    {
        strcpy(buffer, "403 Forbidden\n");
        return;
    }

    int result = inBlackList(hostname, blacklist);
    if(result == 1){
        strcpy(buffer, "403 Forbidden\n");
        return;
    }
    result = loadCache(buffer, hostname, path, cacheList, expiration);
    if(result == 3){
        fprintf(stderr,"Cache hit for %s\n", hostname);
        return;
    }

	int serverConn, n;
	serverConn = open_connectfd(hostname, 80);
    if(serverConn == -2)
    {
        strcpy(buffer, "404 Not Found\n");
        return;
    }

	n = send(serverConn, buffer, strlen(buffer), 0);
	if(n < 0)
	{
        strcpy(buffer, "400 Bad Request\n");
		fprintf(stderr,"error sending request to web host %s\n", hostname);
        return;
	}

	fprintf(stderr,"Sent %d bytes.\n", n);

	n = recv(serverConn, buffer, MAXBUF, 0);
	if(n < 0)
	{
        strcpy(buffer, "400 Bad Request");
		fprintf(stderr,"error receiving request to web host %s\n", hostname);
        return;
	}	

    cache(buffer, hostname, path, cacheList);
	fprintf(stderr,"Recieved %d bytes.\n", n);

	return;
}

void forwardServerResponse(int connid, char* buffer)
{
	int n;
	n = send(connid, buffer, strlen(buffer), 0);
	if(n < 0)
	{
		fprintf(stderr, "error sending request to client \n");
	}

	return;
}

/*************************
* processClientRequest parsing helpers
*************************/
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

    return i;
}

void getBody(char* buffer, char* dest, int start, int limit)
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

	while(!(charDump == '\0' || start + i >= limit - 1 ))
    {
    	strDump[strI] = charDump;
        i++;
        strI++;
        charDump = buffer[i + start];
    }
    strDump[strI] = '\0';
    strcpy(dest, strDump);

    return;
}

void getHostAndPath(char* path, char* host, int limit)
{
    int i = 0;
    int mode = 0; // 1 = in host, 2 = all others
    int currentBufPos= 0;
    while(i < limit)
    {
    	if(path[i] == '/' && mode < 2){
			if(i + 1 < limit && path[i+1] == '/')
    		{	
    			i+= 2;
    		}

    		else if(mode == 1)
	    	{
	    		host[currentBufPos] = '\0';
	    	}

    		currentBufPos = 0;
    		mode++;
    	}

    	if(mode == 1)
    	{
    		host[currentBufPos] = path[i];
    	}
    	else if(mode >= 2)
    	{
    		path[currentBufPos] = path[i];
    	}
    	currentBufPos++;
        i++;
    }

    if((currentBufPos == 0 && mode == 2) || (mode == 1)){
    	path[0] = '/';
    	path[1] = '\0';
    }
    else
    {
    	path[currentBufPos] = '\0';
    }

    return;
}


/*************************
* caching functions and objects
*************************/
struct CacheObject{
    char* host;
    char* path;
    int* accessed;
    long int* size;
    struct CacheObject* next;
} CacheObject;

int hash(char* host, char* path)
{
    unsigned long int hostHash = 1;
    unsigned long int pathHash = 1;

    int i = 0;
    while(i < strlen(host))
    {
        hostHash *= (int) host[i] - 30;
        i++;
    }

    i = 0;
    while(i < strlen(path))
    {
        pathHash *= (int) path[i] - 30;
        i++;
    }

    hostHash += pathHash;

    return (hostHash % 4999);
}

void cache(char* buffer, char* host, char* path, struct CacheObject** cacheList)
{
    struct CacheObject* newCache;
    char cacheName[MAXLINE];
    int hashCode = hash(host, path);
    sprintf(cacheName, "%d%s", hashCode, host);

    newCache = (struct CacheObject*) malloc(sizeof(struct CacheObject));
    newCache->host = (char*) malloc(MAXLINE*sizeof(char));
    newCache->path = (char*) malloc(MAXLINE*sizeof(char));
    newCache->accessed = (long int*) malloc(sizeof(long int));
    newCache->size = (int*) malloc(sizeof(int));
    newCache->next = NULL;

    strcpy(newCache->host, host);
    strcpy(newCache->path, path);

    FILE* cacheFile = fopen(cacheName, "w");
    fprintf(cacheFile, "%s", buffer);
    newCache->size = ftell(cacheFile);
    fclose(cacheFile);

    newCache->accessed = time(NULL);
    newCache->next = NULL;

    cacheList[hashCode] = newCache;
    return;
}

int loadCache(char* buffer, char* host, char* path, struct CacheObject** cacheList, int expiration)
{
    int hashCode = hash(host, path);

    if(cacheList[hashCode] == NULL)
    {
        return 0;
    }

    else if(strcmp(path, cacheList[hashCode]->path) == 0 && (time(NULL) - (long int)cacheList[hashCode]->accessed) > expiration)
    {
        return 2;
    }

    char cacheName[MAXLINE];
    sprintf(cacheName, "%d%s", hashCode, host);
    FILE* fp = fopen(cacheName, "r");
    fread(buffer, 1, cacheList[hashCode]->size, fp);
    fclose(fp);

    return 3;
}


/*************************
* processClientRequest parsing helpers
*************************/

void loadBlacklist(struct CacheObject** blackList)
{
    FILE* fp = fopen("blacklist", "r");
    char * dump = NULL;
    size_t len = 0;
    ssize_t read;

    while((read = getline(&dump, &len, fp)) != -1){
        if(dump[strlen(dump)-1] == '\n'){
            dump[strlen(dump)-1] = '\0';
        }
        cache("b", dump, "", blackList);
    }

    return;
}

int inBlackList(char* host, struct CacheObject** blackList)
{
    int hashCode = hash(host, "");

    if (blackList[hashCode] == NULL){
        return 0;
    }

    return 1;
}
