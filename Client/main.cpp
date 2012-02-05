#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT "8000"
#define DEFAULT_PROCS 20
#define DEFAULT_REQUESTS_PER_CHILD 5
#define MAX_RESPONSE_SIZE 8192
#define REQUEST "GET /test10.html HTTP/1.0\r\n\r\n"

using namespace std;

char *composeRequest(size_t *);
size_t getSizeOfFile(char *);
size_t getSizeOfHeader(char *);

int main(int argc, char *argv[])
{
    struct addrinfo *serverInfo;
    struct addrinfo *curr;
    struct addrinfo hint;
    int numOfProcs, socketFileDes, retVal, i;
    int numOfBytesSent, numOfBytesRecvd;
    size_t numOfBytesLeft, sizeOfRequest;
    char response[MAX_RESPONSE_SIZE], answer;
    char *request, *host, *port;
    pid_t childPid;

    numOfProcs = 0;

    if(argc > 1)
    {
        if(argc >= 2)
        {
            host = new char[strlen(argv[1])];
            strcpy(host, argv[1]);

            if(argc >= 3)
            {
                port = new char[strlen(argv[2])];
                strcpy(port, argv[2]);

                if(argc == 4)
                    numOfProcs = atoi(argv[3]);
                else
                    numOfProcs = DEFAULT_PROCS;
            }
            else
            {
                port = new char[strlen(DEFAULT_PORT)];
                strcpy(port, DEFAULT_PORT);
            }
        }
        else
        {
            host = new char[strlen(DEFAULT_HOST)];
            strcpy(host, DEFAULT_HOST);
        }
    }
    else
    {
        host = new char[strlen(DEFAULT_HOST)];
        strcpy(host, DEFAULT_HOST);

        port = new char[strlen(DEFAULT_PORT)];
        strcpy(port, DEFAULT_PORT);

        numOfProcs = DEFAULT_PROCS;
    }

    cout<<"\nI will send requests to "<<host<<" at port number "<<port;
    cout<<"\nI am going to spawn "<<numOfProcs<<" child processes for this";
    cout<<"\nShould I go? (y/n) ";
    cin>>answer;
    if(answer == 'y' || answer == 'Y')
    {
        for(i = 0; i < numOfProcs; i++)
        {
            if((childPid = fork()) <= 0)
                break;
        }

        if(i < numOfProcs && childPid == 0)
        {
            struct timeval start;
            struct timeval stop;
            unsigned int diff;
            size_t fileSize, headerSize;
            char fileName[5];
            FILE *logFile;

            memset(&hint, 0, sizeof(hint));
            hint.ai_family = AF_UNSPEC;
            hint.ai_socktype = SOCK_STREAM;

            retVal = getaddrinfo(host, port, &hint, &serverInfo);
            if(retVal != 0)
            {
                cerr<<gai_strerror(retVal);
                return -1;
            }

            for(curr = serverInfo; curr != NULL; curr = curr->ai_next)
            {
                socketFileDes = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
                if(socketFileDes == -1)
                {
                    perror("socket() failed ");
                    continue;
                }

                int yes = 1;
                setsockopt(socketFileDes, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

                if(connect(socketFileDes, curr->ai_addr, curr->ai_addrlen) == -1)
                {
                    close(socketFileDes);
                    perror("connect() failed ");
                    continue;
                }
                break;
            }

            freeaddrinfo(serverInfo);

            if(curr == NULL)
            {
                cerr<<"\nFailed to connect";
                return -1;
            }

            request = composeRequest(&sizeOfRequest);

            // Code instrumentation starts

            gettimeofday(&start, NULL);

            // Code instrumentation ends

            for(int i = 0; i < DEFAULT_REQUESTS_PER_CHILD; i++)
            {
                numOfBytesSent = send(socketFileDes, request, sizeOfRequest, 0);
                if(numOfBytesSent == -1)
                {
                    perror("send() failed");
                    close(socketFileDes);
                    return -1;
                }

                numOfBytesRecvd = recv(socketFileDes, response, MAX_RESPONSE_SIZE, 0);
                if(numOfBytesRecvd == -1)
                {
                    perror("recv() failed");
                    close(socketFileDes);
                    return -1;
                }
                else
                    cout<<response;

                // Extracting the size of the message body
                fileSize = getSizeOfFile(response);
                headerSize = getSizeOfHeader(response);
                numOfBytesLeft = fileSize + headerSize - numOfBytesRecvd;
                while(numOfBytesLeft > 0)
                {
                    numOfBytesRecvd = recv(socketFileDes, response, MAX_RESPONSE_SIZE, 0);
                    if(numOfBytesRecvd == -1)
                    {
                        perror("recv() failed");
                        close(socketFileDes);
                        return -1;
                    }
                    cout<<response;
                    numOfBytesLeft -= numOfBytesRecvd;
                }
            }
            // Close the socket
            close(socketFileDes);

            // Code instrumentation starts

            gettimeofday(&stop, NULL);

            // Code instrumentation ends

            diff = stop.tv_sec - start.tv_sec;
            if(diff == 0)
                diff = stop.tv_usec - start.tv_usec;
            else
                diff *= pow(10,6);

            sprintf(fileName, "./%d.txt", i);
            logFile = fopen(fileName, "w");
            if(logFile)
            {
                char diffTime[10];
                char responseSize[10];
                char buff[50];

                fwrite("Throughput\tFileSize\n", sizeof(char), strlen("Throughput\tFileSize\n"), logFile);
                sprintf(diffTime, "%u", diff);
                strcpy(buff, diffTime);
                strcat(buff, "\t");
                sprintf(responseSize, "%d", (int)fileSize);
                strcat(buff, responseSize);
                strcat(buff,"\n");
                fwrite(buff, sizeof(char), strlen(buff), logFile);
                fclose(logFile);
            }
        }
        else
        {
            while(1)
            {
                pid_t childDone;
                int status;

                childDone = wait(&status);
                if(childDone == -1)
                {
                    if(errno == ECHILD)
                        break;
                }
            }
        }

        delete[] host;
        host = NULL;
        delete[] port;
        port = NULL;
        delete[] request;
        request = NULL;
    }
    else
        cout<<"\nOkay, bye!";

    return 0;
}

char *composeRequest(size_t *sizeOfRequest)
{
    char *request;

    *sizeOfRequest = strlen(REQUEST);
    request = new char[*sizeOfRequest];
    strcpy(request, REQUEST);
    return request;
}

size_t getSizeOfFile(char *msg)
{
    char compareString[] = "Content-Length:";
    char *start;
    int sizeOfResponse;


    start = strstr(msg, compareString);
    while(*start != ' ')
        start++;

    start++;
    sizeOfResponse = 0;
    while(*start != '\r' && *start != '\n')
    {
        char digit;

        digit = *start;
        sizeOfResponse = sizeOfResponse*10 + (digit-48);
        start++;
    }
    return sizeOfResponse;
}

size_t getSizeOfHeader(char *msg)
{
    char *pos, *start;

    pos = NULL;
    start = msg;

    pos = strstr(msg, "\r\n\r\n");
    if(pos)
    {
        int count = 1;
        pos += 3;
        while(start != pos)
        {
            count++;
            start++;
        }
        return count;
    }
    pos = strstr(msg, "\n\n");
    if(pos)
    {
        int count = 1;
        pos += 1;
        while(start != pos)
        {
            count++;
            start++;
        }
        return count;
    }
    return 0;
}
