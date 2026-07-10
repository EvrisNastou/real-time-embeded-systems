#include "my_pthreads.h"

message_stats app_stats;

int main() {
    pthread_t producer_id, consumer_id, monitor_id;

    // allocate memory in heap for the queue
    queue *my_fifo = (queue *)malloc(sizeof(queue));
    if (my_fifo == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    //initialize the queue
    queueInit(my_fifo);
       
    //initialize the stats mutex and variables
    pthread_mutex_init(&app_stats.stats_mut, NULL);
    
    app_stats.commit_count = 0;
    app_stats.identity_count = 0;
    app_stats.account_count = 0;
    app_stats.info_count = 0;

    // create the threads
    if(pthread_create(&producer_id, NULL, producer, (void *)my_fifo) != 0) exit(1);
    if(pthread_create(&consumer_id, NULL, consumer, (void *)my_fifo) != 0) exit(1);
    if(pthread_create(&monitor_id, NULL, monitor, (void *)my_fifo) != 0) exit(1);

    // wait for the threads to finish
    if(pthread_join(producer_id, NULL) != 0) exit(1);
    if(pthread_join(consumer_id, NULL) != 0) exit(1);
    if(pthread_join(monitor_id, NULL) != 0) exit(1);
    
    // destroy mutexes and condition variables to free system resources
    pthread_mutex_destroy(&my_fifo->mut);
    pthread_cond_destroy(&my_fifo->notEmpty);
    pthread_cond_destroy(&my_fifo->notFull);
    pthread_mutex_destroy(&app_stats.stats_mut);
    
    //free the heap
    free(my_fifo); 

    return 0;
}