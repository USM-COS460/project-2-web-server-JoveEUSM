#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

//#include <signal.h>
//#include <sys/types.h>
//#include <errno.h>
//#include <string.h>
//#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iphlpapi.h>



/*
GOALS

    CONVERT TO WINDOWS C, use winsock2 header instead of linux malarkey
    

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
 

void __cdecl clientHandler(void*);
void argHandler(char* argv[],char* portnum);
struct addrinfo* serverSetup(char* myPort);
void acceptHandler(SOCKET listener);
void requestParser(char* request,SOCKET sock);
void sigHan(int sig);
char* getErr(DWORD errNum);
void responseMaker(char* code, char* URI, SOCKET sock);
char* dateHeaderMaker();
char* mimeFinder(char* fileExt);

char* docRoot;
char* serverName;

int main(int argc,char* argv[]){
    //printf("heap max is %llu\n",_HEAP_MAXREQ);
    WSADATA wsadata;
    memset(&wsadata,0,sizeof wsadata);
    int checkStat;
    if((checkStat = WSAStartup(MAKEWORD(2,2),&wsadata)) !=0){
        printf("WSAStartup failed: it returned %s\n",getErr(checkStat));
        WSACleanup();
        exit(1);
    }


    signal(SIGSEGV,&sigHan);
    signal(SIGABRT,&sigHan);



    docRoot = malloc(512);
    serverName = malloc(255);
    memset(docRoot,0,512);
    memset(serverName,0,255);
    strcpy(serverName,argv[0]);

    if(argc!=3){
        printf("Improper number of arguments passed. Please pass the wanted port number and path to document file directory.\n");
        exit(1);
    }
    char* PORTNUM = malloc(16);
    memset(PORTNUM,0,16);
    argHandler(argv,PORTNUM);//does argv stuff. gets the wanted portnum and file directory
    
    //printf("portnum is %s\n",PORTNUM);
    
    
    struct addrinfo *ServResults = &(struct addrinfo){0};
    ServResults = serverSetup(PORTNUM);

    /*/ //kill all of this?
    printf("ai_flags: %d\n",ServResults->ai_flags);
    printf("ai_family: %d\n",ServResults->ai_family);//should be 2
    printf("ai_socktype: %d\n",ServResults->ai_socktype);//should be 1
    printf("ai_protocol: %d\n",ServResults->ai_protocol);//should be 6
    printf("ai_addrlen: %lld\n",ServResults->ai_addrlen);//should be 16
    printf("ai_canonname: %s\n",ServResults->ai_canonname);
    struct sockaddr* sockstuff = ServResults->ai_addr;
    printf("sa_family: %d\n",sockstuff->sa_family);
    printf("sa_data: %s\n",sockstuff->sa_data);
    
    struct sockaddr_in* sockin = (struct sockaddr_in *) sockstuff;
    printf("port num from sock is %d\n",sockin->sin_port);
    struct in_addr sinaddr = sockin->sin_addr;
    printf("addr is %ld ?\n",sinaddr.S_un.S_addr);
    

    PIP_ADAPTER_ADDRESSES paddrs = NULL;
    ULONG paddrret = 0;
    ULONG paddrlen = 10l;
    printf("paddrlen is %ld\n",paddrlen);
    while((paddrret = GetAdaptersAddresses(AF_INET,0,NULL,paddrs,&paddrlen))!=NO_ERROR){
        switch(paddrret){
            case ERROR_BUFFER_OVERFLOW:
                paddrs = NULL;
                printf("insufficient mem alloc for paddrs is %ld\n",paddrlen);
                paddrlen += 100;
                paddrs = malloc(paddrlen);
                printf("increased mem alloc for paddrs to %ld\n",paddrlen);
                break;
            case ERROR_ADDRESS_NOT_ASSOCIATED:
                printf("addr not assoc error\n");
                exit(1);
            case ERROR_INVALID_PARAMETER:
                printf("invalid param error\n");
                exit(1);
            case ERROR_NO_DATA:
                printf("no data error\n");
                exit(1);
            case ERROR_NOT_ENOUGH_MEMORY:
                printf("not enough mem error\n");
                exit(1);
            printf("other error: %s\n",getErr(paddrret));
            exit(1);
        }
        
    }
    struct sockaddr_in *psockin = (struct sockaddr_in*) paddrs->FirstUnicastAddress->Address.lpSockaddr;
    //psockin->sin_addr.S_un.S_addr
    printf("paddr is %ld ?\n",psockin->sin_addr.S_un.S_addr);

    printf("paddr b1 is %d ?\n",psockin->sin_addr.S_un.S_un_b.s_b1);
    printf("paddr b2 is %d ?\n",psockin->sin_addr.S_un.S_un_b.s_b2);
    printf("paddr b3 is %d ?\n",psockin->sin_addr.S_un.S_un_b.s_b3);
    printf("paddr b4 is %d ?\n",psockin->sin_addr.S_un.S_un_b.s_b4);
    //psockin->sin_addr.S_un.S_un_b.s_b1
    //psockin->sin_addr.S_un.S_un_b
    **/





    //gets the socket we will be listening on
    SOCKET listener = socket(ServResults->ai_family,ServResults->ai_socktype,ServResults->ai_protocol);
    if(listener==INVALID_SOCKET){
        printf("socket call error: %s\n",getErr(WSAGetLastError()));
        freeaddrinfo(ServResults);
        WSACleanup();
        exit(1);
    }
    
    //allows for the reuse of local addresses
    BOOL opt = TRUE;
    if((setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt)))!=0){
        printf("setsockopt error: %s\n",getErr(WSAGetLastError()));
        WSACleanup();
        freeaddrinfo(ServResults);
        exit(1);
    }

    if((setsockopt(listener,SOL_SOCKET,SO_KEEPALIVE,(char*)&opt,sizeof(opt)))!=0){
        printf("setsockopt error: %s\n",getErr(WSAGetLastError()));
        WSACleanup();
        freeaddrinfo(ServResults);
        exit(1);
    }

    int nopt = 2048;
    if((setsockopt(listener,SOL_SOCKET,SO_RCVBUF,(char *)&nopt,sizeof(int)))!=0){
        printf("setsockopt error: %s\n",getErr(WSAGetLastError()));
        WSACleanup();
        freeaddrinfo(ServResults);
        exit(1);
    }





    //binds the socket to the first option given by getaddrinfo, by now in the ServResults struct
    if((bind(listener,ServResults->ai_addr,ServResults->ai_addrlen))!=0){
        printf("bind error: %s\n",getErr(WSAGetLastError()));
        WSACleanup();
        freeaddrinfo(ServResults);
        exit(1);
    }
    freeaddrinfo(ServResults);

    //starts listening on the socket we have set up
    if((listen(listener,5))!=0){
        printf("listen error: %s\n",getErr(WSAGetLastError()));
        WSACleanup();
        exit(1);
    }

    free(PORTNUM);


    u_long ioctlmode = 0;
    ioctlsocket(listener,FIONBIO,&ioctlmode);//disables nonblocking


    acceptHandler(listener);  //handles everything after we start listening

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
    if((requestedPortNum<=1024) || (requestedPortNum>65535)){
        //if the given number is one of the reserved port numbers or is out of range
        printf("Invalid port number input. Please input a number both greater than 1024 and less than 65535\n");
        exit(1);
    }
    sprintf(PORTNUM,"%d",requestedPortNum);

    //printf("portnum is %s\n",PORTNUM);  //temp testing
    //printf("file path is %s\n",argv[2]);   //^

    if(strlen(argv[2])>=512){
        printf("oversized docroot provided. please include one less than 1024 bytes in length\n");
        exit(1);
    }
    strcpy(docRoot,argv[2]);
    strcat(docRoot,"\0");



    DWORD rootWord = GetFileAttributes(docRoot);
    
    if(rootWord == INVALID_FILE_ATTRIBUTES){
        printf("Error in given file path: %s\n",getErr(GetLastError()));
        exit(1);
    }else{
        if((rootWord & FILE_ATTRIBUTE_DIRECTORY) == 0){//for if file path given doesn't go to a directory
            printf("The given file path does not correspond to a directory/folder\n");
            exit(1);
        }
    }

    return;
}


/*
Sets up and returns the addrinfo struct that will contain the sockets available for us to use that fit our criteria

char* myPort - the string representation of the requested port number
*/
struct addrinfo* serverSetup(char* myPort){
    int status;                     //to store return val from getaddrinfo in case it's necessary for error handling
    struct addrinfo MyStandards = {0};    //will contain requested specifications for the server
    struct addrinfo *ServResults = &(struct addrinfo){0};   //pointer that will point to the results given by getaddrinfo

    memset(&MyStandards, 0, sizeof MyStandards);//empties wanted specs' existing arbitrary memory
    MyStandards.ai_family = AF_INET;          //use ipv4
    MyStandards.ai_socktype = SOCK_STREAM;      //want stream socket
    MyStandards.ai_flags = AI_PASSIVE;          //want sockets that can accept connections
    MyStandards.ai_flags |= AI_ADDRCONFIG;
    MyStandards.ai_protocol = IPPROTO_TCP;

    //initial getaddrinfo call to get available sockets
    if ((status = getaddrinfo(NULL, myPort, &MyStandards, &ServResults)) != 0) {
        printf("server initial getaddrinfo error: %s\n", getErr(status));//error message if above call fails
        WSACleanup();
        exit(1);
    }
    return ServResults;
}


/*
Handles the accept() calls for accepting connections
When a connection is accepted, that is handed off to clientHandler()

int sockFD - the file descriptor of the socket that is listening for connections
*/
void acceptHandler(SOCKET listener){
    struct sockaddr_storage GuesserAddr = {0};    //where info about the new connection will go
    socklen_t addrSize; //length of the above struct
    addrSize = sizeof GuesserAddr;
    memset(&GuesserAddr,0,sizeof(struct sockaddr_storage)); //empties struct


    SOCKET guesserSock; //socket for the new connection
    unsigned long long threadStat = 0;  //the returned handle for each new thread. used to check success of call
    unsigned int stackSize = 24000; //size in bytes that each thread should get

    printf("we're now open to connections\n");
    while(1){
        guesserSock = accept(listener,(struct sockaddr*)&GuesserAddr,&addrSize);//converted to winsock
        //printf("accept returned socket %llu\n",guesserSock);
        if(guesserSock==INVALID_SOCKET){
            printf("accept error, returned val is %llu : %s\n",guesserSock,getErr(WSAGetLastError()));
            shutdown(listener,SD_BOTH);
            shutdown(guesserSock,SD_BOTH);
            closesocket(listener);
            closesocket(guesserSock);
            WSACleanup();
            exit(1);
        }else{
            //makes new thread for each connection
            if((threadStat = _beginthread(clientHandler,stackSize,(void *)&guesserSock))==-1L){
                printf("Thread creation failed with following error: %s\n",strerror(errno));
                shutdown(listener,SD_BOTH);
                shutdown(guesserSock,SD_BOTH);
                closesocket(listener);
                closesocket(guesserSock);
                WSACleanup();
                exit(1);
            }
        }
    }
    closesocket(listener);
}


/*
Handle each client in here


*/
void __cdecl clientHandler(void* guesserSock){
    char* requestBuff = malloc(2048);

    SOCKET* sock = (SOCKET*)guesserSock;

    char* trash = malloc(2048);

    int talkStat = 0;
    while(1){
        memset(requestBuff,0,2048);
        talkStat = recv(*sock,requestBuff,2048,0);
        if(talkStat==0){    //case for client disconnecting
            printf("a client disconnected\n");
            free(requestBuff);
            free(trash);
            _endthread();
        }else if(talkStat==SOCKET_ERROR){ //case for error in recv
            printf("receive error: %s\n",getErr(WSAGetLastError()));
        }else if(talkStat==2048){ //case for oversized request
            printf("recv was too large\n");
            u_long ioctlmode = 1;
            ioctlsocket(*sock,FIONBIO,&ioctlmode);//enables nonblocking
            while(recv(*sock,trash,2048,0) > 0){
                //this removes bytes that - within a single send from the user - exceed the allocated buffer
            }
            ioctlmode = 0;
            ioctlsocket(*sock,FIONBIO,&ioctlmode);//disables nonblocking
        }
        //printf("Received:\n%s\n",requestBuff);
        requestParser(requestBuff,*sock);

    }
    free(requestBuff);
    free(trash);
    //printf("somehow got to end of clientHandler\n");
}


void requestParser(char* request,SOCKET sock){
    char* copy = malloc(2048);
    char* URI = malloc(512);
    memset(copy,0,2048);
    memset(URI,0,512);
    strcpy(copy,request);

    char* tok = malloc(2048);
    memset(tok,0,2048);

    if((tok = strtok(copy," ")) == NULL){
        responseMaker("405\0",URI,sock);
        free(copy);
        free(URI);
        return;
    }
    if((strcmp(tok,"\r\nGET")!=0) & (strcmp(tok,"GET")!=0)){
        //printf("GET request not found: %s instead\n",tok);
        responseMaker("405\0",URI,sock);
        free(copy);
        free(URI);
        return;
    }

    if((tok = strtok(NULL," ")) == NULL){
        responseMaker("405\0",URI,sock);
        free(copy);
        free(URI);
        return;
    }
    strcpy(URI,tok);    //the requested resource


    if((tok = strtok(NULL," ")) == NULL){
        responseMaker("405\0",URI,sock);
        free(copy);
        free(URI);
        return;
    }
    if((strstr(tok,"HTTP/1."))==NULL){
        //case for http part of request header missing
        //printf("No HTTP 1.x ver found in request-line: %s instead\n",tok);
        responseMaker("505\0",URI,sock);
        free(copy);
        free(URI);
        return;
    }

    if(URI[strlen(URI)-1]=='/'){
        //case for last char of uri being /, meaning should send index.html
        responseMaker("200\0",URI,sock);
    }



    char* path = malloc(1050);
    memset(path,0,1050);
    strcpy(path,docRoot);//512
    strcat(path,URI);//512
    //make path here

    DWORD fileWord = GetFileAttributes(path);
    free(path);
    if(fileWord == INVALID_FILE_ATTRIBUTES){
        //printf("Requested file not found: %s",getErr(GetLastError()));
        responseMaker("404\0",URI,sock);
        free(copy);
        free(URI);
        return;
    }
    free(copy);
    responseMaker("200\0",URI,sock);

    
    free(URI);
    return;
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
void responseMaker(char* code,char* URI,SOCKET sock){
    char* httpVer = "HTTP/1.1 \0";  //11 bytes
    int statusCode;
    char* reasonPhrase = malloc(32);//32

    



    memset(reasonPhrase,0,32);
    if((strcmp(code,"405"))==0){
        strcpy(reasonPhrase," Method Not Allowed\r\n\0");//23
    }else if((strcmp(code,"505"))==0){
        strcpy(reasonPhrase," HTTP Version not supported\r\n\0");//31
    }else if((strcmp(code,"404"))==0){
        strcpy(reasonPhrase," Not Found\r\n\0");//14
    }else{
        strcpy(reasonPhrase," OK\r\n\0");//7
    }

    /*
    for files over responsebuff size, just make header like its all one send, then send that with as much message body as possible,
    then send the rest of the body in subsequent send calls without any headers or formatting.
    just pretend its all one send call i guess
    */

    char* responseBuff = malloc(920);
    memset(responseBuff,0,920);
    

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

    strcpy(responseBuff,httpVer);//+11
    strcat(responseBuff,code);//+4
    strcat(responseBuff,reasonPhrase);//+32
    free(reasonPhrase);
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
    char* serverHeader = malloc(270);   //255 servername + 8 "Server: " + 4 \r\n + wiggle room
    memset(serverHeader,0,270);
    strcpy(serverHeader,"Server: ");//
    strcat(serverHeader,serverName);//
    strcat(serverHeader,"\r\n\0");//
    strcat(responseBuff,serverHeader);//+270  //server header is now added to responsebuff
    free(serverHeader);

    strcat(responseBuff,dateHeaderMaker());//+50
    strcat(responseBuff,"\r\n");//+3


    //all will go in with last added chars as \r\n i.e. newline
    if((strcmp(code,"404"))==0){  //if 404 bad uri
        //DONE
        //printf("send 404\n");
        strcat(responseBuff,"\r\n\0");//+4
        if((send(sock,responseBuff,strlen(responseBuff),0))==SOCKET_ERROR){
            printf("send error: %s\n",getErr(WSAGetLastError()));
            free(responseBuff);
            return;
        }
        return;
    }else if((strcmp(code,"405"))==0){    //if 405 method not allowed
        //DONE
        strcat(responseBuff,"Allow: GET\r\n\0");//+14
        strcat(responseBuff,"\r\n\0");//+4
        //printf("send 405 Allow\n");
        if((send(sock,responseBuff,strlen(responseBuff),0))==SOCKET_ERROR){
            printf("send error: %s\n",getErr(WSAGetLastError()));
            free(responseBuff);
            return;
        }
        return;
    }else if((strcmp(code,"505"))==0){    //if 505 http ver not supported
        //DONE
        strcat(responseBuff,"\r\n\0");//+4
        //printf("send 505 http ver\n");
        if((send(sock,responseBuff,strlen(responseBuff),0))==SOCKET_ERROR){
            printf("send error: %s\n",getErr(WSAGetLastError()));
            free(responseBuff);
            return;
        }
        return;
    }else if((strcmp(code,"200"))==0){    //if ok
        //use send flag MSG_MORE if not complete file

        //buffer has <= 370 by here. well within allocated mem


        char* path = malloc(1024);
        memset(path,0,1024);
        strcpy(path,docRoot);//+512
        strcat(path,URI);//+512
        //make path here
        
        char* fileExt = malloc(255);
        memset(fileExt,0,255);



        char* typeHeader = malloc(272);
        memset(typeHeader,0,272);
        strcpy(typeHeader,"Content-Type: \0");
        //let mediatype be up to 255

        char* lengthHeader = malloc(272);
        memset(lengthHeader,0,272);
        strcpy(lengthHeader,"Content-Length: \0");


        FILE* fp;

        if(URI[strlen(URI)-1]=='/'){
            //printf("send index.html\n");
            //should send index.html
            strcat(path,"index.html");
            fp = fopen(path,"r");
            free(path);
            strcat(typeHeader,"text/html\r\n\0");
            strcat(responseBuff,typeHeader);
            //printf("end of if\n");
        }else if((strstr(URI,"."))==NULL){//no . found in filename
            free(fileExt);
            //printf("send some file %s\n",URI);
            //treat file as binary
            fp = fopen(path,"rb");
            free(path);
            strcat(typeHeader,mimeFinder(NULL));
            strcat(typeHeader,"\r\n\0");
            strcat(responseBuff,typeHeader);    //type header added
        }else{
            fileExt = strrchr(URI,'.') + 1;
            //check if .html is requested
            if(fileExt==NULL){
                //treat file as binary
                //printf("send some file %s\n",URI);
                fp = fopen(path,"rb");
                free(path);
                strcat(typeHeader,mimeFinder(NULL));
                strcat(typeHeader,"\r\n\0");
                strcat(responseBuff,typeHeader);

            }else if((strcmp(fileExt,"html"))==0){
                //printf("send some html file %s\n",URI);
                //html was requested
                fp = fopen(path,"r");
                free(path);
                strcat(typeHeader,"text/html\r\n\0");
                strcat(responseBuff,typeHeader);    //type header added
            }else{//.html not file type
                //treat as binary
                //printf("send some file %s\n",URI);
                fp = fopen(path,"rb");
                free(path);
                strcat(typeHeader,mimeFinder(fileExt));//+255
                strcat(typeHeader,"\r\n\0");//+4
                strcat(responseBuff,typeHeader);    //type header added
            }
        }
        //buff has at most 629

        fseek(fp,0,SEEK_END);
        char* len = malloc(50);
        memset(len,0,50);
        int lenNum = ftell(fp);
        sprintf(len,"%d\r\n\r\n",lenNum);
        strcat(lengthHeader,len);
        strcat(responseBuff,lengthHeader);  //length header added
        free(lengthHeader);
        free(len);
        fseek(fp,0,SEEK_SET);
        //printf("after file length stuff\n");

        //buff has <=901
        
        if(lenNum==0){
            //printf("the requested file is empty\n");
            send(sock,responseBuff,strlen(responseBuff),0);
            fclose(fp);
            free(responseBuff);
            return;
        }
        
        struct _WSABUF wsabuf;// = (struct _WSABUF *)malloc(sizeof(struct _WSABUF));
        wsabuf.buf = responseBuff;
        wsabuf.len = strlen(responseBuff);
        //sends the http status line and headers
        long unsigned length = 0;
        length = strlen(responseBuff);
        if((statusCode = WSASend(sock,&wsabuf,1,&length,MSG_PARTIAL,NULL,NULL))!=0){
            printf("send error: %s\n",getErr(WSAGetLastError()));
            free(responseBuff);
            fclose(fp);
            return;
        }
        memset(responseBuff,0,920);
        memset(&wsabuf,0,sizeof wsabuf);
        while((statusCode = fread(responseBuff,1,920,fp))!=0){
            if(statusCode<920){
                if((statusCode = send(sock,responseBuff,strlen(responseBuff),0))==SOCKET_ERROR){
                    printf("send error: %s\n",getErr(WSAGetLastError()));
                    fclose(fp);
                    free(responseBuff);
                    return;
                }
                fclose(fp);
                free(responseBuff);
                return;
            }

            wsabuf.buf = responseBuff;
            wsabuf.len = strlen(responseBuff);
            length = strlen(responseBuff);
            if((statusCode = WSASend(sock,&wsabuf,1,&length,MSG_PARTIAL,NULL,NULL))!=0){
                printf("send error: %s\n",getErr(WSAGetLastError()));
                fclose(fp);
                free(responseBuff);
                return;
            }
            

            memset(responseBuff,0,920);
            memset(&wsabuf,0,sizeof wsabuf);
        }
    }
}



/*
unfinished

*/
char* dateHeaderMaker(){
    char* buffer = malloc(256);
    memset(buffer,0,256);
    strcpy(buffer,"Date:");//6
    time_t rawtime;
    time(&rawtime);
    struct tm *converted = &(struct tm){0};
    memset(converted,0,sizeof *converted);
    converted = gmtime(&rawtime);



    switch(converted->tm_wday){
        case 0:
            strcat(buffer,"Sun, ");
            break;
        case 1:
            strcat(buffer,"Mon, ");
            break;
        case 2:
            strcat(buffer,"Tue, ");
            break;
        case 3:
            strcat(buffer,"Wed, ");
            break;
        case 4:
            strcat(buffer,"Thu, ");
            break;
        case 5:
            strcat(buffer,"Fri, ");
            break;
        case 6:
            strcat(buffer,"Sat, ");
            break;
    }//6

    char* buff2 = malloc(20);
    memset(buff2,0,20);
    sprintf(buff2,"%02d ",converted->tm_mday);    //day of the month
    strcat(buffer,buff2);//6

    switch(converted->tm_mon){
        case 0:
            strcat(buffer,"Jan ");
            break;
        case 1:
            strcat(buffer,"Feb ");
            break;
        case 2:
            strcat(buffer,"Mar ");
            break;
        case 3:
            strcat(buffer,"Apr ");
            break;
        case 4:
            strcat(buffer,"May ");
            break;
        case 5:
            strcat(buffer,"Jun ");
            break;
        case 6:
            strcat(buffer,"Jul ");
            break;
        case 7:
            strcat(buffer,"Aug ");
            break;
        case 8:
            strcat(buffer,"Sep ");
            break;
        case 9:
            strcat(buffer,"Oct ");
            break;
        case 10:
            strcat(buffer,"Nov ");
            break;
        case 11:
            strcat(buffer,"Dec ");
            break;
    }//5

    memset(buff2,0,20);
    sprintf(buff2,"%04d ",(converted->tm_year+1900));    //year
    strcat(buffer,buff2);//6
    //date1 SP done

    memset(buff2,0,20);
    sprintf(buff2,"%02d:",converted->tm_hour);    //hour
    strcat(buffer,buff2);//6

    memset(buff2,0,20);
    sprintf(buff2,"%02d:",converted->tm_min);    //minute
    strcat(buffer,buff2);//6

    memset(buff2,0,20);
    sprintf(buff2,"%02d GMT",converted->tm_sec);    //second
    strcat(buffer,buff2);//9
    free(buff2);
    return buffer;//50
}


/*
    Gets the mime type for the given file extension
*/
char* mimeFinder(char* fileExt){
    if(fileExt==NULL){
        return "application/octet-stream";
    }

    char* buff = malloc(255);
    memset(buff,0,255);
    FILE* fp = fopen("mimeTypesExt.txt","rt");

    while(fgets(buff,255,fp)!=NULL){
        char* subBuff = strstr(buff,strcat(fileExt,"\t"));
        if(subBuff){
            subBuff = strstr(subBuff,"\t");
            if(subBuff){
                subBuff = subBuff + 1;
                return subBuff;
            }
        }

        memset(buff,0,255);
    }

    return "application/octet-stream";
}


void sigHan(int sig){
    if(sig==11){
        printf("segv caught\n");
    }else if(sig==22){
        printf("abrt caught\n");
    }else{
        printf("%d caught\n",sig);
    }
    
    exit(1);
}


/*
gets the error message of the corresponding error number
*/
char* getErr(DWORD errNum){
    char* errorMsg = malloc(256);
    memset(errorMsg,0,256);
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,0,errNum,0,errorMsg,256,0);
    return errorMsg;
}