#include "my_pthreads.h"


//helper function to read the CPU from /proc/stat
static void get_cpu_times(unsigned long long *total, unsigned long long *idle) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return;

    char buffer[1024];
    unsigned long long user, nice, system, idl, iowait, irq, softirq;
    
    if (fgets(buffer, sizeof(buffer), fp)) {
        sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu", 
               &user, &nice, &system, &idl, &iowait, &irq, &softirq);
        
        *idle = idl + iowait;
        *total = user + nice + system + idl + iowait + irq + softirq;
    }
    fclose(fp);
}


void *monitor(void *arg) {
    queue *fifo = (queue *)arg;
    struct timespec next_time, ts;
    unsigned long long cycle_count = 0;
    
    unsigned long long prev_total = 0, prev_idle = 0;
    unsigned long long curr_total = 0, curr_idle = 0;
    get_cpu_times(&prev_total, &prev_idle);

    // open the log file in append mode
    FILE *log_file = fopen("metrics_log.txt", "a");

    //ensure that the file opened successfully
    if (!log_file) {
        fprintf(stderr, "[Monitor] Critical error: unable to open metrics_log.txt!!\n");
    }

    // Read the INITIAL monotonic time to avoid clock drift
    clock_gettime(CLOCK_MONOTONIC, &next_time);

    while (keep_running) {
        cycle_count++; 
        next_time.tv_sec += 1;
        
        //the thread sleeps strictly until the exact next second
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);

        // ---thread wakes up: get the real time for the CSV timestamp---
        clock_gettime(CLOCK_REALTIME, &ts);

        // --- fetch and reset the message counters ---
        unsigned int commits, identities, accounts, infos;
        pthread_mutex_lock(&app_stats.stats_mut);
        commits    = app_stats.commit_count;
        identities = app_stats.identity_count;
        accounts   = app_stats.account_count;
        infos      = app_stats.info_count;
        
        app_stats.commit_count = 0;
        app_stats.identity_count = 0;
        app_stats.account_count = 0;
        app_stats.info_count = 0;
        pthread_mutex_unlock(&app_stats.stats_mut);

        // --- calculate the queue usage (%) ---
        int items_in_queue = 0;
        pthread_mutex_lock(&fifo->mut);
        if (fifo->full) items_in_queue = QUEUESIZE;
        else if (fifo->empty) items_in_queue = 0;
        else if (fifo->tail >= fifo->head) items_in_queue = fifo->tail - fifo->head;
        else items_in_queue = QUEUESIZE - fifo->head + fifo->tail;
        pthread_mutex_unlock(&fifo->mut);
        
        float queue_fullness = ((float)items_in_queue / QUEUESIZE) * 100.0;

        // --- calculate the queue usage (%) ---
        get_cpu_times(&curr_total, &curr_idle);
        unsigned long long delta_total = curr_total - prev_total;
        unsigned long long delta_idle  = curr_idle - prev_idle;
        float cpu_usage = 0.0;
        if (delta_total > 0) cpu_usage = (1.0 - ((float)delta_idle / delta_total)) * 100.0;
        prev_total = curr_total;
        prev_idle  = curr_idle;

        // --- if the producer reported something since the last tick, print it ---
        char status_msg[LOG_MSG_SIZE];
        if (status_check_and_clear(&prod_status, status_msg, sizeof(status_msg))) {
            printf("%s\n", status_msg);
            if (log_file) fprintf(log_file, "# %s\n", status_msg);
        }

        // --- print metrics to the terminal ---
        printf("\n[Monitor] --- Tick %lu ---\n", ts.tv_sec);
        printf("  Δεδομένα : Commits: %u | Identities: %u | Accounts: %u | Infos: %u\n", commits, identities, accounts, infos);
        printf("  Σύστημα  : Ουρά: %5.2f%% | CPU: %5.2f%%\n", queue_fullness, cpu_usage);
        fflush(stdout);

        // --- write in the file the status ---
        if (log_file) {
            fprintf(log_file, "%ld,%ld,%u,%u,%u,%u,%.2f,%.2f\n",
                    (long)ts.tv_sec, 
                    (long)ts.tv_nsec, 
                    commits, 
                    identities, 
                    accounts, 
                    infos, 
                    queue_fullness, 
                    cpu_usage);
            fflush(log_file);
        }
    }

    if (log_file) fclose(log_file);
    return NULL;
}