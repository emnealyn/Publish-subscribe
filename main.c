#include "queue.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>
#include <stdlib.h>

void* subscriber_thread(void* arg) {
    TQueue *queue = (TQueue*)arg;
    pthread_t thread_id = pthread_self();
    // usleep(rand() % 2000);
    subscribe(queue, thread_id);

    int active = 1;

    while (1) {
        if (active == 0) {
            if (rand() % 100 <= 5) {
                subscribe(queue, thread_id);
                active = 1;
                continue;
            }
            
            usleep(1000);
            continue;
        }
        
        int available = getAvailable(queue, thread_id);
        // printf("[R] Messages available for thread: %d\n", available);

        char *msg = (char*)getMsg(queue, thread_id);
        if (msg != NULL) {
            printf("Received message: %s\n", msg);
        }

        if (rand() % 100 <= 5) {
            active = 0;
            printf("Try UNSUB\n");
            unsubscribe(queue, thread_id);
            printf("UNSUB\n");
            continue;
        };

        usleep(700);
    }

    return NULL;
}

void* writer_thread(void* arg) {
    TQueue *queue = (TQueue*)arg;
    pthread_t thread_id = pthread_self();

    while (1) {
        char *msg = "Hello from thread";
        addMsg(queue, msg);
        printf("[W] Added message\n");
        usleep(500);
    }

    return NULL;
}

int main() {
    TQueue *q = createQueue(10);

    #define NUM 30
    pthread_t wlist[NUM];
    pthread_t rlist[NUM];
    for (int i = 0; i < NUM; i++) {
        pthread_create(&wlist[i], NULL, subscriber_thread, (void*)q);
        pthread_create(&rlist[i], NULL, writer_thread, (void*)q);
    }

    for (int i = 0; i < NUM; i++) {
        pthread_join(wlist[i], NULL);
        pthread_join(rlist[i], NULL);
    }
    destroyQueue(q);
    return 0;
}