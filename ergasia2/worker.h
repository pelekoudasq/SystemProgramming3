#ifndef _WORKER_
#define _WORKER_

typedef struct wcResults {
	char *fileName;
	int bytes;
	int words;
	int lines;
	struct wcResults *next;
}wcResults;

typedef struct countResults {
	char *fileName;
	int wordFrq;
	struct countResults *next;
}countResults;

countResults *newCountResNode(char*);

wcResults *newWcResultsNode(char*, int, int, int);

int worker(int);


#endif