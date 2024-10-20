// Server Code

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct pdu { //pdu struct
	char type;
	char peerName[100]; //may need to change to 10
	char contentName[100];
	char host;
  	int port;
};

typedef struct  { //register entry struct
	char contentName[100];
  	char peerName[100];
  	char host;
  	int port;
}RegisterEntry;

RegisterEntry contentRegister[20]; //array of register entries
int registerAccessCount[20]; //array of access counts
int numRegistrations = 0;  	 //number of registrations

void registerContent();
void search();
void display();
void deregister();

int main(int argc, char **argv)
{
    
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	char	buf[100];  	  /* "input" buffer; any size > 0	*/
	char    *pts;
	//char	fin
	int	sock;  		  /* server socket  	  */
	time_t	now;  		  /* current time  		  */
	struct pdu p;  		  /* from-address length  	  */
	struct  sockaddr_in sin;
  				  /* an Internet endpoint address		 */
   	 int     s;  			  /* socket descriptor and socket type    */
	int 	port=10000;
                                                                      		 

	switch(argc){
  	  case 1:
  		  break;
  	  case 2:
  		  port = atoi(argv[1]);
  		  break;
  	  default:
  		  fprintf(stderr, "Usage: %s [port]\n", argv[0]);
  		  exit(1);
	}

   	 memset(&sin, 0, sizeof(sin));
   	 sin.sin_family = AF_INET;
   	 sin.sin_addr.s_addr = INADDR_ANY;
   	 sin.sin_port = htons(port);
                                                                                       		 
    /* Allocate a socket */
   	 s = socket(AF_INET, SOCK_DGRAM, 0);
   	 if (s < 0)
  	  fprintf(stderr, "can't creat socket\n");
                                                                      		 
    /* Bind the socket */
   	 if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  	  fprintf(stderr, "can't bind to %d port\n",port);
   	 listen(s, 5);    
	int alen;

	while (1)
	{
  	 
  	  memset(p.peerName, 0, sizeof(p.peerName));
  	  memset(p.contentName, 0, sizeof(p.contentName));
  	  memset(&p.type, 0, sizeof(p.type));
  	  alen = sizeof(fsin);
  	 
  	  recvfrom(s, &p, sizeof(p), 0,(struct sockaddr *)&fsin, &alen);
  	 
  	  //printf("%c, %s, %s\n", p.type, p.peerName, p.contentName);
  	 
  	  if(p.type == 'R')
  	  {
  		  printf("register...\n");
  		  //char client_ip[INET_ADDRSTRLEN];    
  		  registerContent(s, p, fsin, alen);
  	  }
  	 
  	  else if(p.type == 'S')
  	  {
  		  printf("search...\n");    
  		  search(s, p, fsin, alen);  	 
  	  }
  	 
  	  else if(p.type == 'T')
  	  {
  		  printf("deregister...\n");
   	  deregister(s,p, fsin, alen);  		 
  		 
  	  }
  	 
  	  else if(p.type == 'O')
  	  {
  		  printf("Display...\n");
  		  display(s, p, fsin, alen);    
  			 
  	  }
  	  else
  	  {
  		  printf("invalid type\n");
  	  }    
  	 
	}
	close(s);
	return 0;    
}

void registerContent(int s, struct pdu p, struct sockaddr_in fsin, int alen)
//when we register we need the content name, peer name, host, port
{
	  printf("Peer Name: %s\n", p.peerName);
  	  printf("Content Name: %s\n", p.contentName);
  	  int matching = 0;
  	 
  	  for(int i = 0; i < 20; i++)
	{
  	  if(strcmp(contentRegister[i].peerName, p.peerName) == 0 && (contentRegister[i].port != p.port))
  	  {
  		  struct pdu err;
  		  err.type = 'E';
  		  printf("matching peer name\n");
  		  printf("%s and %s\n", contentRegister[i].peerName, p.peerName);
  		  printf("%d and %d\n", contentRegister[i].port, p.port);
  		  sendto(s, &err, sizeof(err), 0,(struct sockaddr *)&fsin, sizeof(fsin));
  		  matching = 1;
  	  }
  	  else
  	  {
  		  matching = 0;
  	  }
	}
    
	if(matching == 0)
  	  {
  		  // Print client IP and port
  			  char client_ip[INET_ADDRSTRLEN];
  			  inet_ntop(AF_INET, &(fsin.sin_addr), client_ip, INET_ADDRSTRLEN);
  			  //printf("Client IP: %s\n", client_ip);
  			  //printf("Client Port: %d\n", ntohs(fsin.sin_port));
  	 
  			  if (numRegistrations < 20)
  			  {
  				  strncpy(contentRegister[numRegistrations].peerName, p.peerName,   		 
  					  sizeof(contentRegister[numRegistrations].peerName));
  				  strncpy(contentRegister[numRegistrations].contentName, p.contentName,
  					  sizeof(contentRegister[numRegistrations].contentName));
  				 
  		 
  				  contentRegister[numRegistrations].host = ntohl(fsin.sin_addr.s_addr);
  				  contentRegister[numRegistrations].port = ntohs(fsin.sin_port);
  				  (numRegistrations)++;
  				  struct pdu ack;
  				  ack.type = 'A';
  				  //printf("ack: %d\n", ack.type);
  				  sendto(s, &ack, sizeof(ack), 0,(struct sockaddr *)&fsin, sizeof(fsin));
  				 
  			  }
  			  else
  			  {
  				  printf("Maximum registrations reached.\n");
  			  }    
  		  }    
    
}

void search(int s, struct pdu p, struct sockaddr_in fsin, int alen)
{

	printf("in search function...\n");
    
	for(int i = 0; i < 20; i++)
	{
  	  if((strcmp(contentRegister[i].contentName, p.contentName) == 0) && (registerAccessCount[i] == 0)) //check for matching content name and if the peer at i in contentRegister has been accessed
  	  {
  		  printf("Searching...\n");
  		  printf("Peer Name: %s\n", contentRegister[i].peerName);
  			  printf("Content Name: %s\n", contentRegister[i].contentName);
  			  printf("Host: %d\n", contentRegister[i].host);
  			  printf("Port: %d\n", contentRegister[i].port);
  			 
  			  struct pdu searchResult;
  			 
  			  searchResult.type = 'S';
  			  strcpy(searchResult.peerName, contentRegister[i].peerName);
  			  strcpy(searchResult.contentName, contentRegister[i].contentName);
  			  //strcpy(searchResult.host, contentRegister[i].host);
  			  searchResult.host = contentRegister[i].host;
  			  searchResult.port = contentRegister[i].port;
  			 
  			  sendto(s, &searchResult, sizeof(searchResult), 0,(struct sockaddr *)&fsin, sizeof(fsin));
  			  registerAccessCount[i] = 1;
  			 
  			  break;
  	 
  	  }
  	  else if((strcmp(contentRegister[i].contentName, p.contentName) != 0) && (i == 19))
  	  {
  		  struct pdu notFound;
  		  notFound.type = 'E';
  		  sendto(s, &notFound, sizeof(notFound), 0,(struct sockaddr *)&fsin, sizeof(fsin));
  	  }
  	 
    
	}

}

void display(int s, struct pdu p, struct sockaddr_in fsin, int alen)
{

	for(int i = 0; i < 20; i++)
	{
  	  struct pdu content;
  	  content.type = 'O';
  	 
  	  if(contentRegister[i].contentName[0] != '\0')
  	  {
  		  strcpy(content.contentName, contentRegister[i].contentName);
  		  sendto(s, &content, sizeof(content), 0,(struct sockaddr *)&fsin, sizeof(fsin));
  	  }
	}
    
	struct pdu ack;
   	ack.type = 'A';
    
	sendto(s, &ack, sizeof(ack), 0,(struct sockaddr *)&fsin, sizeof(fsin));

}

void deregister(int s, struct pdu p, struct sockaddr_in fsin, int alen){

    int matching = 0;
  	 int location=0;
    int c;
  	 for(int i = 0; i < 20; i++)
   	 {
         //make sure that what the peer wants to deregister actually exist
  	      if((strcmp(contentRegister[i].peerName, p.peerName) == 0) && strcmp(contentRegister[i].contentName, p.contentName) == 0)
  	      {
   		 location=i; // save the location
   		 matching = 1;
   		 break;
  	      }
   	 }
    
	if(matching == 0) //send an error if there was no match
  	  {   struct pdu err;
  		  err.type = 'E';
  		  printf("content not found\n");
  		  sendto(s, &err, sizeof(err), 0,(struct sockaddr *)&fsin, sizeof(fsin));
  	  }
     else
     {

   	 strncpy(contentRegister[location].contentName, "",
   	 sizeof(contentRegister[location].contentName));
   	 strncpy(contentRegister[location].peerName, "",
   	 sizeof(contentRegister[location].peerName));
   	 
   	 contentRegister[location].host = 0;
   	 contentRegister[location].port = 0;
   	 
   	 printf("content deregistered \n");
   	 struct pdu ack;
   	 ack.type = 'A';
   	 printf("ack: %d\n", ack.type);
   	 sendto(s, &ack, sizeof(ack), 0,(struct sockaddr *)&fsin, sizeof(fsin));   	 
   	 
     }

    
}