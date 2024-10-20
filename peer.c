/* A P2P client
It provides the following functions:
- Register the content file to the index server (R)
- Contact the index server to search for a content file (D) 
	- Contact the peer to download the file
	- Register the content file to the index server
- De-register a content file (T)
- List the local registered content files (L)
- List the on-line registered content files (O)
*/

#include <unistd.h>                                                        				                                                             				
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h>


#define QUIT "quit"
#define SERVER_PORT 10000 /* well-known port */
#define BUFLEN 100         /* buffer length */
#define NAMESIZ 100
#define MAXCON 200
#define MAXLINE 100

struct PDU { // Struct for content registration
	char type;
	char peerName[NAMESIZ];
	char contentName[NAMESIZ];
	char host;
	int port;
};

struct PDU_C { // Struct for content download
	char type;
	char data[MAXLINE];
    
};

typedef struct { // Struct for local content registration
	char contentName[NAMESIZ];
	char peerName[NAMESIZ];
	char host;
	int port;
}LocalContent;

void registerContent(int s, struct PDU p, struct sockaddr_in sin, char* cont); //use a fork here to create a passive tcp connection
void localList();
void TCPDownloadHost(int port); //hosting for content download
void TCPDownloadClient(int port, char *contentname); //client to download from TCPDownloadHost
void deRegisterContent();
void displayAllContent();
void searchContent();
void quit();

LocalContent localcontentRegister[20];
char peerName[20] =""; //this peers name
int localContentCount = 0;
int TCPopen = 0;
int s_sock;

int main(int argc, char **argv)
{

	int s_port = SERVER_PORT;
    int n;
    int alen = sizeof(struct sockaddr_in);
    struct hostent *hp;
    struct sockaddr_in server;
    char c, *host, name[NAMESIZ];
    struct sigaction sa;

    switch (argc)
    {
    case 2:
        host = argv[1];
        break;
    case 3:
        host = argv[1];
        s_port = atoi(argv[2]);
        break;
    default:
        printf("Usage: %s host [port]\n", argv[0]);
        exit(1);
    }

    /* UDP Connection with the index server		*/
    memset(&server, 0, alen);
    server.sin_family = AF_INET;
    server.sin_port = htons(s_port);
    if (hp = gethostbyname(host))
        memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    else if ((server.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
    {
        printf("Can't get host entry \n");
        exit(1);
    }
    s_sock = socket(PF_INET, SOCK_DGRAM, 0); // Allocate a socket for the index server
    if (s_sock < 0)
    {
        printf("Can't create socket \n");
        exit(1);
    }
    if (connect(s_sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Can't connect \n");
        exit(1);
    }
 	 
 	 struct PDU spud;
	 char option; 

printf("P2P Network\n");

	
while(1){
    
	strcpy(spud.peerName, "");
	strcpy(spud.contentName, "");

	printf("R-Content Registration\n");
	printf("T-Content Deregistration\n");
	printf("L-List Local Content\n");
	printf("D-Download Content\n");
	printf("O-List all the On-line Content\n");
	printf("Q-Quit\n");

	printf("Please enter an option: ");
	scanf(" %c",&option);
	printf("\n");
    
	switch (option)
	{
        case 'R':
     	   registerContent(s_sock, spud, server, "");
     	   //register function
     	   break;
		
		case 'L':
			localList();
			break;

        case 'T':
  		   deRegisterContent(s_sock,spud,server, "");
     	   //deregister function
     	   break;
        case 'O':
  		   displayAllContent(s_sock, spud, server);
     	   //display content register
     	   break;
        case 'D':
  		   searchContent(s_sock, spud, server);
     	   //search content register
     	   break;
        case 'Q':
     	   quit(s_sock, spud, server);
     	   //go through local content list and deregister it all
     	   exit(0);
     	   break;
        default:
     	   printf("Invalid option.\n");
    	}
	}

	exit(0);    
}

void localList(){
	printf("\nDisplaying Local Content...\n");
	for(int i = 0; i < localContentCount; i++)
	{
		printf("Content Name: %s\n", localcontentRegister[i].contentName);
	}
	printf("\n");

}

void registerContent(int s, struct PDU p, struct sockaddr_in sin, char *autoReg)
//when we register we need the content name, peer name, host, port
{
	struct sockaddr_in local_address;
	socklen_t len = sizeof(local_address);
	getsockname(s, (struct sockaddr*)&local_address, &len);
    
  	  if(autoReg[0] == '\0')
  	  {
  		  printf("\n Please enter the name of the content you want to register: ");
  	  scanf("%s",p.contentName); //content name
  	  printf("\n");
  	  }
  	  else
  	  {
  		  strcpy(p.contentName, autoReg);
  	  }
    
    
	FILE *f = fopen(p.contentName, "rb");
    
	if(f == NULL)
	{
  	  printf("content does not exist locally\n");
	}
	else
	{
  	  if(peerName[0] == '\0')//if the peerName has not been made
  	  {
  			  printf("\n Please enter a unique name for this Peer: ");
  			  scanf("%s",p.peerName);
  			  printf("\n");
  			  strcpy(peerName, p.peerName);
  	  }
  	  else
  	  {
  			  strcpy(p.peerName, peerName);
  	  }
    
  	  p.type = 'R';
  	 
   	  int myport = ntohs(local_address.sin_port);
   	  p.port = ntohs(local_address.sin_port);
   	  //printf("%d\n", p.port);
   	 
  	  sendto(s, &p, sizeof(p), 0, (struct sockaddr *)&sin, sizeof(sin));
  	 
  	  struct PDU ACKorERR;
  	  recvfrom(s, &ACKorERR, sizeof(ACKorERR), 0, (struct sockaddr *)&sin, sizeof(sin));
  	  //printf("%d\n", ACKorERR.type);
  	  if(ACKorERR.type == 'E')
  	  {
  		  printf("peer is not unique\n");
  	  }
  	  else if(ACKorERR.type == 'A')
  	  {
  		 
  		  if(getsockname(s, (struct sockaddr*)&local_address, &len) == -1)
  		  {
  				  printf("get socket name failed");
    
  		  }
  		  int myport = ntohs(local_address.sin_port);
  		  //printf("%d",myport);
  		 
    
  		  if (fork() == 0 && TCPopen == 0)
  		  {
			  	  //Child process: start TCP server in the background
			  	  TCPopen = 1;
		  		  TCPDownloadHost(myport);
		  		  //exit(0); // Child process exits after starting the TCP server
  			  }

  			  //copy the needed info into the local content register
   		 
   		 strncpy(localcontentRegister[localContentCount].peerName, peerName,   		 
   			 sizeof(localcontentRegister[localContentCount].peerName));
   		 strncpy(localcontentRegister[localContentCount].contentName, p.contentName,
   			 sizeof(localcontentRegister[localContentCount].contentName));
   			 
   			 
  			 localContentCount++;//increment by 1
  		 
  	  }
  	 
	}

}

void displayAllContent(int s, struct PDU p, struct sockaddr_in sin){
	p.type = 'O'; // set the type to O
	sendto(s, &p, sizeof(p), 0, (struct sockaddr *)&sin, sizeof(sin)); //sends the request to the index server
    
	socklen_t alen = sizeof(sin);
    
	printf("\nDisplaying Content...\n");
    
	while(1)
	{
  	  struct PDU content;  //creatse a struct to hold the content
  	  recvfrom(s, &content, sizeof(content), 0, (struct sockaddr *)&sin, &alen); //recieves data from server
  	 
  	  if(content.type =='A')
  	  {
  		  printf("All content has been displayed\n\n");
  		  break;
  	 
  	  }
  	  else if(content.type =='O')
  	  {
  		  printf("Content Name: %s\n",content.contentName); //print the content of the index server.
  	  }
	}
    
}

void deRegisterContent(int s, struct PDU p, struct sockaddr_in sin, char *autoReg){ //deregister content
	p.type='T';
    
	if(autoReg[0] == '\0')
	{
  	 printf("\nplease enter the name of the content you would like to de-register:");
  	 scanf("%s",p.contentName); //content name
  	 printf("\n");
   	 strcpy(p.peerName, peerName);
	}
	else
	{
  	 strcpy(p.contentName, autoReg);
  	 strcpy(p.peerName, peerName);
  	 printf("autodereg: %s\n", autoReg);
	}
   
    
	FILE *f = fopen(p.contentName, "rb");
    
	if(f == NULL)
	{
  	  printf("content does not exist locally\n");
	}
	else
	{
    
   	 printf("%s\n", p.contentName);
   	 sendto(s, &p, sizeof(p), 0, (struct sockaddr *)&sin, sizeof(sin)); //sends the request to the index server

   	 struct PDU content;  //creatse a struct to hold the content
   	 socklen_t alen = sizeof(sin);

   	 recvfrom(s, &content, sizeof(content), 0, (struct sockaddr *)&sin, &alen); //recieves acknowledgement from server

    
   	 if(content.type=='A')
   	 {
   		 printf("Deregistration Complete\n");    
   		 localContentCount--;
   	 }
  	  else if(content.type=='E'){
   		 printf("Content does not exist or has already been deregistered\n");
   	 }
   	 //now the peer can close the socket at which the content was registered at
   	 //add that code here
   	 
   }
}

void deRegisterContent2(int s, struct PDU p, struct sockaddr_in sin){ //deregister content
	p.type='T';
    
	sendto(s, &p, sizeof(p), 0, (struct sockaddr *)&sin, sizeof(sin)); //sends the request to the index server

	struct PDU content;  //creatse a struct to hold the content
	socklen_t alen = sizeof(sin);

	recvfrom(s, &content, sizeof(content), 0, (struct sockaddr *)&sin, &alen); //recieves acknowledgement from server

    
	if(content.type=='A'){
   	 printf("Complete");
   	 //the code below is the remove the local content for our array
   	 int location=0;
   	 int c;
   	 for(int i = 0; i < 20; i++)
   	 {
   		 //make sure that what the peer wants to deregister actually exist
   		 if((strcmp(localcontentRegister[i].peerName, p.peerName) == 0) && strcmp(localcontentRegister[i].contentName, p.contentName) == 0)
   			 {
   				 location=i; // save the location
   				 break;
   			 }
   		 }
   	 for (c = location - 1; c < 20 - 1; c++){
   		 localcontentRegister[c] = localcontentRegister[c+1]; //get rid of the registered element
   		 }
   		 //removes the duplicate from the end of the array
   		 localContentCount--;
   		 strncpy(localcontentRegister[localContentCount].contentName, NULL,
   		 sizeof(localcontentRegister[localContentCount].contentName));
   		 strncpy(localcontentRegister[localContentCount].peerName, NULL,
   		 sizeof(localcontentRegister[localContentCount].peerName));
   	 }
    else{
   	 printf("Something went wrong");
	}
	//now the peer can close the socket at which the content was registered at
}

void searchContent(int s, struct PDU p, struct sockaddr_in sin){
	p.type='S';
	printf("\n please enter the name of the content you would like to download: ");
	scanf("%s",p.contentName); //content name
	printf("\n");
	sendto(s, &p, sizeof(p), 0, (struct sockaddr *)&sin, sizeof(sin)); //sends the request to the index server

	struct PDU content; 
	socklen_t alen = sizeof(sin);

	recvfrom(s, &content, sizeof(content), 0, (struct sockaddr *)&sin, &alen); //recieves acknowledgement from server
    
	if(content.type == 'S')
	{
  	  printf("The location peer is %s\n",content.peerName);
  	  printf("The location address is %d\n",content.host);
  	  printf("The location port is %d\n",content.port);
  	  //make a TCPDownload request to get the content from said peer
  	  TCPDownloadClient(content.port,content.contentName );
  	  //after the content is downloaded a tcp socket has to be opened for the content
  	 
  	  registerContent(s,content,sin, content.contentName);
	}
	else if(content.type == 'E')
	{
  	  printf("content does not exist or has not been registered\n");
	}
    
}

void quit(int s, struct PDU p, struct sockaddr_in sin){

   	 for(int x=0; x<20; x++)
   	 {
   		 if(localcontentRegister[x].contentName[0] != '\0')
   		 {
   			 p.type = 'T';
  		  		 deRegisterContent(s,p,sin,localcontentRegister[x].contentName);
   		 }
   	 
   	 }
   	 

    

    for(int x=0; x<localContentCount; x++){
   	 strcpy(p.contentName, localcontentRegister[x].contentName); // change the name of the content you want to deregister
   	 deRegisterContent2(s,p,sin);
    }

}

void TCPDownloadHost(int port)
{
    
	int sd, new_sd, client_len;
	struct sockaddr_in server, client;

	client_len = sizeof(client);
    
	// Create a socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
  	  fprintf(stderr, "Can't creat a socket\n");
  	  exit(1);
	}


	// Configure server address
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);  
	server.sin_addr.s_addr = htons(INADDR_ANY);
	// Bind the socket to the specified address and port
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
  	  //fprintf(stderr, "Can't bind name to socket\n");
  	  exit(1);
	}


	// Listen for incoming connections
	//listen(sd, 5);
	if (listen(sd, 5) == -1) {
  	  fprintf(stderr, "Error listening for connections\n");
  	  exit(1);
	}
    
  	  while(1)
  	  {
  		  //printf("Server is listening...\n");


  	  // Accept a connection from a client
  	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
  	  if (new_sd == -1) {
  		  fprintf(stderr, "Can't accept client \n");
  		  exit(1);
  	  }
    
  	  //printf("\naccepted client....\n");

  	  struct PDU downloadRequest;    
    
  	  //char filename[100];
  	  read(new_sd, &downloadRequest, sizeof(downloadRequest));	//may need &
  	  printf("%s\n", downloadRequest.contentName);
    
  	  FILE *f = fopen(downloadRequest.contentName, "rb");

  	  struct PDU_C download;
  	  int bytes;
  	 
  		  printf("looking for file locally....\n");
  	 
  	  if (f == NULL)
  	  {
     		  printf("file not found...\n");
      		  //buf[0] = 'E';
      		  struct PDU error;
      		  error.type = 'E';
      		  write(new_sd, &error, sizeof(error)); //may need &
    
  	  }
  	  else
  	  {
      		  printf("file found...\n");
      		  while((bytes = fread(download.data,1 ,sizeof(download.data), f)) > 0)
      		  {
      
     	 		  //buf[0] = 0;
     	 		  download.type = 'C';
     	 		  write(new_sd, &download, bytes+1); //may need &
      		  }

  	  }
    
  	  fclose(f);
  	  close(new_sd);
  	 
  	  }


}

void TCPDownloadClient(int port, char * contentname)
{
    
	int sd;
  	  struct hostent *hp;
  	  struct sockaddr_in server;
  	  char *host = "localhost";
  	  char filename[100], buf[100];
  	 
  	  // Create a socket
  	  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
   	 fprintf(stderr, "Can't create a socket\n");
   	 exit(1);
  	  }


       // Configure server address
       bzero((char *)&server, sizeof(struct sockaddr_in));
       server.sin_family = AF_INET;
       server.sin_port = htons(port);
       if (hp = gethostbyname(host))
  	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
       else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
   	  fprintf(stderr, "Can't get server's address\n");
     exit(1);
  	  }


  	  // Connect to the server
  	  if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
   	 fprintf(stderr,"Can't connect\n");
   	 exit(1);
  	  }
    
  	  printf("Connected to Host\n");
  	 
  	  //file download start
  	  printf("%s\n", contentname);
  	 
  	  struct PDU downloadRequest;  			  //
  	  downloadRequest.type ='D';  			  //
  	  strcpy(downloadRequest.contentName, contentname);	//
  	  write(sd, &downloadRequest, sizeof(downloadRequest));	
    
	struct PDU_C download;
	while (1) {
  	  int bytes = read(sd, &download, sizeof(download)); 
  	 
  	  if (bytes <= 0) {
  				  break;
  		  }
  		  if (download.type == 'E') {
  				  printf("Server Error: File not found...\n");
  				  break;
  		  }
  		  else { 		 
  		 
  			  if (download.type == 'C') {
  		 
  				  int data_length = bytes - 1;
  					  const char *downloaded_file = contentname; //
  					  FILE *df = fopen(downloaded_file, "ab");
  					  if (df == NULL) {
  					 
  						  printf("cannot open downloaded file\n");
  						  fprintf(stderr, "Error opening downloaded file\n");
  						  exit(1);
  					  }
  					  fwrite(download.data, 1, data_length, df);
  					  fclose(df);

  				  }

  		  }
    
	}

	printf("File Downloaded...\n");
    
    close(sd);

}