//////////////////////////////////////////////////////////
//                                                      //
// This program simulates a file system based on I-node //
//                                                      //
//////////////////////////////////////////////////////////



///////////////////////////////////
//                               //
//        INODE FORMAT           //
//                               //
///////////////////////////////////


// Directory blocks constitues a root tree

// INODE OF DIR:
// block[0]: parent directory position
// block[1]: directory size (>0)
// block[2]-[188]: direct blocks
// block[189]-[190]: single indirect
// block[191]: double indirect
// block[192]-[255]: absolute path


// Each file is a doubly-linked list of blocks

// FIRST BLOCK OF FILE:
// block[0]: parent directory position
// block[1]: file size (>0)
// block[2]-block[190]: data bytes
// block[191]: ptr to next data block
// block[192]-block[255]: absolute path

// OTHER BLOCK OF FILE:
// block[0]: ptr to previous data block
// block[1]-block[254]: data bytes
// block[255]: ptr to next data block


// Ways to represent a file/dir block:
// (1) block number (file/dir position)
// (2) its parent dir pos + relative pos
// (3) its previous inode pos + ptr offset


/////////////////////////////////////////
//                                     //
//         VECTOR BIT BLOCK            //
//                                     //
/////////////////////////////////////////

// To test whether block[pos] is occupied,
//  you can test the (pos%8)th bit of the
//  (pos%2048/8)th byte of block[pos/2048*2048],
// because block[2048*k] (k=0,1,2...) stores  
//  the space information of itself and its 
//  subsequent 2047 blocks (1 bit per block)
// 1 represents not occupied, 0 means occupied
// There will be VECTNUM such vector bit blocks


#include "cse356header.h"
#define SIZE SECTORS*CYLINDERS
#define VECTNUM (SIZE-1)/2048+1
#define TAIL buffer[strlen(buffer)]
#define BLOCKSIZE 256
#define FSIGN '#'
#define DELT 13


//////////////////////////////
//                          //
//     GLOBAL VARIABLES     //
//                          //
//////////////////////////////


// For socket communication:
socklen_t clilen;
int sockfd,cSockfd,dSockfd;
struct hostent *server;
struct sockaddr_in serv_addr1, cli_addr;
struct sockaddr_in serv_addr2;

// buffer[] is widely used in socket communication
char buffer[512]; 
// curr[] is used as a cache of block[currPos]
char curr[512];  
// vect[] is used as a cache of vector bit blocks
//  vect[i*256]-vect[i*256+255] maps block[i*2048]
char vect[131072]; 

// Other global variables
int CYLINDERS,SECTORS;
int cNum=0,dNum=0,currPos=1;


////////////////////////////////
//                            //
//    BASIC FUNCTION TOOLS    //
//                            //
////////////////////////////////


// Print error message and exit
void error(const char *msg)
{
   perror(msg);
   exit(-1);
}

// Return the physical location of
//  block[pos] in the 'disk'
bool getPos(int pos,int &c,int &s)
{
   if (pos<0||pos>=SIZE)
      return false;
   c = pos/SECTORS;
   s = pos%SECTORS;
   return true;
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


//////////////////////////////////////
//                                  //
//    SOCKET COMMUNICATION TOOLS    //
//                                  //
//////////////////////////////////////


// Preparation for communication
void init(int DISKPORT,int CLIENTPORT)
{
    // Create a new socket
    dSockfd = socket(AF_INET,SOCK_STREAM,0);
    if (dSockfd<0) error("Error opening socket");  
    // Clear and set 'serv_addr2'
    bzero((char *) &serv_addr2, sizeof(serv_addr2));
    serv_addr2.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr2.sin_addr.s_addr,
         server->h_length);
    serv_addr2.sin_port = htons(DISKPORT);
    // Establish a connection to the disk server
    if (connect(dSockfd,(struct sockaddr *) &serv_addr2,sizeof(serv_addr2))<0) 
        error("Error connecting");
    // Create a new socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd<0) error("Error opening socket");
    // Clear and set 'serv_addr1'
    bzero((char *) &serv_addr1,sizeof(serv_addr1));
    serv_addr1.sin_family = AF_INET;
    serv_addr1.sin_addr.s_addr = INADDR_ANY;
    serv_addr1.sin_port = htons(CLIENTPORT);
    // Bind the socket to serv_addr1
    if (bind(sockfd,(struct sockaddr *) &serv_addr1,sizeof(serv_addr1))<0) 
       error("Error on binding"); 
    // Listen on the socket for connections
    listen(sockfd,5);  
    clilen = sizeof(cli_addr);
    // Wait for conncetion from a file client
    cSockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
    if (cSockfd<0) error("Error on accept");
}



// Phrase the start of an R/W request to the
//  disk, which is referred to block[pos];
//  then you can send an R request directly
//  or append buffer with data before send W
void startReq(char *op,int pos)
{
   bzero(buffer,512);
   strcat(buffer,op);
   strcat(buffer," ");
   int c,s; 
   getPos(pos,c,s);
   encode(buffer,c);
   strcat(buffer," ");
   encode(buffer,s);
   strcat(buffer," ");
}


// Return the starting position of data
//   that is read from disk (R respond)
int readPos()
{
   int pos = 0;
   decode(buffer,pos);
   if (strncmp(buffer+pos," Yes",4))
      return -1; // reading fails
   pos += 5;
   return pos;
}


// Communication with disk through buffer[]
void callDisk(char *str=(char*)"",int len=192)
{
   // Phrase and send the request
   if (strlen(str)>0)
      bzero(buffer,512);
   strcat(buffer,str);
   char op = buffer[0];
   printf("FS Request %d: %s\n",dNum++,buffer);
   if (op=='W') { // Integer Encryption
      int pos = 2;
      decode(buffer,pos);
      decode(buffer,pos);
      decode(buffer,pos);
      for (int i=0;i<len;i++)
          buffer[pos+i] += DELT;
   }
   if (write(dSockfd,buffer,strlen(buffer))<0)
      error("Error writing to socket");
   // Get the respond from the disk
   bzero(buffer,512);
   if (read(dSockfd,buffer,511)<0)
      error("Error reading from socket");
   if (op=='R') { // Integer Decryption
      for (int i=0;i<len;i++)
          buffer[readPos()+i] -= DELT;
   }
   if (buffer[0]!='#')
      printf("Disk respond %s\n",buffer);
}

// Send an block content tmp through buffer
void writeBlock(char *tmp,int len=192)
{
    encode(buffer,256);
    TAIL = ' ';
    int pos = strlen(buffer);
    for (int i=0;i<256;i++)
        buffer[pos+i] = tmp[i];
    callDisk((char*)"",len);
}


// When block[currPos] is likely to be dirty
//  call this to renew the cache curr[]
void getCurr()
{
    char tmp[512];
    bzero(tmp,512);
    strcat(tmp,"R ");
    int c,s; 
    getPos(currPos,c,s);
    encode(tmp,c);
    strcat(tmp," ");
    encode(tmp,s);
    strcat(tmp," ");
    // Send request
    if (write(dSockfd,tmp,strlen(tmp))<0)
      error("Error writing to socket");
    printf("FS Request %d: %s\n",dNum++,tmp);
    // Get respond
    bzero(tmp,512);
    if (read(dSockfd,tmp,511)<0)
       error("Error reading from socket");
    int pos=0; decode(tmp,pos);
    // Integer Decryption
    for (int i=0;i<192;i++)
        tmp[i+pos+5] -= DELT;
    if (tmp[0]!='#')
       printf("Disk respond %s\n",tmp);
    for (int i=0;i<256;i++)
       curr[i] = tmp[pos+i+5];
}

// Write back block[currPos] directly once 
//  the cache curr[] is dirty
void sendCurr()
{
    char tmp[512];
    bzero(tmp,512);
    strcat(tmp,"W ");
    int c,s; 
    getPos(currPos,c,s);
    encode(tmp,c);
    strcat(tmp," ");
    encode(tmp,s);
    strcat(tmp," ");
    encode(tmp,strlen(curr));
    strcat(tmp," ");
    int len = strlen(tmp);
    for (int i=0;i<256;i++)
        tmp[len+i] = curr[i];
    printf("FS Request %d: %s\n",dNum++,tmp);
    // Integer Encryption
    for (int i=0;i<192;i++)
        tmp[i+len] += DELT;
    // Send request
    if (write(dSockfd,tmp,strlen(tmp))<0)
      error("Error writing to socket");
    // Get respond
    bzero(tmp,512);
    if (read(dSockfd,tmp,511)<0)
       error("Error reading from socket");
    if (tmp[0]!='#')
       printf("Disk respond %s\n",tmp);
}


// Occupy a block of disk or release an
//  occupied block, which ensures that cache
//  vect[] is written back directly
bool wrVect(int pos)  
{
   printf("wrVect %d\n",pos);
   if (pos<0||pos>=SIZE)
      return false;
   int tmp = (1<<(pos%8));
   // XOR change the bits you want
   vect[pos/8] = (vect[pos/8]^tmp);
   startReq((char*)"W",pos/2048*2048);
   writeBlock(vect,256);
   return true;
}


// Get a request from FC
void getClient()
{
   bzero(buffer,512);
   if (read(cSockfd,buffer,511)<0)
      error("Error reading from socket");
   printf("Client request %d: %s\n",cNum,buffer);
}

// Send data in buffer to FC
void sendClient()
{
   if (write(cSockfd,buffer,strlen(buffer))<0)
      error("Error wrting to socket");
   printf("FS Respond %d: %s\n",cNum++,buffer);
}

// Send an integer to FC
void sendClient(int code)
{
   bzero(buffer,512);
   encode(buffer,code);
   if (write(cSockfd,buffer,strlen(buffer))<0)
      error("Error wrting to socket");
   printf("FS Respond %d: %s\n",cNum++,buffer);
}


///////////////////////////////////
//                               //
//     REQUEST IMPELMENTATION    //
//                               //
///////////////////////////////////


// Get CYLINDERS and SECTORS of disk server
void firstReq()
{
    callDisk((char*)"I");
    int pos=0; 
    decode(buffer,pos); 
    CYLINDERS = decode(buffer,++pos); 
    SECTORS = decode(buffer,pos); 
    bzero(vect,131072);
    for (int i=0;i<VECTNUM;i++) {
        startReq((char*)"R",2048*i);
        callDisk((char*)"",256);
        for (int j=0;j<256;j++)
            vect[j+256*i] = buffer[j+readPos()];
    }
    getCurr();
}

// Format the disk
void format()
{
    // Revise the cache of vector bit
    for (int i=0;i<256*VECTNUM;i++) 
        vect[i] = -2; // sorry to waste space
    vect[0] = -4;
    // Write back to vector bit blocks
    for (int i=0;i<VECTNUM;i++) {
        startReq((char*)"W",2048*i);
        writeBlock(vect+256*i,256);
    }
    // Create the root directory
    currPos = 1;
    bzero(curr,512);
    curr[0] = -1;  // parent is NULL
    curr[1] = 1;   // size is one
    for (int i=2;i<192;i++)
        curr[i] = -1; // To ensure no ptr=-13
    strcat(curr,"/root"); // path
    sendCurr();
}



///////////////////////////////////
//                               //
//    FILE/DIRECTORY CREATING    //
//                               //
///////////////////////////////////


// Move the string by delt rightwards
void strMove(char *str,int delt)
{
    if (delt<0) {
       for (int i=0;i<strlen(str);i++)
           str[i+delt] = str[i];
    }
    else {
       for (int i=strlen(str)-1;i>=0;i--)
           str[i+delt] = str[i];
    }
}

// Insert string rt into the head of string sub
void insHead(char *sub,char *rt)
{
    int len = strlen(rt);
    strMove(sub,len);
    for (int i=0;i<len;i++)
        sub[i] = rt[i];
}


// Return the position of a file/directory
//  in light of its parent's inode and 
//  its relative position r
int getIndex(int r,char *sub=curr)
{
    if (sub==curr) getCurr();
    if (r<0||r>=sub[1])
       return -1;
    else if (r<=187)
       return sub[r+2];
    else if (r<=443) {
       // get block[prev]
       startReq((char*)"R",sub[189]);
       callDisk((char*)"",256);
       return buffer[r-188];
    }
    else if (r==444)
       return sub[190];
    else if (r<=700) {
       // get block[prev]
       startReq((char*)"R",sub[190]);
       callDisk((char*)"",256);
       return buffer[r-445];
    }
    else if (r==701)
       return sub[191];
    else if ((r-702)%257==0) {
       // get block[prev]
       startReq((char*)"R",sub[191]);
       callDisk((char*)"",256);
       return buffer[(r-702)/257];
    }
    else {
       // get block[grand]
       startReq((char*)"R",sub[191]);
       callDisk((char*)"",256);
       // get block[prev]
       startReq((char*)"R",buffer[(r-702)/257]);
       callDisk((char*)"",256);
       return buffer[(r-703)%257];
    }
}


// Find a vacant block and occupy it,
//  return its logical position, -1
//  indicates finding failure
int findVacant()
{
    int tmp,pos;
    bool flag=true; 
    for (int i=0;i<256*VECTNUM&&flag;i++){
        if (!vect[i]) continue;
        tmp = vect[i];
        for (int j=0;j<8;j++) {
            if (tmp%2) {
               pos = i*8+j;
               flag = false;
               break;
             }
             tmp /= 2;
        }
    }
    if (flag||pos>=SIZE)
       return -1;
    else {
       wrVect(pos);
       return pos;
    }
}


// Input a block's relative r and get its 
//  offset, return its previous inode position
int findPrev(int r,int &offset)
{
    if (r<=187) {
       offset=r+2; 
       return currPos;
    }
    else if (r>=188 && r<=443) {
       offset=r-188; 
       return getIndex(187);
    }
    else if (r==444) {
       offset=190; 
       return currPos;
    }
    else if (r>=445 && r<=700) {
       offset=r-445; 
       return getIndex(444);
    }
    else if (r==701) {
       offset=191; 
       return currPos;
    }
    else if ((r-702)%257==0) {
       offset=(r-702)/257;
       return getIndex(701);
    }
    else {
       offset=(r-703)%257;
       return getIndex(702+(r-702)/257*257);
    }
}


// Create a new block whose parent is curr[]
//  and relative position is r, return its
//  position, -1 indicates creating failure
int createBlock(int r)
{
    int offset, prev=findPrev(r,offset);
    int pos = findVacant();
    if (pos<0) return -1;
    // get block[prev]
    startReq((char*)"R",prev);
    if (r<=187||r==444||r==701)
       callDisk();
    else callDisk((char*)"",256);
    // backup block[prev]
    char tmp[256];
    bzero(tmp,256);
    for (int i=0;i<256;i++)
        tmp[i] = buffer[readPos()+i];
    tmp[offset] = pos;
    // modify block[prev]
    startReq((char*)"W",prev);
    if (tmp[1]<0) 
       writeBlock(tmp,256);
    else writeBlock(tmp);
    getCurr();
    curr[1]++;
    sendCurr();
    if (r==187||r==444||r==701||
        (r>701&&(r-702)%257==0)) {
       for (int i=0;i<256;i++)
           tmp[i] = -1;
       startReq((char*)"W",pos);
       writeBlock(tmp,256);
       return createBlock(r+1);
    }
    return pos;
}


// Try creating a file/directory
int mkCall(int len)
{
    // Path of new file cannot be longer than 63
    if (strlen(buffer+len)+strlen(curr+192)>62)
       return 2;
    if (len<6) TAIL = FSIGN;
    // Backup the path of the new file/dir
    char path[256]; 
    bzero(path,256); 
    path[0]='/';
    strcpy(path+1,buffer+len); 
    insHead(path,curr+192);
    int pos;
    // Search the files/dirs that have existd
    for (int r=0;r<curr[1]-1;r++) {
       if (r==187||r==444||r==701)
          continue; // single indirect
       if (r>701&&(r-702)%257==0)
          continue; // double indirect
       // get the block of file/dir
       pos = getIndex(r);
       startReq((char*)"R",pos);
       callDisk();
       if (!strcmp(path,buffer+readPos()+192))
          return 1;
    }
    // Parent has a maximal size
    if (curr[1]>701+256*257)
       return 2;
    pos = createBlock(curr[1]-1);  
    if (pos<0) return 2;
    // Modify the new file/dir block
    startReq((char*)"W",pos);
    strMove(path,192);
    path[0] = currPos;
    path[1] = 1;
    if (len==6) {
       for (int i=2;i<192;i++)
           path[i] = -1;
    }
    else {
       for (int i=2;i<191;i++)
           path[i] = 0;
       path[191] = -1;
    }
    writeBlock(path);
    return 0;

}

/////////////////////////////
//                         //
//     FILE OPERATIONS     //
//                         //
/////////////////////////////


// Catch a file whose block # is pos
void catHelp(int pos,char *fstr)
{
    char fnode[256];
    startReq((char*)"R",pos);
    callDisk();
    for (int i=0;i<256;i++)
       fnode[i] = buffer[readPos()+i];
    if (fnode[255]) 
       strcat(fstr,fnode+1);
    else strcat(fstr,fnode+2);
    if (fnode[255]>0) 
       catHelp(fnode[255],fnode);
    else if (!fnode[255] && fnode[191]>0)
       catHelp(fnode[191],fnode);
}


// Catch a file with certain path
void catFile()
{
    // Path of a file cannot be longer than 63
    if (strlen(buffer+4)+strlen(curr+192)>62) {
       sendClient(1);
       return;
    }
    TAIL = FSIGN;
    // Backup the path of the new file/dir
    char path[256]; 
    bzero(path,256); 
    path[0]='/';
    strcpy(path+1,buffer+4); 
    insHead(path,curr+192);
    for (int r=0;r<curr[1]-1;r++) {
        if (r==187||r==444||r==701) 
           continue; // single indirect
        if (r>701&&(r-702)%257==0) 
           continue; // double indirect
        int pos = getIndex(r);
        startReq((char*)"R",pos);
        callDisk();
        if (strcmp(path,buffer+readPos()+192)) 
           continue;
        char fstr[512];
        bzero(fstr,512);
        catHelp(pos,fstr);
        bzero(buffer,512);
        encode(buffer,0);
        TAIL = ' ';
        encode(buffer,strlen(fstr));
        TAIL = ' ';
        strcat(buffer,fstr);
        sendClient();
        return;
    }
    sendClient(1);
}


// Return the tail block # of a file
//  and its remaining data space
int getTail(int pos,int &sp)
{
   startReq((char*)"R",pos);
   callDisk();
   int next = buffer[readPos()+255];
   // the 1st block of a file
   if (!next) {
      next = buffer[readPos()+191];
      if (next<0) {
         sp = 191-strlen(buffer+readPos()+1);
         return pos;
      }
      else return getTail(next,sp);
   }
   // block[pos] is a tail
   else if (next<0) {
      sp = 253-strlen(buffer+readPos()+1);
      return pos;
   }
   // a block other than a tail
   else return getTail(next,sp);
}
   


// Append a file that has block[pos]
bool appendHelp(int pos,char *data)
{
    int sp,bkNum,next;
    pos = getTail(pos,sp);
    if (strlen(data)>sp)
       bkNum = (strlen(data)-sp-1)/253+1;
    else bkNum = 0;
    char tmp[256];
    startReq((char*)"R",pos);
    callDisk();
    for (int i=0;i<256;i++)
       tmp[i] = buffer[readPos()+i];
    tmp[1] += bkNum;
    strncat(tmp+1,data,sp);
    for (int i=0;i<bkNum;i++) {
       next = findVacant();
       if (next<0) return false;
       // Modify the previous data block
       if (tmp[255]<0) tmp[255] = next;
       else tmp[191] = next;
       startReq((char*)"W",pos);
       writeBlock(tmp,256);
       // Modify the new data block
       bzero(tmp,256);
       tmp[0] = pos;
       for (int i=1;i<255;i++) {
           if (!data[sp]) break;
           tmp[i] = data[sp++];
       }   
       pos = next;  
    }
    startReq((char*)"W",pos);
    if (tmp[255])
       writeBlock(tmp,256);  
    else writeBlock(tmp); 
    return true;
}

// Rectify the size of the file
void rectSize(int pos)
{
    int sz = 1, next;
    char tmp[256];
    startReq((char*)"R",pos);
    callDisk();
    for (int i=0;i<256;i++)
        tmp[i] = buffer[readPos()+i];
    next = buffer[readPos()+191];
    while (next>0) {
       sz++;
       startReq((char*)"R",next);
       callDisk();
       next = buffer[readPos()+255];
    }
    tmp[1] = sz;
    startReq((char*)"W",pos);
    writeBlock(tmp);
}

// Executed on request 'a'
int append()
{
    char path[128];
    bzero(path,128);
    path[0] = '/';
    int p = 2;
    for (;buffer[p]!=' ';p++)
        path[strlen(path)] = buffer[p];
    path[strlen(path)] = FSIGN;
    insHead(path,curr+192);
    // Path of a file cannot be longer than 63
    if (strlen(path)>63)
        return 1;
    int len = decode(buffer,++p);
    // Can not append more than 511 bytes
    if (len>511) return 2;
    char data[512];
    bzero(data,512);
    for (int i=0;i<len;i++)
        data[i] = buffer[p++];
    // Search the file according to the path
    for (int r=0;r<curr[1]-1;r++) {
        if (r==187||r==444||r==701) 
           continue; // single indirect
        if (r>701&&(r-702)%257==0) 
           continue; // double indirect
        int pos = getIndex(r);
        startReq((char*)"R",pos);
        callDisk();
        if (strcmp(path,buffer+readPos()+192)) 
           continue;
        if (appendHelp(pos,data))
           return 0;
        else {
           rectSize(pos);
           return 2;
        }
    }
    return 1;
}


// Clear the contents of a file
//   whose first block is # pos
void fileClear(int pos)
{
    int sp,tail = getTail(pos,sp);
    while (tail!=pos) {
        startReq((char*)"R",tail);
        callDisk();
        wrVect(tail);
        tail = buffer[readPos()];
    }  
    startReq((char*)"R",pos);
    callDisk();
    char tmp[256];
    for (int i=0;i<256;i++)
        tmp[i] = buffer[i+readPos()];
    tmp[1] = 1;
    for (int i=2;i<191;i++)
        tmp[i] = 0;
    tmp[191] = -1;
    startReq((char*)"W",pos);
    writeBlock(tmp);
}



// Executed on request 'w'
int write()
{
    char path[128];
    bzero(path,128);
    path[0] = '/';
    int p = 2;
    for (;buffer[p]!=' ';p++)
        path[strlen(path)] = buffer[p];
    path[strlen(path)] = FSIGN;
    insHead(path,curr+192);
    // Path of a file cannot be longer than 63
    if (strlen(path)>63)
        return 1;
    int len = decode(buffer,++p);
    // Can not append more than 511 bytes
    if (len>511) return 2;
    char data[512];
    bzero(data,512);
    for (int i=0;i<len;i++)
        data[i] = buffer[p++];
    // Search the file according to the path
    for (int r=0;r<curr[1]-1;r++) {
        if (r==187||r==444||r==701) 
           continue; // single indirect
        if (r>701&&(r-702)%257==0) 
           continue; // double indirect
        int pos = getIndex(r);
        startReq((char*)"R",pos);
        callDisk();
        if (strcmp(path,buffer+readPos()+192)) 
           continue;
        fileClear(pos);
        if (appendHelp(pos,data))
           return 0;
        else {
           rectSize(pos);
           return 2;
        }
    }
    return 1;
}



///////////////////////////////////
//                               //
//    FILE/DIRECTORY DELETION    //
//                               //
/////////////////////////////////// 


// Remove a directory with inode #pos
//  or a file with 1st block #pos
void removeHelp(int pos)
{
    // Get block[pos]
    startReq((char*)"R",pos);
    callDisk();
    strMove(buffer,-readPos());
    if (buffer[strlen(buffer+192)+191]!=FSIGN) {
       // Delete content blocks
       for (int r=0;r<buffer[1]-1;r++) {
          if (r==187||r==444||r==701)
             continue; // single indirect
          if (r>701&&(r-702)%257==0)
             continue; // double indirect
          removeHelp(getIndex(r,buffer));
       }
       // Delete pointer blocks
       for (int r=702;r<buffer[1]-1;r+=257)
          wrVect(getIndex(r,buffer));
       if (buffer[1]-1>187) {
          wrVect(getIndex(187,buffer));
          if (buffer[1]-1>444) {
             wrVect(getIndex(444,buffer));
             if (buffer[1]-1>701)
                wrVect(getIndex(701,buffer));
          }
       }
    }
    else fileClear(pos);
    wrVect(pos);
}
    


// Try removing a file/dir from the current dir
int rmCall(int len)
{
    // Path of the file cannot be longer than 63
    if (strlen(buffer+len)+strlen(curr+192)>62)
       return 1;
    if (len<6) TAIL = FSIGN;
    // Backup the path of the file/dir
    char tmp[256]; 
    bzero(tmp,256); 
    tmp[0]='/';
    strcpy(tmp+1,buffer+len); 
    insHead(tmp,curr+192);
    int pos,prev,offset;
    // Search the files/dirs that have existd
    for (int r=0;r<curr[1]-1;r++) {
       if (r==187||r==444||r==701)
          continue; // single indirect
       if (r>701&&(r-702)%257==0)
          continue; // double indirect
       pos = getIndex(r);
       prev = findPrev(r,offset);
       startReq((char*)"R",pos);
       callDisk();
       if (!strcmp(tmp,buffer+readPos()+192)) {
          removeHelp(pos); 
          if (--curr[1]<2) {
             sendCurr(); return 0;
          } else sendCurr();
          // Get block[prev]
          startReq((char*)"R",prev);
          callDisk();
          // Backup and modify block[prev]
          bzero(tmp,256);
          for (int i=0;i<256;i++)
              tmp[i] = buffer[readPos()+i];
          tmp[offset]=getIndex(curr[1]-1);
          // Write back to block[prev]
          startReq((char*)"W",prev);
          writeBlock(tmp);
          if (prev==currPos)
              getCurr();
          return 0;
       }
    }
    return 1; // yet existed
}



////////////////////////////
//                        // 
//    CHANGE DIRECTORY    //
//                        //
////////////////////////////

// Try entering block #pos
bool enter(int pos,char *path)
{
   startReq((char*)"W",pos);
   callDisk();
   char tmp[256];
   for (int i=0;i<256;i++)
      tmp[i] = buffer[readPos()+i];
   //int c; scanf("%d",c);
   if (strncmp(path,tmp+192,strlen(tmp+192)))
      return false;
   else if (!strcmp(path,tmp+192)) {
      currPos = pos;
      getCurr();
      return true;
   }
   for (int r=0;r<tmp[1]-1;r++) {
      if (r==187||r==444||r==701)
         continue; // single indirect
      if (r>701&&(r-702)%257==0)
         continue; // double indirect
      if (enter(getIndex(r,tmp),path))
         return true;
   }
   return false;
}


// Executed on request of 'cd'
bool change()
{
   // Backup the path of directory
   char path[256];
   bzero(path,256);
   strcat(path,buffer+3);
   if (!strcmp(path,"..")) {
      if (curr[0]<0) return false;
      currPos = curr[0];
      getCurr();
      return true;
   }
   else if (path[0]=='.') {
      insHead(path+1,curr+192);
      for (int r=0;r<curr[1]-1;r++) {
         if (r==187||r==444||r==701)
             continue; // single indirect
         if (r>701&&(r-702)%257==0)
             continue; // double indirect
         if (enter(getIndex(r),path+1))
             return true;
      }
      return false;
   }
   return enter(1,path);
}
       


//////////////////////////////
//                          //
//    LIST FILE CONTENTS    //
//                          //
//////////////////////////////

// Calculate the space that a file/dir
//  actually occupies
int calSize(int pos)
{
   int val = 1;
   startReq((char*)"W",pos);
   callDisk();
   char tmp[256];
   for (int i=0;i<256;i++)
       tmp[i] = buffer[readPos()+i]; 
   for (int r=0;r<tmp[1]-1;r++) {
       if (r==187||r==444||r==701) {
          val++; continue; // single indirect
       }
       if (r>701&&(r-702)%257==0) {
          val++; continue; // double indirect
       }
       pos = getIndex(r,tmp);
       val += calSize(pos);
   }
   return val;
}


// Executed on request 'ls'
void list(char mode)
{
   char tmp[512];
   bzero(tmp,512);
   if (mode=='0') {
      int len = strlen(curr+192);
      for (int r=0;r<curr[1]-1;r++) {
          if (r==187||r==444||r==701)
             continue; // single indirect
          if (r>701&&(r-702)%257==0)
             continue; // double indirect
          for (int i=0;i<11;i++)
              strcat(tmp," ");
          startReq((char*)"W",getIndex(r));
          callDisk();
          if (buffer[strlen(buffer+readPos()+192)+readPos()+191]==FSIGN)
             buffer[strlen(buffer+readPos()+192)+readPos()+191] = 0;
          strcat(tmp,buffer+readPos()+len+193);
          strcat(tmp,"\n");
      }
      bzero(buffer,512);
      encode(buffer,curr[1]-1);
      strcat(buffer," Objects\n\n");
      strcat(buffer,tmp);
      strcat(buffer,"\n");
      sendClient();
   }
   else if (mode=='1') {
      int len = strlen(curr+192);
      for (int r=0;r<curr[1]-1;r++) {
          if (r==187||r==444||r==701)
             continue; // single indirect
          if (r>701&&(r-702)%257==0)
             continue; // double indirect
          strcat(tmp,"   ");
          int pos = getIndex(r);
          startReq((char*)"R",pos);
          callDisk();
          // print file (file or directory)
          if (buffer[strlen(buffer+readPos()+192)+readPos()+191]==FSIGN) {
             buffer[strlen(buffer+readPos()+192)+readPos()+191] = 0;
             strcat(tmp,"FILE:\t");
          }
          else strcat(tmp,"DIR: \t");
          // print file/dir name
          strcat(tmp,buffer+readPos()+len+193);
          for (int i=0;i<20-strlen(buffer+readPos()+len+193);i++)
              strcat(tmp," ");
          strcat(tmp,"\t\t");
          // print file/dir size
          encode(tmp,calSize(pos));
          strcat(tmp,"B\n");
      }
      bzero(buffer,512);
      encode(buffer,curr[1]-1);
      strcat(buffer," Objects\n\n");
      strcat(buffer,tmp);
      strcat(buffer,"\n");
      sendClient();
   }
   else sendClient(1);
}



int main(int argc,char *argv[])
{
// Decode the arguments of the program
    server = gethostbyname(argv[1]);
    if (server==NULL) 
       error("Error: no such host.\n");
    init(atoi(argv[2]),atoi(argv[3]));
    firstReq();
// Loop of services
    while (true) {
       getClient();
       if (strncmp(buffer,"exit",4)==0){
          callDisk((char*)"E");
          bzero(buffer,512);
          strcat(buffer,"Exit successfully.\n");
          sendClient();
          break;
       }
       else if (!strncmp(buffer,"f",1)){
          format();
          bzero(buffer,512);
          strcat(buffer,"Format successfully.\n");
          sendClient();
       }
       else if (!strncmp(buffer,"mk ",3))
          sendClient(mkCall(3));
       else if (!strncmp(buffer,"mkdir ",6))
          sendClient(mkCall(6));
       else if (!strncmp(buffer,"rm ",3)) 
          sendClient(rmCall(3)); 
       else if (!strncmp(buffer,"rmdir ",6))
          sendClient(rmCall(6)); 
       else if (!strncmp(buffer,"cd ",3)) {
          if (change()) {
             bzero(buffer,512);
             strcat(buffer,"Finish.\n");
          }
          else {
             bzero(buffer,512);
             strcat(buffer,"No such directory.\n");
          }
          sendClient();
       } 
       else if (!strncmp(buffer,"ls ",3))
          list(buffer[3]);
       else if (!strncmp(buffer,"cat ",4))
          catFile(); 
       else if (!strncmp(buffer,"w ",2))
          sendClient(write());
       else if (!strncmp(buffer,"a ",2))
          sendClient(append());
       else {
          bzero(buffer,512);
          sendClient(1);
       }
    }
// End of the program
    close(sockfd);
    close(cSockfd);
    close(dSockfd);
    return 0;
}


