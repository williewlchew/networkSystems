
/* 
 * httpServer.c - A simple TCP web server with threads by Willie Chew
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

/* make-shift header */
int open_listenfd(int port); /* open and listen to a socket */
void *thread(void *vargp); /* start a new thread to handel a new request */
void processRequest(int connfd); /* process the request given from a socket */
void nextWord(char* buffer, char* dest, int start, int limit); /* parse the next word in the specifed location in a buffer */
void parseAction(char* buffer, char* action, char* location, char* version, int* n); /* parse out relevant information in a request */
const char* get_filename_ext(const char* filename); /* convert the extension of the requested docuemnt to the content type constant */
void fileDataToBuffer(FILE* stream, char* buffer, int* n, int* filesize, char* path); /* write the contents of a specified file into a buffer */

int main(int argc, char **argv) 
{    
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }
}

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


/* 
* start a new thread to handel a new request 
*/
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    processRequest(connfd);
    close(connfd);
    return NULL;
}

/* 
* process the request given from a socket 
*/
void processRequest(int connfd) 
{
    int n; 
    char buf[MAXLINE]; 
    n = read(connfd, buf, MAXLINE);
    printf("Recieved:\n%s", buf);

    char action[100] = "";
    char location[100] = ".";
    char version[100] = "";
    parseAction(buf, &action, &location[1], &version, &n); 

    /* check validity of path; return an error message if file not found */
    FILE* fp = fopen(location, "r");
    if (fp == NULL)
    {
        sprintf(buf, version);
        n = strlen(buf);
        sprintf(&buf[n], " 500 Internal Server Error\r\n");
        n = strlen(buf);
        write(connfd, buf, n);
        printf("Sent:\n%s", buf);
    }

    /* process valid paths */
    else
    {
        sprintf(buf, version);
        n = strlen(buf);
        sprintf(&buf[n], " 200 Document Follows\r\n");
        n = strlen(buf);
        write(connfd, buf, n);

        /* Send file metadata */
        int filesize = 0;
        fileDataToBuffer(fp, &buf[n], &n, &filesize, location);
        write(connfd, buf, n);
        printf("Sent:\n%s", buf);
        
        /* send file contents*/
        size_t bytesRead = 0;

        while((n = fread(buf, 1, MAXLINE, fp)) > 0)
        {
            write(connfd, buf, n);  
        }

    }
}

/*
*	WORD PARSING
*/

/*
* parse the next word in the specifed location in a buffer
*/
void nextWord(char* buffer, char* dest, int start, int limit)
{
    char charDump = buffer[start];
    char strDump[limit];
    int i = 0;
    while(!(charDump == ' ' || charDump == '\r' || charDump == '\n'||i >= limit - 1))
    {
        strDump[i] = charDump;
        i++;
        charDump = buffer[i + start];
    }
    strDump[i] = '\0';
    strcpy(dest, strDump);

    return;
}

/*
* parse out relevant information in a request
*/
void parseAction(char* buffer, char* action, char* location, char* version, int* n)
{
    /* PARSE ACTION */
    nextWord(buffer, action, 0, *n);
    int charStart = 0;
    if(strcmp(action, "GET") == 0)
    {
        charStart = 4;
    }
    else if(strcmp(action, "HEAD") == 0 || strcmp(action, "POST") == 0)
    {
        charStart = 5;
    }

    /* PARSE LOCATION */
    nextWord(buffer, location, charStart, *n);
    charStart += (strlen(location) + 1);
    if(strcmp(location, "/") == 0 || strcmp(action, "/inside/") == 0){
        strcpy(location, "/index.html");
    }

    /* PARSE VERSION */
    nextWord(buffer, version, charStart, *n);

    return;
}


/*
*	PROCESS REQUEST
*/

/* code based on source:
    https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
 */
const char* get_filename_ext(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    else if(strcmp(dot,".html") == 0)
        return "text/html";
    else if(strcmp(dot,".txt") == 0)
        return "text/plain";
    else if(strcmp(dot,".png") == 0)
        return "image/png";
    else if(strcmp(dot,".gif") == 0)
        return "image/gif";
    else if(strcmp(dot,".jpg") == 0)
        return "image/jpg";
    else if(strcmp(dot,".css") == 0)
        return "text/css";
    else if(strcmp(dot,".js") == 0)
        return "application/javascript";
    return "";
}

/*
* write the contents of a specified file into a buffer
*/
void fileDataToBuffer(FILE* stream, char* buffer, int* n, int* filesize, char* path)
{
    /* get the file type*/
    char* filetype = get_filename_ext(path);

    /* get the file size */
	fseek(stream, 0L, SEEK_END);
    *filesize = ftell(stream);
    fseek(stream, 0L, SEEK_SET);

    sprintf(buffer, "Content-Type:%s\r\nContent-Length:%d\r\n\r\n", filetype, *filesize);
	*n += strlen(buffer);

    return;
}