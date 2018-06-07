#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <dirent.h>

#include "worker.h"
#include "textmap.h"
#include "trie.h"
#include "postinglist.h"

volatile sig_atomic_t stop;

trieNode *trieRoot = NULL;

wcResults *wc = NULL;

FILE* f;

char *fifoName;
char *buffer;
int fdRead, fdWrite;

time_t seconds;

time_t     now;
struct tm  ts;
char       buf[80];


wcResults *newWcResultsNode(char* fileName, int bytes, int words, int lines){
	wcResults *temp = malloc(sizeof(wcResults));
	temp->fileName = fileName;
	temp->bytes = bytes;
	temp->words = words;
	temp->lines = lines;
	temp->next = NULL;
	return temp;
}

void freeWC(wcResults *root){
	if (root != NULL){
		freeWC(root->next);
		free(root);
	}
}

countResults *newCountResNode(char *fileName){
	countResults *temp = malloc(sizeof(countResults));
	temp->fileName = fileName;
	temp->wordFrq = 1;
	temp->next = NULL;
	return temp;
}

void freeRes(countResults *root){
	if (root != NULL){
		freeRes(root->next);
		free(root);
	}
}

void sig_handler(int signo) {

	if (signo == SIGIO){
		fprintf(stderr, "--> received SIGIO\n");
		stop = 1;
	} else if (signo == SIGTERM){
		fprintf(stderr, "--> received SIGTERM\n");
		freeTrie(trieRoot);
		freeWC(wc);
		free(fifoName);
		free(buffer);
		//close the files-pipes
		close(fdWrite);
		close(fdRead);
		fclose(f);
		//kill process
		exit(EXIT_SUCCESS);
	}
}

void waitForSignal(int fdRead, int fdWrite){

	signal(SIGIO, sig_handler);
	signal(SIGTERM, sig_handler);
	stop = 0;

	while(!stop)
		pause();

	time(&now);
	ts = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ts);

	char str[80];
	int returnValue = read(fdRead, str, 80);
    while ( returnValue < 0){
       	sleep(1);
       	returnValue = read(fdRead, str, 80);
    }

    if( strcmp(str,"wc") == 0 ){
    	int res[3];
    	int i;
    	for ( i = 0; i < 3; i++) res[i] = 0;
    	wcResults *curWC = wc;
    	while ( curWC != NULL ){
    		fprintf(stderr, "Name: %s, bytes: %d, words: %d, lines: %d\n", curWC->fileName, curWC->bytes, curWC->words, curWC->lines);
    		res[0]+=curWC->bytes;
    		res[1]+=curWC->words;
    		res[2]+=curWC->lines;
    		//next
    		curWC = curWC->next;
    	}
    	//send results
    	for ( i = 0; i < 3; i++){
			fprintf(stderr, "--> sending to %d this: %d\n", getppid(), res[i]);
			if( write(fdWrite, &(res[i]), sizeof(int)) < 0 ){
				perror("write path");
			}
    	}
    	fprintf(f, "%s : wc : %d : %d : %d\n", buf, res[0], res[1], res[2]);
    } 
    else if ( strcmp(str,"max") == 0 ){
    	//fprintf(stderr, "--> MAXCOUNT\n");
    	//sleep(1);
    	char keyword[80];
		int returnValue = read(fdRead, keyword, 80);
    	while ( returnValue < 0){
       		sleep(1);
       		returnValue = read(fdRead, keyword, 80);
    	}
    	fprintf(f, "%s : maxcount : %s", buf, keyword);
    	//fprintf(stderr, "keyword: %s\n", keyword);
    	plistNode *post = NULL;
    	post = searchWordInTrie(trieRoot, keyword);
    	//fprintf(stderr, "Done with search\n");
    	countResults *cRes = NULL;
    	cRes = createCountResults(post);
    	/*countResults *curRes = cRes;
    	while (curRes != NULL){
    		fprintf(f, " : %s", curRes->fileName);
    		curRes = curRes->next;
    	}*/
    	//fprintf(stderr, "Done with results\n");
    	cRes = maxFinder(cRes);
    	if (cRes != NULL)
    		fprintf(f, " : %s", cRes->fileName);
    	//fprintf(stderr, "Done with sort\n");
    	//fprintf(stderr, "Sending..\n");
    	char *fileName = NULL;
    	int wordFrq = 0;
    	if ( cRes != NULL){
    		fileName = cRes->fileName;
    		wordFrq = cRes->wordFrq;
    	} else {
    		fileName = malloc(sizeof(char)*7);
    		strcpy(fileName, "nofile");
    	}
    	int length = strlen(fileName);
    	fprintf(stderr, "--> fileName: %s, wordFrq: %d\n", fileName, wordFrq);
    	if( write(fdWrite, fileName, length+1) < 0 ){
			perror("write path");
		}
		sleep(3);
		if( write(fdWrite, &wordFrq, sizeof(int)) < 0 ){
			perror("write path");
		}
		fprintf(f, "\n");
		freeRes(cRes);
    }
    else if ( strcmp(str,"min") == 0 ){
    	//fprintf(stderr, "--> MINCOUNT\n");
    	//sleep(1);
    	char keyword[80];
		int returnValue = read(fdRead, keyword, 80);
    	while ( returnValue < 0){
       		sleep(1);
       		returnValue = read(fdRead, keyword, 80);
    	}
    	fprintf(f, "%s : mincount : %s", buf, keyword);
    	//fprintf(stderr, "keyword: %s\n", keyword);
    	plistNode *post = NULL;
    	post = searchWordInTrie(trieRoot, keyword);
    	//fprintf(stderr, "Done with search\n");
    	countResults *cRes = NULL;
    	cRes = createCountResults(post);
    	/*countResults *curRes = cRes;
    	while (curRes != NULL){
    		fprintf(f, " : %s", curRes->fileName);
    		curRes = curRes->next;
    	}*/
    	//fprintf(stderr, "Done with results\n");
    	cRes = minFinder(cRes);
    	if (cRes != NULL)
    		fprintf(f, " : %s", cRes->fileName);
    	//fprintf(stderr, "Done with sort\n");
    	//fprintf(stderr, "Sending..\n");
    	char *fileName = NULL;
    	int wordFrq = 0;
    	if ( cRes != NULL){
    		fileName = cRes->fileName;
    		wordFrq = cRes->wordFrq;
    	} else {
    		fileName = malloc(sizeof(char)*7);
    		strcpy(fileName, "nofile");
    	}
    	int length = strlen(fileName);
    	fprintf(stderr, "--> fileName: %s, wordFrq: %d\n", fileName, wordFrq);
    	if( write(fdWrite, fileName, length+1) < 0 ){
			perror("write path");
		}
		sleep(3);
		if( write(fdWrite, &wordFrq, sizeof(int)) < 0 ){
			perror("write path");
		}
		fprintf(f, "\n");
		freeRes(cRes);
    }
    waitForSignal(fdRead, fdWrite);

}


int worker(int i){

	fifoName = malloc(sizeof(char)*20);
	buffer = malloc(sizeof(char)*5);
	int returnValue;

	//construct the name of the already created pipe, 1nd
	memset(fifoName, 0, 15);
	memset(buffer, 0, 5);
	strcpy(fifoName, "/tmp/pipe");
	sprintf(buffer,"%d", i+2);
	strcat(fifoName, buffer);

	//open the pipe-file, get the file decriptor, this is gonna be the read only pipe
	fdRead = open(fifoName, O_RDWR);
	if (fdRead < 0){
		perror("fdRead child");
	}

    //construct the name of the already created pipe, 2st
	memset(fifoName, 0, 15);
	memset(buffer, 0, 5);
	strcpy(fifoName, "/tmp/pipe");
	sprintf(buffer,"%d", i+1);
	strcat(fifoName, buffer);

	//open the pipe-file, get the file decriptor, this is gonna be the write only pipe
	fdWrite = open(fifoName, O_RDWR);
	if (fdWrite < 0){
		perror("fdWrite child");
	}

	fcntl(fdRead, F_SETFL, O_NONBLOCK);
	fcntl(fdWrite, F_SETFL, O_NONBLOCK);

	//create log file
	char *logfile = malloc(sizeof(char)*20);
	char *pid = malloc(sizeof(char)*5);
	memset(logfile, 0, 20);
	memset(pid, 0, 5);
	strcpy(logfile, "./log/Worker_");
	sprintf(pid,"%d", getpid());
	strcat(logfile, pid);

	f = fopen(logfile, "w");
	free(logfile);
	free(pid);

	//DIR *dir;
	//struct dirent *ent;



  	//get paths
	while(1){

		char str[80];
		returnValue = read(fdRead, str, 80);
        while ( returnValue < 0){
        	sleep(1);
        	returnValue = read(fdRead, str, 80);
        }
       	fprintf(stderr, "--> pid: %d, %s, %d\n", getpid(), str, (int)(strlen(str)));

       	if( strcmp(str,"stop") == 0 ){
       		break;
       	}

       	/*if ((dir = opendir (str)) != NULL) {
			while ((ent = readdir (dir)) != NULL) {
				if ( strcmp(ent->d_name,".") != 0 && strcmp(ent->d_name,"..") != 0){

					//create fileName's path from workers root dir
					int l = strlen(str) + strlen(ent->d_name) + 1;
					char* fileName = malloc(sizeof(char)*l);
					memset(fileName, 0, l);
					strcpy(fileName, str);
					strcat(fileName, "/");
					strcat(fileName, ent->d_name);

					int bytes = 0, words = 0;

					//create map
					printf ("--> FILE: %s, %s\n", ent->d_name, fileName);
					int N;
					text *newText = addFileToMaps(fileName, &N, &bytes);

					if (newText == NULL){
						fprintf(stderr, "--> Error with file, on to next one\n");
					} else {
						//add to trie(or create if trieRoot is NULL)
						trieRoot = createTrieFromTextMap(newText, N, fileName, trieRoot, &words);
						fprintf(stderr, "WORDS: %d\n", words);
						if (trieRoot == NULL){
							fprintf(stderr, "--> Error with trie\n");
							exit(EXIT_FAILURE);
						}
					}

					if ( wc == NULL ){
						wc = newWcResultsNode(fileName, bytes, words, N);
					} else {
						wcResults *curWC = wc;
						while (1){
							if ( curWC->next != NULL ){
								curWC = curWC->next;
							} else {
								break;
							}
						}
						curWC->next = newWcResultsNode(fileName, bytes, words, N);
					}
					freeTexts(newText);
					fprintf(stderr, "********************************************************\n");

				}
			}
			closedir (dir);
		} else {
			perror ("");
			exit(EXIT_FAILURE);
		}*/		
	}

	fprintf(stderr, "--> got all paths\n");

	waitForSignal(fdRead, fdWrite);

	exit(EXIT_SUCCESS);
}