#include "my_pthreads.h"

message_stats app_stats;

// global flag checked by all thread loops; only this sig_atomic_t write
// happens inside the signal handler, so it stays async-signal-safe.
volatile sig_atomic_t keep_running = 1;

static void handle_shutdown_signal(int signum) {
    (void)signum;
    keep_running = 0;
}

int main() {
    pthread_t producer_id, consumer_id, monitor_id;

    // catch Ctrl+C / kill to trigger a clean shutdown instead of an abrupt exit
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_shutdown_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // allocate memory in heap for the queue
    queue *my_fifo = (queue *)malloc(sizeof(queue));
    if (my_fifo == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    //initialize the queue
    queueInit(my_fifo);

    //initialize the status (producer -> monitor status messages)
    status_init(&prod_status);
       
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

    // wait here until Ctrl+C / SIGTERM requests a shutdown
    while (keep_running) {
        sleep(1);
    }

    printf("\n[Main] Shutdown requested, waking up threads...\n");

    // the consumer may be blocked in pthread_cond_wait: wake it up so it can
    // notice keep_running == 0 and return cleanly (safe here, we're in a
    // normal thread context, not inside the signal handler)
    pthread_mutex_lock(&my_fifo->mut);
    pthread_cond_broadcast(&my_fifo->notEmpty);
    pthread_mutex_unlock(&my_fifo->mut);

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