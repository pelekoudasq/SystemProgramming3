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

struct get_request {
	char* name;
};

int validate (char *a){

	for (unsigned int i = 0; i < strlen(a); i++){
		if(!isdigit(a[i])){
			return -1;
		}
	}
	return 0;
}

char *inputStringFd(int fd, size_t size){
	
	//The size is extended by the input with the value of the provisional
    char *str;
    size_t len = 0;

    str = realloc(NULL, sizeof(char)*size);//size is start size

    //if realloc fails
    if( !str )
    	return str;

    char ch;
    do {
		while (read(fd, &ch, 1) != 1);
        str[len++] = ch;
        if( len == size ){
            str = realloc(str, sizeof(char)*(size+=16));
            if( !str )
            	return str;
        }
    } while (ch != '\n');
    str[--len]='\0';
    printf(">%s<\n", str);
    return realloc(str, sizeof(char)*len);
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

int recv_get(int sock, struct get_request* request) {
	char* line = inputStringFd(sock, 10);
	if (line == NULL) return -1;
	int length = strlen(line);
	if (length <= strlen("GET  HTTP/1.1")) {
		return -1;
	}
	line[length - 9] = '\0';
	char* name = line+4;
	request->name = malloc(sizeof(char)*(strlen(name)+1));
	strcpy(request->name, name);
	free(line);

	for (int i = 0; i < 5; ++i)
	{
		line = inputStringFd(sock, 10);
		free(line);
	}
	return 0;
}

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

	//create threads
	pthread_t threads[num_of_threads];

	int i;
	for (i = 0; i < num_of_threads; ++i){
		//The second argument specifies attributes. The fourth argument is used to pass arguments to thread.
		printf("Creating thread\n");
		pthread_create(&(threads[i]), NULL, threadFun, NULL);
	}


	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(serving_port);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	if (bind(sock, (struct sockaddr*) &server, sizeof(server)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	if (listen(sock, num_of_threads) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

    struct sockaddr_in client;
    socklen_t clientlen;
	int newsock = accept(sock, (struct sockaddr*) &client, &clientlen);
	if (newsock < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	printf("Accepted connection\n");

	struct get_request request;
	if (recv_get(newsock, &request) != 0) {
		perror("get");
		exit(EXIT_FAILURE);
	}
	printf(">%s<\n", request.name);

	char* status = "404 Not Found";
	char* msg = "<html>Sorry dude, couldn’t find this file.</html>";
	char* buf = malloc(10000);
	sprintf(buf, "HTTP/1.1 %s\nDate: %s\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\n", status, date, strlen(msg));
	if (socket_write(newsock, buf, strlen(buf)) < 0) {
		perror("header");
		exit(EXIT_FAILURE);
	}
	if (socket_write(newsock, msg, strlen(msg)) < 0) {
		perror("body");
		exit(EXIT_FAILURE);
	}

	free(buf);
	free(request.name);

	close(newsock);

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