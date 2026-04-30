#include "server.h"
#include "handleClients.h"
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

int letterCount[26];
int completedClients = 0;
int num_clients; // the number of client threads


int main(int argc, char *argv[]){
  pthread_t threads[1000];
  struct thd_data threadData[1000];
  int msgqueue;
  key_t key;

  setbuf(stdout, NULL);

  // server only take one argument
  // Usage: ./server [process_num]
  if (argc != NUMARGS) {
    fprintf(stderr, "Usage: ./server [process_num]\n");
    return 1;
  }

  num_clients = atoi(argv[1]);
  if (num_clients <= 0 || num_clients > 1000) {
    fprintf(stderr, "Invalid number of clients.\n");
    return 1;
  }

  // Print log for server start
  timestamp();
  printf("Server starts...\n");

  // initialize letterCount content to zero
  for (int i = 0; i < 26; i++) {
    letterCount[i] = 0;
  }

  // get access to msg Queue using msgget()
  key = ftok("..", 'q');
  if (key == -1) {
    perror("ftok");
    return 1;
  }

  msgqueue = msgget(key, 0666 | IPC_CREAT);
  if (msgqueue == -1) {
    perror("msgget");
    return 1;
  }

  // create threads to handle incoming client requests
  // num_clients: the number of threads
  // pthread_create() should call processClients() (defined in handleClients.c) and the param should be struct thd_data* 
  // NOTE: clientID starts from 1
  for (int i = 0; i < num_clients; i++) {
    threadData[i].msgqueue = msgqueue;
    threadData[i].clientID = i + 1;

    if (pthread_create(&threads[i], NULL, processClients, &threadData[i]) != 0) {
      perror("pthread_create");
      msgctl(msgqueue, IPC_RMID, NULL);
      return 1;
    }
  }

  // join all the threads
  for (int i = 0; i < num_clients; i++) {
    pthread_join(threads[i], NULL);
  }

  // close msgqueue and return
  timestamp();
  printf("Server ends...\n");

  if (msgctl(msgqueue, IPC_RMID, NULL) == -1) {
    perror("msgctl");
    return 1;
  }

  return 0;
}
