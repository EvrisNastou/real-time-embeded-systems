#include "my_pthreads.h"

void *consumer(void *arg){
    queue *fifo=(queue *) arg;

    char local_buffer[MAX_JSON_SIZE]; //local buffer to store the JSON from the queue and parse it
    
    while(keep_running) {
        // ---Take the object from the queue---
        pthread_mutex_lock(&fifo->mut);// lock the queue to read/remove object with safety

        //if the queue is empty the consumer sleeps and wait for the signal notEmpty
        while (fifo->empty && keep_running) {
            //unlcok mutex and wait for data
            pthread_cond_wait(&fifo->notEmpty, &fifo->mut);
        }

        // if shutdown was requested while waiting, unlock and exit cleanly
        if (!keep_running) {
            pthread_mutex_unlock(&fifo->mut);
            break;
        }

        //take the JSON form the queue to the local variable
        queueDel(fifo, local_buffer);

        //unlock the queue
        pthread_mutex_unlock(&fifo->mut);


        // ---Parsing the JSON---
        cJSON *json = cJSON_Parse(local_buffer);
        
        // if the json is not valid continue and dont count it
        if (json == NULL) {
            pthread_mutex_lock(&app_stats.stats_mut);
            app_stats.info_count++;
            pthread_mutex_unlock(&app_stats.stats_mut);
            continue; 
        }

        //find the 'kind' of the message
        cJSON *kind = cJSON_GetObjectItemCaseSensitive(json, "kind");
        
        // lock the status mutex
        pthread_mutex_lock(&app_stats.stats_mut);
        // --- Update the status ---
        if (cJSON_IsString(kind) && (kind->valuestring != NULL)) {
            if (strcmp(kind->valuestring, "commit") == 0) {
                app_stats.commit_count++;
            } 
            else if (strcmp(kind->valuestring, "identity") == 0) {
                app_stats.identity_count++;
            } 
            else if (strcmp(kind->valuestring, "account") == 0) {
                app_stats.account_count++;
            } 
            else{
                app_stats.info_count++;
            }
        }
        
        else{
            app_stats.info_count++;
        }

        //unlock the status mutex
        pthread_mutex_unlock(&app_stats.stats_mut);
        // --- free memory ---
        cJSON_Delete(json); // to avoid memory leak we must free the memory that the parser allocates
    }
    
    return NULL;
}