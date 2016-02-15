//////////////////////////////////////////////////////////////
//                                                          //
// This program serves as a intelligent disk-storage system //
//                                                          //
//////////////////////////////////////////////////////////////

#include "cse356header.h"
#define INFINITE CYLINDERS+1
#define BLOCKSIZE 256


//////////////////////
//                  //
// GLOBAL VARIABLES //
//                  //
//////////////////////

// for responds to be sent
char buffer[512];
// for requests to be executed
char cache[512][512];

// file simulation and socket communication
int fd,sockfd,newsockfd;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

// for main program arguments
int CYLINDERS,SECTORS,DELAY,PORT_NO,NUM;
// service for random-client
int num=0,track=0,delay;
// for scheduling queues
int head,len,mode=0; 
int delayTable[3],dNum=0;
int cyli[512];  // track#
int reno[512];  // request#


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
   int t=2, pos=len; 
   bzero(cache[pos],512);
   if (read(newsockfd,cache[pos],511)<0)
      error("Error reading from socket");
   // Void Request "\n"
   if (cache[pos][0]=='\n') {
      cyli[pos] = INFINITE;
      return;
   }
   len = (len+1)%NUM;
   printf("Request %d: %s\n",num,cache[pos]);
   reno[pos] = num++;
   // Request of EXIT
   if (!strcmp(cache[pos],"E")) {
      cyli[pos] = -INFINITE;
      return;
   }
   // R/W request, record the cylinder
   cyli[pos] = decode(cache[pos],t);
}

// Start to phrase a respond
void startRes(int pos)
{
   bzero(buffer,512);
   encode(buffer,reno[pos]);
   strcat(buffer,": ");
}

// Write respond to the client
void sendMsg(char *str=(char*)"")
{
   strcat(buffer,str);
   if (write(newsockfd,buffer,strlen(buffer))<0)
      error("Error writing to socket");
   if (strcmp(buffer,"#"))
      printf("Respond %s\n",buffer);
}


////////////////////////////
//                        //
// REQUEST IMPLEMENTATION //
//                        //
////////////////////////////


// Get request 'I' and send CYLINDERS & SECTORS
// I request is forbidden in any other place
void firstRes(char *argv[])
{
    getMsg();
    if (!strcmp(buffer,"I")) {
       printf("I'm sorry, this IDS only serves BDC_random\n");
       printf("It's like a batch system rather than command-line driven.\n");
       printf("I'm more than grateful for your consideration.\n");
       close(fd); close(sockfd); close(newsockfd); exit(0);
    }
    startRes(511);
    strcat(buffer,argv[2]);
    strcat(buffer," ");
    strcat(buffer,argv[3]);
    sendMsg((char*)"\n");
}


// Execute the request in cache[pos]
// It will change the current track#
// and return if it's an EXIT request
bool exec(int pos)
{
    // Request of exit
    if (cache[pos][0]=='E') {
       startRes(pos);
       switch (mode) {
       case 0: 
         strcat(buffer,"FCFS takes "); break;
       case 1: 
         strcat(buffer,"SSTF takes "); break; 
       default: 
         strcat(buffer,"C-LOOK takes ");
       }
       delayTable[dNum++]=delay;
       encode(buffer,delay);
       sendMsg((char*)" microseconds.\n\n");
       return true;
    } // Void request
    else if (cache[pos][0]=='\n'){ 
       bzero(buffer,512);
       sendMsg((char*)"#");
       return false;
    }
    int t=2,c,s,l,offset;
    // Analyze the request
    c = decode(cache[pos],t);
    s = decode(cache[pos],t);
    offset = seek(c,s);
    // Request of reading a block
    if (cache[pos][0]=='R'){
       // Invalid reading request
       if (offset<0) { 
          startRes(pos);
          sendMsg((char*)"No\n");
       }
       else { // Valid reading request
          startRes(pos);
          strcat(buffer,"Yes ");
          lseek(fd,offset,0);
          if (read(fd,buffer+strlen(buffer),256)<0)
             error("Error reading the file");
          strcat(buffer,"\n");
          sendMsg();
       }
    }
    // Request of writing a block
    else if (cache[pos][0]=='W'){
       l = decode(cache[pos],t);
       // Invalid writing request
       if (offset<0||l<0||l>256){ 
          startRes(pos);
          sendMsg((char*)"No\n");
       }
       else { // Valid writing request
          lseek(fd,offset,0);
          char tmp[256];
          bzero(tmp,256);
          if (write(fd,tmp,256)<0)
             error("Error writing the file");
          lseek(fd,offset,0);
          if (write(fd,cache[pos]+t,l)<0)
             error("Error writing the file");
          startRes(pos);
          sendMsg((char*)"Yes\n");
       }
    }
    
    return false;
}

// Move request in the cache from i to j
void move(int i,int j)
{
    if (i==j) return;
    bzero(cache[j],512);
    for (int k=0;k<512;k++){
        if (!cache[i][k])
            break;
        cache[j][k] = cache[i][k];
    }
    cyli[j] = cyli[i];
    reno[j] = reno[i];
}

// SSTF scheduler executes a request
bool sstf_deq()
{
    len = (len+NUM-1)%NUM;
    int minId=len, min=abs(cyli[minId]-track);
    // Choose a request with minimal track delay
    for (int i=minId;i>=0;i--){
        if (abs(cyli[i]-track)<min){
           min = abs(cyli[i]-track);
           minId = i;
        }
    }
    exec(minId);
    move(len,minId);
    return len;
}

// Swap two items of the cache
void swap(int i,int j)
{
    if (i==j) return;
    char tmp; int t;
    for (int k=0;k<512;k++){
        if (!cache[i][k]&&!cache[j][k])
           break;
        tmp = cache[i][k];
        cache[i][k] = cache[j][k];
        cache[j][k] = tmp;
    }
    t = cyli[i];
    cyli[i] = cyli[j];
    cyli[j] = t;
    t = reno[i];
    reno[i] = reno[j];
    reno[j] = t;
}

// Quicksort concerning track#
void c_sort(int p,int r)
{
    if (r<=p) return;
    int q = (p+r)/2, i=p, j=r;
    while (i<j) {
       while (i<q && cyli[i]<cyli[q]) i++;
       while (j>q && cyli[j]>=cyli[q]) j--;
       if (i==q || j==q)
          q = i+j-q;
       swap(i,j);
    }
    c_sort(p,q-1);
    c_sort(q+1,r);
}

// Seek for a request in cache that has
//   the minimal track# larger than track
int c_search(int tail)
{
    for (int i=0;i<=tail;i++) {
       if (cyli[i]>=track)
          return i;
    }
    return tail+1;
}

// Execute all current requests in C_LOOK
// Precondition: EXIT request is in cache
//          or cache is full
void c_empty()
{
    // When there's only EXIT remains
    //  we have to execute it at once and
    //  there's no need to get request again
    if (len==1) {
        exec(0); return;
    }
    // Partition cache into two parts:
    //  from cache[0] to cache[len-1] are new requests
    //  from cache[head] to cache[tail] are old requests
    // There will be no contention between two parts
    //  since len will never increment faster than head
    int tail = (len+NUM-1)%NUM;
    len = head = 0;
    // sort in light of track#
    c_sort(0,tail); 
    // locate the first request to be executed
    int i=c_search(tail);
    if (cyli[0]<0) {  // EXIT will be regarded as a new request
       len++; head++; //  unless it is the only remaining request
    }
    // Execute requests from cache[i] to cache[tail]
    //  after which from cache[head] to cache[tail] are
    //  still old requests that remain to be executed
    while (i<=tail) {
       exec(i);
       // Move old requests to ensure they are contiguous
       for (int k=i++;k>head;k--)
           move(k-1,k);
       head++; // to ensure new requests never lack space
       getMsg();
    }
    // Jump across the disk since it's C-LOOK
    while (head<=tail) { // and execute other old requests
       exec(head++);
       getMsg();
    }
    printf(" \n");
    // Recursive function call to execute remaining requests
    c_empty();
}


int main(int argc,char *argv[])
{
// Decode the arguments of the program
    char *FILENAME = argv[1];
    CYLINDERS = atoi(argv[2]);
    if (CYLINDERS<0||CYLINDERS>1024) {
       printf("CYLINDERS out of range.\n");
       exit(-1);
    }
    SECTORS = atoi(argv[3]);
    if (SECTORS<0||SECTORS>65536) {
       printf("SECTORS out of range.\n");
       exit(-1);
    }
    DELAY = atoi(argv[4]);
    PORT_NO = atoi(argv[5]);
    NUM = atoi(argv[6]);
    if (NUM>512 || NUM<=0) {
       printf("Cache size out of range\n");
       exit(-1);
    }

    init(FILENAME);
    firstRes(argv);

// FCFS Scheduler using cache as a deque:
    printf("This is FCFS scheduling:\n\n");
    // cache[head] is the earliest request
    // cache[len-1] is the latest request
    delay=0; head=0; len=0;
    while (true) {
       getMsg();
       // Request of EXIT
       if (!strcmp(cache[(len+NUM-1)%NUM],"E")) { 
          // Execute all remaining requests
          while (!exec(head)) {
             head = (head+1)%NUM;
             getMsg();
          }  
          break;
       }
       // If cache is full, execute the earliest request
       else if (len==head) {
          exec(head); 
          head = (head+1)%NUM;
       }  
       else { // Send a nonsensical signal '#'
          bzero(buffer,512);
          sendMsg((char*)"#");
       } 
    }
// SSTF Scheduler using cache as a stack:
    delay=0; len=0; ++mode;
    printf("This is SSTF scheduling:\n\n");
    while (true) {
       getMsg();
       // Request of Exit
       if (!strcmp(cache[(len+NUM-1)%NUM],"E")) {  
          // Excute all remaining commands
          while (sstf_deq())
             getMsg();
          break;
       }
       // Execute one request when the cache is full 
       else if (!len) sstf_deq();
       else { // Send a nonsensical signal '#'
          bzero(buffer,512);
          sendMsg((char*)"#");
       } 
    }
// C-LOOK Scheduler using cache as a stack:
    delay=0; len=0; ++mode;
    printf("This is C-LOOK scheduling:\n\n");
    while (true) {
       getMsg();
       // Request of Exit or a full cache
       if (!strcmp(cache[len-1],"E")||!len) {
          printf(" \n");
          c_empty();
          break;
       }
       else { // Send a nonsensical signal '#'
          bzero(buffer,512);
          sendMsg((char*)"#");
       } 
    }
// End of the program
    printf("Here are the total seek time:\n ");
    printf("FCFS %d ms;  ",delayTable[0]);
    printf("SSTF %d ms;  ",delayTable[1]);
    printf("C-LOOK %d ms\n\n\n",delayTable[2]);
    close(sockfd);
    close(newsockfd);
    close(fd);
    return 0;
}


