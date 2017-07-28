/*
 * queue.h
 *
 *  Created on: 2017年7月28日
 *      Author: Lenny_Hsu
 */

#ifndef SRC_QUEUE_H_
#define SRC_QUEUE_H_

#include "board.h"
/* Queue structure */
#define QUEUE_ELEMENTS 1024
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)

extern bool flag;

Bool QAdd(uint16_t item);
Bool QGet(uint16_t* get_item);
int QisFull();
int QisEmpty();
uint32_t QBuffLength();
#endif /* SRC_QUEUE_H_ */
