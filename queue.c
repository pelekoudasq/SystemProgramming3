#include <stdlib.h>
#include "queue.h"


void Queue_create(Queue *oura, int size)
{/*	Pro: 		kamia
  *	Meta: 		Dhmioyrgia kenhs oyras */	
	oura->embros = 0;
	oura->piso = 0;
	oura->size = size;
	oura->buffer = malloc(size * sizeof(QueueType));
}

void Queue_destroy(Queue *oura) {	
	free(oura->buffer);
}

int Queue_empty(Queue *oura)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta:		Epistrefei 1 an h oyra einai kenh, diaforetika 0 */
	return ( oura->embros == oura->piso );
}

int Queue_full(Queue *oura)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta:		Epistrefei 1 an h oyra einai gemath, diaforetika 0 */
	int neo_piso = (oura->piso+1) % oura->size;
	if (neo_piso == oura->embros )
		return 1;
	else 
		return 0;
}

int Queue_push(Queue *oura, QueueType element)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta: 		Oyra exei epayksh8ei me to element */
	if ( Queue_full(oura) == 0 ){
		oura->buffer[oura->piso] = element;
		oura->piso = ( oura->piso + 1 ) % oura->size;
		return 1;
	}
	return 0;
}

int Queue_pop(Queue *oura, QueueType* element)
{/*	Pro: 		Dhmioyrgia oyras
  *	Meta: 		Oura exei meiw8ei kata ena stoixeio */
	if ( Queue_empty(oura) == 0 ){
		*element = oura->buffer[oura->embros];
		oura->embros = ( oura->embros + 1 ) % oura->size;
		return 1;
	}
	return 0;
}