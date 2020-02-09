/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

/*
* example code to test fuctions
*/
void extras(){
    char* filename = "foo2";
    FILE* fp;
    char bigBuf[6000];
    int filesize = 0;

    fp = fopen(filename, "r");
    if(fp == NULL)
        fprintf(stderr, "Cannot open file\n");
    fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

    fread(&bigBuf, 1, filesize, fp);
    fprintf(stderr, "foo2: %d\n",strlen(bigBuf));
    fclose(fp);

    fp = fopen("shouldwork", "w+");
    if(fp == NULL)
        fprintf(stderr, "Cannot open file\n");
    fwrite (&bigBuf, 1, filesize, fp);
    fclose(fp);
    
    DIR *dir;
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    struct dirent *ent;
    if ((dir = opendir (cwd)) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        printf ("%s\n", ent->d_name);
      }
      closedir (dir);
    } 
}

int filesizeToBuffer(char* filename, char* buffer, int* messageSize, int* fileSize){
	FILE* fp;

    fp = fopen(filename, "r");
    if(fp == NULL)
        return 1;

	fseek(fp, 0L, SEEK_END);
	*fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	sprintf(buffer, "%d ", *fileSize);
	*messageSize += 5;

	fclose(fp);
	return 0;
}

int fileToBuffer(char* filename, char* buffer, int* messageSize, int fileSize){
    FILE* fp;

    fp = fopen(filename, "r");
    if(fp == NULL)
        return 1;

	fread(buffer, 1, fileSize, fp);

    fclose(fp);
    *messageSize += fileSize;

    return 0;
}

int bufferToFile(char* filename, char* buffer, int* fileSize){
	FILE* fp;

    fp = fopen(filename, "w+");
    if(fp == NULL)
        return 1;

    fwrite(buffer, 1, fileSize, fp);
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    /* Connection variables */
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;

    /* Message variables */
    int msgLength;
    int fileLength;
    char buf[BUFSIZE];
    char* datagram[BUFSIZE]; /* the copy of the buffer datagram */
	char* parsed;

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    serverlen = sizeof(serveraddr);

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    while(1)
    {            
        /* TEMP :: get a message from the user */
        bzero(buf, BUFSIZE);
        printf("Please enter msg: ");
        fgets(buf, BUFSIZE, stdin);
        msgLength = strlen(buf);
		buf[msgLength-1] =  ' \n';
        
        /* ATTEMPT CLIENT ACTION */
        msgLength = strlen(buf);
        strcpy(datagram, buf);
		parsed = strtok(datagram," \n");
		if(strcmp(parsed, "get") == 0)
		{
			/* Construct datagram */
			parsed = strtok (NULL, " \n");
			// end character to space
			buf[msgLength-1] =  ' ';
			filesizeToBuffer(parsed, &buf[msgLength], &msgLength, &fileLength);
			printf("fileLength: %d\n", fileLength);
			fileToBuffer(parsed, &buf[msgLength], &msgLength, fileLength);
			
			/* Send the datagram off */
			printf("Message: %s %d %d\n", buf, strlen(buf), msgLength);

			/* temp to simulate server */
			bufferToFile("shouldwork2", &buf[msgLength-fileLength], fileLength);

			/* Await Confirmation */
			n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
	        if (n < 0) 
	          error("ERROR in recvfrom");
	        printf("Echo from server: %s", buf);
		}

		else if(strcmp(parsed, "put") == 0)
		{
			printf("Put called\n");
			break;
		}

		else if(strcmp(parsed, "extra") == 0)
		{
			extras();
			printf("%s\n", buf);
			break;
		}

		else 
		{
			/* Send message */
			n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
	        if (n < 0) 
	          error("ERROR in sendto");
	        
	        /* print the server's reply */
	        n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
	        if (n < 0) 
	          error("ERROR in recvfrom");
	        printf("Echo from server: %s", buf);
		}
    }
    return 0;
}
