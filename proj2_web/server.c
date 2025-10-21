#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>



/*
GOALS
    make server like in proj1
    have it parse html requests
    have it give html responses
    have it threaded and thread safe

TODO
    make server
    have it make a new thread per connection
    then have request parser
    then have response maker
    write html part
    write css part

    look at header formats and whatnot


TO THREAD
    use pthread_create(&pthread_t,)


NOTES/REMINDS
    only need one socket and related stuff since it threads
    so have main thread loop through accepting, then have threads handle each client
    pass the client's fd as arg for pthread_create
    
    command line should take port number and file directory for site
        handle this in argHandler
        have it return [int portnum,char* file directory]
    
    the file directory is where to pull files from for responses
    its where the html and images should be. basically the www example.
    its up to the person that starts the server to make sure they use the right file directory
    
    BUT if a request from a client has a GET /stuff/smth/ and it ends with a /, just default to sending the html from the chosen filedir
    open images and non text stuff in binary mode


*/
 

void* clientHandler(void*);
void argHandler(char* argv[],char* portnum);
struct addrinfo* serverSetup(char* myPort);
void acceptHandler(int sockFD);
char* requestParser(char* request,int clientFD);


char* docRoot;

int main(int argc,char* argv[]){
    docRoot = malloc(4096);
    memset(docRoot,0,4096);


    if(argc!=3){
        printf("Improper number of arguments passed. Please pass the wanted port number and path to document file directory.\n");
        exit(1);
    }
    char* PORTNUM = malloc(16);
    memset(PORTNUM,0,16);
    argHandler(argv,PORTNUM);//does argv stuff. gets the wanted portnum and file directory
    printf("portnum is %s\n",PORTNUM);
    
    struct addrinfo *ServResults = serverSetup(PORTNUM);

    //gets the socket we will be listening on
    int sockFD = socket(ServResults->ai_family,ServResults->ai_socktype,ServResults->ai_protocol);
    if(sockFD==-1){
        fprintf(stdout,"socket call error: %s \n",strerror(errno));
        exit(1);
    }
    
    //allows for the reuse of local addresses
    int enabled = 1;
    if(setsockopt(sockFD,SOL_SOCKET,SO_REUSEADDR,&enabled,sizeof(enabled))!=0){
        fprintf(stdout,"setsockopt error: %s\n",strerror(errno));
        exit(1);
    }

    //binds the socket to the first option given by getaddrinfo, by now in the ServResults struct
    if(bind(sockFD,ServResults->ai_addr,ServResults->ai_addrlen)!=0){
        fprintf(stdout,"bind error: %s\n",strerror(errno));
        exit(1);
    }

    //starts listening on the socket we have set up
    if(listen(sockFD,5)!=0){
        fprintf(stdout,"listen error: %s\n",strerror(errno));
        exit(1);
    }

    free(PORTNUM);

    acceptHandler(sockFD);  //handles everything after we start listening

    /*
    set up the server part above here
    */
}






/*
SHOULD WORK

For parsing the expected portnum and file directory arguments passed with this program

expected argv is [programName,digits,string]

don't need to return file path since it doesn't change in here, it's just validated
so can safely return just the portnum. actually don't need to do that either.
*/
void argHandler(char* argv[],char* PORTNUM){
    int requestedPortNum = strtol(argv[1],NULL,10); //converts to an int for the sake of ease of comparison
    if(requestedPortNum<=1024 || requestedPortNum>65535){
        //if the given number is one of the reserved port numbers or is out of range
        printf("Invalid port number input. Please input a number both greater than 1024 and lesser than 65535\n");
        exit(1);
    }
    sprintf(PORTNUM,"%d",requestedPortNum);

    printf("portnum is %s\n",PORTNUM);  //temp testing
    printf("file path is %s\n",argv[2]);   //^


    struct stat filedir;    //structure to contain information about the file from the given path
    memset(&filedir,0,sizeof filedir);  //empties the aforementioned struct

    if(stat(argv[2],&filedir)!=0){  //returns -1 if there is an error in getting file info
        printf("Error in given file path: %s\n",strerror(errno));
        exit(1);
    }else{
        if(!S_ISDIR(filedir.st_mode)){//for if file path given doesn't go to a directory
            printf("The given file path does not correspond to a directory/folder\n");
            exit(1);
        }
    }
    
    strcpy(docRoot,argv[2]);


    return;
}


/*
Sets up and returns the addrinfo struct that will contain the sockets available for us to use that fit our criteria

char* myPort - the string representation of the requested port number
*/
struct addrinfo* serverSetup(char* myPort){
    int status;                     //to store return val from getaddrinfo in case it's necessary for error handling
    struct addrinfo MyStandards;    //will contain requested specifications for the server
    struct addrinfo *ServResults;   //pointer that will point to the results given by getaddrinfo

    memset(&MyStandards, 0, sizeof MyStandards);//empties wanted specs' existing arbitrary memory
    MyStandards.ai_family = AF_UNSPEC;          //we don't care which we use
    MyStandards.ai_socktype = SOCK_STREAM;      //want stream socket
    MyStandards.ai_flags = AI_PASSIVE;          //want sockets that can accept connections
    
    //initial getaddrinfo call to get available sockets
    if ((status = getaddrinfo(NULL, myPort, &MyStandards, &ServResults)) != 0) {
        fprintf(stdout, "server initial getaddrinfo error: %s\n", gai_strerror(status));//error message if above call fails
        exit(1);
    }
    return ServResults;
}


/*
Handles the accept() calls for accepting connections
When a connection is accepted, that is handed off to clientHandler()

int sockFD - the file descriptor of the socket that is listening for connections
*/
void acceptHandler(int sockFD){
    struct sockaddr_storage GuesserAddr;
    socklen_t addrSize;
    addrSize = sizeof GuesserAddr;
    memset(&GuesserAddr,0,sizeof(struct sockaddr_storage));


    int guesserFD;
    int threadStat = 0;
    pthread_t newestThreadID = 0;
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);

    fprintf(stdout,"we're now open to connections\n");
    while(1){
        guesserFD = accept(sockFD,(struct sockaddr*)&GuesserAddr,&addrSize);
        printf("accept returned %d\n",guesserFD);
        if(guesserFD==-1){
            fprintf(stdout,"accept error, returned val is %d : %s\n",guesserFD,strerror(errno));
            exit(1);
        }else{
            //make new thread, pass it guesserfd
            if(threadStat = pthread_create(newestThreadID,&attributes,clientHandler,guesserFD)!=0){
                fprintf(stdout,"Thread creation failed with following error number: %d\n",threadStat);
                exit(1);
            }
        }
    }
}


/*
Handle each client in here


*/
void* clientHandler(void* clientFD){
    //just have loop for parsing requests and sending responses?
    //limit size of response?
    char* requestBuff = malloc(2048);

    char* trash = malloc(2048);

    int talkStat = 0;
    while(1){
        memset(requestBuff,0,2048);
        talkStat = recv(clientFD,requestBuff,2048,0);

        if(talkStat==0){    //case for client disconnecting
            fprintf(stdout,"client disconnected\n");
            free(requestBuff);
            free(trash);
            pthread_exit(pthread_self());
        }else if(talkStat==-1){ //case for error in recv
            fprintf(stdout,"receive error: %s\n",strerror(errno));
        }else if(talkStat==2048){ //case for oversized request
            printf("recv was too large\n");
            while(recv(clientFD,trash,2048,MSG_DONTWAIT) > 0){
                //this removes bytes that - within a single send from the user - exceed the allocated buffer
            }
        }
        printf("Received:\n%s\n",requestBuff);
        requestParser(requestBuff,clientFD);

    }
    free(requestBuff);
    free(trash);
}


char* requestParser(char* request,int clientFD){
    char* copy = malloc(2048);
    char* URI = malloc(512);
    memset(copy,0,2048);
    memset(URI,0,512);
    strcpy(copy,request);
    char* code = malloc(3);
    strcpy(code,"200");


    char* tok = strtok(copy," ");
    int tokCounter = 0;
    if(strcmp(tok,"\r\nGET")!=0 & strcmp(tok,"GET")!=0){
        printf("GET request not found: %s instead\n",tok);
        //handle improper request here somehow. maybe call func with codes for improper reason
        strcpy(code,"405");
    }
    tok = strtok(NULL," ");
    strcpy(URI,tok);    //the requested resource
    tok = strtok(NULL," ");
    if(strstr(tok,"HTTP/1.")==NULL){
        //case for http part of request header missing
        printf("No HTTP 1.x ver found in request-line: %s instead\n",tok);
        strcpy(code,"505");
    }

    char* path = malloc(4610);

    struct stat reqFile;
    memset(&reqFile,0,sizeof reqFile);
    if(stat(path,&reqFile)==-1){
        printf("Requested file not found: %s",strerror(errno));
        strcpy(code,"404");
    }

    responseMaker(code,path,clientFD);

    free(copy);
    free(URI);
    free(code);
    return URI; //kill return? have null?
}



/*
minimum of 4096 bytes read and write size in ipv4
default code is 200
implemented codes:
200 - ok
    send date, server, content type, content length
    body of requested file
405 - method not allowed
    send date, server, allow
505 - http ver not supported
    send date, server
    include body of what vers are supported (1.x)
404 - not found
    send date, server
*/
void responseMaker(char* code,char* path,int clientFD){
    char* httpVer = "HTTP/1.1 "; //9 bytes
    int statusCode = code;  //always 3 chars, so 3 bytes
    char* reasonPhrase = malloc(32);
    memset(reasonPhrase,0,32);
    if(strcmp(code,"405")==0){
        strcpy(reasonPhrase," Method Not Allowed\r\n");
    }else if(strcmp(code,"505")==0){
        strcpy(reasonPhrase," HTTP Version not supported\r\n");
    }else if(strcmp(code,"404")==0){
        strcpy(reasonPhrase," Not Found\r\n");
    }else{
        strcpy(reasonPhrase," OK\r\n");
    }

    /*
    for files over responsebuff size, just make header like its all one send, then send that with as much message body as possible,
    then send the rest of the body in subsequent send calls without any headers or formatting.
    just pretend its all one send call i guess
    */

    char* responseBuff = malloc(4096);
    memset(responseBuff,0,4096);
    int fillSize = 0;

    /*
    don't need to worry about extra / in filepath, fopen dgaf
    */

    /*
    have status line made here

    add date and server

    then check code, do extra stuff from this func comm

    then send

    loop body send if ok and over buff size
    */

    strcpy(responseBuff,httpVer);
    strcat(responseBuff,code);
    strcat(responseBuff,reasonPhrase);
    //status line is added

    /*
    response headers needed

    content-length
        length of body message in bytes, so not including headers. see 14.13
        only for 200 code
    content-type
        to get the list of media types, open registry editor, look for HKEY_CLASSES_ROOT\MIME\Database\Content Type
        see 14.17
        only for 200 code
    date
        only necessary one
        have own function for this. look at page 20 of rfc 2616 to see format
    server
        see 14.38
        include in all

    for 405 have allow header 14.7
    
    */

    //cat on server header here

    //check code here
    if(strcmp(code,"404")==0){  //if 404 bad uri

    }else if(strcmp(code,"200")==0){    //if ok
        FILE* fp = fopen(path,"rb");
        
        //put content type here
        //put content length here
        //to get file size, fseek to end, then do ftell, then fseek back to front

        //fill in rest of buffer here
        //send above
        //loop if needed

        //put below in loop. capture return val so it can be checked against -1 and 0
        if(send(clientFD,responseBuff,fillSize,0)==-1){ //have this be blocking so the message body is at least sent in the right order
            printf("Error in sending response: %s\n",strerror(errno));
        }

    }

    




    


}