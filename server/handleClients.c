#include "handleClients.h"
pthread_mutex_t clientCountLock = PTHREAD_MUTEX_INITIALIZER, letterLock = PTHREAD_MUTEX_INITIALIZER;

// print timestamp
// You can get timestamp using localtime()
void timestamp(){
}

// Count the occurence of all 26 letters, and update letterCount[] correspondingly
void calculateLetterCount(char *filename){
}

// Create a string based on letterCount[], the final return character array 
// The string looks like count1#count2#....#count26#
char* convertLetterCountToChar(){
}

// called by threads created on server
// args: pointer to the thd_data struct of current client
void* processClients(void* args){

  struct thd_data tdata = *((struct thd_data *) args);

  while (1) {
    struct msg_buffer buff;

    // Waiting to received from client process
    // Should store the message in buff

    // Handle the received message
    if (strcmp(buff.mesg_text, "END") == 0){
      // if the message is END:

      // wait for all threads to complete

      // Convert letter array to character array using convertLetterCountToChar()
      // and send it back to client
      // After that, break the while loop to exit the server

    }else{
      // if the message is not END, then it can only be a file name (one line in clienti.txt)
      // Call calculateLetterCount() to count letters
    }    

    // After the file is read completely, send an "ACK" message back
  }
}
