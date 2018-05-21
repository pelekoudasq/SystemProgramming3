#ifndef _LIST_
#define _LIST_

typedef struct list{
	char *page;
	struct list *next;
}list;

void list_add(list **l, char *newPage);
char *list_rem(list **l);
int list_empty(list *l);
int list_check(list *l, char* newPage);
void printListToFile(list *l, char *save_dir, char *fileName);

#endif