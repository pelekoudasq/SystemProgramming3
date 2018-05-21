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
pthread_mutex_t listToAddLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_listnonempty = PTHREAD_COND_INITIALIZER;
int pagesDownloaded;
int bytesDownloaded;

int port;
char *host;
char *save_dir;

int workers = 0;

int main(int argc, char **argv){

	clock_t start, end;
	start = clock();

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

	//command port
	struct sockaddr_in commandReceiver;
	commandReceiver.sin_family = AF_INET;
	commandReceiver.sin_addr.s_addr = htonl(INADDR_ANY);
	commandReceiver.sin_port = htons(command_port);

	//create socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	//bind to the socket
	if (bind(sock, (struct sockaddr*) &commandReceiver, sizeof(commandReceiver)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	//listen to the socket
	if (listen(sock, 1) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//accept conection, create new socket
	struct sockaddr_in client;
	socklen_t clientlen = sizeof(client);

	int ready = 0 ;

	while(1){

		int newsock = accept(sock, (struct sockaddr*) &client, &clientlen);
		if (newsock < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		printf("Accepted connection\n");

		char *command = inputStringFd(newsock, 10);
		if ( strcmp(command, "STATS") == 0 ){
			end = clock();
			char buf[73];
			sprintf(buf, "Crawler up for %f, downloaded %d pages, %d bytes.\n", ((double) (end - start)) / CLOCKS_PER_SEC, pagesDownloaded, bytesDownloaded);
			socket_write(newsock, buf, strlen(buf));
		}
		else if ( strncmp(command, "SEARCH ", 7) == 0 ){
			printf("command: SEARCH\n");
			
			if( ready == 0 ){
				if (pthread_mutex_lock(&listToAddLock) < 0){
					perror("lock");
					exit(EXIT_FAILURE);
				}
				if (workers == 0 && list_empty(pagesToAdd)){
					ready++;
					//create file
					//printListToFile(pagesAdded, save_dir, "pathsfile");
					int pid = fork();
					if ( pid == 0 ){
						execl("bashls", "bashls", save_dir, NULL);
					}
					wait(NULL);
					int returnValue = fork();
					if ( returnValue == 0 ){
						execl("ergasia2/jobExecutor", "jobExecutor", "-d", "pathsfile", "-w", "5", NULL);
					}
				}
				if (pthread_mutex_unlock(&listToAddLock) < 0){
					perror("unlock");
					exit(EXIT_FAILURE);
				}
			} 
			if (ready){
				//search(command+7);
			} else {
				socket_write(newsock, "Crawling in progress", strlen("Crawling in progress"));
			}
			
		}
		else if ( strcmp(command, "SHUTDOWN") == 0 ){
			printf("command: SHUTDOWN\n");
			free(command);
			close(newsock);
			break;
		}
		else {
			printf("command: wrong command\n");
		}
		free(command);
		close(newsock);
	}
	close(sock);

	//code
	//join threads
	for (int i = 0; i < num_of_threads; ++i){
		if (pthread_cancel(threads[i]) != 0){
			perror("cancel");
		}
		printf("cancel thread\n");
	}

	while (!list_empty(pagesToAdd))
		free(list_rem(&pagesToAdd));

	while (!list_empty(pagesAdded))
		free(list_rem(&pagesAdded));
}
