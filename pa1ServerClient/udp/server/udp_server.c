/* 
 * udpserver.c - A simple UDP file server by Willie Chew 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFSIZE 30000

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
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
	int sockfd; /* socket */
	int portno; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */

	/* Message Variables */
	int msgLength;
    int fileLength;
	char buf[BUFSIZE]; /* message buf */
	int n; /* message byte size */
	char* datagram[BUFSIZE]; /* the copy of the buffer datagram */
	char* parsed; /* parse the message sent from the client */


	/* 
	* check command line arguments 
	*/
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);


	// Create the parent socket 
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets 
	* us rerun the server immediately after we kill it; 
	* otherwise we have to wait about 20 secs. 
	* Eliminates "ERROR on binding: Address already in use" error. 
	*/
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

	/*
	* build the server's Internet address
	*/
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)portno);

	/* 
	* bind: associate the parent socket with a port 
	*/
	if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		error("ERROR on binding");

	/* 
	* main loop: wait for a datagram, then echo it
	*/
	clientlen = sizeof(clientaddr);
	while (1) {
		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		bzero(buf, BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");

		/* 
		 * gethostbyaddr: determine who sent the datagram
		 */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
					sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
		printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

		/*
		* Attempt client action
		*/
		strcpy(datagram, buf);
		parsed = strtok(datagram," \n");

		/* Get action */
		if(strcmp(parsed, "get") == 0)
		{
			/* Deconstruct datagram */
			parsed = strtok(NULL, " \n");

			/* Construct response */
			msgLength = 0;
			filesizeToBuffer(parsed, buf, &msgLength, &fileLength);
			fileToBuffer(parsed, &buf[msgLength], &msgLength, fileLength);
		}

		/* Put action */
		else if(strcmp(parsed, "put") == 0)
		{
			/* Deconstruct datagram */
			parsed = strtok(NULL, " \n");
			fileLength = atoi(strtok(NULL, " \n"));

			bufferToFile(parsed, &buf[n-fileLength], fileLength);

			/* Construct response */
			sprintf(buf, "Recieved %d of %s\0", fileLength, parsed);
			msgLength = strlen(buf);
		}

		/* delete action */
		else if(strcmp(parsed, "delete") == 0)
		{
			parsed = strtok(NULL, " \n");
			remove(parsed);
			sprintf(buf, "Removed %s\n", parsed);
			msgLength = strlen(buf);
		}

		/* ls action */
		else if(strcmp(parsed, "ls") == 0)
		{
			/* 
				code used from link below: 
				https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c 
			*/
			DIR *dir;
			char cwd[PATH_MAX];
			getcwd(cwd, sizeof(cwd));
			struct dirent *ent;

			sprintf(buf, "Local Directory: \n");
			//msgLength = 18;
			if ((dir = opendir (cwd)) != NULL) 
			{
				/* print all the files and directories within directory */
				while ((ent = readdir (dir)) != NULL) 
				{
					strcat(buf, ent->d_name);
					strcat(buf, "\n");
			    }
		    	closedir (dir);
		    }
		    msgLength = strlen(buf);
		}

		/* exit action */
		else if(strcmp(parsed, "exit") == 0){
			sprintf(buf, "Shutting down server...\n");
			msgLength = strlen(buf);
		}

		/* Unrecognizable command */
		else 
		{
			printf("Unrecognizable command\n");
		}

		/* 
		 * sendto: echo the input back to the client 
		 */
		n = sendto(sockfd, buf, msgLength, 0, (struct sockaddr *) &clientaddr, clientlen);
		if (n < 0) 
			error("ERROR in sendto");
	}
}
