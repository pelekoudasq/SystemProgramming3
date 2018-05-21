#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "postinglist.h"
#include "trie.h"




trieNode *newTrieNode(char c){
	trieNode *temp = malloc( sizeof(trieNode) );
	temp->letter = c;
	temp->cont = NULL;
	temp->next = NULL;
	temp->index = NULL;
	return temp;
}


void freeTrie(trieNode *pTrieNode){
	if ( pTrieNode->index != NULL ){
		freePlist( pTrieNode->index );
	}
	if ( pTrieNode->cont !=NULL ){
		freeTrie( pTrieNode->cont );
	}
	if ( pTrieNode->next !=NULL ){
		freeTrie( pTrieNode->next );
	}
	free(pTrieNode);
}


trieNode *createTrieFromTextMap(text *allTextsRoot, int N, char *fileName, trieNode *trieRoot, int* words){
	
	text *curText = allTextsRoot;
 	int textLength, counter = 0;
 	int totalWords = 0;
 	int i, j;

 	trieNode *curTrieNode;
 	int resultFlag;
 	int newContFlag;
 	plistNode *curPlistNode;
	resultFlag = 0;

 	printf("Loading words into Trie... ");
 	//for every text
 	for(i = 0; i < (N-0); i++){
 		//for every text
 		//printf("%d\n", i);
 		textLength = strlen(curText->content);
 		for(j = 0; j < textLength; j++){
 			//if we have spaces etc
 			if ( curText->content[j] == ' ' || curText->content[j] == '\t' ){
 				//if we just finished parsing a word
 				if ( resultFlag == 1 ){
 					//add one to the number words in text
 					curText->numberOfWords++;
 					//process after parsing one word
 					//if there is no inverted index at the end of the word in Trie structure
 					if ( curTrieNode->index == NULL ){
 						curTrieNode->index = newPlistNode(i, fileName);
 					}
 					//if there is already an inverted index node attached to it
 					else{
 						//point to the first index
 						curPlistNode = curTrieNode->index;
 						while(1){
 							//if the node is about the current text
 							if ( curPlistNode->textId == i && (!strcmp(curPlistNode->file,fileName))){
 								//then increase by one the word frequency and break
 								curPlistNode->wordFrq++;
 								break;
 							}
 							//if we are in the last node -and we didn't find same textId
 							if ( curPlistNode->next == NULL ){
 								//create new inverted index node
	 							curPlistNode->next = newPlistNode(i, fileName);
 								break;
 							}
 							curPlistNode = curPlistNode->next;
 						}
 					}
 					resultFlag = 0;
 					newContFlag = 0;
 					curTrieNode = trieRoot;
 					(*words)++;
 				}
 			}
 			//if we are in the first word of the process (empty Trie)
 			else if ( counter == 0 ){
 				if ( trieRoot == NULL ){
 					printf("creating new Trie... ");
 					trieRoot = newTrieNode( curText->content[j] );
 				} else {
 					printf("adding to already existing Trie... ");
 				}
 				counter++;
 				curTrieNode = trieRoot;
 				newContFlag = 1;
 				resultFlag = 1;
 			}
 			//if we have a word character
 			else {
 				resultFlag = 1;
 				//if there was a cont, i change it here 
 				//thats because if I find cont but is the end of the word i still need the pointer as it was
 				if ( newContFlag == 3 ){
 					curTrieNode = curTrieNode->cont;
 				}
 				//if we have more characters in a word and there is no cont node
 				if ( newContFlag == 1 ){
 					//create new node cont 
	 				curTrieNode->cont = newTrieNode(curText->content[j]);
	 				curTrieNode = curTrieNode->cont;
 				}
 				//if we find the letter
 				else if ( curTrieNode->letter == curText->content[j]){
 					//but there is no continue node
 					if( curTrieNode->cont == NULL ){
 						//keep pointer as it is, in order to create new trie node at next letter
 						//if there is no new letter then a inverted index is going to be created/expanded at the end of it
 						newContFlag = 1;
 					}
 					else {
 						//cont found, change the pointer if its not the end of the word
 						newContFlag = 3;
 					}
 				} 
 				else {
 					//not found
 					while(1){
 						if ( curTrieNode->next == NULL ){
 							//if there is no next, create one
	 						curTrieNode->next = newTrieNode(curText->content[j]);
	 						curTrieNode = curTrieNode->next;
	 						newContFlag = 1;
	 						break;
 						}
 						else {
 							//examing next
 							curTrieNode = curTrieNode->next;
 							//if we found it
 							if ( curTrieNode->letter == curText->content[j] ){
 								if( curTrieNode->cont == NULL ){
 									//if there is no cont node
			 						//keep pointer as it is, in order to create new trie node at next letter
			 						newContFlag = 1;
			 					}
			 					else {
			 						//cont node found, change the pointer if its not the end of the word
			 						newContFlag = 3;
			 					}
 								break;
 							}
 						}
 					}
 				}
 				counter++;
 			}
 			if ( (j + 1) == textLength ){
 				if ( resultFlag == 1 ){
 					//add one to the number words in text
 					curText->numberOfWords++;
 					//process after parsing one word
 					//if there is no inverted index at the end of the word in Trie structure
 					if ( curTrieNode->index == NULL ){
 						curTrieNode->index = newPlistNode(i, fileName);
 					}
 					//if there is already an inverted index node attached to it
 					else{
 						//point to the first index
 						curPlistNode = curTrieNode->index;
 						while(1){
 							//if the node is about the current text
 							if ( curPlistNode->textId == i && (!strcmp(curPlistNode->file,fileName))){
 								//then increase by one the word frequency and break
 								curPlistNode->wordFrq++;
 								break;
 							}
 							//if we are in the last node -and we didn't find same textId
 							if ( curPlistNode->next == NULL ){
 								//create new inverted index node
	 							curPlistNode->next = newPlistNode(i, fileName);
 								break;
 							}
 							curPlistNode = curPlistNode->next;
 						}
 					}
 					resultFlag = 0;
 					newContFlag = 0;
 					curTrieNode = trieRoot;
 					(*words)++;
 				}
 			}
 		}
 		totalWords += curText->numberOfWords;
 		curText = curText->next;
 	}
 	printf("Success.\n");
 	return trieRoot;
}



plistNode *searchWordInTrie(trieNode *root, char *word){


	trieNode *curTrieNode = root;
	plistNode *curPlistNode;

	for(int i = 0; i < strlen(word); i++){
 		while (1){
 			if (curTrieNode == NULL){
 				//end proccess
 				//printf("Word not found.\n");
 				return NULL;
 			}
 			//if we find the letter
	 		if (curTrieNode->letter == word[i]){
	 			//if it is the last letter
	 			if ( (i + 1) == strlen(word) ){
	 				//get index of the word formed thus far
	 				curPlistNode = curTrieNode->index;
	 				return curPlistNode;
	 			}
	 			curTrieNode = curTrieNode->cont;
	 			break;
	 		}
	 		//if we didn't find the letter 
	 		else {
 				//if there is no next node in Trie
  				if (curTrieNode->next == NULL){
  					//end proccess
 					//printf("Word not found.\n");
 					return NULL;
 				}
 				//else check next node
 				else {
 					curTrieNode = curTrieNode->next;
 				}
		 	}
 		}
	}
	return NULL;
}

countResults *createCountResults(plistNode *post){

	countResults *cRes = NULL;
	int flag = 0;
	countResults *curRes = NULL;

	while(post != NULL) {
		if ( flag == 0 ){
			cRes = newCountResNode(post->file);
			flag++;
		} else {
			while (curRes != NULL){
				if ( strcmp(curRes->fileName, post->file) == 0){
					curRes->wordFrq++;
					break;
				} 
				if ( curRes->next == NULL ){
					curRes->next = newCountResNode(post->file);
					break;
				}
				curRes = curRes->next;
			}
		}
		curRes = cRes;
		post = post->next;
	}
	return cRes;
}

countResults *maxFinder(countResults *cRes){

	int flag = 0;
	countResults *largest, *current;
	largest = cRes;
	current = cRes;
	if ( cRes == NULL ){
		return cRes;
	}
	while ( current->next != NULL ){
		if ( largest->wordFrq <= current->next->wordFrq ){
			largest = current->next;
			flag = 1;
		}
		current = current->next;
	}

	if ( flag == 1 ){

		int wordFrq = cRes->wordFrq;
		char *fileName = cRes->fileName;

		cRes->wordFrq = largest->wordFrq;
		cRes->fileName = largest->fileName;

		largest->wordFrq = wordFrq;
		largest->fileName = fileName;
	}
	return cRes;
}

countResults *minFinder(countResults *cRes){

	int flag = 0;
	countResults *smallest, *current;
	smallest = cRes;
	current = cRes;
	if ( cRes == NULL ){
		return cRes;
	}
	while ( current->next != NULL ){
		if ( smallest->wordFrq > current->next->wordFrq ){
			smallest = current->next;
			flag = 1;
		}
		current = current->next;
	}

	if ( flag == 1 ){
		int wordFrq = cRes->wordFrq;
		char *fileName = cRes->fileName;

		cRes->wordFrq = smallest->wordFrq;
		cRes->fileName = smallest->fileName;

		smallest->wordFrq = wordFrq;
		smallest->fileName = fileName;
	}
	return cRes;
}

/*
void searchWordInTrieTF(trieNode *root, char *word, int textId){


	trieNode *curTrieNode = root;
	plistNode *curPlistNode;

	for(int i = 0; i < strlen(word); i++){
 		//
 		int counter = 0;
 		while (1){
 			if (curTrieNode == NULL){
 				//end proccess
 				printf("Word not found.\n");
 				return;
 			}
 			//if we find the letter
	 		if (curTrieNode->letter == word[i]){
	 			//if it is the last letter
	 			if ( (i + 1) == strlen(word) ){
	 				//get index of the word formed thus far
	 				curPlistNode = curTrieNode->index;
	 				while(1){
	 					//if there is no node, end proccess
	 					if ( curPlistNode == NULL ){
	 						printf("No match.\n");
	 						break;
	 					}
	 					//else print 
	 					else {
	 						if ( curPlistNode->textId == textId ){
	 							printf("%d %s %d\n", textId, word, curPlistNode->wordFrq);
	 							return;	
	 						}
	 						curPlistNode = curPlistNode->next;
	 						counter++;
	 					}
	 				}
	 				
	 			}
	 			curTrieNode = curTrieNode->cont;
	 			break;
	 		}
	 		//if we didn't find the letter 
	 		else {
 				//if there is no next node in Trie
  				if (curTrieNode->next == NULL){
  					//end proccess
 					printf("Word not found.\n");
 					return;
 				}
 				//else check next node
 				else {
 					curTrieNode = curTrieNode->next;
 				}
		 	}
 		}
	}
}



void allWords(trieNode* node, char *word, int counter){

	int i = 1;
	//printf("%d\n", counter);
	if ( node->index != NULL ){
		strncat(word, &(node->letter), sizeof(char));
		printf("%s ",word);
		//memcpy(word, &a, sizeof(char));
		plistNode *pIndex = node->index;
		while ( pIndex->next != NULL ){
			i++;
			pIndex = pIndex->next;
		}
		printf("%d\n", i);
	}


	if ( node->cont !=NULL ){
		if ( node->index == NULL ){
			strncat(word, &(node->letter), sizeof(char));
		}
		allWords(node->cont, word, counter+1);
		char a = '\0';
		memcpy(word+counter, &a, sizeof(char));
	}

	if ( node->next !=NULL ){
		char a = '\0';
		memcpy(word+counter, &a, sizeof(char));
		allWords(node->next, word, counter);
	}

}
*/

