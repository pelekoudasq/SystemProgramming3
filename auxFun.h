#ifndef _AUXILIARYFUNCTIONS_
#define _AUXILIARYFUNCTIONS_


int validate (char *a);

char *inputStringFd(int fd, size_t size);

char *inputString(FILE* fp, size_t size);

int socket_write(int sock, char* buf, int len);

int socket_read(int sock, char* buf, int len);

int consumeLines(int sock, int lines);

int readAnswerFromServer(int sock);

#endif