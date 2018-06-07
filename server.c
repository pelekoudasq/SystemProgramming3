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

extern Queue *socketQueue;
//mutex init
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_nonfull = PTHREAD_COND_INITIALIZER;
extern int num_of_threads;

int sock;
pthread_t *threads;

void sig_handler(int signo) {

	if (signo == SIGTERM){
		printf("Server: Received SIGTERM\n");
		if (pthread_mutex_lock(&queueLock) < 0){
			perror("lock");
			exit(EXIT_FAILURE);
		}

		Queue_destroy(socketQueue);

		if (pthread_mutex_unlock(&queueLock) < 0){
			perror("lock");
			exit(EXIT_FAILURE);
		}

		//join threads
		for (int i = 0; i < num_of_threads; ++i){
			if (pthread_cancel(threads[i]) != 0){
				perror("cancel");
			}
			printf("Server: Cancel thread\n");
		}
		
		close(sock);
		free(threads);
		//kill process
		exit(EXIT_SUCCESS);
	}
}

void serverProc(int serving_port){

	Queue_create(socketQueue, num_of_threads);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(serving_port);

	//create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("servering socket");
		exit(EXIT_FAILURE);
	}
	//bind to the socket
	if (bind(sock, (struct sockaddr*) &server, sizeof(server)) < 0) {
		perror("serving bind");
		exit(EXIT_FAILURE);
	}
	//listen to the socket
	if (listen(sock, 128) < 0) {
		perror("serving listen");
		exit(EXIT_FAILURE);
	}

	//create threads
	threads = malloc(sizeof(pthread_t)*num_of_threads);
	for (int i = 0; i < num_of_threads; ++i){
		//The second argument specifies attributes. The fourth argument is used to pass arguments to thread.
		pthread_create(&(threads[i]), NULL, serverThread, NULL);
	}
	
	//accept conection, create new socket

	while(1){
		
		signal(SIGTERM, sig_handler);

		int newsock = accept(sock, NULL, NULL);
		if (newsock < 0){
			perror("accept");
			exit(EXIT_FAILURE);
		}
		printf("Server: Accepted connection\n");

		if (pthread_mutex_lock(&queueLock) < 0){
			perror("server lock");
			exit(EXIT_FAILURE);
		}
		while (Queue_full(socketQueue) == 1){
			//printf("Server: Found Buffer Full \n");
			pthread_cond_wait(&cond_nonfull, &queueLock);
		}
		
		Queue_push(socketQueue, newsock);
		//printf("Pushing: %d\n", newsock);

		if (pthread_mutex_unlock(&queueLock) < 0){
			perror("server lock");
			exit(EXIT_FAILURE);
		}
		pthread_cond_signal(&cond_nonempty);
	}
}
