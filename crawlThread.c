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
extern pthread_mutex_t listToAddLock;
extern pthread_mutex_t statsLock;
extern pthread_cond_t cond_listnonempty;
extern int pagesDownloaded;
extern int bytesDownloaded;

extern int port;
extern char *host;
extern char *save_dir;

extern int workers;

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
	free(name);

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
			//printf("%s %d\n", link, len);
			if (pthread_mutex_lock(&listToAddLock) < 0){
				perror("lock");
				exit(EXIT_FAILURE);
			}
			if (list_check(pagesAdded, link) || list_check(pagesToAdd, link))
				free(link);
			else{
				list_add(&pagesToAdd, link);
				pthread_cond_signal(&cond_listnonempty);
			}
			if (pthread_mutex_unlock(&listToAddLock) < 0){
				perror("unlock");
				exit(EXIT_FAILURE);
			}
		}
		content = tok+1;
		tok = strchr(content, '\n');
	}
}

int readAnswerFromServer(int sock, char *fileName){
	printf("Crawler: Getting answer from server...\n");
	char *line = inputStringFd(sock, 15);
	if ( line == NULL ) return -1;
	int length = strlen(line);
	if ( length < strlen("HTTP/1.1 xxx xx") ) {
		printf("Crawler: Error with length\n");
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
			printf("Crawler: Error getting page %d\n", contentSize);
			return -1;
		}
		content[contentSize] = '\0';
		if (code == 200){
			if (pthread_mutex_lock(&statsLock) < 0){
				perror("lock");
				exit(EXIT_FAILURE);
			}
			pagesDownloaded++;
			bytesDownloaded+=contentSize;
			if (pthread_mutex_unlock(&statsLock) < 0){
				perror("unlock");
				exit(EXIT_FAILURE);
			}
			contentProcessing(content, fileName);
		}
		else 
			printf("%s\n", content);
		free(content);
		free(lengthLine-16);
	} else {
		printf("Crawler: wrong code\n");
		return -1;
	}
	return 0;
}

void get(char *page) {
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
		perror("gethostbyname");
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
	//printf("Connecting to %s port %d\n", host, port);

	char msg[10000];
	sprintf(msg, "GET /%s HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nHost: www.tutorialspoint.com\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Keep-Alive\r\n", page);

	if (socket_write(sock, msg, strlen(msg)+1) < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
	if (readAnswerFromServer(sock, page) < 0){
		printf("Crawler: Error reading answer\n");
		exit(EXIT_FAILURE);
	}
	close(sock);
}

void *threadFun(void *arg){
	
	while (1){
		//pagesToAdd list
		if (pthread_mutex_lock(&listToAddLock) < 0){
			perror("lock");
			exit(EXIT_FAILURE);
		}
		while (list_empty(pagesToAdd)) {
			//printf("Crawler: Found Buffer Empty \n");
			pthread_cond_wait(&cond_listnonempty, &listToAddLock);
		}
		char *page=list_rem(&pagesToAdd);
		list_add(&pagesAdded, page);
		workers++;
		printf("Crawler: Getting page: %s\n", page);
		if (pthread_mutex_unlock(&listToAddLock) < 0){
			perror("unlock");
			exit(EXIT_FAILURE);
		}
		get(page);
		if (pthread_mutex_lock(&listToAddLock) < 0){
			perror("lock");
			exit(EXIT_FAILURE);
		}
		workers--;
		if (pthread_mutex_unlock(&listToAddLock) < 0){
			perror("unlock");
			exit(EXIT_FAILURE);
		}
	}
	pthread_exit(NULL);
}
