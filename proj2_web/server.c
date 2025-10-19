#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
//#include <sys/socket.h>



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
char* PORTNUM;
char* argHandler(char* argv[]);


int main(int argc,char* argv[]){
    if(argc!=3){
        printf("Improper number of arguments passed. Please pass the wanted port number and path to document file directory.\n");
        exit(1);
    }
    PORTNUM = malloc(16);
    memset(PORTNUM,0,16);
    argHandler(argv);//does argv stuff. gets the wanted portnum and file directory
    
    struct 
    /*
    set up the server part above here
    */
    


    printf("%s\n",PORTNUM);

}






/*
SHOULD WORK

For parsing the expected portnum and file directory arguments passed with this program

expected argv is [programName,digits,string]
*/
char* argHandler(char* argv[]){
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

    return argv[2];//should be the file directory name
}