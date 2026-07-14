#include "my_pthreads.h"
#include <stdarg.h>

// the shared instance used by producer (writer) and monitor (reader)
producer_status prod_status;

void status_init(producer_status *s) {
    s->has_event = 0;
    s->message[0] = '\0';
    pthread_mutex_init(&s->mut, NULL);
}

void status_set(producer_status *s, const char *fmt, ...) {
    char formatted[LOG_MSG_SIZE];

    // format into a local buffer first, so vsnprintf never runs while
    // holding the lock
    va_list args;
    va_start(args, fmt);
    vsnprintf(formatted, sizeof(formatted), fmt, args);
    va_end(args);

    pthread_mutex_lock(&s->mut);
    strncpy(s->message, formatted, LOG_MSG_SIZE - 1);
    s->message[LOG_MSG_SIZE - 1] = '\0';
    s->has_event = 1;
    pthread_mutex_unlock(&s->mut);
}


int status_check_and_clear(producer_status *s, char *out, size_t out_size) {
    int got = 0;

    pthread_mutex_lock(&s->mut);
    if (s->has_event) {
        strncpy(out, s->message, out_size - 1);
        out[out_size - 1] = '\0';
        s->has_event = 0;
        got = 1;
    }
    pthread_mutex_unlock(&s->mut);

    return got;
}
