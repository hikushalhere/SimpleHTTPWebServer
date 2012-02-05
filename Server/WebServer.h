#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "HttpHandler.h"

#define BACKLOG 5
#define MAX_NUM_OF_CONNS 100
#define MAXREQUESTSIZE 2048
#define FILE_CONNECTION_VS_FILESIZE "./Logs/log_file_connection_vs_filesize.txt"
#define FILE_TRASMISSION_VS_FILESIZE "./Logs/log_file_transmission_vs_filesize.txt"

using namespace std;

class WebServer
{
    private:
        // Member Variables
        unsigned int serverTimeout;
        unsigned int currentTimeout;

        size_t sizeOfResponse[MAX_NUM_OF_CONNS];

        HttpHandler *myHttpHandler[MAX_NUM_OF_CONNS];

        FILE *connectionVsFileSize;
        FILE *transmissionVsFileSize;

        time_t timer[MAX_NUM_OF_CONNS];

        bool serverHttp10;
        bool done[MAX_NUM_OF_CONNS];
        bool timedOut[MAX_NUM_OF_CONNS];

        char *serverPort;
        char *response[MAX_NUM_OF_CONNS];

        int listenerSocket;
        int nextActiveConn;
        int numOfConns;
        int activeConns[MAX_NUM_OF_CONNS];
        int connTime[MAX_NUM_OF_CONNS];
        int transTime[MAX_NUM_OF_CONNS];

        // Methods
        int listenAndAccept();
        int findPosOfConn(int);
        int findNextActiveConnSlot();
        int findNumOfActiveConns();

    public:
        WebServer(bool, char *, unsigned int);
        ~WebServer();
        int runWebServer();
};
