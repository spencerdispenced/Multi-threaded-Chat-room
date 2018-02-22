
// chat_room_sever.c


#include "utils.h"

int fd; //socket descriptor for signal handler

chat_user default_users[20]; // users in normal chat
int default_size = 0;

chat_user all_users[20]; // all users connected to server, used for private messages
int all_users_size = 0;

chat_room arr_chat_room[20]; // array of private chat rooms
int total_rooms = 0;

int total_users = 0; // count of all users that have connected

pthread_mutex_t lock_unlock; 



void *client_session(void *args)
{
   int client_sd;
   client_sd = *(int *)args;
   free(args);
   /*declare buffers*/
   char request[1000] = {0};
   char username[100] = {0};
   char buffer[1000] = {0};
   char chat_log[1000] = {0};
   char response[256] = {0};
   char input[20] = {0}; // used for special commands
   char name[100] = {0}; // used to hold names of chat rooms


   chat_user current_user;

   special_command sc; //enum for special commands

   pthread_detach(pthread_self());   
      
   read(client_sd,username, sizeof(username)); // get chat_user name from client to validate

     
   for(int i = 0; i < default_size; ++i) // check for duplicate usernames
   {
      if(strcmp(username,default_users[i].username) == 0)
      {
         write(client_sd,"duplicate user",sizeof("duplicate user")); //reject chat_user name
         close(client_sd);
         pthread_exit(0);
      }
   }

   if(default_size >= 20) // check to see if chatroom is full
   {
     write(client_sd,"server full",sizeof("server full")); 
     close(client_sd);
     pthread_exit(0);
   }


   if (total_users > 20)
   {
      total_users = 0;
   }

   /*create user*/
   current_user.username = username;  
   current_user.user_sd = client_sd; 
   current_user.color = set_user_color(total_users);
   current_user.in_room = 0; // not currently in private room
   default_users[default_size] = current_user;
   all_users[all_users_size] = current_user;
   default_size++;
   all_users_size++;
   write(client_sd,"1",strlen("1"));

   /*write chat history from file*/
   FILE *fp;
   char *line = NULL;
   size_t len = 0;
   ssize_t read_file;
   fp = fopen("default_log.txt", "a+");

   while((read_file = getline(&line, &len, fp)) != -1)
   {
      write(current_user.user_sd, response, sprintf(response, "%s",line));
   }

   fclose(fp);


   printf("%s has joined the chat room\n",current_user.username);
   all_message(current_user,response, "<has joined the chat room>\n");

   memset(buffer,0,sizeof(buffer));
   
      /*begin chatting*/
   while(read(client_sd,request,sizeof(request)) > 0)
   {
      sscanf(request, "%s %s", input,name);
     
      sc = special_input(input);

      if(sc == sc_create)
      {
         if(create_chat_room(current_user, name, &lock_unlock) == 0)
         {
            sprintf(buffer, "***Chat room <%s> has been created***\n", name);
            all_message(current_user, response, buffer);

            memset(buffer,0,sizeof(buffer));

            continue;
         }

         continue;
      }
                  
      if (sc == sc_enter)
      {
         if(enter_room(current_user,name, &lock_unlock) == 0)
         {
            current_user.in_room++; // entered into private room
            continue; 
         }

         else
            continue;       
      }
                  
      if (sc == sc_leave)
      {
         if(leave_room(current_user,name, &lock_unlock) == 0)
         {
            current_user.in_room--;

            if(current_user.in_room == 0)
            {
               default_users[default_size] = current_user;
               all_users[all_users_size] = current_user;
               default_size++;
               all_users_size++;
               write(current_user.user_sd, response, sprintf(response, "Returned to default chat room\n"));
            } 

            continue;        
         }
        
         continue; 
      } 
                      
      if (sc == sc_remove)
      {
         if(remove_room(current_user, name, &lock_unlock) == 0)
         {
            sprintf(buffer, "***Custom chat room has been removed***\n");
            all_message(current_user, response, buffer);

            memset(buffer,0,sizeof(buffer));

            continue;
         }

         continue;
      }
                                    
      if(sc == sc_private)
      { 
         /*char *tok =*/ strtok(request+8, " ");
         char *msg = strtok(NULL, "");

         private_message(current_user,name,buffer,msg); 
         write(current_user.user_sd, response, sprintf(response, "***Private message sent***\n"));
         continue;
      }
                
      if(sc == sc_exit)
      {     
         printf("%s has left the chat room\n",current_user.username); 
         all_message(current_user,buffer,"<has left the chat room>\n");
         write(current_user.user_sd, response, sprintf(response, "\n***EXITED***\n") );

         memset(buffer,0,sizeof(buffer));
         close(current_user.user_sd);
         exit_user(current_user);
         continue;

      }

      if(current_user.in_room > 0)
      {
         room_message(current_user, buffer, request);
         memset(buffer,0,sizeof(buffer));
      }

      else
      {
         sprintf(chat_log,"%s%s: %s%s",current_user.color,current_user.username,request,"\x1b[0m");
         record_chat(chat_log);
         normal_message(current_user,buffer,request);
         memset(buffer,0,sizeof(buffer));
      }   
   }
      
   exit_user(current_user); // client leaves the program
   close(client_sd);

   pthread_exit(0);
}


int create_chat_room(chat_user user, char *name, pthread_mutex_t *lock_unlock)
{
   char response[100];
   chat_room room;

   pthread_mutex_lock(lock_unlock); // lock the thread

   
   if(strlen(name) <= 0)
   {
      write(user.user_sd, response, sprintf(response, "ERROR: Must specify name of chat room\n"));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else if(total_rooms >= 10) // max capacity reached
   {
      write(user.user_sd, response, sprintf(response, "ERROR: Can't create any new chatrooms\n"));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else if(find_room(name) > 0)
   {
      write(user.user_sd, response, sprintf(response, "ERROR: %s chat room already exists\n",name));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else
   { 
      room.room_name = name;  
      room.users_in_room_size = 0; 

      arr_chat_room[total_rooms] = room;
      total_rooms++;

      pthread_mutex_unlock(lock_unlock);

      return 0;

   }   
}



int remove_room(chat_user user, char *name, pthread_mutex_t *lock_unlock)
{
   char response[100];

   pthread_mutex_lock(lock_unlock); // lock the thread

   if (total_rooms == 0)
   {
      write(user.user_sd, response, sprintf(response, "No room exists to remove\n"));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   if(strlen(name) <= 0)
   {
      write(user.user_sd, response, sprintf(response, "ERROR: Must specify name of chat room\n"));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   if(find_room(name) < 0)
   {
      write(user.user_sd, response, sprintf(response, "ERROR: %s chat room doesn't exist\n",name));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else
   { 
      for (int i = 0; i < total_rooms; ++i)
      {
         if(strcmp(arr_chat_room[i].room_name,name) == 0)
         {
            if (arr_chat_room[i].users_in_room_size > 0)
            {
               write(user.user_sd, response, sprintf(response, "ERROR: cannot remove custom room while users are inside.\n"));
               pthread_mutex_unlock(lock_unlock);

               return 1;
            }

            strcpy(arr_chat_room[i].room_name, "xzzsdsdwdsfdf");

            arr_chat_room[i] = arr_chat_room[total_rooms + 1]; 
            total_rooms--; 
            break;                  
         }
      }

      pthread_mutex_unlock(lock_unlock);

      return 0;

   }   
}



int find_room(char *name) 
{
   for(int i = 0; i < total_rooms; i++)
   {
      if(strcmp(arr_chat_room[i].room_name,name) == 0) 
         return 1; 
   }

   return -1; 
}



int enter_room(chat_user user, char *name, pthread_mutex_t *lock_unlock)
{
   char response[100];

   pthread_mutex_lock(lock_unlock); // lock the thread

   if(strlen(name) <= 0)
   {
      write(user.user_sd, response, sprintf(response, "ERROR: Must specify name of chat room\n"));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else if(find_room(name) < 0 )
   {
      write(user.user_sd, response, sprintf(response, "ERROR: %s chat room does not exist\n",name));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else
   { 
      for (int i = 0; i < total_rooms; i++)
      {
         if(strcmp(arr_chat_room[i].room_name,name) == 0)
         {
            arr_chat_room[i].users_in_room[arr_chat_room[i].users_in_room_size] = user;
            arr_chat_room[i].users_in_room_size++;
            remove_user_default(user);
            write(user.user_sd, response, sprintf(response, "You have entered chat room: <%s>\n",name));
            break;
         }
      }

      pthread_mutex_unlock(lock_unlock); 
      return 0;
   }   
}



int leave_room(chat_user user, char *name, pthread_mutex_t *lock_unlock)
{
   char response[100];


   pthread_mutex_lock(lock_unlock); 

   if(strlen(name) <= 0)
   {
      write(user.user_sd, response, sprintf(response, "ERROR: Must specify name of chat room\n"));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else if(find_room(name) < 0 )
   {
      write(user.user_sd, response, sprintf(response, "ERROR: %s chat room does not exist\n",name));
      pthread_mutex_unlock(lock_unlock);

      return 1;
   }

   else
   { 
       for (int i = 0; i < total_rooms; ++i)
      {
         if(strcmp(arr_chat_room[i].room_name,name) == 0)
         {
            arr_chat_room[i].users_in_room[arr_chat_room[i].users_in_room_size - 1] = user;
            arr_chat_room[i].users_in_room_size--;
            write(user.user_sd, response, sprintf(response, "You have left chat room: <%s>\n",name));
            break;
         }
      }

      pthread_mutex_unlock(lock_unlock); 

      return 0;
   }   
}



void room_message(chat_user user, char *buffer, char *message)
{
   for(int i = 0; i < total_rooms; i++)
   {
      for (int j = 0; j < arr_chat_room[i].users_in_room_size; j++)
      {    
         if (strcmp(arr_chat_room[i].users_in_room[j].username, user.username) == 0)
         {
            for (int k = 0; k < arr_chat_room[i].users_in_room_size; k++)
            {
               write(arr_chat_room[i].users_in_room[k].user_sd,buffer,sprintf(buffer,"%s%s: <room %s message> %s%s",
                  user.color,user.username,arr_chat_room[i].room_name,message,"\x1b[0m"));          
            }
         }                     
      }                  
   }  
}



void private_message(chat_user user, char *receiver, char *buffer, char *message)
{
   for(int i = 0; i < all_users_size; i++)
   {
      if(strcmp(all_users[i].username,receiver) == 0)
      {
         write(all_users[i].user_sd,buffer,sprintf(buffer,"%s%s: <private msg> %s%s",
            user.color,user.username,message,"\x1b[0m"));         
      }
   }     
}



void all_message(chat_user user, char *buffer, char *message)
{
   for(int i = 0; i < all_users_size; i++)
   {
      write(all_users[i].user_sd,buffer,sprintf(buffer,"%s%s: %s%s",
            user.color,user.username,message,"\x1b[0m")); 
   }
}


void normal_message(chat_user user, char *buffer, char *message)
{
   for(int i = 0; i <= default_size - 1; i++)
   {
      write(default_users[i].user_sd,buffer,sprintf(buffer,"%s%s: %s%s",
            user.color,user.username,message,"\x1b[0m")); 
   }
}



void signal_handler(int signo)
{
   for(int i = 0; i < all_users_size; i++)
   {
      close(all_users[i].user_sd);
   }	
	
   close(fd);

   FILE *log_file;
   log_file = fopen("default_log.txt", "w");
   fclose(log_file);

   exit(0);
}



void record_chat(char *message)
{
	FILE *log_file;
	log_file = fopen("default_log.txt", "a");
	fprintf(log_file, "%s",message);
	fclose(log_file);
}



special_command special_input(char *input)
{
   for(int i = 0; input[i] != '\0'; i++)
   {
      input[i] = tolower(input[i]);
   }

   if(!strcmp(input,"@create")) 
      return sc_create;

   if(!strcmp(input,"@enter")) 
      return sc_enter;

   if(!strcmp(input,"@leave")) 
      return sc_leave;

   if(!strcmp(input,"@remove")) 
      return sc_remove;

   if(!strcmp(input,"@private")) 
      return sc_private;

   if(!strcmp(input,"@exit")) 
      return sc_exit;

   else 
      return sc_notexit;
}



void exit_user(chat_user user)
{
   for(int i = 0; i < default_size; i++)
   {
      if(strcmp(default_users[i].username,user.username) == 0)
      {
         strcpy(default_users[i].username, "zxcxzx");
         strcpy(default_users[i].username, "zxcxzx");
         default_size--;
         all_users_size--;
      }
   }     
}



void remove_user_default(chat_user user) // removes user from default chat room
{
   for(int i = 0; i < default_size; i++)
   {
      if(strcmp(default_users[i].username,user.username) == 0)
      {
         default_users[i] = default_users[default_size-1];
         default_size--;  
      }
   }     
}



const char *set_user_color(int x)
{
   const char *color;

   if(x == 0)
      color = "\x1B[1;33;45m";

   if(x == 1)
      color = "\x1B[1;32m";
               
   if(x == 2)
      color = "\x1B[1;33m";
                
   if(x == 3)
      color = "\x1B[1;36;45m";
               
   if(x == 4)
      color = "\x1B[1;35m";

   if(x == 5)
       color = "\x1B[1;36m";
               
   if(x == 6)
      color = "\x1B[1;37;42m";
               
   if(x == 7)
      color = "\x1B[31m";
                
   if(x == 8)
      color = "\x1B[37;41m";
               
   if(x == 9)
      color = "\x1B[37;44m";
                
   if(x == 10)
      color = "\x1B[1;37m";
                
   if(x == 11)
      color = "\x1B[31;44m";
                
   if(x == 12)
      color = "\x1B[1;31;40m";
               
   if(x == 13)
      color = "\x1B[30;43m";
                
   if(x == 14)
      color = "\x1B[30;43m" ;
                
   if(x == 15)
      color = "\x1B[1;31m";
                 
   if(x == 16)
      color = "\x1B[1;32;44m";
                
   if(x == 17)
      color = "\x1B[1;36;41m";
                
   if(x == 18)
      color = "\x1B[1;36;45m";
                
   if(x == 19)
      color = "\x1B[1;33;42m";

   if(x == 20)
      color = "\x1B[2;31;45m";

                
    return color;   
}



int main(int argc, char *argv[])
{
   int sd;
   struct addrinfo addrinfo;
   struct addrinfo *result;
    
   char message[256];
   int on = 1;

   addrinfo.ai_flags = AI_PASSIVE;
   addrinfo.ai_family = AF_INET;
   addrinfo.ai_socktype = SOCK_STREAM;
   addrinfo.ai_protocol = 0;
   addrinfo.ai_addrlen = 0;
   addrinfo.ai_addr = 0;
   addrinfo.ai_canonname = NULL;
   addrinfo.ai_next = NULL;

   char port[10];
   strcpy(port, argv[1]);

   /*setup signal handler*/
   struct sigaction sa;
   sa.sa_handler = &signal_handler;
   sigaction(SIGINT,&sa,NULL);

   getaddrinfo(0, port, &addrinfo, &result);

   if((sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) < 0)
   {
      fprintf(stderr,"ERROR: SOCKET CANNOT BE CREATED\n");
      exit(1);
   }

   else if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR , &on, sizeof(on)) == -1)
   {
      write(1, message, sprintf(message, "setsockopt() failed.  File %s line %d.\n", __FILE__, __LINE__ ));
      freeaddrinfo(result);
      close(sd);
      exit(1);
   }

   else if(bind(sd, result->ai_addr, result->ai_addrlen) < 0)
   {
      fprintf(stderr, "ERROR: COULD NOT BIND TO PORT\n");
      freeaddrinfo(result);
      close(sd);
      exit(1);
   }
     
   write(1, message, sprintf(message, "\x1B[1;35mSUCCESS : Bind to port %s\n" "\x1b[0m", port));

   pthread_mutex_init(&lock_unlock, 0);// initiate mutex lock

   listen(sd,20); 
   void *csa;   // socket argument for every client thread created
   pthread_t client;

   while(1)
   {
      if((fd = accept(sd, 0, 0)) < 0)
      {
         fprintf(stderr, "ERROR: FAILED TO ACCEPT CLIENT\n");
         exit(1);
      }

      csa = malloc(sizeof(int));
      memcpy(csa, &fd, sizeof(on));
      total_users++;

      pthread_create(&client, NULL, &client_session, csa);// create thread for client - service
      pthread_detach(client);
   }

   freeaddrinfo(result);

   return 0;
}
