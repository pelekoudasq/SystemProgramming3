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

int validate (char *a){

	for (unsigned int i = 0; i < strlen(a); i++){
		if(!isdigit(a[i])){
			return -1;
		}
	}
	return 0;
}

//read string until first change of line 
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

int consumeLines(int sock, int lines){

    for (int i = 0; i < lines; ++i) {
        char *line = inputStringFd(sock, 10);
        if (line!=NULL)
            free(line);
    }
    return 0;
}

int readAnswerFromServer(int sock){

    char *line = inputStringFd(sock, 15);
    if ( line == NULL ) return -1;
    int length = strlen(line);
    if ( length <= strlen("HTTP/1.1 xxx xx") ) {
        return -1;
    }
    //get from line the url name
    line[12] = '\0';
    char* codeLetters = line+9;
    int code = atoi(codeLetters);
    free(line);
    if (code == 404 || code == 403 || code == 200) {
        printf("got 404 or 403 or 200\n");
        consumeLines(sock, 2);
        char *lengthLine = inputStringFd(sock, 10);
        lengthLine = lengthLine+16;
        int contentSize = atoi(lengthLine);
        printf("contentSize: %d\n", contentSize);
        consumeLines(sock, 3);
        char *content = malloc(sizeof(char)*contentSize);
        if (socket_read(sock, content, contentSize) < 0){
            printf("Error getting page\n");
            return -1;
        }
        if (code == 404 || code == 403)
            printf("%s\n", content);
    } else {
        printf("wrong code\n");
        return -1;
    }
    return 0;
}
