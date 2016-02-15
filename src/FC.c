#include "cse356header.h"

// GLOBAL VARIABLES

char buffer[512];
struct hostent *server;
struct sockaddr_in serv_addr;
int PORT_NO,sockfd, num = 0;

// FUNCTION DEFINITIONS

// Print error message and exit
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// Test whether the server permits exit
bool exitPermit()
{
    return !strncmp(buffer,"Exit ",5);
}

// Preparation for communication
void init()
{ 
    // Create a new socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd<0) error("Error opening socket");  
    // Clear and set 'serv_addr'
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(PORT_NO);
    // Establish a connection to the server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0) 
        error("Error connecting");
}


int main(int argc, char *argv[])
{
// Decode the arguments of the program
    server = gethostbyname(argv[1]);
    if (server==NULL) 
       error("Error: no such host.\n");
    PORT_NO = atoi(argv[2]); 

    init();
    printf("\nThis is a command-line driven client.\n\n");

// Loop of service
    while (true) {
        printf("Request %d: ",num);
        bzero(buffer,512);
        fgets(buffer,255,stdin); // User input
        buffer[strlen(buffer)-1] = '\0';
        // Send the request to the client
        if (write(sockfd,buffer,strlen(buffer))<0)
           error("Error writing to socket");
        if (buffer[0]=='\n') continue;
        // Get the respond from the server
        bzero(buffer,512);
        if (read(sockfd,buffer,511)<0)
           error("Error reading from socket");
        if (strcmp(buffer,"#"))
           printf("Respond %d: %s\n",num++,buffer);
        if (exitPermit())
           break; // Exit the loop
        
    }
    close(sockfd);
    return 0;
}
