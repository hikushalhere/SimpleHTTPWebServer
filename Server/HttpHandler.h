#include <iostream>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <ifaddrs.h>

#define INVALID 0
#define GET 1
#define HEAD 2
#define PUT 3
#define DELETE 4
#define TRACE 5
#define OPTIONS 6

#define OK "OK"
#define BAD_REQUEST "Ill-formed request"
#define NOT_ALLOWED "Not Allowed"
#define NOT_FOUND "Not Found"
#define REQUEST_TIMEOUT "Request Timed Out"

#define SUPPORTED_METHODS_HTTP10 "GET HEAD"
#define SUPPORTED_METHODS_HTTP11 "GET HEAD PUT DELETE TRACE OPTIONS"
#define ROOT "./Files"
#define DEFAULT_PAGE "index.html"
#define MAXBUFFSIZE 4096

#define LEN_STATUS_CODE 3   // The code will always be a 3 digit code.
#define LEN_HTTP_VERSION 9  // Length of HTTP/1.x
#define HTTP_10 "HTTP/1.0"
#define HTTP_11 "HTTP/1.1"
#define CONNECTION_CLOSE "Connection: close\r\n"
#define CONNECTION_PERSISTENT "Connection: persistent\r\n"
#define DATE "Date: "
#define SERVER "Server: myhttpd/1.0\r\n"
#define CONTENT_LENGTH "Content-Length: "

using namespace std;

class HttpHandler
{
    private:
        // Member variables
        bool clientHttp10;
        bool serverHttp10;
        bool isNew;

        int methodType;
        int statusCode;
        int serverTimeout;

        size_t sizeOfResponse;
        size_t fileSize;
        size_t dataLeft;
        size_t bufferSize;

        FILE *fileToFetch;
        FILE *fileToWrite;

        char *request;
        char *URL;
        char *version;
        char *statusPhrase;
        char *statusLine;
        char *header;
        char *response;
        char *msgBuffer;
        char *clientMsg;

        // Methods
        bool parseRequest();
        bool myStrCmp(const char *, int, const char *, int);
        void composeStatusLine();
        void composeHeader();
        void composeResponse();
        void getResponse();
        void responseGetMethod();
        void responseHeadMethod();
        void responsePutMethod();
        void responseDeleteMethod();
        void responseTraceMethod();
        void responseOptionsMethod();
        void responseBadRequest();
        void responseTimeOut();

    public:
        HttpHandler(bool, int);
        ~HttpHandler();
        size_t fileSizeForPublic; // Public variable just so it can be used for logging results. No use in the actual functioning of the server.
        bool serveConnection(char *, char **, size_t *, bool);
};
