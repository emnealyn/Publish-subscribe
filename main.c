#include "queue.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h> 

void msg(const char *message) {
    printf("%s\n", message);
}

void doExit(int code) {
    exit(code);
}

void gmsg(const char *message) {
    printf("%s\n", message);
}

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
        
        // int available = getAvailable(queue, thread_id); 
        // printf("[R] Messages available for thread: %d\n", available);

        char *msg = (char*)getMsg(queue, thread_id);
        if (msg != NULL) {
            //printf("Received message: %s\n", msg);
        }

        if (rand() % 100 <= 5) {
            active = 0;
            //printf("Try UNSUB\n");
            unsubscribe(queue, thread_id);
            //printf("UNSUB\n");
            continue;
        };

        usleep(700);
    }

    return NULL;
}

void* writer_thread(void* arg) {
    TQueue *queue = (TQueue*)arg;
    // pthread_t thread_id = pthread_self(); 

    while (1) {
        char *msg = "Hello from thread";
        addMsg(queue, msg);
        //printf("[W] Added message\n");
        usleep(500);
    }

    return NULL;
}

int testAvailable() {
     TQueue *q;
     msg("Avail:");
     q = createQueue(10);
     int *m1, *m2, *p;
     m1 = malloc(sizeof(int)); *m1 = 10;
     m2 = malloc(sizeof(int)); *m2 = 20;
     pthread_t t1=101, t2=102;
     //subscribe(q, t2);
     subscribe(q, t1);
     printf("Subscribed\n");
     //unsubscribe(q, t2);
     addMsg(q, m1);
     printf("Added\n");
     if (getAvailable(q, t1) != 1) doExit(101);
     printf("Available1\n");
     subscribe(q, t2);
     printf("Subscribed2\n");
     if (getAvailable(q, t1) != 1) doExit(102);
     printf("Available2\n");
     if (getAvailable(q, t2) != 0) doExit(103);
     printf("Available3\n");
     addMsg(q, m2);
     printf("Added2\n");
     if (getAvailable(q, t1) != 2) doExit(104);
     printf("Available4\n");
     if (getAvailable(q, t2) != 1) doExit(105);
     printf("Available5\n");
     p = getMsg(q, t1);
     printf("%d\n", *p);
     printf("Got1\n");
     if (p != m1) { printf("[%d≠%d]",*p, *m1); doExit(106); }
     //
     p = getMsg(q, t2);
     printf("%d\n", *p);
     printf("Got3\n");
     if (p != m2) { printf("[%d≠%d]",*p, *m2); doExit(108); }
     //
     p = getMsg(q, t1);
     printf("Got2\n");
     printf("%d\n", *p);
     if (p != m2) { printf("[%d≠%d]",*p, *m2); doExit(107); }
    //  p = getMsg(q, t2);
    //  printf("%d\n", *p);
    //  printf("Got3\n");
    //  if (p != m2) { printf("[%d≠%d]",*p, *m2); doExit(108); }
     printf("DidExit\n");
     if (getAvailable(q, t1) != 0) doExit(109);
     printf("Available6\n");
     if (getAvailable(q, t2) != 0) doExit(110);
     printf("Available7\n");
     gmsg(" OK");
     destroyQueue(q);
     return 0;
}

int main() {
    testAvailable();
    // TQueue *q = createQueue(10);

    // #define NUM 30
    // pthread_t wlist[NUM];
    // pthread_t rlist[NUM];
    // for (int i = 0; i < NUM; i++) {
    //     pthread_create(&wlist[i], NULL, subscriber_thread, (void*)q);
    //     pthread_create(&rlist[i], NULL, writer_thread, (void*)q);
    // }

    // for (int i = 0; i < NUM; i++) {
    //     pthread_join(wlist[i], NULL);
    //     pthread_join(rlist[i], NULL);
    // }
    // destroyQueue(q);
    // return 0;
}