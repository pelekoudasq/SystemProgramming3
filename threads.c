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


void *threadFun(void *arg){

	int threadId = pthread_self();
	printf("Thread number: %d\n", threadId);
	pthread_exit(NULL);
}

int socket_write(int sock, char* buf, int len) {
    do {
		int count = write(sock, buf, len);
		if (count < 0)
			return count;
    	len -= count;
    	buf += count;
    } while (len > 0);
    return 0;
}

int socket_read(int sock, char* buf, int len) {
    do {
		int count = read(sock, buf, len);
		if (count < 0)
			return count;
    	len -= count;
    	buf += count;
    } while (len > 0);
    return 0;
}

