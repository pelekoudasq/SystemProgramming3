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

#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "crawlThread.h"
#include "auxFun.h"
#include "list.h"

list *pagesToAdd = NULL;
list *pagesAdded = NULL;

int port;
char *host;
char *save_dir;

int main(int argc, char **argv){


	//twelve arguements, format: ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL
	if ( argc != 12 ){
		printf("Error. Arguement related error.\n");
		return -1;
	}

	//arguements' check
	if (strcmp(argv[1], "-h") != 0 || strcmp(argv[3], "-p") != 0 || strcmp(argv[5], "-c") != 0 || strcmp(argv[7], "-t") != 0 || strcmp(argv[9], "-d") != 0) {
		printf("Error. Arguement related error.\n");
		return -1;
	}

	if (validate(argv[4]) == -1 || validate(argv[6]) == -1 || validate(argv[8]) == -1) {
		printf("Error. Arguement related error.\n");
		return -1;
	}

	host = argv[2];
	port = atoi(argv[4]);
	int command_port = atoi(argv[6]);
	int num_of_threads = atoi(argv[8]);
	save_dir = argv[10];
	char *starting_URL = malloc(sizeof(char)*(strlen(argv[11])+1));
	strcpy(starting_URL, argv[11]);

	mkdirs(save_dir);
	mkdir(save_dir, 755);

	list_add(&pagesToAdd, starting_URL);

	//create threads
	pthread_t threads[num_of_threads];

	int i;
	for (i = 0; i < num_of_threads; ++i){
		//The second argument specifies attributes. The fourth argument is used to pass arguments to thread.
		pthread_create(&(threads[i]), NULL, threadFun, NULL);
	}

    //wait until list empty and threads all sleep

	//send stuff
	do{
	 	printf("Give command: ");
		char *command = inputString(stdin, 10);
		if ( strcmp(command, "STATS") == 0 ){
			printf("command: STATS\n");
		}
		else if ( strcmp(command, "SHUTDOWN") == 0 ){
			printf("command: SHUTDOWN\n");
			free(command);
			break;
		}
		else {
			printf("command: CRAWLER\n");
		}
		free(command);
	}
	while(1);

	//code
	for (i = 0; i < num_of_threads; ++i){
		printf("join\n");
		pthread_join(threads[i], NULL);
	}

	while (!list_empty(pagesToAdd))
		free(list_rem(&pagesToAdd));

	while (!list_empty(pagesAdded))
		free(list_rem(&pagesAdded));
}
