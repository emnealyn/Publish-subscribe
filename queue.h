#ifndef LCL_QUEUE_H
#define LCL_QUEUE_H

// ==============================================
//
//  Version 1.1, 2025-01-16
//
// ==============================================

#include <pthread.h>

struct Message {
    pthread_mutex_t lock;
    void *data;
    int remaining_read_count;
    struct Message *next;
};
typedef struct Message Message;

struct Subscriber {
    pthread_t thread;
    Message *head;
    struct Subscriber *next;
};
typedef struct Subscriber Subscriber;



struct TQueue {
	int max_size;
    int current_size;
    Message *head;
    Message *tail;
    pthread_mutex_t rw_lock; // Add this line
    pthread_cond_t cond;
    int subscriber_count;
    Subscriber *subscribers;
};
typedef struct TQueue TQueue;

TQueue* createQueue(int size);

void destroyQueue(TQueue *queue);

void subscribe(TQueue *queue, pthread_t thread);

void unsubscribe(TQueue *queue, pthread_t thread);

void addMsg(TQueue *queue, void *msg);

void* getMsg(TQueue *queue, pthread_t thread);

int getAvailable(TQueue *queue, pthread_t thread);

void removeMsg(TQueue *queue, void *msg);

void unsafeRemoveMsg(TQueue *queue, void *msg);

void setSize(TQueue *queue, int size);

#endif //LCL_QUEUE_H