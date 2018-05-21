#ifndef _PATHS_
#define _PATHS_


typedef struct Paths{
	char *content;
	struct Paths *next;
}Paths;

Paths *newPathNode ();

Paths *getFileOfPaths(char*, size_t, int *);

void freePaths(Paths *);

void printPaths(Paths *P);


#endif