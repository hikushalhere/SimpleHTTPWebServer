#include "WebServer.h"

//Constructor
WebServer::WebServer(bool http10, char *myPort, unsigned int myTimeout)
{
    this->serverHttp10 = http10;
    this->serverPort = myPort;
    this->serverTimeout = myTimeout;
    this->nextActiveConn = 0;
    this->numOfConns = 0;

    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
        this->myHttpHandler[i] = NULL;

    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
        this->activeConns[i] = -1;

    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
        this->response[i] = NULL;

    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
        this->sizeOfResponse[i] = 0;

    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
        this->done[i] = false;

    if(!this->serverHttp10)
    {
        for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
            this->timer[i] = 0;

        for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
            this->timedOut[i] = false;

        this->currentTimeout = this->serverTimeout;
    }
    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
        this->connTime[i] = 0;
    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
        this->transTime[i] = 0;

    connectionVsFileSize = fopen(FILE_CONNECTION_VS_FILESIZE, "a");
    if(connectionVsFileSize)
    {
        fwrite("Connection Time\tFileSize\n", sizeof(char), strlen("Connection Time\tFileSize\n"), connectionVsFileSize);
        fclose(connectionVsFileSize);
    }
    transmissionVsFileSize = fopen(FILE_TRASMISSION_VS_FILESIZE,"a");
    if(transmissionVsFileSize)
    {
        fwrite("Transmission Time\tFileSize\n", sizeof(char), strlen("Transmission Time\tFileSize\n"), transmissionVsFileSize);
        fclose(transmissionVsFileSize);
    }
}

//Destructor
WebServer::~WebServer()
{
    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
    {
        if(this->myHttpHandler[i] != NULL)
        {
            delete this->myHttpHandler[i];
            this->myHttpHandler[i] = NULL;
        }
    }
}

int WebServer::runWebServer()
{
    struct addrinfo hint;
    struct addrinfo *serverInfo;
    struct addrinfo *curr;
    int retVal, yes;

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;

    if((retVal = getaddrinfo(NULL, this->serverPort, &hint, &serverInfo)) != 0)
    {
        cerr<<gai_strerror(retVal);
        return -1;
    }

    for(curr = serverInfo; curr != NULL; curr = curr->ai_next)
    {
        this->listenerSocket = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if(this->listenerSocket == -1)
        {
            perror("socket() failed ");
            continue;
        }

        yes = 1;
        setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if(bind(this->listenerSocket, curr->ai_addr, curr->ai_addrlen) == -1)
        {
            close(this->listenerSocket);
            cout<<"\nI have closed the connection at socket "<<this->listenerSocket;
            perror("bind() failed ");
            continue;
        }
        break;
    }

    freeaddrinfo(serverInfo);

    if(curr == NULL)
    {
        cerr<<"\nFailed to create/bind any socket.";
        return -1;
    }

    cout<<"\nThe server is now ready. Bring it on!";
    return listenAndAccept();
}

int WebServer::listenAndAccept()
{
    struct sockaddr clientSockAddr;
    fd_set masterReadList, readList, masterWriteList, writeList;
    int fileDesMax;

    if(listen(this->listenerSocket, BACKLOG) == -1)
    {
        close(this->listenerSocket);
        cout<<"\nI have closed the connection at socket "<<this->listenerSocket;
        perror("listen() failed ");
        return -1;
    }
    cout<<"\nI am listening now...";

    FD_ZERO(&masterReadList);
    FD_ZERO(&readList);
    FD_ZERO(&masterWriteList);
    FD_ZERO(&writeList);
    FD_SET(this->listenerSocket, &masterReadList);
    fileDesMax = this->listenerSocket;

    while(1)
    {
        struct timeval timeout;

        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        readList = masterReadList;
        writeList = masterWriteList;
        if(select(fileDesMax+1, &readList, &writeList, NULL, &timeout) == -1)
        {
            close(this->listenerSocket);
            cout<<"\nI have closed the connection at socket "<<this->listenerSocket;
            perror("select() failed");
                return -1;
        }
        for(int i = this->listenerSocket; i <= fileDesMax; i++)
        {
            // Looking for any data to be read
            if(FD_ISSET(i, &readList))
            {
                // New incoming connection
                if(i == this->listenerSocket)
                {
                    int newSocketConn;
                    socklen_t clientSockAddrSize;
                    clientSockAddrSize = sizeof(clientSockAddr);

                    if(findNumOfActiveConns() < MAX_NUM_OF_CONNS)
                    {
                        // Code instrumentation starts

                        // Record the time before accepting the connection
                        struct timeval startTime;
                        gettimeofday(&startTime, NULL);

                        // Code instrumentation ends

                        // Accepting the incoming connection
                        newSocketConn = accept(this->listenerSocket, &clientSockAddr, &clientSockAddrSize);
                        if(newSocketConn == -1)
                            perror("accept() failed");
                        else
                        {
                            // Code instrumentation starts

                            // Update the accept() time
                            struct timeval stopTime;
                            unsigned int diffSec, diffUsec;

                            gettimeofday(&stopTime, NULL);
                            diffSec = stopTime.tv_sec - startTime.tv_sec;
                            if(diffSec == 0)
                            {
                                diffUsec = stopTime.tv_usec - startTime.tv_usec;
                                this->connTime[this->nextActiveConn] = diffUsec;
                            }
                            else
                                this->connTime[this->nextActiveConn] = diffSec*pow(10,6);
                            // Code instrumentation ends

                            FD_SET(newSocketConn, &masterReadList);
                            if(newSocketConn > fileDesMax)
                                fileDesMax = newSocketConn;
                            this->activeConns[this->nextActiveConn] = newSocketConn;
                            this->nextActiveConn = findNextActiveConnSlot();
                            if(!this->serverHttp10)
                            {
                                this->numOfConns++;
                                this->currentTimeout = this->serverTimeout / this->numOfConns;
                            }
                            char ipAddr[INET_ADDRSTRLEN];
                            inet_ntop(clientSockAddr.sa_family, &((struct sockaddr_in *)&clientSockAddr)->sin_addr, ipAddr, sizeof(ipAddr));
                            cout<<"\nI have received a new connection from "<<ipAddr<<" at socket "<<newSocketConn;
                        }
                    }
                    else
                        cout<<"\nI am already doing my best. I can not take any more connection";
                }
                // Service the existing connections
                else
                {
                    int numBytesRecvd;
                    char request[MAXREQUESTSIZE];
                    int posOfConn = findPosOfConn(i);
                    bool recvConn = true;

                    if(!this->serverHttp10 && (this->timer[posOfConn] != 0))
                    {
                        time_t currTime;
                        double diffOfTime;

                        time(&currTime);
                        diffOfTime = difftime(currTime, timer[posOfConn]);
                        cout<<"\n"<<diffOfTime<<"has elapsed for connection at socket "<<i;

                        // Checking if the time elapsed since the last request was sent is greater than the timeout
                        // Timeout period available to the connection is calculated as the total timeout divided by the number of active connections a present
                        if(diffOfTime > this->currentTimeout)
                        {
                            recvConn = false;
                            this->timedOut[posOfConn] = true;
                            this->myHttpHandler[posOfConn] = new HttpHandler(this->serverHttp10, this->serverTimeout);
                            this->done[posOfConn] = this->myHttpHandler[posOfConn]->serveConnection(request, &this->response[posOfConn], this->sizeOfResponse+posOfConn, true);
                        }
                    }
                    if(recvConn)
                    {
                        numBytesRecvd = recv(i, request, MAXREQUESTSIZE, 0);
                        cout<<"\nRequest: "<<request;
                        if(numBytesRecvd <= 0)
                        {
                            if(numBytesRecvd == 0)
                                cout<<"\nSocket "<<i<<" is gone. Can not read from it anymore.";
                            else
                                perror("recv() failed");
                            close(i);
                            cout<<"\nI have closed the connection at socket "<<i;
                            FD_CLR(i, &masterReadList);
                            if(FD_ISSET(i, &masterWriteList))
                                FD_CLR(i, &masterWriteList);
                            if(!this->serverHttp10)
                            {
                                this->numOfConns--;
                                if(this->numOfConns == 0)
                                    this->currentTimeout = this->serverTimeout;
                                else
                                    this->currentTimeout = this->serverTimeout / this->numOfConns;
                            }
                            // Resetting the values as this recv() failure may happen the 2nd time for a persistent connection
                            if(posOfConn >= 0)
                            {
                                this->timedOut[posOfConn] = false;
                                this->activeConns[posOfConn] = -1;
                                this->done[posOfConn] = false;
                                timer[posOfConn] = 0;
                            }
                        }
                        else
                        {
                            if(posOfConn >= 0)
                            {
                                //time(this->startTime+posOfConn);
                                this->myHttpHandler[posOfConn] = new HttpHandler(this->serverHttp10, this->serverTimeout);
                                this->done[posOfConn] = this->myHttpHandler[posOfConn]->serveConnection(request, &this->response[posOfConn], this->sizeOfResponse+posOfConn, false);
                            }
                            FD_SET(i, &masterWriteList);
                        }
                    }
                } // else (Serve the existing connections)
            } // if (There is data to be read)

            // Looking for data to be written
            else if(FD_ISSET(i, &writeList))
            {
                int posOfConn = findPosOfConn(i);
                if(posOfConn >= 0)
                {
                    if(this->response[posOfConn] != NULL)
                    {
                        // Code instrumentation starts

                        // Record the time before closing the connection
                        struct timeval startTime;
                        gettimeofday(&startTime, NULL);

                        // Code instrumentation ends

                        // Send the response to the client
                        int numBytesSent = send(i, this->response[posOfConn], this->sizeOfResponse[posOfConn], 0);
                        if(numBytesSent == -1)
                        {
                            perror("send() failed");
                            done[posOfConn] = true;
                        }
                        else
                        {
                            // Code instrumentation starts

                            // Update the transmission time
                            struct timeval stopTime;
                            unsigned int diffSec, diffUsec;

                            gettimeofday(&stopTime, NULL);
                            diffSec = stopTime.tv_sec - startTime.tv_sec;
                            if(diffSec == 0)
                            {
                                diffUsec = stopTime.tv_usec - startTime.tv_usec;
                                this->transTime[i] += diffUsec;
                            }
                            else
                                this->transTime[i] += diffSec*pow(10,6);

                            // Code instrumentation ends
                        }
                    }
                    if(this->done[posOfConn])
                    {
                        int fileSize = this->myHttpHandler[posOfConn]->fileSizeForPublic; // Just for logging results. No use in the actual functioning of the server.

                        this->response[posOfConn] = NULL;
                        this->sizeOfResponse[posOfConn] = 0;
                        delete this->myHttpHandler[posOfConn];
                        this->myHttpHandler[posOfConn] = NULL;
                        if(!this->serverHttp10 && (this->timer[posOfConn] != 0.0))
                        {
                            time_t currTime;
                            double diffOfTime;

                            time(&currTime);
                            diffOfTime = difftime(currTime, timer[posOfConn]);
                            cout<<"\n"<<diffOfTime<<"has elapsed for connection at socket "<<i;

                            // Checking if the time elapsed since the last request was sent is greater than the timeout
                            if(diffOfTime > this->currentTimeout)
                                this->timedOut[posOfConn] = true;
                        }
                        if(this->serverHttp10 || (!this->serverHttp10 && this->timedOut[posOfConn]))
                        {
                            // Code instrumentation starts

                            // Record the time before closing the connection
                            struct timeval startTime;
                            gettimeofday(&startTime, NULL);

                            // Code instrumentation ends

                            //Close the connection
                            close(i);

                            // Code instrumentation starts

                            // Update the close() time
                            struct timeval stopTime;
                            unsigned int diffSec, diffUsec, diffTransTime;

                            gettimeofday(&stopTime, NULL);
                            diffSec = stopTime.tv_sec - startTime.tv_sec;
                            if(diffSec == 0)
                            {
                                diffUsec = stopTime.tv_usec - startTime.tv_usec;
                                this->connTime[i] += diffUsec;
                            }
                            else
                                this->connTime[i] += diffSec*pow(10,6);


                            char buff[50];
                            char connectionTime[10], transmissionTime[10], fileLength[10];
                            sprintf(fileLength, "%d", fileSize);

                            // Logging connection time results
                            sprintf(connectionTime, "%d", this->connTime[i]);
                            strcpy(buff, connectionTime);
                            strcat(buff,"\t");
                            strcat(buff, fileLength);
                            strcat(buff,"\n");
                            this->connTime[i] = 0;
                            this->connectionVsFileSize = fopen(FILE_CONNECTION_VS_FILESIZE, "a");
                            {
                                fwrite(buff, sizeof(char), strlen(buff), connectionVsFileSize);
                                fclose(connectionVsFileSize);
                            }

                            // Logging transmission time results
                            sprintf(transmissionTime, "%d", this->transTime[i]);
                            strcpy(buff, transmissionTime);
                            strcat(buff,"\t");
                            strcat(buff, fileLength);
                            strcat(buff,"\n");
                            this->transTime[i] = 0;
                            this->transmissionVsFileSize = fopen(FILE_TRASMISSION_VS_FILESIZE, "a");
                            if(this->transmissionVsFileSize)
                            {
                                fwrite(buff, sizeof(char), strlen(buff), this->transmissionVsFileSize);
                                fclose(this->transmissionVsFileSize);

                            }
                            // Code instrumentation ends

                            cout<<"\nI have closed the connection at socket "<<i;
                            if(!this->serverHttp10)
                            {
                                this->numOfConns--;
                                if(this->numOfConns == 0)
                                    this->currentTimeout = this->serverTimeout;
                                else
                                    this->currentTimeout = this->serverTimeout / this->numOfConns;
                            }
                            this->activeConns[posOfConn] = -1;
                            this->done[posOfConn] = false;
                            this->timedOut[posOfConn] = false;
                            timer[posOfConn] = 0;
                            FD_CLR(i, &masterReadList);
                            FD_CLR(i, &masterWriteList);
                        }
                        else if(this->timer[posOfConn] == 0.0)
                            time(this->timer+posOfConn);
                    }
                    else if(this->myHttpHandler[posOfConn])
                        this->done[posOfConn] = this->myHttpHandler[posOfConn]->serveConnection(NULL, &this->response[posOfConn], this->sizeOfResponse+posOfConn, false);
                }
            }// If (There is data to be written)
        } // for looping over all existing connections;
    } // "the forever" while loop
}

int WebServer::findPosOfConn(int socketFileDes)
{
    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
    {
        if(socketFileDes == this->activeConns[i])
            return i;
    }
    return -1;
}

int WebServer::findNextActiveConnSlot()
{
    int count, next;

    count = 0;
    next = this->nextActiveConn;

    while(count < MAX_NUM_OF_CONNS)
    {
        if(next == MAX_NUM_OF_CONNS-1)
            next = 0;
        else
            next++;
        if(this->activeConns[next] == -1)
            return next;
        count++;
    }
    return -1;
}

int WebServer::findNumOfActiveConns()
{
    int count = 0;

    for(int i = 0; i < MAX_NUM_OF_CONNS; i++)
    {
        if(this->activeConns[i] != -1)
            count++;
    }
    return count;
}
