/////////////////////////////////////////////////
//                                             //
// This is a random-data client of BDS and IDS //
//                                             //
/////////////////////////////////////////////////

#include "cse356header.h"
#define TAIL buffer[strlen(buffer)]

// GLOBAL VARIABLES

char buffer[512];
struct hostent *server;
struct sockaddr_in serv_addr;
int PORT_NO,CYLINDERS,SECTORS;
int sockfd, num = 0;


//////////////////////////
//                      //
// BASIC FUNCTION TOOLS //
//                      //
//////////////////////////


// Print error message and exit
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// Convert char* into int
int decode(char *str,int &pos)
{
   int val = 0;
   for (;str[pos]>='0'&&str[pos]<='9';pos++)
       val = 10*val+str[pos]-'0';
   pos++;
   return val;
}

// Convert int into char*
void encode(char *str,int k)
{
    int pos = strlen(str);
    // It's painful to ignore 0
    if (k==0) { 
       str[pos++] = '0';
       return;
    }
    int t = k, len = 0;
    while (t>0){
       t /= 10; len++;
    } 
    pos += len;
    for (len=1;k>0;len++){
        str[pos-len]='0'+k%10;
        k /= 10;
    }
}

// Generate a random number from p to q
int random(int r,int p=0)
{
    return rand()%(r-p)+p;
}

// Generate a random character
char genChar()
{
    int val;
    do val = random(127,32);
       while (val==92);
    return (char) val;
}


////////////////////////////////
//                            //
// SOCKET COMMUNICATION TOOLS //
//                            //
////////////////////////////////


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

// Read respond from the server
void getMsg()
{
   bzero(buffer,512);
   if (read(sockfd,buffer,511)<0)
      error("Error reading from socket");
   if (buffer[0]!='#') {
      int pos = 0;
      decode(buffer,pos);
      if (strncmp(buffer+pos," Yes",4))
         printf("Respond %s\n\n",buffer);
      else {
         pos += 5;
         buffer[pos+1] = 0;
         printf("Respond %s\n\n",buffer);
      }
   }
}

// Write request to the server
void sendMsg(char *str=(char*)"")
{
   if (strlen(str)>0) {
      bzero(buffer,512);
      strcpy(buffer,str);
   }
   if (write(sockfd,buffer,strlen(buffer))<0)
      error("Error writing to socket");
}


/////////////////////
//                 //
// REQUEST WRITING //
//                 //
/////////////////////


// Test whether the server permits exit
bool exitPermit()
{
    int pos=0;
    for (;pos<strlen(buffer);pos++) {
        if (buffer[pos]=='T')
           break;
    }
    return !strncmp(buffer+pos,"Total seek time is ",19);
}

// Test whether the server will run next scheduler
bool nextPermit()
{
    for (int pos=0;pos<strlen(buffer);pos++) {
        if (!strncmp(buffer+pos,"takes ",6))
           return true;
    }
    return false;
}


// Loop of R/W requests
void clientLoop(int n)
{
    int j,c,s,pos;
    char op[2] = {'R','W'};
    for (int i=0;i<n;i++) {
       // Phrase a request
        bzero(buffer,512);
        j = random(2);
        buffer[0] = op[j];
        buffer[1] = ' ';
        pos = 2;
        c = random(CYLINDERS);
        encode(buffer,c);
        strcat(buffer," ");
        s = random(SECTORS);
        encode(buffer,s);
        strcat(buffer," ");
        if (j==1) { // data of W request
           encode(buffer,256);
           strcat(buffer," ");
           TAIL = genChar();
           printf("Request %d: %s\n",num++,buffer);
           for (int k=1;k<255;k++)
               TAIL = genChar();
        }
        else printf("Request %d: %s\n",num++,buffer);
        sendMsg();
        getMsg();
    }
}


int main(int argc, char *argv[])
{
    srand(time(NULL));
// Decode the arguments of the program
    server = gethostbyname(argv[1]);
    if (server==NULL) 
       error("Error: no such host.\n");
    PORT_NO = atoi(argv[2]); 
    
    init();
    int n, pos=0; 
// Send request I and get data
    sendMsg((char*)"I");
    printf("Request %d: I\n",num++);
    getMsg();
    decode(buffer,pos);
    CYLINDERS = decode(buffer,++pos);  
    printf("Totally %d cylinders, ",CYLINDERS);
    SECTORS = decode(buffer,pos);  
    printf("%d sectors per cylinder.\n",SECTORS); 
// Set the number of R/W requests
    printf("Please input the number of R/W requests: ");
    scanf("%d",&n);
    printf("\n\n");
    if (n<0 || n>100000){
       close(sockfd);
       printf("Number of requests out of range.\n");
       exit(-1);
    }
// Send requests to the server
    for (int i=0;i<3;i++) {
       clientLoop(n);
       sendMsg((char*)"E"); // Send request of exit
       printf("Request %d: E\n",num++);
       getMsg();
       if (exitPermit()) break;
       while (!nextPermit()) {
          sendMsg((char*)"\n");
          getMsg();
       }
    }
    close(sockfd);
    return 0;
}
