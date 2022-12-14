#include <stdlib.h>
#include <stdbool.h>
typedef struct node
{
    void* dataptr;
    struct node* next;
} Node;

typedef struct
{
    Node* front;
    Node* rear;
    int count;
} queue;

queue* createqueue(void);
bool dequeue(queue* queue, void** itemptr);
bool enqueue(queue* queue, void* itemptr);
bool queuefront(queue* queue, void** itemptr);
bool queuerear(queue* queue, void** itemptr);
int queuecount(queue* queue);
bool emptyqueue(queue* queue);