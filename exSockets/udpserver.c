/*

Steps:
	1. Create UDP socket.
	2. Bind the socket to server address.
	3. Wait until datagram packet arrives from client.
	4. Process the datagram packet and send a reply to client.
	5. Go back to Step 3.

*/

// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <ctype.h>
#include <errno.h>

#define PORT	 8080 
#define MAXLINE 1024 

// Driver code 
int main() { 
	int sockfd; 
	char rcvbuf[MAXLINE]; 
	char sndbuf[MAXLINE]; 
	struct sockaddr_in servaddr, cliaddr; 
	socklen_t len, n; 
	int i = 0;
	
	// SOCKET CREATION
	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(PORT); 
	
	// Bind the socket with the server address 
	if ( bind(sockfd, (const struct sockaddr *)&servaddr, 
			sizeof(servaddr)) < 0 ) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}

	printf("The server is ready to receive\n"); 
	
	len = sizeof(cliaddr);

	n = recvfrom(sockfd, (char *)rcvbuf, MAXLINE, 
				0, ( struct sockaddr *) &cliaddr, 
				&len); 

	/* captalize the received string */
	while (rcvbuf[i]) {
		sndbuf[i] = toupper(rcvbuf[i]);
		i++;
	}
	sndbuf[i] = '\0'; 

	int nc = sendto(sockfd, (const char *)sndbuf, strlen(sndbuf), 
		0, (const struct sockaddr *) &cliaddr, len); 

	printf("%d\n",nc);
	printf("%s\n",sndbuf);

	close(sockfd);
	
	return 0; 
} 

