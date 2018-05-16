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

extern Queue socketQueue;
extern pthread_mutex_t queueLock;
extern int num_of_threads;

int sock;
pthread_t *threads;

void sig_handler(int signo) {

	if (signo == SIGTERM){
		printf("--> received SIGTERM\n");
		if (pthread_mutex_lock(&queueLock) < 0) { 
			/* Lock mutex */
			perror("lock");
			exit(EXIT_FAILURE);
		}

		Queue_destroy(&socketQueue);

		if (pthread_mutex_unlock(&queueLock) < 0) {  
			/* Unlock mutex */
			perror("lock");
			exit(EXIT_FAILURE);
		}

		//join threads
		for (int i = 0; i < num_of_threads; ++i){
			printf("join\n");
			pthread_join(threads[i], NULL);
		}

		if (pthread_mutex_destroy(&queueLock) < 0) {  
			/* Unlock mutex */
			perror("destroy");
			exit(EXIT_FAILURE);
		}
		
		close(sock);
		//kill process
		exit(EXIT_SUCCESS);
	}
}

void serverProc(int serving_port){

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(serving_port);

	//create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
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
	threads = malloc(sizeof(pthread_t)*num_of_threads);
	for (int i = 0; i < num_of_threads; ++i){
		//The second argument specifies attributes. The fourth argument is used to pass arguments to thread.
		pthread_create(&(threads[i]), NULL, serverThread, NULL);
	}
	
	//accept conection, create new socket
	struct sockaddr_in client;
	socklen_t clientlen  = sizeof(client);

	while(1){
		signal(SIGTERM, sig_handler);

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
}