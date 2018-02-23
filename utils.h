#ifndef UTILS_H
#define UTILS_H

#define _GNU_SOURCE

/*includes*/
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



typedef enum special_command_ 
{ 
    sc_create,
    sc_enter,
    sc_leave,
    sc_remove,
    sc_exit,
    sc_private,
    sc_notexit,

} special_command;



typedef struct chat_user
{
    char *username;
    const char *color;
    int user_sd; 
    int in_room; //flag to determine if in private room
    

} chat_user;


typedef struct chat_room
{
   char room_name[20];
   chat_user users_in_room[20];
   int users_in_room_size;

} chat_room;


const char *set_user_color(int x);
void record_chat(char *message);
void exit_user(chat_user user);
void normal_message(chat_user temp, char* buffer,char *message);
void private_message(chat_user user, char *receiver, char *buffer, char *message);
special_command special_input(char *command);
int create_chat_room(chat_user user, char *name, pthread_mutex_t *lock_unlock);
int find_room(char *name);
void remove_user_default(chat_user user);
int enter_room(chat_user user, char *name, pthread_mutex_t *lock_unlock);
void room_message(chat_user user, char *buffer, char *message);
void all_message(chat_user user, char *buffer, char *message);
int remove_room(chat_user user, char *name, pthread_mutex_t *lock_unlock);
int leave_room(chat_user user, char *name, pthread_mutex_t *lock_unlock);
void record_custom_chat(char *message, chat_user user);
void read_custom_chat(chat_user user);
void clear_all_files();



#endif