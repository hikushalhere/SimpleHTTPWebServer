#include "WebServer.h"

#define DEFAULT_HTTP10 false
#define DEFAULT_PORT "8000"
#define DEFAULT_TIMEOUT 30

using namespace std;

unsigned int getIntFromString(char *);

int main(int argc, char *argv[])
{
    unsigned int myTimeout;
    bool correctArguments, http10;
    char myPort[6];

    http10 = DEFAULT_HTTP10;
    strcpy(myPort, DEFAULT_PORT); //handle error case
    myTimeout = DEFAULT_TIMEOUT;
    correctArguments = true;

    // Reading the arguments provided. Sample call --> myhttpd 1.1 -p=65000 -t=20
    if(argc > 1)
    {
	  	for(int i = 1; i < argc; i++)
	    {
	        char *argument;
	        argument = argv[i];

            // Looking for '-' switches
	        if(argument[0] == '-')
            {
                if(argument[1] == 'p')
                {
                    if(argument[2] == '=')
                    {
                        int j = 3;
                        char inputPort[6];

                        while(argument[j])
                            inputPort[j-3] = argument[j++];
                        inputPort[j-3] = argument[j];

                        unsigned int myPortInt = getIntFromString(inputPort);
                        if(myPortInt < 1024 || myPortInt > 65536)
                        {
                            correctArguments = false;
                            cout<<"\nThe port number should be larger than 1024 and less than 65536.";
                            break;
                        }
                        else
                            strcpy(myPort, inputPort); // handle error case
                    }
                    else
                    {
                        correctArguments = false;
                        cout<<"\nAn '=' sign expected after p";
                    }
                }
                else if(argument[1] == 't')
                {
                    if(argument[2] == '=')
                    {
                        int j = 3;
                        char inputTimeout[10];

                        while(argument[j])
                            inputTimeout[j-3] = argument[j++];
                        inputTimeout[j-3] = argument[j];

                        myTimeout = getIntFromString(inputTimeout);
                        if(myTimeout == 0)
                        {
                            correctArguments = false;
                            cout<<"\nThe timeout should be a number greater than 0";
                            break;
                        }
                    }
                    else
                    {
                        correctArguments = false;
                        cout<<"\nAn '=' sign expected after t";
                    }
                }
                else
                {
                    correctArguments = false;
                    cout<<"\nInvalid switch";
                    break;
                }
            }
            // Looking for the http version
            else if(argument[0] == '1' && argument[1] == '.')
            {
                if(argument[2] == '0')
                    http10 = true;
                else if(argument[2] == '1')
                    http10 = false;
                else
                {
                    correctArguments = false;
                    cout<<"\nIncorrect HTTP protocol. Should be either 1.0 or 1.1";
                    break;
                }

            }
            else
            {
                correctArguments = false;
                cout<<"\nInvalid switch";
                break;
            }
	    }
    }

    if(!correctArguments)
    {
        cout<<"\nHere is a sample call --> myhttpd 1.1 -p=65000 -t=20";
        cout<<"\nCan not start the web server. Sorry!\n";
        return -1;
    }

    WebServer myWebServer(http10, myPort, myTimeout);

    return myWebServer.runWebServer();
}

unsigned int getIntFromString(char *string)
{
    unsigned int myInt;

    if(string[0] == '-')
    {
        myInt = 0;
        cout<<"\nThe argument's value must be positive";
    }
    else
    {
        myInt = 0;
        while(*string)
            myInt = myInt*10 + *(string++)-48;
    }

    return myInt;
}
