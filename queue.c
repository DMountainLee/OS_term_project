#include "queue.h"

queue* createqueue(void)
{
    queue* q;
    q = (queue*)malloc(sizeof(queue));

    if (q)
    {
        q->front = NULL;
        q->rear = NULL;
        q->count = 0;
    }

    return q;
}

bool enqueue(queue* queue, void* itemptr)
{
    Node* node;
    node = (Node*)malloc(sizeof(Node));
    node->dataptr = itemptr;
    node->next = NULL;
    
    if(queue->count == 0) queue->front = node;
    else queue->rear->next = node;
    
    (queue->count)++;
    queue->rear = node;
    
    return true;
}

bool dequeue(queue* pqueue, void** itemptr)
{
    Node* deleteloc;
    if (!pqueue->count) return false;

    // queue front는 run -> wait, wait -> run 으로 이동해야하므로 저장
    *itemptr = pqueue->front->dataptr;
    deleteloc = pqueue->front;
    
    if (pqueue->count == 1) pqueue->rear = pqueue->front = NULL;
    else pqueue->front = pqueue->front->next;
    
    (pqueue->count)--;
    free(deleteloc);
    
    return true;
}

bool queuefront(queue* pqueue, void** itemptr)
{
    if (!pqueue->count) return false;

    else
    {
        *itemptr = pqueue->front->dataptr;
        return true;
    }
}

bool queuerear(queue* pqueue, void** itemptr)
{
    if (!pqueue->count) return true;
    else
    {
        *itemptr = pqueue->rear->dataptr;
        return false;
    }
}

bool emptyqueue(queue* pqueue)
{
    if(pqueue->count == 0) return true;
    else return false;
}

int queuecount(queue* pqueue)
{
    return pqueue->count;
}