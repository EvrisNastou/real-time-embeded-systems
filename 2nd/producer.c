#include "my_pthreads.h"

static volatile int connection_dropped = 0; // Flag to trigger reconnection if the connection drops

static int callback_jetstream(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    //get the queue pointer back from the context's user data
    struct lws_context *ctx = lws_get_context(wsi);
    queue *my_fifo = (queue *)lws_context_user(ctx);

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            // confirm the connection (buffered, printed later by the monitor)
            status_set(&prod_status, "[Producer] Successfully connected to Bluesky Jetstream!");
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            // retrieve the per-session data struct (our temporary buffer for fragmented messages)
            per_session_data *pss = (per_session_data *)user;

            // append the newly received chunk (in) to the end of our accumulated buffer
            if (pss->msg_len + len < MAX_JSON_SIZE - 1) {
                memcpy(pss->msg_buf + pss->msg_len, in, len);
                pss->msg_len += len;
            } else {
                // buffer overflow protection in case of exceptionally large messages
                status_set(&prod_status, "[Producer] Message exceeds MAX_JSON_SIZE!");
                pss->msg_len = 0; 
            }

            // check if this chunk is the final fragment of the WebSocket message
            if (lws_is_final_fragment(wsi)) {
                // null-terminate the buffer to create a valid C-string for the cJSON parser
                pss->msg_buf[pss->msg_len] = '\0';

                // safely write the FULLY reassembled message to the queue
                pthread_mutex_lock(&my_fifo->mut);
                
                // add the new object in the queue
                queueAdd(my_fifo, pss->msg_buf, pss->msg_len + 1); 
                
                pthread_cond_signal(&my_fifo->notEmpty); // Notify the consumer that new data is available
                pthread_mutex_unlock(&my_fifo->mut);

                // reset the buffer length counter for the NEXT incoming message
                pss->msg_len = 0; 
            }
            break;
        }
        
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            // libwebsockets passes a human-readable reason string in 'in' for this event
            if (in && len > 0) {
                status_set(&prod_status, "[Producer] Connection error: %.*s", (int)len, (char *)in);
            } else {
                status_set(&prod_status, "[Producer] Connection error: unknown reason");
            }
            connection_dropped = 1; // raise the flag indicating the connection dropped
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            status_set(&prod_status, "[Producer] Connection closed.");
            connection_dropped = 1; // raise the flag indicating the connection dropped
            break;

        default:
            break;
    }
    return 0; 
}

// Define the protocols used by the WebSocket client
static struct lws_protocols protocols[] = {
    {
        .name = "jetstream-protocol",
        .callback = callback_jetstream,
        .per_session_data_size = sizeof(per_session_data), // allocate memory for message reassembly
        .rx_buffer_size = MAX_JSON_SIZE,                   // maximum size of the receive buffer
        .id = 0                 
    },
    { .name = NULL, .callback = NULL, .per_session_data_size = 0, .rx_buffer_size = 0, .id = 0 } // terminator
};


void *producer(void *args) {
    queue *fifo = (queue *)args; 

    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws_context *context;

    // initialize the structs with zeros to prevent garbage values
    memset(&info, 0, sizeof(info));
    memset(&ccinfo, 0, sizeof(ccinfo));

    info.port = CONTEXT_PORT_NO_LISTEN; // specify that we are a client, not a server
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT; 
    info.user = (void *)fifo; // pass the queue pointer into the LWS context


    // keep-alive settings to prevent idle disconnections from the server
    info.ka_time = 30;      // idle time (in seconds) before sending the first PING
    info.ka_probes = 3;     // number of unanswered PINGs tolerated before dropping the connection
    info.ka_interval = 10;  // time (in seconds) between PING attempts

    // connection details for the Bluesky Jetstream endpoint
    ccinfo.address = "jetstream1.us-east.bsky.network";
    ccinfo.port = 443; 
    ccinfo.path = "/subscribe?wantedCollections=app.bsky.feed.post";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.ssl_connection = LCCSCF_USE_SSL; 
    ccinfo.protocol = protocols[0].name;

    // infinite loop to ensure persistent reconnection if the network drops
    while (keep_running) {
        context = lws_create_context(&info);
        if (!context) {
            status_set(&prod_status, "[Producer] Failed to create context. Retrying in 5 seconds...");
            sleep(5);
            continue;
        }

        ccinfo.context = context;
        
        if (lws_client_connect_via_info(&ccinfo) == NULL) {
            status_set(&prod_status, "[Producer] Initial connection failed. Retrying in 5 seconds...");
            lws_context_destroy(context);
            sleep(5);
            continue;
        }

        status_set(&prod_status, "[Producer] Network event loop started...");

        connection_dropped = 0;

        // main event loop: keeps the connection alive and processes incoming packets
        while (!connection_dropped && keep_running) {
            if (lws_service(context, 50) < 0) {
                break; // if lws_service returns an error, break out of the inner loop
            }
        }

        lws_context_destroy(context); // free resources and destroy context

        if (!keep_running) break; // shutdown requested: exit the reconnect loop too

        status_set(&prod_status, "[Producer] Connection lost! Attempting to reconnect in 5 seconds...");
        sleep(5); // wait 5 seconds to avoid spamming the server
    }

    return NULL;
}