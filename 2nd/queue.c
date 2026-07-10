#include "my_pthreads.h"

void queueInit(queue *qu) {
    //initialize the variables
    qu->empty = 1;
    qu->full = 0;
    qu->head = 0;
    qu->tail = 0;
    
    pthread_mutex_init(&qu->mut, NULL);
    pthread_cond_init(&qu->notEmpty, NULL);
    pthread_cond_init(&qu->notFull, NULL);
}

void queueAdd(queue *qu, const char *json_message, size_t len) {
    //if the queue is full, overwrite the oldest unread message
    if (qu->full) {
        qu->head++;
        if (qu->head == QUEUESIZE) qu->head = 0;
    }

    //copy the message and put it in the queue
    size_t copy_len = (len < MAX_JSON_SIZE - 1) ? len : (MAX_JSON_SIZE - 1);
    memcpy(qu->buf[qu->tail], json_message, copy_len);
    qu->buf[qu->tail][copy_len] = '\0';

    qu->tail++; // increase the tail index
    
    //wrap around to the start if the tail reaches the end of the buffer
    if (qu->tail == QUEUESIZE) qu->tail = 0; 

    //raise a flag if the queue is full
    if (qu->tail == qu->head) qu->full = 1; 
    
    qu->empty = 0;
}

void queueDel(queue *qu, char *out_message) {
    //copy the message from the queue to the out_message buffer
    strncpy(out_message, qu->buf[qu->head], MAX_JSON_SIZE - 1);
    out_message[MAX_JSON_SIZE - 1] = '\0';

    qu->head++;
    
    //wrap around to the start if the head reaches the end of the buffer
    if (qu->head == QUEUESIZE) qu->head = 0; 

    //raise a flag if the queue is empty
    if (qu->head == qu->tail) qu->empty = 1; 
    
    qu->full = 0;
}