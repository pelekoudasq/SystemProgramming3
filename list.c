#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "list.h"

void list_add(list **l, char *newPage){
	list *temp = malloc(sizeof(list));
	temp->next = *l;
	temp->page = newPage;
	*l = temp; 
}

char *list_rem(list **l){
	char *buf = (*l)->page;
	list *temp = *l;
	*l = (*l)->next;
	free(temp);
	return buf;
}

int list_empty(list *l){
	return l == NULL;
}

int list_check(list *l, char* newPage) {
	while (l != NULL) {
		if (strcmp(l->page, newPage) == 0)
			return 1;
		l = l->next;
	}
	return 0;
}

void printListToFile(list *l, char *save_dir, char *fileName){
	FILE *file = fopen(fileName, "w");
	while(l != NULL){
		fprintf(file, "%s/%s\n", save_dir, l->page);
		l = l->next;
	}
	fclose(file);
}
