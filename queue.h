#ifndef _QUEUE_
#define _QUEUE_
/*dhlwseis typwn*/

typedef int QueueType;

typedef struct 
{	int embros;		/*8esh toy prwtoy stoixeoy ths oyras*/
	int piso;		/*8esh toy teleytaioy stoixeioy ths oyras*/
	int size;
	int pagesServed;
	int bytesReturned;
	QueueType* buffer;	/*o pinakas twn stoixeiwn*/
} Queue;

void Queue_serve(Queue *oura);

int Queue_getPages(Queue *oura);

void Queue_bytes(Queue *oura, int bytes);

int Queue_getBytes(Queue *oura);

void Queue_create(Queue *oura, int size);

void Queue_destroy(Queue *oura);

int Queue_empty(Queue *oura);

int Queue_full(Queue *oura);

int Queue_push(Queue *oura, QueueType element);

int Queue_pop(Queue *oura, QueueType* element);



#endif