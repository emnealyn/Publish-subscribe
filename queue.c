#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

TQueue* createQueue(int size) {
    if (size <= 0) return NULL;
    TQueue *queue = (TQueue *)malloc(sizeof(TQueue));
    if (queue == NULL) return NULL; 
    queue->max_size = size;
    queue->current_size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->rw_lock, NULL); 
    pthread_cond_init(&queue->not_empty, NULL); \
    pthread_cond_init(&queue->not_full, NULL); 
    queue->subscriber_count = 0;
    queue->subscribers = NULL;
    return queue;
}

void addMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->rw_lock); 

    if (queue->subscriber_count == 0){
        pthread_mutex_unlock(&queue->rw_lock);
        return;
    }

    while (queue->current_size >= queue->max_size) {
        pthread_cond_wait(&queue->not_full, &queue->rw_lock);
    }

    Message *new_msg = (Message *)malloc(sizeof(Message));
    new_msg->data = msg;
    new_msg->next = NULL;
    new_msg->remaining_read_count = queue->subscriber_count;


    if (queue->tail == NULL) {
        queue->head = new_msg;
    } else {
        queue->tail->next = new_msg;
    }
    queue->tail = new_msg;
    queue->current_size++;

    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (sub->head == NULL) {
            sub->head = new_msg;
        }
        sub = sub->next;
    }

    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->rw_lock);
}

void unsafeRemoveMsg(TQueue *queue, void *msg) {
    if (queue == NULL) return;
    if (msg == NULL) return;

    Message *prev = NULL;
    Message *current = queue->head;

    while (current) {
        if (current->data == msg) {
            break;
        }
        prev = current;
        current = current->next;
    }

    if (!current) {
        current = queue->head;
        prev = NULL;
        while (current) {
            if (current->data == msg) {
                break;
            }
            prev = current;
            current = current->next;
        }
    }

    if(!current) return;

    if (prev == NULL) {
        queue->head = current->next;
    } else {
        prev->next = current->next;
    }

    if (current->next == NULL) {
        queue->tail = prev;
    }

    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (sub->head == current) {
            sub->head = current->next;
        }
        sub = sub->next;
    }

    queue->current_size--;
    pthread_cond_signal(&queue->not_full);

}

void removeMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->rw_lock);
    unsafeRemoveMsg(queue, msg);
    pthread_mutex_unlock(&queue->rw_lock);
}

void* getMsg(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->rw_lock); 

    Subscriber *sub = queue->subscribers;

    while (sub != NULL) {

        if (pthread_equal(sub->thread, thread)) {
            while (sub->head == NULL) {
                pthread_cond_wait(&queue->not_empty, &queue->rw_lock);
            }
            Message *msg = sub->head;

            if (msg != NULL) {
                sub->head = sub->head->next;
                msg->remaining_read_count--;
                void *data = msg->data;
                if (msg->remaining_read_count == 0) {
                    unsafeRemoveMsg(queue, msg->data);
                }
                pthread_mutex_unlock(&queue->rw_lock);
                return data;
            }
  
        }

        sub = sub->next;

    }

    pthread_mutex_unlock(&queue->rw_lock);
    return NULL;
}

void subscribe(TQueue *queue, pthread_t thread) {
    if (queue == NULL) return;

    pthread_mutex_lock(&queue->rw_lock);

    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (pthread_equal(sub->thread, thread)) {
            pthread_mutex_unlock(&queue->rw_lock);
            return; 
        }
        sub = sub->next;
    }

    Subscriber *new_sub = (Subscriber *)malloc(sizeof(Subscriber));
    new_sub->thread = thread;
    new_sub->head = NULL; 
    new_sub->next = queue->subscribers;
    queue->subscribers = new_sub;
    queue->subscriber_count++;

    pthread_mutex_unlock(&queue->rw_lock);
}

void unsubscribe(TQueue *queue, pthread_t thread) {
    if (queue == NULL) return;

    pthread_mutex_lock(&queue->rw_lock);

    Subscriber *prev = NULL;
    Subscriber *current = queue->subscribers;

    int is_subscribed = 0;
    while (current != NULL) {
        if (pthread_equal(current->thread, thread)) {
            is_subscribed = 1;
            break;
        }
        prev = current;
        current = current->next;
    }

    if (!is_subscribed) {
        pthread_mutex_unlock(&queue->rw_lock);
        return; 
    }

    Message *msg = current->head;
    while (msg != NULL) {
        msg->remaining_read_count--;
        if (msg->remaining_read_count == 0) {
            unsafeRemoveMsg(queue, msg->data);
        }
        msg = msg->next;
    }

    if (prev == NULL) {
        queue->subscribers = current->next;
    } else {
        prev->next = current->next;
    }

    queue->subscriber_count--;

    pthread_mutex_unlock(&queue->rw_lock);
}

int getAvailable(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->rw_lock);

    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (pthread_equal(sub->thread, thread)) {
            break;
        }
        sub = sub->next;
    }

    if (sub == NULL) {
        pthread_mutex_unlock(&queue->rw_lock);
        return 0;
    }

    int count = 0;
    Message *msg = sub->head;
    while (msg != NULL) {
        count++;
        msg = msg->next;
    }

    pthread_mutex_unlock(&queue->rw_lock);
    return count;
}

void setSize(TQueue *queue, int size) {
    if (queue == NULL) return;

    pthread_mutex_lock(&queue->rw_lock);

    while (queue->current_size > size) {
        if (queue->head != NULL) {
            Message *old_msg = queue->head;
            queue->head = old_msg->next;
            if (queue->head == NULL) {
                queue->tail = NULL;
            }

            Subscriber *sub = queue->subscribers;
            while (sub != NULL) {
                if (sub->head == old_msg) {
                    sub->head = old_msg->next;
                }
                sub = sub->next;
            }

            free(old_msg->data);
            free(old_msg);
            queue->current_size--;
        }

    }

    queue->max_size = size;

    
    if (queue->current_size < size) {
        pthread_cond_broadcast(&queue->not_full);
        pthread_mutex_unlock(&queue->rw_lock);
        return;
    }

    pthread_mutex_unlock(&queue->rw_lock);

}

void destroyQueue(TQueue *queue) {
    if (queue == NULL) return; 

    pthread_mutex_lock(&queue->rw_lock);

    Message *current = queue->head;
    while (current != NULL) {
        Message *next = current->next;
        free(current);
        current = next;
    }

    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        Subscriber *next = sub->next;
        free(sub);
        sub = next;
    }

    pthread_mutex_unlock(&queue->rw_lock);
    pthread_mutex_destroy(&queue->rw_lock);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);

    free(queue);
}
