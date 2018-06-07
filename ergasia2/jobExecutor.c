#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "paths.h"
#include "worker.h"
#include "trie.h"

volatile sig_atomic_t jobexec;


int validate (char *a){

	for (unsigned int i = 0; i < strlen(a); i++){
		if(!isdigit(a[i])){
			return -1;
		}
	}
	return 0;
}

char *inputString(FILE* fp, size_t size){
	
	//The size is extended by the input with the value of the provisional
    char *str;
    int ch;
    size_t len = 0;

    str = realloc(NULL, sizeof(char)*size);//size is start size

    //if realloc fails
    if( !str )
    	return str;

    while( EOF != ( ch = fgetc(fp) ) && ch != '\n' ){

        str[len++] = ch;
        if( len == size ){
            str = realloc(str, sizeof(char)*(size+=16));
            if( !str )
            	return str;
        }
    }
    str[len++]='\0';

    return realloc(str, sizeof(char)*len);
}


int main(int argc, char **argv){


	//five arguements, format: ./minisearch -i docfile -k K 
	if ( argc != 5 ){
		fprintf(stderr, "Error. Arguement related error. %d\n", argc);
		return -1;
	}

	int numWorkers;
	char *docfile;

	//arguements' check
	if ( strcmp(argv[1], "-d") == 0 && strcmp(argv[3], "-w") == 0 && !validate(argv[4]) ){
		docfile = argv[2];
		numWorkers = atoi(argv[4]);
	} else if ( strcmp(argv[1], "-w") == 0 && strcmp(argv[3], "-d") == 0 && !validate(argv[2]) ){
		docfile = argv[4];
		numWorkers = atoi(argv[2]);
	} else {
		fprintf(stderr, "Error. Arguement related error.\n");
		return -1;
	}

	if ( numWorkers == 0 ){
		fprintf(stderr, "Invalid number of workers\n");
		return -1;
	}

	fprintf(stderr, "Docfile name: %s. ", docfile);
	fprintf(stderr, "Number of workers: %d.\n", numWorkers);

	size_t size = 16;
	int numOfPaths;

	//get paths' file
	Paths *P = getFileOfPaths(docfile, size, &numOfPaths);
	
	fprintf(stderr, "Number of Paths, %d\n", numOfPaths);


	//create pipes, two per worker process
	char *fifoName = malloc(sizeof(char)*20);
	char *buffer = malloc(sizeof(char)*5);

	for(int i = 0; i < numWorkers*2; i++){
		
		memset(fifoName, 0, 15);
		memset(buffer, 0, 5);
		strcpy(fifoName, "/tmp/pipe");
		sprintf(buffer,"%d", i+1);
		strcat(fifoName, buffer);

		if(mkfifo(fifoName, 0666) == -1)
        	perror("fifo make");
        fprintf(stderr, "pipe made, name: %s\n", fifoName);
	}
	

	//create workers
	int returnValue;
	int i;
	int workerId[numWorkers];

	for(i = 0; i < numWorkers; i++) {
	    
	    //fork
	    returnValue = fork();
	    if (returnValue == -1){
	       	fprintf(stderr, "fork failed\n");
	       	return -13;
	   	}
	    if (returnValue == 0) {
	        break;
	    } else {
	    	workerId[i] = returnValue;
	    }
	}

	//if we are a worker:
	if ( returnValue == 0 ) {
		worker(i);
	}
	//job executor:
	else {

		//open all pipes (R), jobExecutor side
		int FIFOS[numWorkers*2];
		int fdWrite, fdRead;

		//for every pair of pipes per Worker
		for (int i = 0; i < numWorkers*2; i+=2){
			//construct the name of the already created pipe, 1st
			memset(fifoName, 0, 15);
			memset(buffer, 0, 5);
			strcpy(fifoName, "/tmp/pipe");
			sprintf(buffer,"%d", i+1);
			strcat(fifoName, buffer);

			//open the pipe-file, read-only for this side, save the file descriptor
			fdRead = open(fifoName, O_RDWR);
			FIFOS[i] = fdRead;

			//construct the name of the already created pipe, 2nd
			memset(fifoName, 0, 15);
			memset(buffer, 0, 5);
			strcpy(fifoName, "/tmp/pipe");
			sprintf(buffer,"%d", i+2);
			strcat(fifoName, buffer);
  
			//open the pipe-file, write-only for this side, save the file descriptor
			fdWrite = open(fifoName, O_RDWR);
			FIFOS[i+1] = fdWrite;
			fcntl(fdRead, F_SETFL, O_NONBLOCK);
			fcntl(fdWrite, F_SETFL, O_NONBLOCK);
		}

		//printPaths(P);

		int length;
		for (int j = 0; j < numOfPaths; j++){
			for (int i = 0; i < numWorkers; i++){
				if (P != NULL){
					jobexec = 0;
					fprintf(stderr, "JE: sending to %d %d this: %s\n", FIFOS[i+1], workerId[i], P->content);
					length = strlen(P->content);
					if( write(FIFOS[i+1], P->content, length+1) < 0 ){
						perror("write path");
					}
					sleep(1);
				}
				if(P->next == NULL){
					break;
				}
				P = P->next;
			}
			j = j + i - 1;
		}

		//send signals, meaning end of files
		for (int i = 0; i < numWorkers; i++){
			fprintf(stderr, "JE: sending to %d %d this: %s\n", FIFOS[i+1], workerId[i], "stop");
			length = strlen("stop");
			if( write(FIFOS[i+1], "stop", 4+1) < 0 ){
				perror("write path");
			}
		}

		struct stat st = {0};

		if (stat("./log", &st) == -1) {
		    mkdir("./log", 0777);
		}

		sleep(1);

		char *command = NULL;

		do{
			if (command != NULL){
				free(command);
			}
	 		
		 	command = inputString(stdin, 10);
		 	//fprintf(stderr, "command given %s\n", command);
			
			if ( strncmp(command, "/search", strlen("/search")) == 0 ){
				//fprintf(stderr, "search\n");
				printf("Search not implemented\n");
				fflush(stdout);
			}
		}
		while( strncmp(command, "/exit", strlen("/exit")) != 0);
		
		free(command);

		//close the files-pipes
		close(fdWrite);
		close(fdRead);
		//sleep(5);
		for (int i = 0; i<numWorkers; i++){
			fprintf(stderr, "JE: sending SIGTERM to %d\n", workerId[i]);
			kill(workerId[i], SIGTERM);
			sleep(0.1);
			//wait(NULL);
		}
	}

	//unlinked named-pipes
	for(int i = 0; i < numWorkers*2; i++){
		
		memset(fifoName, 0, 15);
		memset(buffer, 0, 5);
		strcpy(fifoName, "/tmp/pipe");
		sprintf(buffer,"%d", i+1);
		strcat(fifoName, buffer);

		if(unlink(fifoName) < 0)
        	perror("fifo unlink");
        fprintf(stderr, "JE: unlink %s\n", fifoName);
	}

	freePaths(P);
	return 0;
}
