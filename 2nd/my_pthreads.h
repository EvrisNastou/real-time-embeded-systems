#ifndef MY_PTHREADS
#define MY_PTHREADS

#define _POSIX_C_SOURCE 200809L

#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "cJSON.h"
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#define QUEUESIZE 300       //maximum number of elements in the queue
#define MAX_JSON_SIZE 131072 //maximum size of a single JSON message in bytes


// per-session data for reassembling fragmented WebSocket messages.
// libwebsockets allocates this automatically per connection.
typedef struct {
    char msg_buf[MAX_JSON_SIZE]; // buffer to accumulate fragmented incoming data
    size_t msg_len;              // current length of the accumulated data
} per_session_data;


//queue object
typedef struct {
    char buf[QUEUESIZE][MAX_JSON_SIZE]; //buffer for storing data
    long head, tail; // head and tail index
    int full, empty; // full and empty flags
    pthread_mutex_t mut; //mutex to protect queue access
    pthread_cond_t notFull; //condution variable: signals when queue is NOT full
    pthread_cond_t notEmpty; //condition variable: signals when queue is NOT empty
} queue;

// struct for metrixs
typedef struct {
    unsigned int commit_count; //number of 'commit' messages received
    unsigned int identity_count; //number of 'identity' messages received
    unsigned int account_count; //number of 'account' messages received
    unsigned int info_count; //number of 'info' messages received
    pthread_mutex_t stats_mut; // mutex only for status to make the update safe
} message_stats;

//make the struct global
extern message_stats app_stats;


//queue functions
void queueInit(queue *qu); //initialization of the queue
void queueAdd(queue *qu,const char *json_message, size_t len); //add a message to the queue (producer)
void queueDel(queue *qu, char *out_message); //remove and read a message from the queue (consumer)

// Thread entry point functions
void *producer(void *arg); // producer thread: reads from WebSocket and adds to queue
void *consumer(void *arg); // consumer thread: reads from queue and parses JSON
void *monitor(void *arg);  // monitor thread: periodically logs metrics to CSV

#endif /*MY_PTHREADS*/