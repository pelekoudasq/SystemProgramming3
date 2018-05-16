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

#include "auxFun.h"
#include "queue.h"

extern Queue socketQueue;

extern pthread_mutex_t queueLock;

struct get_request {
	char* name;
};

int recv_get(int sock, struct get_request* request) {

	//get until firts "enter"
	char* line = inputStringFd(sock, 10);
	//if there is no string or there is no get value
	if ( line == NULL ) return -1;
	int length = (int)strlen(line);
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


void getRequest(int newsock){
	//read http request
	struct get_request request;
	if (recv_get(newsock, &request) != 0) {
		perror("get");
		exit(EXIT_FAILURE);
	}
	printf(">%s<\n", request.name);

	//build answer, send it to client
	char* status;
	char* msg = NULL;

	if ( access(request.name, F_OK) == -1 ){
		msg = malloc(sizeof(char)*50);
		status = "404 Not Found";
		strcpy(msg, "<html>Sorry dude, couldn't find this file.</html>");
	} else {
		if ( access(request.name, R_OK) == -1 ){
			msg = malloc(sizeof(char)*71);
			status = "403 Forbidden";
			strcpy(msg, "<html>Trying to access this file but don't think I can make it.</html>");
		} else {
			FILE *input_file = fopen(request.name, "r+");
			if (input_file == NULL) {
				msg = malloc(sizeof(char)*50);
				status = "404 Not Found";
				strcpy(msg, "<html>Sorry dude, couldn't find this file.</html>");
			} else {
				fseek(input_file, 0, SEEK_END);
				long input_file_size = ftell(input_file);
				rewind(input_file);
				msg = malloc((input_file_size+1) * sizeof(char));
				fread(msg, sizeof(char), input_file_size, input_file);
				fclose(input_file);
				msg[input_file_size]='\0';
				status = "200 OK";
			}
		}
	}
	
	time_t     now;
	struct tm  ts;
	char       timeBuffer[80];
	time(&now);
	ts = *localtime(&now);
	strftime(timeBuffer, sizeof(timeBuffer), "%a, %d %b %Y %H:%M:%S %Z", &ts);
	printf("%s\n", timeBuffer);

	
	char* buf = malloc(sizeof(char)*10000);

	sprintf(buf, "HTTP/1.1 %s\r\nDate: %s\r\nServer: myhttpd/1.0.0 (Ubuntu64)\r\nContent-Length: %d\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n", status, timeBuffer, (int)strlen(msg));
	
	if (socket_write(newsock, buf, strlen(buf)) < 0) {
		perror("header");
		exit(EXIT_FAILURE);
	}
	if (socket_write(newsock, msg, strlen(msg)) < 0) {
		perror("body");
		exit(EXIT_FAILURE);
	}
	sleep(0.1);
	//printf("Sending answer:\n%s%s\n", buf, msg);

	free(buf);
	free(msg);
	//free(status);
	free(request.name);
	close(newsock);
}


void *serverThread(void *arg){

	//int threadId = pthread_self();
	while(1){
		if (pthread_mutex_lock(&queueLock) < 0) { 
			/* Lock mutex */
			perror("lock");
			exit(EXIT_FAILURE);
		}
		
		int newsock;
		if (Queue_pop(&socketQueue, &newsock) == 0){
			
			//printf("Empty Queue\n");
			if (pthread_mutex_unlock(&queueLock) < 0) {
				perror("lock");
				exit(EXIT_FAILURE);
			}

		} else {
			printf("Poping: %d\n", newsock);
			if (pthread_mutex_unlock(&queueLock) < 0) {
				perror("lock");
				exit(EXIT_FAILURE);
			}
			getRequest(newsock);
		}
	}

	//printf("Thread number: %d\n", threadId);
	pthread_exit(NULL);
}

