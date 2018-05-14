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

struct get_request {
	char* name;
};

int recv_get(int sock, struct get_request* request) {

	//get until firts "enter"
	char* line = inputStringFd(sock, 10);
	//if there is no string or there is no get value
	if ( line == NULL ) return -1;
	int length = strlen(line);
	if ( length <= strlen("GET  HTTP/1.1") ) {
		return -1;
	}
	//get from line the url name
	line[length - 9] = '\0';
	char* name = line+4;

	//assign the url to the name field of request
	request->name = malloc(sizeof(char)*(strlen(name)+1));
	strcpy(request->name, name);
	free(line);

	//consume the rest of the lines
	//potentially we add to the request structure all the other values of the given request
	for (int i = 0; i < 5; ++i) {
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
		//printf("Creating thread\n");
		pthread_create(&(threads[i]), NULL, threadFun, NULL);
	}


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

	//accept conection, create new socket
    struct sockaddr_in client;
    socklen_t clientlen;
	int newsock = accept(sock, (struct sockaddr*) &client, &clientlen);
	if (newsock < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	printf("Accepted connection\n");

	//read http request
	struct get_request request;
	if (recv_get(newsock, &request) != 0) {
		perror("get");
		exit(EXIT_FAILURE);
	}
	printf(">%s<\n", request.name);

	//build answer, send it to client
	char* status;
	char* msg;

	if ( access(request.name, F_OK) == -1 ){
		status = "404 Not Found";
		msg = "<html>Sorry dude, couldn’t find this file.</html>";
	} else {
		if ( access(request.name, R_OK) == -1 ){
			status = "403 Forbidden";
			msg = "<html>Trying to access this file but don’t think I can make it.</html>";
		} else {
			status = "200 OK";
			long input_file_size;
			FILE *input_file = fopen(request.name, "rb");
			fseek(input_file, 0, SEEK_END);
			input_file_size = ftell(input_file);
			rewind(input_file);
			msg = malloc(input_file_size * (sizeof(char)));
			fread(msg, sizeof(char), input_file_size, input_file);
			fclose(input_file);
		}
	}
	
	time_t     now;
	struct tm  ts;
	char       timeBuffer[80];
	time(&now);
	ts = *localtime(&now);
	strftime(timeBuffer, sizeof(timeBuffer), "%a, %d %b %Y %H:%M:%S %Z", &ts);
	printf("%s\n", timeBuffer);

	
	char* buf = malloc(10000);
	sprintf(buf, "HTTP/1.1 %s\nDate: %s\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\n", status, timeBuffer, (int)strlen(msg));
	if (socket_write(newsock, buf, strlen(buf)) < 0) {
		perror("header");
		exit(EXIT_FAILURE);
	}
	if (socket_write(newsock, msg, strlen(msg)) < 0) {
		perror("body");
		exit(EXIT_FAILURE);
	}
	printf("Sending answer:\n%s\n", buf);

	free(buf);
	free(request.name);

	close(newsock);
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