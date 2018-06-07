#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "paths.h"

Paths *newPathNode (){
	Paths *temp = malloc( sizeof(Paths) );
	temp->content = NULL;
	temp->next = NULL;
	return temp;
}

Paths *getFileOfPaths(char* docfile, size_t size, int *N){

	FILE *file;
	Paths *P;
	file = fopen(docfile, "r");
	perror("");
	*N = 0;
	int flag = 0;
	Paths *curPath = NULL;

	if (file){
		
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
		        if( len == size ){
		            str = realloc(str, sizeof(char)*(size+=16));
		            if( !str )
		            	return NULL;
		        }
		    }
		    str[len++]='\0';
		    if (strlen(str) != 0){
		    	if ( flag == 0){
		    		flag = 1;
		    		P = newPathNode();
		    		P->content = malloc( sizeof(char)*(strlen(str)) + 1 );
	   				strcpy(P->content, str);
					curPath = P;
		    	} else {
		    		curPath->next = newPathNode();
					curPath = curPath->next;
			    	curPath->content = malloc( sizeof(char)*(strlen(str)) + 1 );
	   				strcpy(curPath->content, str);
	   			}
	   			(*N)++;
			}
   			

   			free(str);

			if (ch == EOF){
				break;
			}
		}
    	fclose(file);
	}else {
    	fprintf(stderr, "Error. An error occurred while opening the file %s.\n", docfile);
    	return NULL;
 	}
 	return P;
}

void freePaths(Paths *pPath){
	if ( pPath != NULL ){
		if ( pPath->next != NULL ){
			freePaths( pPath->next );
		}
		free(pPath->content);
	 	free(pPath);
	 }
}

void printPaths(Paths *P){
	while(P!=NULL){
		fprintf(stderr, "<<_%s_>>\n", P->content);
		P=P->next;
	}
}