#include "server.h"
#include "handleClients.h"

int letterCount[26];
int completedClients = 0;
int num_clients; // the number of client threads


int main(int argc, char *argv[]){
  // server only take one argument
  // Usage: ./server [process_num]

  // Print log for server start

  // intialize letterCount content to zero

  // get access to msg Queue using msgget()

  // create threads to handle incoming client requests
  // num_clients: the number of threads
  // pthread_create() should call processClients() (defined in handleClients.c) and the param should be struct thd_data* 
  // NOTE: clientID starts from 1

  // join all the threads

  //close msgqueue and return
  return 0;
}
