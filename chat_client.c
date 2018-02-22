
//chat_room_client.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>


int fd; // socket descriptor for signal handler


void signal_handler(int signo)
{
   close(fd);
   exit(0);
}


void *read_server(void *args)
{
   char message[512];
   memset(message,0,sizeof(message));			

   int server_fd;
   server_fd = *(int *)args;
   free(args);
   int status;

   while((status = read(server_fd,message,sizeof(message))) > 0) // read message from the server
   {
      write(1,message,sizeof(message));
      memset(message,0,sizeof(message));
   }

   printf("The Chat Server has closed connection\n");

   close(server_fd);
   exit(0);	
}


int main(int argc, char *argv[])
{
   if(argc < 3) 
   {
      fprintf(stderr, "ERROR: USAGE: <SERVER MACHINE> <PORT NUMBER> <USER NAME>\n");
      exit(1);       
   }

   int server_fd;
   char message[256];
   pthread_t server_user;
   struct addrinfo addrinfo;
   struct addrinfo *result;
   char read_from_server[512];

   char port[10];
   strcpy(port, argv[2]);

   char name[100];
   strcpy(name, argv[3]);



   /*setup signal handler*/
   struct sigaction sa;
   sa.sa_handler = &signal_handler;
   sigaction(SIGINT,&sa,NULL);

   
   addrinfo.ai_flags = 0;
   addrinfo.ai_family = AF_INET;
   addrinfo.ai_socktype = SOCK_STREAM;
   addrinfo.ai_protocol = 0;
   addrinfo.ai_addrlen = 0;
   addrinfo.ai_addr = NULL;
   addrinfo.ai_canonname = NULL;
   addrinfo.ai_next = NULL;


   if(gethostbyname(argv[1]) == NULL)
   {
      printf("ERROR: Invalid Host\n");
      exit(1);
   }

   getaddrinfo(argv[1], argv[2], &addrinfo, &result );

   server_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
   

   void *socket_arg2 = malloc(sizeof(server_fd));
   memcpy(socket_arg2, &server_fd, sizeof(int));
   int status = connect(server_fd, result->ai_addr, result->ai_addrlen);


   while(status < 0)
   {
      close(server_fd);
      server_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

      if(errno != ECONNREFUSED)
      {
         printf("ERROR: %s\n", strerror(errno));
         exit(1);
      }

      printf("Could not connect to server. Attempting to reconnect...\n");
      sleep(3);
      status = connect(server_fd, result->ai_addr, result->ai_addrlen);
   }

				
   write(server_fd, name, strlen(name)+1);

   read(server_fd,read_from_server,sizeof(read_from_server)); // check validation from server 				
		
   
   if(strcmp(read_from_server,"duplicate user") == 0)
   {
      fprintf(stderr, "ERROR: User name is already taken, try a different name.\n" );
	   close(server_fd);
	   exit(1);
   }

   if(strcmp(read_from_server,"server full") == 0)
   {
      fprintf(stderr, "ERROR: chat room is full, try again later.\n" );
      close(server_fd);
      exit(1);
   }

   printf("\n+++++ Welcome to the Chatroom +++++\n\n");   
   printf("User Options:\n\n\
   @create -name- (creates a named private chat room)\n\
   @enter  -name- (enter into named chatroom)\n\
   @leave  -name- (leave the named chatroom)\n\
   @remove -name- (destroy the named chatroom)\n\
   @exit (exit the program)\n\n\
   @private <user name> -message- (send a private message to <user name>)\n\n"); //input options
	printf("Your username is: %s\n",name);

   fd = server_fd; // assignment for signal handler
		
   if(pthread_create(&server_user, NULL, &read_server, socket_arg2) != 0)
   {
      fprintf(stderr, "Error creating thread");
	   exit(1);;
   }

   pthread_detach(server_user);
			
   while(read(0,message,sizeof(message)))
   {
	   write(server_fd,message,sizeof(message));
		memset(message, 0, sizeof(message)); //clear message
		sleep(1);
	  
   }

   close(server_fd);
   return 0;	
} 
