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
#define NUMARGS 2
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
        if (dirContentPtr->d_type == DT_REG){
			fp[nFiles] = dirContentPtr->d_name;
          // For a file, you write its name into a mapper file (pointed by one entry in fp[])
          // NOTE: to balance the number of files per client, you can loop though all clients when distributing files
          // e.f. Assume you have 3 clients, then file1 for client1, file2 for client2, file3 for client3, file4 for client1, file 5 for client2...
        }else if (dirContentPtr->d_type == DT_DIR){
			recursiveTraverseFS(mappers, basepath, fp, &toInsert, &nFiles);
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
	chae *filename[100];

	//Create a folder 'ClientInput' to store CLient Input Files
	mkdir("ClientInput", 0777);
	// open client input files to store paths of files to be processed by each server thread
	int i;
	for (i = 0; i < clients; i++){
		fp[i] = fopen(filename, "w");
		// create the mapper file name (ClinetInput/clienti.txt)
		snprintf(filename, sizeof(filename), "ClientInput/client%d.txt", i);
	}

	// Call recursiveTraverseFS
	int toInsert = 0; //refers to the File to which the current file path should be inserted
	int nFiles = 0;
	recursiveTraverseFS(clients, path, fp, toInsert, nFiles);
	// close all the file pointers
	for (i = 0; i < clients; i++){
		fclose(fp[i]);
}

int main(int argc, char *argv[]){ 
  // Usage: ./client [input folder] [process num]
  char folderName[100] = {'\0'};
  strcpy(folderName, argv[1]);
  int num_clients = atoi(argv[2]);
  key_t key;

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
  mkdir("ClientOutputs", 0777);
  // Create `num_clients` children processes using fork()
  for (int i=0; i<num_clients; i++){
    pid_t pid = fork();
    if (pid==0){
      // For each client process, send each line of clienti to server
      char line[MSGLEN]={'\0'};
      FILE * ftr; // ftr should point to the correct clienti.txt
      while (fgets (line, MSGLEN, ftr)!=NULL ) {
        // Sned line
		msgsnd(msgqueue, (void *)&line, sizeof(line));
        // wait for ACK from server before sending the next line
		timestamp();
		
      }

      // When finish sending all the lines in clienti.txt
      // send END message to server
	  msgsnd(msgqueue, "END", sizeof("END"));
      //Wait with msgrcv() for the result (output string)
	  char res = msgrcv(msgqueue, (void)&msg, sizeof(msg), 0, 0);
      //write output to file
	  
      exit(0);
    }
  }

  //parent process waits for all children to finish
  for (int i = 0; i < num_clients; i++) {
	  wait(NULL);
  }
  return 0;

}
