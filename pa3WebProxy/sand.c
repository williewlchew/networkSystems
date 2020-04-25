#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h> 
#include <dirent.h>

#define BUFSIZE 30000
#define MAXLINE  1024

/**************************************************************/
void connectTest()
{
	char buf[BUFSIZE];
	char *hostname;
	int sockfd, serverlen, portno, n, status;
	struct hostent *server;
	struct sockaddr_in serveraddr;

	//hostname = argv[1];
    //portno = atoi(argv[2]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
	}
	server = gethostbyname(hostname);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(1);
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
        return 1;
    }

    sprintf(&buf, "GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n");
	n = send(sockfd, buf, strlen(buf), 0);
	printf("Sent %d bytes\n", n);
	n = recv(sockfd, buf, BUFSIZE, 0);
	printf("Rec %d bytes\n", n);

	printf("Buffer was:\n%s", buf);
}

/**************************************************************/
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

    if(currentBufPos == 0 && mode == 2){
    	path[0] = '/';
    	path[1] = '\0';
    }
    else
    {
    	path[currentBufPos] = '\0';
    }
    
    return;
}

void getHostAndPathTest()
{
	char host[MAXLINE];
	char path[MAXLINE];

	//strcpy(path, "https://www.google.com/search?safe=active&sxsrf=ALeKk01Nke6nVCvB-HqkU-NmLJZWAntvnA%3A1586041757590&ei=nROJXpXTI8yrtQbI64_YCg&q=c+strcpy&oq=c+strcpy&gs_lcp=CgZwc3ktYWIQAzICCAAyAggAMgIIADICCAAyAggAMgIIADICCAAyAggAMgIIADICCAA6BAgAEEdKDggXEgo2LTMxZzBnMTEySgsIGBIHNi0zZzBnMVCXDliXDmCnFWgAcAN4AIABWogBWpIBATGYAQCgAQKgAQGqAQdnd3Mtd2l6&sclient=psy-ab&ved=0ahUKEwjVlb388c_oAhXMVc0KHcj1A6sQ4dUDCAw&uact=5");
	strcpy(path, "https://www.google.com/imghp?hl=en&tab=ri&authuser=0&ogbl");
	getHostAndPath(&path, &host, strlen(path));

	fprintf(stderr, "Host: %s\nPath: %s\n\n", host, path);
}

/**************************************************************/
void processClientRequest(int connid, char* buffer, char* hostname)
{
	/*
	int n;
	n = read(connfd, buf, MAXLINE);
	if(n < 0)
	{
		fprintf(stderr,"error sending request to proxy %s\n", hostname);
	}
	*/

	int charPos = 0;
	char action[MAXLINE];
 	char path[MAXLINE];
 	char protocol[MAXLINE];
 	char body[BUFSIZE];

 	charPos += nextWord(buffer, action, charPos, BUFSIZE);
	charPos += nextWord(buffer, path, charPos, BUFSIZE);
	charPos += nextWord(buffer, protocol, charPos, BUFSIZE);
	getBody(buffer, body, charPos, BUFSIZE);

	getHostAndPath(&path, hostname, MAXLINE);

	/*check...*/

	sprintf(buffer, "%s %s %s\r\nHost: %s\r\n%s", 
		action, path, protocol, hostname, body);

	return;
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

void processClientRequestTest()
{
	char host[MAXLINE];
	char buf[BUFSIZE];

	strcpy(buf, "GET https://www.google.com/imghp?hl=en&tab=ri&authuser=0&ogbl HTTP/1.0\r\nuser-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/77.0.3865.75 Safari/537.36");
	processClientRequest(0, &buf, &host);

	fprintf(stderr, "%s", buf);
}

/**************************************************************/
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

void hashTest()
{
	fprintf(stderr, "%d\n", hash("www.google.com", "/imghp"));
	fprintf(stderr, "%d\n", hash("www.youtube.com", "/subscriptions"));
	fprintf(stderr, "%d\n", hash("www.yahoo.com", "/finance"));
	fprintf(stderr, "%d\n", hash("www.yahoo.com", "/news"));
}

/**************************************************************/

struct CacheObject{
    char* host;
    char* path;
    int* accessed;
    long int* size;
    struct CacheObject* next;
} CacheObject;

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

    else if((time(NULL) - (long int)cacheList[hashCode]->accessed) > expiration)
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

void cacheTester(){
	/** Setup **/
	char host[100];
	char path[100];
	char buf[100];
	int result;

	struct CacheObject** cacheArr[5000];
	struct CacheObject** blackArr[5000];
	int i = 0;
	while(i < 5000)
	{
		cacheArr[i] = NULL;
		blackArr[i] = NULL;
		i++;
	}

	/** blacklist **/
	loadBlacklist(&blackArr);

	strcpy(host, "www.facebook.com");
	strcpy(path, "");
	strcpy(buf, "real GET request");

	result = inBlackList(host,&blackArr);
	fprintf(stderr, "blacklist: %d(should be 1)\n", result);

	/** Caching **/
	strcpy(host, "www.google.com");
	strcpy(path, "/");
	strcpy(buf, "google GET request");
	cache(&buf, host, path, &cacheArr);

	/** Loading **/
	strcpy(host, "www.yahoo.com");
	strcpy(path, "/");
	strcpy(buf, "yahoo GET request");
	result = loadCache(&buf, host, path, &cacheArr, 10000);
	fprintf(stderr, "cache1: %d(should be 0)\n", result);

	strcpy(host, "www.google.com");
	strcpy(path, "/");
	strcpy(buf, "not a request");
	result = loadCache(&buf, host, path, &cacheArr, 10000);

	fprintf(stderr, "cache2: %s(should be \"google GET request\")\n", buf);
}

/**************************************************************/
int main(int argc, char **argv) {
	//processClientRequestTest();
	//getHostAndPathTest();
	//hashTest();
	cacheTester();

	return 0;
}