//////////////////////////////////////////////////////////////////
//                                                              //
// This program is to help simulate a basic disk-storage server //
//                                                              //
//////////////////////////////////////////////////////////////////

#include "cse356header.h"
#define BLOCKSIZE 256


// GLOBAL VARIABLES

char buffer[512];
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;
int CYLINDERS,SECTORS,DELAY,PORT_NO;
int num=0,track=0,delay=0;
int fd,sockfd,newsockfd;


//////////////////////////
//                      //
// BASIC FUNCTION TOOLS //
//                      //
//////////////////////////


// Print error message and exit
void error(const char *msg)
{
   close(fd);
   perror(msg);
   exit(-1);
}

// Locate the virtual disk
int seek(int c,int s)
{
   if (c<0 || c>=CYLINDERS)
      return -1;
   if (s<0 || s>=SECTORS)
      return -1;
   usleep(DELAY*abs(track-c));
   delay += DELAY*abs(track-c);
   track = c;
   return BLOCKSIZE*(c*SECTORS+s);
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


////////////////////////////////
//                            //
// SOCKET COMMUNICATION TOOLS //
//                            //
////////////////////////////////

void init(char *FILENAME)
{
   // Open a file
    fd = open(FILENAME,O_RDWR|O_CREAT,0);
    if (fd<0) error("Error opening the file");
    // Stretch the file to the size of the simulated disk
    long FILESIZE = BLOCKSIZE*SECTORS*CYLINDERS;
    if (lseek(fd,FILESIZE-1,SEEK_SET)==-1) 
       error("Error calling lseek() to 'stretch' the file");
    if (write(fd,"",1)!=1)
       error("Error writing last byte of the file");
    // Map the file
    char *diskfile = (char *) mmap(0,FILESIZE,
                     PROT_READ|PROT_WRITE,
                     MAP_SHARED,fd,0);
    if (diskfile==MAP_FAILED)
       error("Error mapping the file");
   
   // Create a new socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd<0) error("Error opening socket");
    // Clear and set 'serv_addr'
    bzero((char *) &serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_NO);
    // Bind the socket to serv_addr
    if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0) 
       error("Error on binding"); 
    // Listen on the socket for connections
    listen(sockfd,5);  
    clilen = sizeof(cli_addr);
    // Wait for conncetion from a client
    newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
    if (newsockfd<0) error("Error on accept");
}

// Read request from the client
void getMsg()
{
   bzero(buffer,512);
   if (read(newsockfd,buffer,511)<0)
      error("Error reading from socket");
   printf("Request %d: %s\n",num,buffer);
}

// Start to phrase a respond
void startRes()
{
   bzero(buffer,512);
   encode(buffer,num++);
   strcat(buffer,": ");
}

// Write respond to the client
void sendMsg(char *str=(char*)"")
{
   strcat(buffer,str);
   if (write(newsockfd,buffer,strlen(buffer))<0)
      error("Error writing to socket");
   if (buffer[0]!='#')
      printf("Respond %s\n\n",buffer);
}


int main(int argc,char *argv[])
{
// Decode the arguments of the program
    char *FILENAME = argv[1];
    CYLINDERS = atoi(argv[2]);
    if (CYLINDERS<1||CYLINDERS>1024) {
       printf("CYLINDERS out of range.\n");
       exit(-1);
    }
    SECTORS = atoi(argv[3]);
    if (SECTORS<1||SECTORS>1024){
       printf("SECTORS out of range.\n");
       exit(-1);
    }
    DELAY = atoi(argv[4]);
    PORT_NO = atoi(argv[5]);
    
    init(FILENAME);
// Loop of service
    while (true) {
       int pos=2,c,s,l,offset;
       getMsg();
       // Request of Exit
       if (buffer[0]=='E') { 
          startRes();   
          strcat(buffer,"Total seek time is ");
          encode(buffer,delay);
          sendMsg((char*)" microseconds.\n");
          break;
       }
       // Inquiry about CYLINDERS and SECTORS
       else if (buffer[0]=='I'){ 
          startRes();
	  strcat(buffer,argv[2]);
          strcat(buffer," ");
          strcat(buffer,argv[3]);
          strcat(buffer,"\n");
	  sendMsg();
       }
       // Request of reading a block
       else if (buffer[0]=='R'){
          c = decode(buffer,pos);
          s = decode(buffer,pos);
          offset = seek(c,s);
          // Invalid reading request
          if (offset<0) {
             startRes();
             sendMsg((char*)"No\n");
          }
          else { // Valid reading request
             startRes();
             strcat(buffer,"Yes\n");
             lseek(fd,offset,0);
             if (read(fd,buffer+strlen(buffer),256)<0)
                error("Error reading the file");
             sendMsg((char*)"");
          }
       }
       // Request of writing a block
       else if (buffer[0]=='W'){
          c = decode(buffer,pos);
          s = decode(buffer,pos);
          l = decode(buffer,pos);
          if (l<256) l++;
          offset = seek(c,s);
          // Invalid writing request
          if (offset<0||l<0||l>257) {
             startRes();
             sendMsg((char*)"No\n");
          }
          else {  // Valid writing request
             lseek(fd,offset,0);
             char tmp[256];
             bzero(tmp,256);
             if (write(fd,tmp,256)<0)
                error("Error writing the file");
             lseek(fd,offset,0);
             if (write(fd,buffer+pos,l)<0)
                error("Error writing the file");
             startRes();
             sendMsg((char*)"Yes\n");
          }
       }
       else { // Nonsensical respond "#"
          bzero(buffer,512);
          sendMsg((char*)"#"); 
       }
    }
    close(sockfd);
    close(newsockfd);
    close(fd);
    return 0;
}

