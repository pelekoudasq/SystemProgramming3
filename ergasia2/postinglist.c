#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "postinglist.h"


plistNode *newPlistNode(int textId, char* fileName){
	plistNode *temp = malloc( sizeof(plistNode) );
	temp->file = fileName;
	temp->textId = textId;
	temp->wordFrq = 1;
	temp->next = NULL;
	return temp;
}


void freePlist(plistNode *pPlistNode){
	if ( pPlistNode->next != NULL ){
		freePlist( pPlistNode->next );
	}
	free(pPlistNode);
}