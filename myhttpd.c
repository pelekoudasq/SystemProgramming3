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
#include <sys/mman.h>

#include "serverThread.h"
#include "auxFun.h"
#include "queue.h"
#include "server.h"

//queue init
Queue *socketQueue;
//num_of_threads
int num_of_threads;
char* root_dir;


int main(int argc, char **argv){

	clock_t start, end;
	start = clock();
	//nine arguements, format: ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir
	if ( argc != 9 ){
		printf("Server: Error. Arguement related error.\n");
		return -1;
	}

	//arguements' check
	if (strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-c") != 0 || strcmp(argv[5], "-t") != 0 || strcmp(argv[7], "-d") != 0) {
		printf("Server: Error. Arguement related error.\n");
		return -1;
	}

	if (validate(argv[2]) == -1 || validate(argv[4]) == -1 || validate(argv[6]) == -1) {
		printf("Server: Error. Arguement related error.\n");
		return -1;
	}

	int serving_port = atoi(argv[2]);
	int command_port = atoi(argv[4]);
	num_of_threads = atoi(argv[6]);
	root_dir = argv[8];

	printf("Serving Port: %d, Command Port: %d, Number of Threads: %d, Root Directory: %s.\n", serving_port, command_port, num_of_threads, root_dir);

	//shared memory
	socketQueue = mmap(NULL, sizeof (Queue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	//fork
	int returnValue;

	returnValue = fork();
	if(returnValue == -1){
		printf("Server: Fork failed\n");
		return -13;
	}

	if(returnValue == 0) {
		//serving port
		serverProc(serving_port);	
		
	} else{
		//command port
		struct sockaddr_in commandReceiver;
		commandReceiver.sin_family = AF_INET;
		commandReceiver.sin_addr.s_addr = htonl(INADDR_ANY);
		commandReceiver.sin_port = htons(command_port);

		//create socket
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			perror("server socket");
			exit(EXIT_FAILURE);
		}
		//bind to the socket
		if (bind(sock, (struct sockaddr*) &commandReceiver, sizeof(commandReceiver)) < 0) {
			perror("server bind");
			exit(EXIT_FAILURE);
		}
		//listen to the socket
		if (listen(sock, 1) < 0) {
			perror("server listen");
			exit(EXIT_FAILURE);
		}

		//accept conection, create new socket

		while(1){

			int newsock = accept(sock, NULL, NULL);
			if (newsock < 0) {
				perror("server accept");
				exit(EXIT_FAILURE);
			}
			printf("Server: Accepted connection for command\n");

			char *command = inputStringFd(newsock, 10);
			if ( strcmp(command, "STATS") == 0 ){
				end = clock();
				char buf[73];
				sprintf(buf, "Server up for %f, served %d pages, %d bytes.\n", ((double) (end - start)) / CLOCKS_PER_SEC, Queue_getPages(socketQueue), Queue_getBytes(socketQueue));
				socket_write(newsock, buf, strlen(buf));
			}
			else if ( strcmp(command, "SHUTDOWN") == 0 ){
				socket_write(newsock, "Server shutting down...\n", strlen("Server shutting down...\n"));
				free(command);
				close(newsock);
				break;
			}
			else {
				socket_write(newsock, "Invalid command. Try again.\n", strlen("Invalid command. Try again.\n"));
			}
			free(command);
			close(newsock);
		}
		close(sock);

		kill(returnValue, SIGTERM);
		wait(NULL);;
		munmap(socketQueue, sizeof (Queue));
	}
}
