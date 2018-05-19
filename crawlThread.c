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

extern list *pagesToAdd;
extern list *pagesAdded;

extern int port;
extern char *host;
extern char *save_dir;

void mkdirs(char *dir) {
	char *p = NULL;
	size_t len = strlen(dir);

	if (dir[len - 1] == '/')
		dir[len - 1] = '\0';
	for (p = dir + 1; *p; p++)
		if (*p == '/') {
			*p = '\0';
			mkdir(dir, 755);
			*p = '/';
		}
}

void contentProcessing(char *content, char *fileName){
	char *name = malloc(sizeof(char)*(strlen(save_dir)+strlen(fileName)+1));
	strcpy(name, save_dir);
	strcat(name, fileName);
	mkdirs(name);
	FILE *file = fopen(name, "w");
	fprintf(file, "%s", content);
	fclose(file);

	char *tok = strchr(content, '\n');
	while (tok != NULL) {
		*tok = '\0';
		if (strncmp("<a href=\"", content, 9) == 0) {
			//extract link
			char *start = content+9;
			char *end = strchr(start, '"');
			*end = '\0';
			int len = strlen(start);
			char *link = malloc(sizeof(char)*(len+1));
			strncpy(link, start, len);
			link[len] = '\0';
			printf("%s %d\n", link, len);
			if (list_check(pagesAdded, link))
				free(link);
			else
				list_add(&pagesToAdd, link);
		}
		content = tok+1;
		tok = strchr(content, '\n');
	}
}

int readAnswerFromServer(int sock, char *fileName){

	char *line = inputStringFd(sock, 15);
	if ( line == NULL ) return -1;
	int length = strlen(line);
	if ( length < strlen("HTTP/1.1 xxx xx") ) {
		printf("error with length\n");
		return -1;
	}
	//get from line the url name
	line[12] = '\0';
	char* codeLetters = line+9;
	int code = atoi(codeLetters);
	free(line);
	if (code == 404 || code == 403 || code == 200) {
		//printf("got %d\n", code);
		consumeLines(sock, 2);
		char *lengthLine = inputStringFd(sock, 10);
		lengthLine = lengthLine+16;
		int contentSize = atoi(lengthLine);
		//printf("contentSize: %d\n", contentSize);
		consumeLines(sock, 3);
		char *content = malloc(sizeof(char)*(contentSize+1));
		if (socket_read(sock, content, contentSize) < 0){
			printf("Error getting page\n");
			return -1;
		}
		content[contentSize] = '\0';
		if (code == 200)
			contentProcessing(content, fileName);
		else {
			printf("%s\n", content);
		}
		free(content);
		free(lengthLine-16);
	} else {
		printf("wrong code\n");
		return -1;
	}
	return 0;
}

void get(char *page) {
	puts(page);
	/* Initiate connection */
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

	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}
	printf("Connecting to %s port %d\n", host, port);

	char msg[10000];
	sprintf(msg, "GET /%s HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nHost: www.tutorialspoint.com\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Keep-Alive\r\n", page);

	if (socket_write(sock, msg, strlen(msg)+1) < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
	if (readAnswerFromServer(sock, page) < 0){
		printf("Error reading answer\n");
		exit(EXIT_FAILURE);
	}
	close(sock);
}

void *threadFun(void *arg){
	while (!list_empty(pagesToAdd)) {
		char *page=list_rem(&pagesToAdd);
		list_add(&pagesAdded, page);
		get(page);
	}
	pthread_exit(NULL);
}
