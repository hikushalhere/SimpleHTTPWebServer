#include "HttpHandler.h"

HttpHandler::HttpHandler(bool http10, int timeout)
{
    this->serverHttp10 = http10;
    this->serverTimeout = timeout;
    this->isNew = true;

    this->sizeOfResponse = 0;
    this->fileSize = 0;
    this->dataLeft = 0;
    this->bufferSize = 0;
    this->fileSizeForPublic = 0;

    this->request = NULL;
    this->URL = NULL;
    this->version = NULL;
    this->statusPhrase = NULL;
    this->statusLine = NULL;
    this->header = NULL;
    this->response = NULL;
    this->msgBuffer = NULL;
    this->clientMsg = NULL;
    this->fileToFetch = NULL;
    this->fileToWrite = NULL;
}

HttpHandler::~HttpHandler()
{
    if(this->URL)
    {
        delete[] this->URL;
        this->URL = NULL;
    }
    if(this->version)
    {
        delete[] this->version;
        this->version = NULL;
    }
    if(this->statusPhrase)
    {
        delete[] this->statusPhrase;
        this->statusPhrase = NULL;
    }
    if(this->statusLine)
    {
        delete[] this->statusLine;
        this->statusLine = NULL;
    }
    if(this->header)
    {
        delete[] this->header;
        this->header = NULL;
    }
    if(this->response)
    {
        delete[] this->response;
        this->response = NULL;
    }
    if(this->msgBuffer)
    {
        delete[] this->msgBuffer;
        this->msgBuffer = NULL;
    }
    this->fileToFetch = NULL;
    this->fileToWrite = NULL;
}

bool HttpHandler::serveConnection(char *clientRequest, char **responseToSend, size_t *sizeOfResponseToSend, bool timedOut)
{
    if(timedOut)
        responseTimeOut();
    else
    {
        if(clientRequest)
        {
            this->request = clientRequest;
            this->isNew = true;
            bool parseOkay = parseRequest();
        }
        else
            this->isNew = false;

        getResponse();
    }

    *responseToSend = this->response;
    *sizeOfResponseToSend = this->sizeOfResponse;
    this->fileSizeForPublic = this->fileSize;

    if(this->dataLeft == 0)
        return true;
    else
        return false;
}

void HttpHandler::getResponse()
{
    switch(this->methodType)
    {
        case GET:
            responseGetMethod();
            break;

        case HEAD:
            responseHeadMethod();
            break;

        case PUT:
            responsePutMethod();
            break;

        case DELETE:
            responseDeleteMethod();
            break;

        case TRACE:
            responseTraceMethod();
            break;

        case OPTIONS:
            responseOptionsMethod();
            break;

        default:
            responseBadRequest();
            break;
    }
}

bool HttpHandler::parseRequest()
{
    int count, sizeOfRequest, start;
    char *clientRequest, *method, *httpString;

    clientRequest = this->request;
    sizeOfRequest = strlen(clientRequest);
    count = 0;

    // Parsing the method type
    start = count;
    while(clientRequest[count] != ' ' && clientRequest[count] != '\t' && count < sizeOfRequest)
        count++;
    method = new char[count-start+1];
    for(int i = start; i < count; i++)  // Constructing the method name from the request
        method[i-start] = clientRequest[i];
    method[count-start] = '\0';

    if(myStrCmp(method, count-start, "GET", strlen("GET")))
        this->methodType = GET;
    else if(myStrCmp(method, count-start, "HEAD", strlen("HEAD")))
        this->methodType = HEAD;
    else if(myStrCmp(method, count-start, "PUT", strlen("PUT")))
        this->methodType = PUT;
    else if(myStrCmp(method, count-start, "DELETE", strlen("DELETE")))
        this->methodType = DELETE;
    else if(myStrCmp(method, count-start, "TRACE", strlen("TRACE")))
        this->methodType = TRACE;
    else if(myStrCmp(method, count-start, "OPTIONS", strlen("OPTIONS")))
        this->methodType = OPTIONS;
    else
    {
        cout<<"\nMethod invalid";
        this->methodType = INVALID;
        this->statusCode= 400;
        return false;
    }
    count++;

    // Parsing the URL
    while((clientRequest[count] == ' ' || clientRequest[count] == '\t') && count < sizeOfRequest) // Skipping all blank spaces
        count++;
    start = count;
    while(clientRequest[count] != ' ' && clientRequest[count] != '\t' && count < sizeOfRequest) // Counting the length of the URL
        count++;
    // for zero length check error
    if(count - start == 1 && clientRequest[start] == '/')
        this->URL = new char[count-start+strlen(DEFAULT_PAGE)+1];
    else
        this->URL = new char[count-start+1];
    for(int i = start; i < count; i++)  // Constructing the URL
        this->URL[i-start] = clientRequest[i];
    if(count - start == 1 && clientRequest[start] == '/')
    {
        char defaultPage[] = DEFAULT_PAGE;
        for(int i = 0; i < strlen(DEFAULT_PAGE); i++)
            this->URL[count-start+i] = defaultPage[i];
        this->URL[count-start+strlen(DEFAULT_PAGE)] = '\0';
    }
    else
        this->URL[count-start] = '\0';

    // Parsing the client HTTP version
    while((clientRequest[count] == ' ' || clientRequest[count] == '\t') && count < sizeOfRequest) // Skipping all blank spaces
        count++;
    start = count;
    while(clientRequest[count] != ' ' && clientRequest[count] != '\t' && clientRequest[count] != '\r' && clientRequest[count] != '\n' && count < sizeOfRequest) // Counting the length of the http version signature
        count++;
    if(count-start == 8)
    {
        httpString = new char[count-start+1];
        for(int i = start; i < count; i++)  // Constructing the HTTP string from the request
            httpString[i-start] = clientRequest[i];
        httpString[count-start] = '\0';
        if(myStrCmp(httpString, count-start, "HTTP/1.0", strlen("HTTP/1.0")))
            this->clientHttp10 = true;
        else if(myStrCmp(httpString, count-start, "HTTP/1.1", strlen("HTTP/1.1")))
            this->clientHttp10 = false;
        else
        {
            cout<<"\nHTTP version invalid";
            this->statusCode = 400;
            delete[] httpString;
            httpString = NULL;
            return false;
        }
        delete[] httpString;
        httpString = NULL;
    }
    else
    {
        cout<<"\nHTTP string size invalid";
        this->statusCode = 400;
        return false;
    }

    if((clientRequest[count] == '\r' && clientRequest[count+1] == '\n') && count+1 < sizeOfRequest)
        count += 2;
    else if(clientRequest[count] == '\n' && count < sizeOfRequest)
        clientRequest++;
    else if(count == sizeOfRequest)
    {
        cout<<"\nNo crlf after first line";
        this->statusCode = 400;
        return false;
    }

    // Parsing the headers
    int crlf = 1;
    while(count < sizeOfRequest)
    {
        int headStart, headStop, valueStart, valueStop;

        if(clientRequest[count] == '\r')
        {
            count++;
            if(clientRequest[count] == '\n' && count < sizeOfRequest)
            {
                count++;
                crlf++;
                if(crlf == 2)
                    break;
                else
                    continue;
            }
            else if(count == sizeOfRequest)
            {
                cout<<"\nNo lf after cr";
                this->statusCode = 400;
                return false;
            }
        }
        else if(clientRequest[count] == '\n' && count < sizeOfRequest)
        {
            crlf++;
            if(crlf == 2)
                break;
            else
                continue;
        }

        headStart = count;
        while(clientRequest[count] != '\r' && clientRequest[count] != '\n' && count < sizeOfRequest)
        {
            if(clientRequest[count] == ':' && (clientRequest[count+1] == ' ' || clientRequest[count+1] == '\t'))
            {
                headStop = count - 1;
                valueStart = count + 1;
            }
            count++;
        }
        valueStop = count - 1;

        if(count == sizeOfRequest)
        {
            cout<<"\nno crlf after headers";
            this->statusCode = 400;
            return false;
        }
        if(clientRequest[count] == '\r')
            crlf = 0;
        else if(clientRequest[count] == '\n')
            crlf = 1;
    }

    // Extracting the message body
    if(count != sizeOfRequest)
    {
        start = count;
        while(count < sizeOfRequest)
            count++;
        this->clientMsg = new char[count-start];
        for(int i = start; i < count; i++)
            this->clientMsg[i-start] = clientRequest[i];
        this->sizeOfResponse = count - start;
    }

    this->statusCode = 200;
    return true;
}

bool HttpHandler::myStrCmp(const char *str1, int sizeOfStr1, const char *str2, int sizeOfStr2)
{
    if(sizeOfStr1 != sizeOfStr2)
        return false;

    for(int i = 0; i < sizeOfStr1; i++)
    {
        if(str1[i] != str2[i])
            return false;
    }
    return true;
}

void HttpHandler::responseGetMethod()
{
    // Brand new connection
    if(this->isNew)
    {
        char *absolutePath;
        int lenAbsolutePath;

        lenAbsolutePath = strlen(ROOT) + strlen(this->URL);
        absolutePath = new char[lenAbsolutePath+1];
        strcpy(absolutePath, ROOT);
        strcat(absolutePath, this->URL);
        this->fileToFetch = fopen(absolutePath, "r");
        if(this->fileToFetch)
        {
            if(!fseek(this->fileToFetch, 0, SEEK_END))
            {
                this->fileSize = ftell(this->fileToFetch);
                this->dataLeft = this->fileSize;
            }
            else
            {
                this->statusCode = 500;
                if(this->fileToFetch)
                {
                    fclose(this->fileToFetch);
                    this->fileToFetch = NULL;
                }
                this->msgBuffer = NULL;
                this->dataLeft = 0;
            }
            if(!fseek(this->fileToFetch, 0, SEEK_SET))
                this->statusCode = 200;
            else
            {
                this->statusCode = 500;
                if(this->fileToFetch)
                {
                    fclose(this->fileToFetch);
                    this->fileToFetch = NULL;
                }
                this->msgBuffer = NULL;
                this->dataLeft = 0;
            }
        }
        else
        {
            this->msgBuffer = NULL;
            this->statusCode = 404;
            this->dataLeft = 0;
        }
        composeStatusLine();
        composeHeader();
    }
    // Old connection
    else
    {
        if(this->fileToFetch)
        {
            if(this->dataLeft > MAXBUFFSIZE)
            {
                this->bufferSize = MAXBUFFSIZE;
                this->dataLeft -= bufferSize;
            }
            else
            {
                this->bufferSize = this->dataLeft;
                this->dataLeft = 0;
            }

            if(this->msgBuffer != NULL)
            {
                delete[] this->msgBuffer;
                this->msgBuffer = NULL;
            }
            this->msgBuffer = new char[this->bufferSize];
            fread(this->msgBuffer, sizeof(char), this->bufferSize, this->fileToFetch);
            this->statusLine = NULL;
            this->header = NULL;
        }
    }
    composeResponse();

    if(this->dataLeft == 0 && this->fileToFetch != NULL)
        fclose(this->fileToFetch);
}

void HttpHandler::responseHeadMethod()
{
    int lenAbsolutePath;
    char *absolutePath;

    lenAbsolutePath = strlen(ROOT) + strlen(this->URL);
    absolutePath = new char[lenAbsolutePath+1];
    strcpy(absolutePath, ROOT);
    strcat(absolutePath, this->URL);
    this->fileToFetch = fopen(absolutePath, "r");
    if(this->fileToFetch)
    {
        this->statusCode = 200;
        fclose(this->fileToFetch);
    }
    else
        this->statusCode = 404;

    composeStatusLine();
    composeHeader();
    composeResponse();
}

void HttpHandler::responsePutMethod()
{
    // Brand new connection
    if(this->isNew)
    {
        char *absolutePath;
        int lenAbsolutePath, sizeOfStatusLine;

        lenAbsolutePath = strlen(ROOT) + strlen(this->URL);
        absolutePath = new char[lenAbsolutePath+1];
        strcpy(absolutePath, ROOT);
        strcat(absolutePath, this->URL);

        this->fileToWrite = fopen(absolutePath, "w");
        if(this->fileToWrite)
            this->dataLeft = this->fileSize;
        else
        {
            this->msgBuffer = NULL;
            this->statusCode = 403;
            this->dataLeft = 0;
        }
    }
    // Old connection
    else
    {
        if(this->fileToWrite)
        {
            if(this->dataLeft > MAXBUFFSIZE)
            {
                this->bufferSize = MAXBUFFSIZE;
                this->dataLeft -= bufferSize;
            }
            else
            {
                this->bufferSize = this->dataLeft;
                this->dataLeft = 0;
            }
            if(this->msgBuffer != NULL)
            {
                delete[] this->msgBuffer;
                this->msgBuffer = NULL;
            }
            this->msgBuffer = new char[this->bufferSize];
            int posInClientBuffer = this->fileSize - this->dataLeft - 1;
            for(int i = bufferSize-1; i >= 0; i--)
                this->msgBuffer[i] = this->clientMsg[posInClientBuffer--];
            fwrite(this->msgBuffer, sizeof(char), this->bufferSize, this->fileToWrite);
        }
    }

    if(this->dataLeft == 0)
    {
        if(this->fileToWrite)
            fclose(this->fileToWrite);
        composeStatusLine();
        composeHeader();
        composeResponse();
    }
}

void HttpHandler::responseDeleteMethod()
{
    char *absolutePath;
    int lenAbsolutePath;

    lenAbsolutePath = strlen(ROOT) + strlen(this->URL);
    absolutePath = new char[lenAbsolutePath+1];
    strcpy(absolutePath, ROOT);
    strcat(absolutePath, this->URL);
    if(remove(absolutePath) != 0)
    {
        perror("Errror deleting file");
        this->statusCode = 404;
    }
    else
        this->statusCode = 200;

    composeStatusLine();
    composeHeader();
    composeResponse();
}

void HttpHandler::responseTraceMethod()
{
    if(this->isNew)
    {
        this->dataLeft = strlen(this->request);
        this->fileSize = this->dataLeft;
        this->bufferSize = this->dataLeft;
        this->statusCode = 200;
        composeStatusLine();
        composeHeader();
    }
    else
    {
        if(this->msgBuffer != NULL)
        {
            delete[] this->msgBuffer;
            this->msgBuffer = NULL;
        }
        this->msgBuffer = new char[this->dataLeft];
        strcpy(this->msgBuffer, this->request);
        this->dataLeft = 0;
        this->statusLine = NULL;
        this->header = NULL;
    }
    composeResponse();
}

void HttpHandler::responseOptionsMethod()
{
    if(this->isNew)
    {
        if(this->serverHttp10)
            this->dataLeft = strlen(SUPPORTED_METHODS_HTTP10);
        else
            this->dataLeft = strlen(SUPPORTED_METHODS_HTTP11);
        this->fileSize = this->dataLeft;
        this->bufferSize = this->dataLeft;
        this->statusCode = 200;
        composeStatusLine();
        composeHeader();
    }
    else
    {
        if(this->msgBuffer != NULL)
        {
            delete[] this->msgBuffer;
            this->msgBuffer = NULL;
        }
        this->msgBuffer = new char[this->dataLeft];
        if(this->serverHttp10)
            strcpy(this->msgBuffer, SUPPORTED_METHODS_HTTP10);
        else
            strcpy(this->msgBuffer, SUPPORTED_METHODS_HTTP11);
        this->dataLeft = 0;
        this->statusLine = NULL;
        this->header = NULL;
    }
    composeResponse();
}

void HttpHandler::responseBadRequest()
{
    composeStatusLine();
    composeHeader();
    composeResponse();
}

void HttpHandler::responseTimeOut()
{
    this->statusCode = 408;
    composeStatusLine();
    composeHeader();
    composeResponse();
}

void HttpHandler::composeStatusLine()
{
    int lenStatusLine, lenStatusCode, lenStatusPhrase, myStatusCode;
    char *statusCode, httpVersion[LEN_HTTP_VERSION];

    myStatusCode = this->statusCode;
    lenStatusCode = LEN_STATUS_CODE;
    statusCode = new char[lenStatusCode+1];
    statusCode[lenStatusCode--] = '\0';
    while(myStatusCode)
    {
        char digit;
        digit = 48 + (myStatusCode % 10);
        myStatusCode /= 10;
        statusCode[lenStatusCode--] = digit;
    }

    if(this->serverHttp10)
        strcpy(httpVersion, HTTP_10);
    else
        strcpy(httpVersion, HTTP_11);

    if(this->statusCode == 200)
    {
        lenStatusPhrase = sizeof(OK);
        this->statusPhrase = new char[lenStatusPhrase+3];
        strcpy(this->statusPhrase, OK);
    }
    else if(this->statusCode == 400)
    {
        lenStatusPhrase = sizeof(BAD_REQUEST);
        this->statusPhrase = new char[lenStatusPhrase+3];
        strcpy(this->statusPhrase, BAD_REQUEST);
    }
    else if(this->statusCode == 403)
    {
        lenStatusPhrase = sizeof(NOT_ALLOWED);
        this->statusPhrase = new char[lenStatusPhrase+3];
        strcpy(this->statusPhrase, NOT_ALLOWED);
    }
    else if(this->statusCode == 404)
    {
        lenStatusPhrase = sizeof(NOT_FOUND);
        this->statusPhrase = new char[lenStatusPhrase+3];
        strcpy(this->statusPhrase, NOT_FOUND);
    }
    else if(this->statusCode == 408)
    {
        lenStatusPhrase = sizeof(REQUEST_TIMEOUT);
        this->statusPhrase = new char[lenStatusPhrase+3];
        strcpy(this->statusPhrase, REQUEST_TIMEOUT);
    }
    strcat(this->statusPhrase, "\r\n");

    lenStatusLine = strlen(httpVersion) + 1 + strlen(statusCode) + 1 + strlen(this->statusPhrase);
    this->statusLine = new char[lenStatusLine+1];
    strcpy(this->statusLine, httpVersion);
    strcat(this->statusLine, " ");
    strcat(this->statusLine, statusCode);
    strcat(this->statusLine, " ");
    strcat(this->statusLine, this->statusPhrase);
    delete[] statusCode;
}

void HttpHandler::composeHeader()
{
    time_t  currTime;
    char *connectionHeader, *date, *dateHeader, *serverHeader, *contentLengthHeader;
    int lenHeader, len, temp, i;

    // Connection header
    if(this->serverHttp10)
    {
        len = strlen(CONNECTION_CLOSE);
        connectionHeader = new char[len+1];
        strcpy(connectionHeader, CONNECTION_CLOSE);
    }
    else
    {
        len = strlen(CONNECTION_PERSISTENT);
        connectionHeader = new char[len+1];
        strcpy(connectionHeader, CONNECTION_PERSISTENT);
    }

    // Date header
    time(&currTime);
    date = ctime(&currTime);
    len = strlen(DATE) + strlen(date);
    dateHeader = new char[len+1];
    strcpy(dateHeader, DATE);
    strcat(dateHeader, date);
    // Not concatenating "\r\n" since the date string contains \n which is fine

    // Server header
    len = strlen(SERVER);
    serverHeader = new char[len+1];
    strcpy(serverHeader, SERVER);

    // Content-Length header
    temp = this->fileSize;
    len = 0;
    while(temp)
    {
        len++;
        temp = temp/10;
    }
    len += strlen(CONTENT_LENGTH) + 2; // +2 for \r\n
    contentLengthHeader = new char[len+1];
    strcpy(contentLengthHeader, CONTENT_LENGTH);
    temp = this->fileSize;
    i = len;
    contentLengthHeader[i--] = '\0';
    contentLengthHeader[i--] = '\n';
    contentLengthHeader[i--] = '\r';
    do
    {
        char digit;
        digit = 48 + (temp % 10);
        temp /= 10;
        contentLengthHeader[i--] = digit;
    }while(temp);

    // Composing the header now
    lenHeader = strlen(connectionHeader) + strlen(dateHeader) + strlen(serverHeader) + strlen(contentLengthHeader); // +2 for the trailing \r\n
    this->header = new char[lenHeader+1];
    strcpy(this->header, connectionHeader);
    strcat(this->header, dateHeader);
    strcat(this->header, serverHeader);
    strcat(this->header, contentLengthHeader);

    delete[] connectionHeader;
    delete[] dateHeader;
    delete[] serverHeader;
    delete[] contentLengthHeader;
}

void HttpHandler::composeResponse()
{
    char *currMemLoc;
    char crlf[] = "\r\n";
    this->sizeOfResponse = 0;

    if(this->isNew)
    {
        if(this->statusLine)
            this->sizeOfResponse += strlen(this->statusLine);
        if(this->header)
            this->sizeOfResponse += strlen(this->header);
        this->sizeOfResponse += strlen(crlf);
    }
    else if(this->msgBuffer)
        this->sizeOfResponse += this->bufferSize;
    if(this->response != NULL)
    {
        delete[] this->response;
        this->response = NULL;
    }
    this->response = new char[this->sizeOfResponse];

    currMemLoc = this->response;
    if(this->isNew)
    {
        if(this->statusLine)
        {
            memcpy(currMemLoc, this->statusLine, strlen(this->statusLine));
            currMemLoc += strlen(this->statusLine);
        }
        if(this->header)
        {
            memcpy(currMemLoc, this->header, strlen(this->header));
            currMemLoc += strlen(this->header);
        }
        memcpy(currMemLoc, crlf, strlen(crlf));
    }
    else if(this->msgBuffer)
        memcpy(currMemLoc, this->msgBuffer, this->bufferSize);
}
