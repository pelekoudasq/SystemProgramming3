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
	//printf("Thread number: %d\n", threadId);
	pthread_exit(NULL);
}

