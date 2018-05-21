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

extern Queue *socketQueue;
extern char* root_dir;

extern pthread_mutex_t queueLock;
extern pthread_cond_t cond_nonempty;
extern pthread_cond_t cond_nonfull;

void cleanUpFun(){
	printf("CLEANUP\n");
}

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
	request->name = malloc(sizeof(char)*(strlen(root_dir)+strlen(name)+1));
	strcpy(request->name, root_dir);
	strcat(request->name, name);
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
				if (pthread_mutex_lock(&queueLock) < 0) { 
					perror("lock add stats");
					exit(EXIT_FAILURE);
				}
				Queue_serve(socketQueue);
				Queue_bytes(socketQueue, input_file_size);
				if (pthread_mutex_unlock(&queueLock) < 0) {
					perror("unlock add stats");
					exit(EXIT_FAILURE);
				}
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
	sleep(1);
	//printf("Sending answer:\n%s%s\n", buf, msg);

	free(buf);
	free(msg);
	free(request.name);
	close(newsock);
}


void *serverThread(void *arg){

	//pthread_cleanup_push(&cleanUpFun, NULL);
	//int threadId = pthread_self();
	while(1){
		
		if (pthread_mutex_lock(&queueLock) < 0) { 
			perror("lock");
			exit(EXIT_FAILURE);
		}

		while (Queue_empty(socketQueue) == 1){
			printf(">> Found Buffer Empty \n");
			pthread_cond_wait(&cond_nonempty, &queueLock);
		}
		
		int newsock;
		Queue_pop(socketQueue, &newsock);
		printf("Poping: %d\n", newsock);

		if (pthread_mutex_unlock(&queueLock) < 0){
			perror("lock");
			exit(EXIT_FAILURE);
		}
		pthread_cond_signal(&cond_nonfull);
		getRequest(newsock);
	}
	pthread_exit(NULL);
}
