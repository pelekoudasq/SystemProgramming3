#ifndef _WEBTHREAD_
#define _WEBTHREAD_

int validate (char *a);

char *inputStringFd(int fd, size_t size);

char *inputString(FILE* fp, size_t size);

int socket_write(int sock, char* buf, int len);

int socket_read(int sock, char* buf, int len);

void *threadFun(void *);


#endif