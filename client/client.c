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
          // For a file, you write its name into a mapper file (pointed by one entry in fp[])
          // NOTE: to balance the number of files per client, you can loop though all clients when distributing files
          // e.f. Assume you have 3 clients, then file1 for client1, file2 for client2, file3 for client3, file4 for client1, file 5 for client2...
        }else if (dirContentPtr->d_type == DT_DIR){
          // For a directory, you call recursiveTraverseFS() 
        }
		}
	}
}

// Wrapper function for recursiveTraverseFS
// create folder ClientInput and inside the folder create txt file for each client (i.e., Client0.txt)
// After that, call traverseFS() to traversal and partition files
void traverseFS(int clients, char *path){
	FILE *fp[clients];

	//Create a folder 'ClientInput' to store CLient Input Files

	// open client input files to store paths of files to be processed by each server thread
	int i;
	for (i = 0; i < clients; i++){
		// create the mapper file name (ClinetInput/clienti.txt)
	}

	// Call recursiveTraverseFS
	int toInsert = 0; //refers to the File to which the current file path should be inserted
	int nFiles = 0;
	recursiveTraverseFS(clients, path, fp, &toInsert, &nFiles);

	// close all the file pointers
}

int main(int argc, char *argv[]){ 
  // Usage: ./client [input folder] [process num]
  char folderName[100] = {'\0'};
  strcpy(folderName, argv[1]);
  int num_clients = atoi(argv[2]);

  // call traverseFS() to traverse and partition files

  //Get access to the msg Queue

  // Create folder for outputs

  // Create `num_clients` children processes using fork()
  for (int i=0; i<num_clients; i++){
    pid_t pid = fork();
    if (pid==0){
      // For each client process, send each line of clienti to server
      char line[MSGLEN]={'\0'};
      FILE * ftr; // ftr should point to the correct clienti.txt
      while (fgets (line, MSGLEN, ftr)!=NULL ) {
        // Sned line

        // wait for ACK from server before sending the next line

      }

      // When finish sending all the lines in clienti.txt
      // send END message to server

      //Wait with msgrcv() for the result (output string)

      //write output to file

      exit(0);
    }
  }

  //parent process waits for all children to finish
  return 0;

}
