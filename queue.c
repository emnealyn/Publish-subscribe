#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

TQueue* createQueue(int size) {
    TQueue *queue = (TQueue *)malloc(sizeof(TQueue)); // Allocate memory for the queue
    queue->max_size = size;
    queue->current_size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->rw_lock, NULL); // Initialize the mutex
    pthread_cond_init(&queue->cond, NULL); // Initialize the condition variable
    queue->subscriber_count = 0;
    queue->subscribers = NULL;
    return queue;
}

void addMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->rw_lock); // Lock the mutex

    if (queue->subscriber_count == 0){
        pthread_mutex_unlock(&queue->rw_lock);
        return;
    }

    // If queue is full, wait until there is space
    while (queue->current_size >= queue->max_size) {
        pthread_cond_wait(&queue->cond, &queue->rw_lock);
    }

    if (queue->subscriber_count == 0){
        pthread_mutex_unlock(&queue->rw_lock);
        return;
    }

    // Create a new message
    Message *new_msg = (Message *)malloc(sizeof(Message));
    new_msg->data = msg;
    new_msg->next = NULL;
    new_msg->remaining_read_count = queue->subscriber_count;

    // Add the message to the queue
    if (queue->tail == NULL) {
        queue->head = new_msg;
    } else {
        queue->tail->next = new_msg;
    }
    queue->tail = new_msg;
    queue->current_size++;

    // Inform all subscribers
    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (sub->head == NULL) {
            sub->head = new_msg;
        }
        sub = sub->next;
    }
    
    pthread_mutex_unlock(&queue->rw_lock);
    //printf("[OUT] AddMsg 3");
    pthread_cond_broadcast(&queue->cond);
}

void unsafeRemoveMsg(TQueue *queue, void *msg) {
    Message *prev = NULL;
    Message *current = queue->head;

    // Traverse the queue to find the message
    while (current != NULL) {
        if (current->data == msg) {
            // If the message is found, update the pointers to remove it
            if (prev == NULL) {
                queue->head = current->next;
            } else {
                prev->next = current->next;
            }

            // If the message is the last one, update the tail pointer
            if (current->next == NULL) {
                queue->tail = prev;
            }

            // Adjust subscribers' heads
            Subscriber *sub = queue->subscribers;
            while (sub != NULL) {
                if (sub->head == current) {
                    sub->head = current->next;
                }
                sub = sub->next;
            }

            // Decrease the current size of the queue
            queue->current_size--;

            // Free the memory allocated for the message
            free(current);
            break;
        }

        // Move to the next message
        prev = current;
        current = current->next;
        pthread_cond_broadcast(&queue->cond);
    }
}

void removeMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->rw_lock);
    unsafeRemoveMsg(queue, msg);
    pthread_mutex_unlock(&queue->rw_lock);
}

void* getMsg(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->rw_lock); // Lock the mutex

    // Find the subscriber
    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (pthread_equal(sub->thread, thread)) {
            break;
        }
        sub = sub->next;
    }

    // If the thread is not subscribed, return NULL
    if (sub == NULL) {
        pthread_mutex_unlock(&queue->rw_lock);
        return NULL;
    }

    // Wait until there is a message available
    while (sub->head == NULL) {
        pthread_cond_wait(&queue->cond, &queue->rw_lock);
    }

    // Get the message
    Message *msg = sub->head;
    if (msg != NULL) {
        sub->head = sub->head->next;
        msg->remaining_read_count--;
        void *data = msg->data;
        if (msg->remaining_read_count == 0) {
            unsafeRemoveMsg(queue, msg);
        }
        pthread_mutex_unlock(&queue->rw_lock);
        pthread_cond_broadcast(&queue->cond);
        return data;
    }

    pthread_mutex_unlock(&queue->rw_lock);
    pthread_cond_broadcast(&queue->cond);
    return NULL;
}

void subscribe(TQueue *queue, pthread_t thread) {
    if (queue == NULL) return;

    pthread_mutex_lock(&queue->rw_lock);

    // Check if the thread is already subscribed
    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (pthread_equal(sub->thread, thread)) {
            pthread_mutex_unlock(&queue->rw_lock);
            return; // Thread is already subscribed, do nothing
        }
        sub = sub->next;
    }

    // Add new subscriber
    Subscriber *new_sub = (Subscriber *)malloc(sizeof(Subscriber));
    new_sub->thread = thread;
    new_sub->head = NULL; // Initialize the head to NULL
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

    // Check if the thread is subscribed
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
        return; // Thread is not subscribed, do nothing
    }

    // Mark unread messages as read
    Message *msg = current->head;
    while (msg != NULL) {
        msg->remaining_read_count--;
        if (msg->remaining_read_count == 0) {
            unsafeRemoveMsg(queue, msg);
        }
        msg = msg->next;
    }

    if (prev == NULL) {
        queue->subscribers = current->next;
    } else {
        prev->next = current->next;
    }

    free(current);
    queue->subscriber_count--;

    pthread_mutex_unlock(&queue->rw_lock);
}

int getAvailable(TQueue *queue, pthread_t thread) {
    //printf("[IN] GetAvailable 1");
    pthread_mutex_lock(&queue->rw_lock);
    //printf("[IN] GetAvailable 2");

    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        if (pthread_equal(sub->thread, thread)) {
            break;
        }
        sub = sub->next;
    }

    // If the thread is not subscribed, return 0
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

    // If the new size is smaller than the current number of messages, remove the oldest messages

    // while (queue->current_size > size) {
    //     unsafeRemoveMsg(queue, queue->head);
    // }
    while (queue->current_size > size) {
        if (queue->head != NULL) {
            Message *old_msg = queue->head;
            queue->head = old_msg->next;
            if (queue->head == NULL) {
                queue->tail = NULL;
            }

            // Adjust subscribers' heads
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

    // If the new size is larger, inform writers that there are free slots
    if (queue->current_size < size) {
        pthread_cond_broadcast(&queue->cond);
    }

    pthread_mutex_unlock(&queue->rw_lock);
}

void destroyQueue(TQueue *queue) {
    if (queue == NULL) return; // Check if queue is already destroyed

    // Lock the mutex to prevent other interactions
    pthread_mutex_lock(&queue->rw_lock);

    // Free all messages in the queue
    Message *current = queue->head;
    while (current != NULL) {
        Message *next = current->next;
        free(current);
        current = next;
    }

    // Free all subscribers
    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        Subscriber *next = sub->next;
        free(sub);
        sub = next;
    }

    // Unlock the mutex and destroy it
    pthread_mutex_unlock(&queue->rw_lock);
    pthread_mutex_destroy(&queue->rw_lock);
    pthread_cond_destroy(&queue->cond);

    // Free the queue itself
    free(queue);
}

void displayQueueInfo(TQueue *queue) {
    if (queue == NULL) return;

    printf("Queue size: %d\n", queue->max_size);
    printf("Current size: %d\n", queue->current_size);
    printf("Number of subscribers: %d\n", queue->subscriber_count);

    Subscriber *sub = queue->subscribers;
    while (sub != NULL) {
        printf("Subscriber: %ld\n", sub->thread);
        Message *msg = sub->head;
        while (msg != NULL) {
            printf("Message: %p\n", msg->data);
            msg = msg->next;
        }
        sub = sub->next;
    }

}
