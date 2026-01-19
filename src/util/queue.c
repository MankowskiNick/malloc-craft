#include "queue.h"
#include <stdlib.h>
#include <assert.h>

void queue_init(queue_node** head) {
    *head = NULL;
}

void queue_cleanup(queue_node** head) {
    while (*head != NULL) {
        queue_node* tmp = *head;
        *head = (*head)->next;
        free(tmp);
    }
}

void queue_push(queue_node** head, void* data, equals_func equals) {
    queue_node* prev = NULL;
    queue_node* cur = *head;

    while (cur != NULL) {
        if (equals(cur->data, data)) {
            cur->data = data;
            return;
        }

        prev = cur;
        cur = cur->next;
    }

    queue_node* new_node = malloc(sizeof(queue_node));
    assert(new_node != NULL && "Failed to allocate memory for queue node");
    new_node->data = data;
    new_node->next = NULL;

    if (prev == NULL) {
        *head = new_node;
    }
    else {
        prev->next = new_node;
    }
}

void* queue_pop(queue_node** head) {
    if (*head == NULL) {
        return NULL;
    }

    queue_node* tmp = *head;
    void* data = tmp->data;
    *head = (*head)->next;
    free(tmp);

    return data;
}

void queue_remove(queue_node** head, void* data, equals_func equals) {
    if (*head == NULL) {
        return;
    }

    queue_node* prev = NULL;
    queue_node* cur = *head;
    while (cur != NULL) {
        if (equals(cur->data, data)) {
            if (prev == NULL) {
                *head = cur->next;
            }
            else {
                prev->next = cur->next;
            }
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}