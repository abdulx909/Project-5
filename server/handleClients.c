#include "handleClients.h"
pthread_mutex_t clientCountLock = PTHREAD_MUTEX_INITIALIZER, letterLock = PTHREAD_MUTEX_INITIALIZER;

// print timestamp
// You can get timestamp using localtime()
void timestamp(){
  time_t now;
  struct tm *timeInfo;
  char timeString[80];

  time(&now);
  timeInfo = localtime(&now);
  strftime(timeString, sizeof(timeString), "%a %b %d %H:%M:%S %Y", timeInfo);

  printf("[%s]", timeString);

}

// Count the occurence of all 26 letters, and update letterCount[] correspondingly
void calculateLetterCount(char *filename){
  FILE *fp;
  char line[MSGLEN];
  int localCount[ALPHACOUNT];

  for (int i = 0; i < ALPHACOUNT; i++) {
      localCount[i] = 0;
  }

  fp = fopen(filename, "r");
  if (fp == NULL) {
      return;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
      if (line[0] == '\0' || line[0] == '\n') {
          continue;
      }

      char first = tolower(line[0]);
      if (first >= 'a' && first <= 'z') {
          localCount[first - 'a']++;
      }
  }

  fclose(fp);

  pthread_mutex_lock(&letterLock);
  for (int i = 0; i < ALPHACOUNT; i++) {
      letterCount[i] += localCount[i];
  }
  pthread_mutex_unlock(&letterLock);
}

// Create a string based on letterCount[], the final return character array 
// The string looks like count1#count2#....#count26#
char* convertLetterCountToChar(){
  char *result;
  char temp[32];

  result = (char *)malloc(512);
  if (result == NULL) {
      return NULL;
  }

  result[0] = '\0';

  for (int i = 0; i < ALPHACOUNT; i++) {
      sprintf(temp, "%d#", letterCount[i]);
      strcat(result, temp);
  }

  return result;
}

// called by threads created on server
// args: pointer to the thd_data struct of current client
void* processClients(void* args){

  struct thd_data tdata = *((struct thd_data *) args);

  while (1) {
    struct msg_buffer buff;
    ssize_t recvValue;

    // Waiting to received from client process
    // Should store the message in buff
    timestamp();
    printf("Waiting to rcv from client process %d\n", tdata.clientID - 1);

    recvValue = msgrcv(
      tdata.msgqueue,
      &buff,
      sizeof(buff.mesg_text),
      tdata.clientID,
      0
    );

    if (recvValue == -1) {
      perror("msgrcv");
      continue;
    }

    // Handle the received message
    if (strcmp(buff.mesg_text, "END") == 0){
      // if the message is END:
      int allDone = 0;
      char *finalResult;
      struct msg_buffer sendBuff;

      timestamp();
      printf("Thread %d received END from client process %d\n",
             tdata.clientID - 1,
             tdata.clientID - 1);

      pthread_mutex_lock(&clientCountLock);
      completedClients++;
      pthread_mutex_unlock(&clientCountLock);

      // wait for all threads to complete
      while (!allDone) {
        pthread_mutex_lock(&clientCountLock);
        if (completedClients == num_clients) {
            allDone = 1;
        }
        pthread_mutex_unlock(&clientCountLock);

        if (!allDone) {
            usleep(1000);
        }
      }

      // Convert letter array to character array using convertLetterCountToChar()
      // and send it back to client
      finalResult = convertLetterCountToChar();
      if (finalResult == NULL) {
        pthread_exit(NULL);
      }

      sendBuff.mesg_type = 2000 + tdata.clientID;
      strcpy(sendBuff.mesg_text, finalResult);

      timestamp();
      printf("Thread %d sending final letter count to client process %d\n",
             tdata.clientID - 1,
             tdata.clientID - 1);

      if (msgsnd(tdata.msgqueue, &sendBuff, sizeof(sendBuff.mesg_text), 0) == -1) {
        perror("msgsnd");
      }

      free(finalResult);
      // After that, break the while loop to exit the server
      break;
    }else{
      // if the message is not END, then it can only be a file name (one line in clienti.txt)
      // Call calculateLetterCount() to count letters
      struct msg_buffer ackBuff;

      timestamp();
      printf("Thread %d received %s from client process %d\n",
             tdata.clientID - 1,
             buff.mesg_text,
             tdata.clientID - 1);

      calculateLetterCount(buff.mesg_text);
      
      // After the file is read completely, send an "ACK" message back **edited line
      ackBuff.mesg_type = 1000 + tdata.clientID;
      strcpy(ackBuff.mesg_text, "ACK");

      timestamp();
      printf("Thread %d sending ACK to client %d for %s\n",
              tdata.clientID - 1,
              tdata.clientID - 1,
              buff.mesg_text);

      if (msgsnd(tdata.msgqueue, &ackBuff, sizeof(ackBuff.mesg_text), 0) == -1) {
        perror("msgsnd");
      }
    }   
  }
  return NULL;
}
