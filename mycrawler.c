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

#include "threads.h"


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

	char* host = argv[2];
	int port = atoi(argv[4]);
	int command_port = atoi(argv[6]);
	int num_of_threads = atoi(argv[8]);
	char* save_dir = argv[10];
	char* starting_URL = argv[11];

	//create threads
	pthread_t threads[num_of_threads];

	int i;
	for (i = 0; i < num_of_threads; ++i){
		//The second argument specifies attributes. The fourth argument is used to pass arguments to thread.
		//printf("Creating thread\n");
		pthread_create(&(threads[i]), NULL, threadFun, NULL);
	}


	/* Create socket */
	int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
    }
	/* Find server address */
	struct hostent *rem = gethostbyname(host);
	if (rem == NULL) {	
		herror("gethostbyname");
		exit(EXIT_FAILURE);
	}

    struct sockaddr_in server;
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);         /* Server port */
    /* Initiate connection */
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
    }
    printf("Connecting to %s port %d\n", argv[1], port);

    //crawl
    char msg[10000] = "GET /site0/page0_1244.html HTTP/1.1\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\nHost: www.tutorialspoint.com\nAccept-Language: en-us\nAccept-Encoding: gzip, deflate\nConnection: Keep-Alive\n";
	if (socket_write(sock, msg, strlen(msg)+1) < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
	//if (socket_read(sock,))


	close(sock);
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
}