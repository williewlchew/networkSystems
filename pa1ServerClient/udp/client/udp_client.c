/* 
 * udp_client.c - A simple UDP file transfer client by Willie Chew 
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

#define BUFSIZE 30000

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

/* Write the size of a file to a buffer */
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

/* Write the contents of a file to a buffer */
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

/* Save a chunk of the buffer in the local directory */
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

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    while(1)
    {            
    	/* Promt the user */
        bzero(buf, BUFSIZE);
        printf("Please enter msg: ");
        fgets(buf, BUFSIZE, stdin);
        msgLength = strlen(buf);
		buf[msgLength-1] =  ' \n';
        
        /* Parse the first word to get the action */
        msgLength = strlen(buf);
        strcpy(datagram, buf);
		parsed = strtok(datagram," \n");

		/* Process get action */
		if(strcmp(parsed, "get") == 0)
		{
			// parse the file name
			parsed = strtok(NULL, " \n"); 

			/* Send get request */
			n = sendto(sockfd, buf, msgLength, 0, &serveraddr, serverlen);
	        if (n < 0) 
	          error("ERROR in sendto");

			/* Recieve datagram */
			n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
	        if (n < 0) 
	          error("ERROR in recvfrom");
	      	buf[n] = '\0';

			/* 
				Write to file the contents of the recieved packet
				Recieved packet in format: <file size> <file contents>
			*/
			// parse the file length
			fileLength = atoi(strtok(buf, " \n")); 
			bufferToFile(parsed, &buf[n-fileLength], fileLength);
			printf("Recieved %d bytes\n", n);
		}

		/* Process put action */
		else if(strcmp(parsed, "put") == 0)
		{
			/* 
				Construct datagram by adding file size and file content to user promt 
				Packet format: <action> <file name>  <file size> <file contents>
			*/
			// parse the file name
			parsed = strtok (NULL, " \n"); 
			// end character to space
			buf[msgLength-1] =  ' ';
			filesizeToBuffer(parsed, &buf[msgLength], &msgLength, &fileLength);
			fileToBuffer(parsed, &buf[msgLength], &msgLength, fileLength);
			/* packet  */

			/* Send datagram */
			n = sendto(sockfd, buf, msgLength, 0, &serveraddr, serverlen);
	        if (n < 0) 
	          error("ERROR in sendto");

			/* Await Confirmation */
			n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
	        if (n < 0) 
	          error("ERROR in recvfrom");

	      	/* End current message and output */
	      	buf[n] = '\0';
	        printf("Response from server: %s \n", buf);
		}

		/* Process all other actions */
		else 
		{
			/* Send message */
			n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
	        if (n < 0) 
	          error("ERROR in sendto");
	        
	        /* print the server's reply */
	        n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
	        if (n < 0) 
	          error("ERROR in recvfrom");
	        printf("Response from server: %s\n", buf);
		}
    }
    return 0;
}
