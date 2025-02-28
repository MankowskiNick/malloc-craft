#ifndef MESH_GEN_QUEUE
#define MESH_GEN_QUEUE

typedef struct node{
    void* data;
    struct node *next;
} queue_node;

typedef int (*equals_func)(void* a, void* b);

void queue_init(queue_node** head);
void queue_cleanup(queue_node** head);
void queue_push(queue_node** head, void* data, equals_func equals);
void* queue_pop(queue_node** head);
void queue_remove(queue_node** head, void* data, equals_func equals);

#endif