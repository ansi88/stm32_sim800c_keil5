
/* Includes ------------------------------------------------------------------*/
#include "queue.h"
#include <stdlib.h>

CycQueue *q = NULL; 

//initialize queue
CycQueue *CycQueueInit(void)
{
	CycQueue *q = (CycQueue *)malloc(sizeof(CycQueue));
	if(q != NULL)
	{
		q->head  = MAXSIZE - 1;
		q->rear  = MAXSIZE - 1;
		q->count = 0;
		return q;
	}
	else
	{
		return NULL;
	}
}


//free the queue
void CycQueueFree(CycQueue *q)
{
	if(q != NULL)
		free(q);
}


//queue is empty?
uint8_t CycQueueIsEmpty(CycQueue *q)
{
	return (q->count == 0);
}

//queue is full?
uint8_t CycQueueIsFull(CycQueue *q)
{
	return (q->count == MAXSIZE);
}


//in queue
uint8_t CycQueueIn(CycQueue *q, uint8_t data)
{
	if(q->count == MAXSIZE)
	{
		//Serial.printf("The queue is full!\n");
		return 0;
	}
	else
	{
		q->rear = (q->rear + 1) % MAXSIZE;
		q->data[q->rear] = data;
		q->count += 1;
		return 1;
	}
}


//out queue
uint8_t CycQueueOut(CycQueue *q)
{
	if(q->count == 0)
	{
		//Serial.printf("The queue is empty!\n");
		return 0;
	}
	else
	{
		q->head = (q->head + 1) % MAXSIZE;
		q->count -= 1;
		return q->data[q->head];
	}
}

//get the length of the queue
uint8_t CycQueueLen(CycQueue *q)
{
	return q->count;
}


//get the head element of the queue
uint8_t CycQueuePeek(CycQueue *q)
{
	if(q->count == 0)
	{
		//Serial.printf("The queue is empty!\n");
		return 0;
	}
	else
	{
		return q->data[(q->head + 1)%MAXSIZE];
	}
}

