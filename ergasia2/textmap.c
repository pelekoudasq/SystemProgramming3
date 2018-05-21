#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/errno.h>

#include "textmap.h"




text *newTextNode (){
	text *temp = malloc( sizeof(text) );
	temp->numberOfWords = 0;
	temp->content = NULL;
	temp->next = NULL;
	return temp;
}

text *addFileToMaps(char *fileName, int *N, int* bytes){

	text *T = NULL;

	FILE *file;
	int check = -1;

	file = fopen(fileName, "r");
	text *curText;
	int size = 16;

	if (file){
		T = newTextNode();
		curText = T;
		while(1){

		 	char *str;
		    int ch;
		    size_t len = 0;

		    str = realloc(NULL, sizeof(char)*size);//size is start size

		    //if realloc fails
		    if( !str )
		    	return NULL;

		    while( EOF != ( ch = fgetc(file) ) && ch != '\n' ){

		        str[len++] = ch;
		        (*bytes)++;
		        if( len == size ){
		            str = realloc(str, sizeof(char)*(size+=16));
		            if( !str )
		            	return NULL;
		        }
		    }
		    str[len++]='\0';

		    //str = realloc(str, sizeof(char)*len);
			//printf("%s\n", str);
			
			char *pos = strpbrk(str, " \t") /*strchr(str,' ')*/;
			int indexx = pos - str;

			char table[indexx+1];
			for ( int i = 0; i < indexx; i++){
				table[i] = str[i];
			}
			int at = atoi(table);
			if ( (at - 1) != check ){
   					printf("Error. Lines of file not correctly numbered.\n");
   					return NULL;
   			}
   			check++;

   			str = str + indexx + 1;
   			//printf("str: %s\n", str);
   			
   			
   			curText->content = malloc( sizeof(char)*(strlen(str)) + 1 );
   			strcpy(curText->content, str);
   			

   			free(str-indexx-1);

			if (ch == EOF){
				break;
			}
			
			curText->next = newTextNode();
			curText = curText->next;
		}
    	fclose(file);
	}else {
		perror("ERROR:");
    	printf("Error. An error occurred while opening the file %s.\n", fileName);
    	return NULL;
 	}

 	//N = check;
 	(*N) = ++check;	//number of lines
 	printf("Number of lines: %d.\n", *N);

 	return T;
}

void freeTexts(text *pText){
	if ( pText->next != NULL ){
		freeTexts( pText->next );
	}
	free(pText->content);
 	free(pText);
}