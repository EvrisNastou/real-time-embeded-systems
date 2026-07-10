#include "prod-cons.h"

double average_time=0.0;
pthread_mutex_t aver_time;

int main(){
    queue *fifo;
    pthread_t pro[p], con[q];
    workFunction task;
    int i;

    pthread_mutex_init(&aver_time, NULL);
    fifo = queueInit ();
    if (fifo ==  NULL) {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }

    for(i=0;i<p;i++){
        if(pthread_create(&pro[i], NULL, producer, fifo)!=0){
            printf("Thread error\n");
            exit(1);
        }
    }

    for(i=0;i<q;i++){
        if(pthread_create(&con[i], NULL, consumer, fifo)!=0){
            printf("Thread error\n");
            exit(1);
        }
    }
    
    for(i=0;i<p;i++){
        if(pthread_join(pro[i], NULL)!=0){
            printf("Thread error\n");
            exit(1);
        }
    }
    
    for (i = 0; i < q; i++) {
        task.work = NULL;
        task.arg = NULL;
        
        pthread_mutex_lock(fifo->mut);
        while (fifo->full) {
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        queueAdd(fifo, task);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }

    for(i=0;i<q;i++){
        if(pthread_join(con[i], NULL)!=0){
            printf("Thread error\n");
            exit(1);
        }
    }

    
    average_time=average_time/(p * LOOP);

    printf("Ο μεσος χρονος ειναι %lf\n", average_time);

    pthread_mutex_destroy(&aver_time);
    queueDelete (fifo);

    return 0;
}