#ifndef _POSTLIST_
#define _POSTLIST_

typedef struct plistNode{
	int textId;
	int wordFrq;
	char *file;
	struct plistNode *next;
}plistNode;

plistNode *newPlistNode(int textId, char* fileName);

void freePlist(plistNode *pPlistNode);

#endif