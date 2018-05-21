#ifndef _TEXTMAP_
#define _TEXTMAP_


typedef struct arrayOfTexts{
	int numberOfWords;
	char *content;
	struct arrayOfTexts *next;
}text;

text *addFileToMaps(char *, int*, int*);

text *newTextNode ();

void freeTexts(text *);


#endif