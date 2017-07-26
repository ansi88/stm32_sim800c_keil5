#ifndef CYCQUEUE_H
#define CYCQUEUE_H

#include "stm32f10x.h"

#define MAXSIZE 255   //define the length of the queue


//queue manager
typedef struct {
	uint8_t data[MAXSIZE];
	uint8_t head;
	uint8_t rear;
	uint8_t count;
}CycQueue;

extern CycQueue *q;

CycQueue *CycQueueInit(void);
void CycQueueFree(CycQueue *q);
uint8_t CycQueueIsEmpty(CycQueue *q);
uint8_t CycQueueIsFull(CycQueue *q);
uint8_t CycQueueIn(CycQueue *q, uint8_t data);
uint8_t CycQueueOut(CycQueue *q);
uint8_t CycQueueLen(CycQueue *q);
uint8_t CycQueuePeek(CycQueue *q);

#endif

