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
		printf("Error. Arguement related error. %d\n", argc);
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
		printf("Error. Arguement related error.\n");
		return -1;
	}

	if ( numWorkers == 0 ){
		printf("Invalid number of workers\n");
		return -1;
	}

	printf("Docfile name: %s. ", docfile);
	printf("Number of workers: %d.\n", numWorkers);

	size_t size = 16;
	int numOfPaths;

	//get paths' file
	Paths *P = getFileOfPaths(docfile, size, &numOfPaths);
	
	printf("Number of Paths, %d\n", numOfPaths);


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
        	perror("server");
        printf("pipe made, name: %s\n", fifoName);
	}
	

	//create workers
	int returnValue;
	int i;
	int workerId[numWorkers];

	for(i = 0; i < numWorkers; i++) {
	    
	    //fork
	    returnValue = fork();
	    if (returnValue == -1){
	       	printf("fork failed\n");
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

		printPaths(P);

		int length;
		for (int j = 0; j < numOfPaths; j++){
			for (int i = 0; i < numWorkers; i++){
				if (P != NULL){
					jobexec = 0;
					printf("JE: sending to %d %d this: %s\n", FIFOS[i+1], workerId[i], P->content);
					length = strlen(P->content);
					if( write(FIFOS[i+1], P->content, length+1) < 0 ){
						perror("write path");
					}
					sleep(5);
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
			printf("JE: sending to %d %d this: %s\n", FIFOS[i+1], workerId[i], "stop");
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

		char *command;
		char **arguements;
		int args;
		arguements = malloc( 11*sizeof(char*) );
	
		char *line;
		line = (char *)malloc(sizeof(char));

		for (i = 0; i < 11; i++){
			arguements[i] = NULL;
		}

		//send stuff
		do{
			if ( arguements[0] != NULL )
 				free(arguements[0]);
	 	
		 	for (i = 0; i < 11; i++){
				arguements[i] = NULL;
			}
		 	printf("Give command: ");
		 	command = inputString(stdin, 10);

		 	char cur;
		 	char a = '\0';
		 	i = 0;
		 	args = 0;
		 	int resultFlag = 0;
			memcpy(line, &a, sizeof(char));

			while ( i <= strlen(command) ){
			
				cur = command[i];

				if ( cur == ' ' || cur == '\0'){
					if ( resultFlag == 1 ){
						if ( args > 13 ){
							printf("Too many arguements.\n");
							return -1;
						}
						arguements[args] = malloc( sizeof(char)*(strlen(line)) + 1 );
						strcpy(arguements[args], line);
						args++;
						memcpy(line, &a, sizeof(char));
						resultFlag = 0;
					}
				}
				else {
					strncat(line, &cur, sizeof(char));
					resultFlag = 1;
				}
				i++;
			}
			if ( strcmp(arguements[0], "/wc") == 0 ){
				printf("WORD COUNT\n");
				
				if ( args == 1 ){
					int res[3];
					for ( int j = 0; j < 3; j++) res[j] = 0;
					for ( i = 0; i < numWorkers; i++ ){
						kill(workerId[i], SIGIO);
						length = strlen("wc");
						if( write(FIFOS[i+1], "wc", length+1) < 0 ){
							perror("write path");
						}
						sleep(2);
						for( int j = 0; j < 3; j++ ){
							int temp = 0;
							int returnValue = read(FIFOS[i], &temp, sizeof(int));
							while ( returnValue < 0 ){
     						  	sleep(1);
						       	returnValue = read(FIFOS[i], &temp, sizeof(int));
						    }
						    res[j]+=temp;
						}
					}
					printf("TOTAL Bytes: %d, Words: %d, Lines: %d\n", res[0], res[1], res[2]);
				} else {
					printf("Retry.\n");
				}
			}
			else if ( strcmp(arguements[0], "/maxcount") == 0 ){

				if ( args == 2 ){
					char *keyword = arguements[1];
					countResults *cRes = NULL;
					int flag = 0;
					for ( i = 0; i < numWorkers; i++ ){
						kill(workerId[i], SIGIO);
						length = strlen("max");
						if( write(FIFOS[i+1], "max", length+1) < 0 ){
							perror("write path");
						}
						sleep(1);
						length = strlen(keyword);
						if( write(FIFOS[i+1], keyword, length+1) < 0 ){
							perror("write path");
						}
						sleep(2);
						//reading end
						char fileName[80];
						int wordFrq = 0;
						int returnValue = read(FIFOS[i], fileName, 80);
						while ( returnValue < 0 ){
							sleep(1);
							returnValue = read(FIFOS[i], fileName, 80);
						}
						sleep(1);
						returnValue = read(FIFOS[i], &wordFrq, sizeof(int));
						while ( returnValue < 0 ){
							sleep(1);
							returnValue = read(FIFOS[i], &wordFrq, sizeof(int));
						}
						printf("JE: fileName: %s, wordFrq: %d\n", fileName, wordFrq);
						char temp[80];
						if ( strcmp(fileName,"nofile") != 0 ){
							if ( flag == 0 ){
								flag++;
								cRes = newCountResNode(NULL);
								strcpy(temp, fileName);
								cRes->fileName = temp;
								cRes->wordFrq = wordFrq;
							} else {
								if ( wordFrq > cRes->wordFrq ){
									strcpy(temp, fileName);
									cRes->fileName = temp;
									cRes->wordFrq = wordFrq;
								}
							}
						}
						sleep(1);
					}
					if (cRes == NULL){
						printf("No results for: %s\n", keyword);
					} else{
						printf("RESULTS for \"%s\": File: \"%s\" with word frequency: %d\n", arguements[1], cRes->fileName, cRes->wordFrq);
					}
				} else {
					printf("Retry.\n");
				}
			}
			else if ( strcmp(arguements[0], "/mincount") == 0 ){

				if ( args == 2 ){
					char *keyword = arguements[1];
					countResults *cRes = NULL;
					int flag = 0;
					for ( i = 0; i < numWorkers; i++ ){
						kill(workerId[i], SIGIO);
						length = strlen("min");
						if( write(FIFOS[i+1], "min", length+1) < 0 ){
							perror("write path");
						}
						sleep(1);
						length = strlen(keyword);
						if( write(FIFOS[i+1], keyword, length+1) < 0 ){
							perror("write path");
						}
						//receiving end
						char fileName[80];
						int wordFrq = 0;
						int returnValue = read(FIFOS[i], fileName, 80);
						while ( returnValue < 0 ){
							sleep(1);
							returnValue = read(FIFOS[i], fileName, 80);
						}
						sleep(1);
						returnValue = read(FIFOS[i], &wordFrq, sizeof(int));
						while ( returnValue < 0 ){
							sleep(1);
							returnValue = read(FIFOS[i], &wordFrq, sizeof(int));
						}
						printf("JE: fileName: %s, wordFrq: %d\n", fileName, wordFrq);
						char temp[80];
						if ( strcmp(fileName,"nofile") != 0 ){
							if ( flag == 0 ){
								flag++;
								cRes = newCountResNode(NULL);
								strcpy(temp, fileName);
								cRes->fileName = temp;
								cRes->wordFrq = wordFrq;
							} else {
								if ( wordFrq < cRes->wordFrq ){
									strcpy(temp, fileName);
									cRes->fileName = temp;
									cRes->wordFrq = wordFrq;
								}
							}
						}
						sleep(1);
					}
					if (cRes == NULL){
						printf("No results for: %s\n", arguements[1]);
					} else{
						printf("RESULTS for \"%s\": File: \"%s\" with word frequency: %d\n", arguements[1], cRes->fileName, cRes->wordFrq);
					}
				} else {
					printf("Retry.\n");
				}
			}
			else if ( strcmp(arguements[0], "/search") == 0 ){
				if ( args >= 4 ){
					int flagNo = 0;
					i = 1;
					int deadline;
					while ( arguements[i] != NULL ){

						if ( strcmp(arguements[i], "-d") == 0 ){
							if ( arguements[i+1] != NULL && validate(arguements[i+1]) == 0 ){
								deadline = atoi(arguements[i+1]);
								printf("deadline: %d, ", deadline);
								flagNo = i;
								printf("%d\n", flagNo);
								break;
							} else {
								printf("Retry.\n");
								break;
							}
						}
						i++;
					}
					if ( i >= args ){
						printf("Retry.\n");
					}
				} else {
					printf("Retry.\n");
				}
			}
			else if ( strcmp(arguements[0], "/exit") != 0 ){
				printf("Retry.\n");
			}

			for (i = 1; i < 11; i++){
				if (arguements[i] != NULL){
					free(arguements[i]);
				}
			}
			free(command);
		}
		while( strcmp(arguements[0], "/exit") != 0);
		
		free(arguements[0]);
		free(arguements);


		//close the files-pipes
		close(fdWrite);
		close(fdRead);
		//sleep(5);
		for (int i = 0; i<numWorkers; i++){
			printf("JE: sending SIGTERM to %d\n", workerId[i]);
			kill(workerId[i], SIGTERM);
			sleep(1);
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
        	perror("server");
        printf("JE: unlink %s\n", fifoName);
	}

	freePaths(P);
	return 0;
}
