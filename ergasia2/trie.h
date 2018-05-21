#ifndef _TRIE_
#define _TRIE_

#include "textmap.h"
#include "postinglist.h"
#include "worker.h"

typedef struct trieNode{
	char letter;
	struct trieNode *next;
	struct trieNode *cont;
	struct plistNode *index;
}trieNode;

trieNode *newTrieNode(char c);

void freeTrie(trieNode *pTrieNode);

trieNode *createTrieFromTextMap(text *allTextsRoot, int N, char *fileName, trieNode *trieRoot, int *words);

plistNode *searchWordInTrie(trieNode *root, char *word);

countResults *createCountResults(plistNode *post);

countResults *maxFinder(countResults *cRes);

countResults *minFinder(countResults *cRes);
/*
void searchWordInTrieTF(trieNode *root, char *word, int textId);

void allWords(trieNode* node, char *word, int counter);
*/
#endif