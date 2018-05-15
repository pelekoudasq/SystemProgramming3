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

#include <time.h>

#include "serverThread.h"
#include "auxFun.h"
#include "queue.h"

//queue init
Queue socketQueue;


//mutex init
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char **argv){


	//nine arguements, format: ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir
	if ( argc != 9 ){
		printf("Error. Arguement related error.\n");
		return -1;
	}

	//arguements' check
	if (strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-c") != 0 || strcmp(argv[5], "-t") != 0 || strcmp(argv[7], "-d") != 0) {
		printf("Error. Arguement related error.\n");
		return -1;
	}

	if (validate(argv[2]) == -1 || validate(argv[4]) == -1 || validate(argv[6]) == -1) {
		printf("Error. Arguement related error.\n");
		return -1;
	}

	int serving_port = atoi(argv[2]);
	int command_port = atoi(argv[4]);
	int num_of_threads = atoi(argv[6]);
	char* root_dir = argv[8];

	printf("%d %d %d Docfile name: %s.\n", serving_port, command_port, num_of_threads, root_dir);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(serving_port);

	//create socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	//bind to the socket
	if (bind(sock, (struct sockaddr*) &server, sizeof(server)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	//listen to the socket
	if (listen(sock, num_of_threads) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	Queue_create(&socketQueue, num_of_threads);

	//create threads
	pthread_t threads[num_of_threads];

	int i;
	for (i = 0; i < num_of_threads; ++i){
		//The second argument specifies attributes. The fourth argument is used to pass arguments to thread.
		pthread_create(&(threads[i]), NULL, serverThread, NULL);
	}
	
	//accept conection, create new socket
	struct sockaddr_in client;
	socklen_t clientlen;

	while(1){

		int newsock = accept(sock, (struct sockaddr*) &client, &clientlen);
		if (newsock < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		printf("Accepted connection\n");

		if (pthread_mutex_lock(&queueLock) < 0) { 
			/* Lock mutex */
			perror("lock");
			exit(EXIT_FAILURE);
		}
		
		Queue_push(&socketQueue, newsock);
		printf("Pushing: %d\n", newsock);

		if (pthread_mutex_unlock(&queueLock) < 0) {  
			/* Unlock mutex */
			perror("lock");
			exit(EXIT_FAILURE);
		}
	}

	close(sock);

	//send stuff
	do{
		printf("Give command: ");
		char* command = inputString(stdin, 10);
		if ( strcmp(command, "STATS") == 0 ){
			printf("command: STATS\n");
		}
		else if ( strcmp(command, "SHUTDOWN") == 0 ){
			printf("command: SHUTDOWN\n");
			free(command);
			break;
		}
		else {
			printf("command: SERVER\n");
		}
		free(command);
	}
	while(1);

	//code
	for (i = 0; i < num_of_threads; ++i){
		printf("join\n");
		pthread_join(threads[i], NULL);
	}
}
