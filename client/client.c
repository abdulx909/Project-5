#include <sys/types.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define MSGLEN 1024
#define NUMARGS 3
#define DIRNULL NULL

struct msg_buffer {
    long mesg_type;
    char mesg_text[MSGLEN];
};

// Should be the same function as in handleClients.c
void timestamp(){
  time_t now;
  struct tm *timeInfo;
  char timeString[80];

  time(&now);
  timeInfo = localtime(&now);
  strftime(timeString, sizeof(timeString), "%a %b %d %H:%M:%S %Y", timeInfo);

  printf("[%s]", timeString);
}

// Traversal the file system recursively and write file pathes to mapper files (ClinetInput/clienti.txt
// mappers: the number of mapper files (equals to the number of clients
// fp[]: an array of file descriptors, each descriptor for the mapper file of one clinet 
// toInsert: the number of the next mapper file to insert file name. Pass the current value of `toInsert` when calling recursiveTraverseFS recursively
// nFiles: the total number of files you traversed
void recursiveTraverseFS(int mappers, char *basePath, FILE *fp[], int *toInsert, int *nFiles){
	struct dirent *dirContentPtr;
	//check if the directory exists
	DIR *dir = opendir(basePath);
	if(dir == DIRNULL){
		printf("Unable to read directory %s\n", basePath);
		exit(1);
	}

	while((dirContentPtr = readdir(dir)) != DIRNULL){
    // This while loop traversal all folder/files under `dir`
    // See https://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html for directory entry formats    
    
		if (strcmp(dirContentPtr->d_name, ".") != 0 &&
			strcmp(dirContentPtr->d_name, "..") != 0 &&
      strcmp(dirContentPtr->d_name, ".DS_Store") != 0 &&
      (dirContentPtr->d_name[0] != '.'))
      {
        char fullPath[MSGLEN];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", basePath, dirContentPtr->d_name);
        if (dirContentPtr->d_type == DT_REG){
			    fprintf(fp[*toInsert], "%s\n", fullPath);
          *toInsert = (*toInsert + 1) % mappers;
          (*nFiles)++;;
          // For a file, you write its name into a mapper file (pointed by one entry in fp[])
          // NOTE: to balance the number of files per client, you can loop though all clients when distributing files
          // e.f. Assume you have 3 clients, then file1 for client1, file2 for client2, file3 for client3, file4 for client1, file 5 for client2...
        }else if (dirContentPtr->d_type == DT_DIR){
			    recursiveTraverseFS(mappers, fullPath, fp, toInsert, nFiles);
          // For a directory, you call recursiveTraverseFS() 
        }
		}
	}
	closedir(dir);
}

// Wrapper function for recursiveTraverseFS
// create folder ClientInput and inside the folder create txt file for each client (i.e., Client0.txt)
// After that, call traverseFS() to traversal and partition files
void traverseFS(int clients, char *path){
	FILE *fp[clients];
	char filename[100];

	//Create a folder 'ClientInput' to store CLient Input Files
	mkdir("ClientInput", 0777);
	// open client input files to store paths of files to be processed by each server thread
	for (int i = 0; i < clients; i++){
		// create the mapper file name (ClinetInput/clienti.txt)
		snprintf(filename, sizeof(filename), "ClientInput/Client%d.txt", i);
    fp[i] = fopen(filename, "w");
    if (fp[i] == NULL) {
      perror("fopen");
      exit(1);
    }
	}


	// Call recursiveTraverseFS
	int toInsert = 0; //refers to the File to which the current file path should be inserted
	int nFiles = 0;
	recursiveTraverseFS(clients, path, fp, &toInsert, &nFiles);
	// close all the file pointers
	for (int i = 0; i < clients; i++){
		fclose(fp[i]);
  }
}

int main(int argc, char *argv[]){ 
  // Usage: ./client [input folder] [process num]
  char folderName[100] = {'\0'};
  int num_clients;
  key_t key;
  int msgqueue;

  setbuf(stdout, NULL);
  if (argc != NUMARGS) {
    fprintf(stderr, "Usage: ./client [input folder] [process num]\n");
    return 1;
  }

  strcpy(folderName, argv[1]);
  num_clients = atoi(argv[2]);
  if (num_clients <= 0) {
    fprintf(stderr, "Invalid number of clients.\n");
    return 1;
  }

  timestamp();
  printf("Client starts...\n");

  timestamp();
  printf("Directory %s traversal and file partitioning...\n", folderName);

  // call traverseFS() to traverse and partition files
  traverseFS(num_clients, folderName);
  //Get access to the msg Queue
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
  // Create folder for outputs
  mkdir("Output", 0777);
  // Create `num_clients` children processes using fork()
  for (int i=0; i<num_clients; i++){
    pid_t pid = fork();
    
    if (pid < 0) {
      perror("fork");
      return 1;
    }
    
    if (pid==0){
      // For each client process, send each line of clienti to server
      char line[MSGLEN]={'\0'};
      FILE * ftr; // ftr should point to the correct clienti.txt
      struct msg_buffer msg;
      struct msg_buffer ack;    
      struct msg_buffer result;

      char filename[100];
      snprintf(filename, sizeof(filename), "ClientInput/Client%d.txt", i);
      ftr = fopen(filename, "r");

      if (ftr == NULL) {
        perror("fopen");
        exit(1);
      }
      
      while (fgets (line, MSGLEN, ftr)!=NULL) {
        line[strcspn(line, "\n")] = '\0';
        // Sned line
        msg.mesg_type = i + 1;
        strcpy(msg.mesg_text, line);
        timestamp();
        printf("Sending %s from client process %d\n", line, i);

        // wait for ACK from server before sending the next line
        if (msgsnd(msgqueue, &msg, sizeof(msg.mesg_text), 0) == -1) {
          perror("msgsnd");
          fclose(ftr);
          exit(1);
        }

        if (msgrcv(msgqueue, &ack, sizeof(ack.mesg_text), 1000 + (i + 1), 0) == -1) {
          perror("msgrcv");
          fclose(ftr);
          exit(1);
        }

        timestamp();
        printf("Client process %d received %s from server for %s\n", i, ack.mesg_text, line);
      }

      fclose(ftr);

      // When finish sending all the lines in clienti.txt
      // send END message to server
	    msg.mesg_type = i + 1;
      strcpy(msg.mesg_text, "END");

      timestamp();
      printf("Sending END from client process %d\n", i);

      if (msgsnd(msgqueue, &msg, sizeof(msg.mesg_text), 0) == -1) {
        perror("msgsnd");
        exit(1);
      }

      if (msgrcv(msgqueue, &result, sizeof(result.mesg_text), 2000 + (i + 1), 0) == -1) {
        perror("msgrcv");
        exit(1);
      }

      timestamp();
      printf("Client process %d received |||%s||| from server\n", i, result.mesg_text);
      //write output to file
      char outputFile[100]; // UPDATED
      snprintf(outputFile, sizeof(outputFile), "Output/Client%d_out.txt", i);
      FILE *out = fopen(outputFile, "w");
      if (out != NULL) {
        fprintf(out, "%s", result.mesg_text);
        fclose(out);
      }
      exit(0);
    }
  }

  //parent process waits for all children to finish
  for (int i = 0; i < num_clients; i++) {
	  wait(NULL);
  }

  timestamp();
  printf("Client ends...\n");
  return 0;
}
