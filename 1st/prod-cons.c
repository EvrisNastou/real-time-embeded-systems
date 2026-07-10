#include "prod-cons.h"

void *my_func(void* arg){
    int base_angle = *(int*) arg;
    double result=0.0;
    int i;

    for(i=0;i<10;i++){
        result = sin(base_angle+i);
    }

    return NULL;
}


void *producer(void *arg){
    queue *fifo = (queue*) arg;
    int i;
    workFunction task;
    int *angle;

    for (i=0;i<LOOP;i++){
        task.work=my_func;
        angle = (int *)malloc(sizeof(int)); 
        *angle = i;
        task.arg = angle;
        gettimeofday(&task.enqueue_time, NULL);

        pthread_mutex_lock(fifo->mut);
        while (fifo->full)
        {
            printf ("producer: queue FULL.\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }
        queueAdd(fifo, task);

        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }


    return NULL;
}


void *consumer(void *arg){
    queue *fifo = (queue *) arg;
    workFunction d;
    struct timeval current_time;
    double waiting_time;

    while(1){
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty)
        {
            printf ("consumer: queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &d);
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);
        if (d.work == NULL) {
            break;
        }

        gettimeofday(&current_time, NULL); 

        waiting_time = (double)(current_time.tv_sec - d.enqueue_time.tv_sec) + 
                       (double)(current_time.tv_usec - d.enqueue_time.tv_usec) / 1000000.0;


        pthread_mutex_lock(&aver_time);
        average_time += waiting_time;
        pthread_mutex_unlock(&aver_time);

        printf("Χρόνος αναμονής στην ουρά: %lf δευτερόλεπτα\n", waiting_time);

        d.work(d.arg);
        free(d.arg);
        //printf("the sin is calculated\n");   
    }

    return NULL;
}

queue *queueInit (void)
{
    queue *qu;

    qu = (queue *)malloc (sizeof (queue));
    if (qu == NULL) return (NULL);

    qu->empty = 1;
    qu->full = 0;
    qu->head = 0;
    qu->tail = 0;
    qu->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (qu->mut, NULL);
    qu->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (qu->notFull, NULL);
    qu->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (qu->notEmpty, NULL);

    return (qu);
}

void queueDelete(queue *qu)
{
    pthread_mutex_destroy (qu->mut);
    free (qu->mut);	
    pthread_cond_destroy (qu->notFull);
    free (qu->notFull);
    pthread_cond_destroy (qu->notEmpty);
    free (qu->notEmpty);
    free (qu);
}

void queueAdd(queue *qu, workFunction in)
{
    qu->buf[qu->tail] = in;
    qu->tail++;

    if (qu->tail == QUEUESIZE)
    qu->tail = 0;

    if (qu->tail == qu->head)
        qu->full = 1;

    qu->empty = 0;

    return;
}

void queueDel(queue *qu, workFunction *out)
{
    *out = qu->buf[qu->head];

    qu->head++;

    if (qu->head == QUEUESIZE)
        qu->head = 0;

    if (qu->head == qu->tail)
        qu->empty = 1;

    qu->full = 0;

    return;
}