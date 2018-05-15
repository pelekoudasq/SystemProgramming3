#include <stdlib.h>
#include "queue.h"

void TSqueue_setValue(QueueType *target, QueueType source)
{   *target=source;
}

void Queue_create(Queue *oura, int size)
{/*	Pro: 		kamia
  *	Meta: 		Dhmioyrgia kenhs oyras */	
	oura->embros =  0;
	oura->piso = 0;
	oura->buffer = malloc(size * sizeof(QueueType));
}

void Queue_destroy(Queue *oura)
{	free(oura->buffer);
}

int Queue_empty(Queue *oura)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta:		Epistrefei 1 an h oyra einai kenh, diaforetika 0 */
	return ( oura->embros == oura->piso );
}

int Queue_full(Queue *oura)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta:		Epistrefei 1 an h oyra einai gemath, diaforetika 0 */
	int neo_piso = (oura->piso+1) % PLITHOS;
	if (neo_piso == oura->embros )
		return 1;
	else 
		return 0;
}

void Queue_push(Queue *oura, QueueType element)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta: 		Oyra exei epayksh8ei me to element */
	oura->buffer[oura->piso] = element;
	oura->piso = ( oura->piso + 1 ) % PLITHOS;
}

QueueType Queue_pop(Queue *oura)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta: 		Oura exei meiw8ei kata ena stoixeio */
	QueueType stoixeio = oura->buffer[oura->embros];
	oura->embros = ( oura->embros + 1 ) % PLITHOS;
	return stoixeio;
}
