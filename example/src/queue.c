/*
 * queue.c
 *
 *  Created on: 2017年7月28日
 *      Author: Lenny_Hsu
 */

#include "queue.h"


uint16_t Queue[QUEUE_SIZE];
int32_t front=-1, rear=-1;
bool flag = 0;

int QisFull() {
	return (rear % QUEUE_ELEMENTS == front);
}

int QisEmpty() {
	return front == rear;
}

uint32_t QBuffLength()
{

	if(front - rear> 1)
	{
		return front -rear;
	}
	else if(front - rear< 0)
	{
		return rear - front;
	}
	return 0;
}

Bool QAdd(uint16_t item) {
	if ((QisFull() && flag == 1)
			|| (rear == QUEUE_ELEMENTS - 1 && front == -1)) {
		// printf("Circular Queue is full!\n");
		return FALSE;
	}
	rear = (rear + 1) % QUEUE_ELEMENTS;
	Queue[rear] = item;
	if (front == rear)
		flag = 1;
	return TRUE;
}

Bool QGet(uint16_t* get_item) {
	if (QisEmpty() && flag == 0) {
		// printf("Circular Queue is empty!\n");
		return FALSE;
	}
	front = (front + 1) % QUEUE_ELEMENTS;
	*get_item = Queue[front];
	if (front == rear)
		flag = 0;

	return TRUE;
}

