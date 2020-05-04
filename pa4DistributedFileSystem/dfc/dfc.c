// Sandbox for testing parts of the project

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
    char hashResult[MAXLINE];
    int dest = (((short)MD5(hashInput, strlen(hashInput), &hashResult))%13)%4;
    return abs(dest);
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


int open_connectfd(char* hostname, int portno, char* creds) 
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

    fprintf(stderr, "Connecting to connection %d\n", sockfd);
    status = connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    
    if (status == -1) {
        perror("connect");
        return -1;
    }

    send(sockfd, creds, strlen(creds), 0);
    char buf[MAXLINE];
    recv(sockfd, &buf, MAXLINE, 0);
    fprintf(stderr, "%s\n", buf);



    return sockfd;
}

int processConfig(char addrs[4][MAXLINE], int connections[4], char* username, char* password)
{
    FILE* fp = fopen("dfc.config", "r");
    char * readLine = NULL;
    char buf[MAXLINE];
    char addr[MAXLINE];
    char port[MAXLINE];
    size_t len = 0;

    for(int i = 0; i < 4; i++)
    {
        len = getline(&readLine, &len, fp);
        strcpy(buf, readLine);
        len = nextWord(&buf, &addr, 0, strlen(buf));
        len = nextWord(&buf, &port, len, strlen(buf));
        
        strcpy(addrs[i], addr);
        connections[i] = atoi(port);
    }

    len = getline(&readLine, &len, fp);
    nextWord(readLine, username, 0, MAXLINE);
    len = getline(&readLine, &len, fp);
    strcpy(password, readLine);

    return 0;
}


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
    bufPtr = nextWord(extract3, &extract1, 0, MAXLINE);
    while(strcmp(extract1, "end") != 0)
    {
        bufPtr += nextWord(extract3, &extract2, bufPtr, MAXLINE);
        bufPtr += nextWord(extract3, &extract4, bufPtr, MAXLINE);
        fprintf(stderr, "%s %s %s\n", extract1, extract2, extract4);
        sprintf(buf,"temp/%s", extract2);
        bufferToFile(buf, &extract3[bufPtr], atoi(extract4), extract1);

        sprintf(extract3, "ack!\n");
        send(connid, extract3, size, 0);
        len = recv(connid, extract3, MAXBUF, 0);
        bufPtr = nextWord(extract3, extract1, 0, MAXLINE);
    }
    return 0;
}

//// File Handing
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

int reconstuctFile(char* filename)
{
    int size, dump;
    char tempPath[MAXLINE];
    char buf[MAXBUF];
    FILE* fp = fopen(filename, "w+");
    fclose(fp);

    fp = fopen(filename, "a");
    for(int i = 0; i < 4; i++)
    {
        sprintf(tempPath, "temp/%s.%d", filename, i);
        getFileData(tempPath, &dump, &size);
        FILE* tfp = fopen(tempPath, "r");
        if(tfp == NULL)
        {
            continue;
        }

        if(size <= MAXBUF)
        {
            fread(&buf, 1, size, tfp);
            fwrite(&buf, 1, size, fp);
        }

        else
        {
            int wroteLen = 0;
            int curLen = 0;

            while(wroteLen < size)
            {
                if(MAXBUF <= (size - wroteLen))
                {
                    curLen = MAXBUF;
                }
                else 
                {
                    curLen = size - wroteLen;
                } 
                fread(&buf, 1, curLen, tfp);
                wroteLen += curLen;
                fwrite(&buf, 1, curLen, fp);
            }
        }
        fclose(tfp);


    }
    fclose(fp);
}

//// Upload
int uploadFilePart(char* filename, int connid, int breaks, int part, int extras)
{
    char buf[MAXBUF];
    int len;
    FILE* fp;
    fp = fopen(filename, "r"); 
    fseek(fp, part*breaks, SEEK_SET);
    breaks = breaks + extras;

    sprintf(&buf, "put %s.%d %d\n", filename, part, breaks);
    len = send(connid, &buf, strlen(buf), MSG_NOSIGNAL);
    if(len < 0)
    {
        fprintf(stderr,"error sending data to connection %d \n", connid);
        return;
    }
    recv(connid, &buf, MAXBUF, 0);

    int sentLen = 0;
    sprintf(buf,"w+ %s", filename);
    int currentPieceSize = MAXBUF - (strlen(buf) + 6);
    if(currentPieceSize > breaks)
    {
        currentPieceSize = breaks;
    }
    sprintf(buf,"%s %4d\n", buf, currentPieceSize);
    fprintf(stderr, "%s\n", buf);
    sendFilePiece(connid, fp, &buf, currentPieceSize);
    sentLen = currentPieceSize;
    while (sentLen < breaks)
    {
        sprintf(buf,"a %s", &filename[5]);
        currentPieceSize = MAXBUF - (strlen(buf) + 6);
        if(currentPieceSize > breaks - sentLen)
        {
            currentPieceSize = breaks - sentLen;
        }
        sprintf(buf,"%s %4d\n", buf, currentPieceSize);
        fprintf(stderr, "%s\n", buf);
        sendFilePiece(connid, fp, &buf, currentPieceSize);
        sentLen += currentPieceSize;
    }
    send(connid, "end", strlen(buf), 0);
    fclose(fp);
}

int uploadFile(char* filename, char addrs[4][MAXLINE], int ports[4], int sendMap[4][4][2], char* creds)
{
    int destCode;
    destCode = hashDest(filename);

    
    int breaks, size, extras, parts1, parts2, len;
    getFileData(filename, &breaks, &size);
    char buf[MAXBUF];
    char lineBuf[MAXLINE];

    int connection;

    // send a part 1 of the file to a server
    for(int destServ = 0; destServ < 4; destServ++)
    {
        parts1 = sendMap[destCode][destServ][0] - 1;
        connection = open_connectfd(addrs[destServ], ports[destServ], creds);
        
        if (parts1 == 3)
        {
            extras = size%4;
        } 
        else
        {
            extras = 0;
        }
        uploadFilePart(filename, connection, breaks, parts1, extras);
    }

    // send a part 2 of the file to a server
    for(int destServ = 0; destServ < 4; destServ++)
    {
        parts2 = sendMap[destCode][destServ][1] - 1;
        connection = open_connectfd(addrs[destServ], ports[destServ], creds);
        if (parts2 == 3)
        {
            extras = size%4;
        } 
        else
        {
            extras = 0;
        }
        uploadFilePart(filename, connection, breaks, parts2, extras);
    }
}

//// Donload
int downloadFile(char* filename, char addrs[4][MAXLINE], int ports[4], char* creds)
{
    char buf[MAXBUF];
    char extract1[MAXLINE];
    char extract2[MAXLINE];
    int len, linePtr, connection;

    for(int destServ = 0; destServ < 4; destServ++)
    {
        connection = open_connectfd(addrs[destServ], ports[destServ], creds);
        sprintf(buf, "get %s\n", filename);
        send(connection, &buf, strlen(buf), 0);
        char filepath[MAXLINE];
        sprintf(filepath, "temp/%s", buf);
        recieveFilePiece(connection, &filepath, MAXLINE);
    }
    reconstuctFile(filename);
    fprintf(stderr, "Successfully downloaded %s\n", filename);
}

int main()
{
    int destMap[4][4][2] = {
                            {{1,2},{2,3},{3,4},{4,1}},
                            {{4,1},{1,2},{2,3},{3,4}},
                            {{3,4},{4,1},{1,2},{2,3}},
                            {{2,3},{3,4},{4,1},{1,2}}
                            };

    /// Config Init
    char user[MAXLINE];
    char pass[MAXLINE];
    char creds[MAXLINE];
    char serverAddrs[4][MAXLINE];
    int serverPorts[4];

    processConfig(&serverAddrs, &serverPorts, &user, &pass);
    sprintf(creds, "%s%s", user, pass);

    int len = 0;
    char extract1[MAXLINE];
    char extract2[MAXLINE];
    while(1)
    {            
        /* Promt the user */
        char buf[MAXLINE];
        printf("Please enter msg: ");
        fgets(buf, MAXBUF, stdin);
        len = nextWord(buf, extract1, 0, MAXLINE);

        if(strcmp(extract1, "put") == 0)
        {
            len += nextWord(buf, extract2, len, MAXLINE);
            FILE* fp;
            fp = fopen(extract2, "r");
            if(fp == NULL)
            {
                fprintf(stderr, "%s not in local directory\n", extract2);
            }
            else
            {
                uploadFile(extract2, serverAddrs, serverPorts, destMap, &creds);
            }
        }

        else if(strcmp(extract1, "get") == 0)
        {
            len += nextWord(buf, extract2, len, MAXLINE);
            downloadFile(extract2, serverAddrs, serverPorts, &creds);
        }

        else if(strcmp(extract1, "list") == 0)
        {
            continue;
        }

        else if(strcmp(extract1, "quit") == 0)
        {
            break;
        }

        else
        {
            fprintf(stderr, "Unable to interpret message.\n");
        }
    }

    return 0;
}