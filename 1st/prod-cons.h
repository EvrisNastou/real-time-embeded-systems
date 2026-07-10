#ifndef PROD_CONS_H
#define PROD_CONS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>

#define p 4
#define q 14
#define QUEUESIZE 10
#define LOOP 2000


extern double average_time;
extern pthread_mutex_t aver_time;

typedef struct{
  void *(*work)(void *);
  void *arg;
  struct timeval enqueue_time;
}workFunction;

typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

void *producer (void *args);
void *consumer (void *args);
void *my_func(void* arg);
queue *queueInit (void);
void queueDelete (queue *qu);
void queueAdd (queue *qu, workFunction in);
void queueDel (queue *qu, workFunction *out);
void *consumer(void *arg);


#endif