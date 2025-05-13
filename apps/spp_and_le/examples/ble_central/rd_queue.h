#ifndef RD_QUEUE_H
#define RD_QUEUE_H

typedef struct {
    void *data;
    int front;
    int rear;
    int maxSize;
    int elementSize;
} rd_queue_t;


int rd_initQueue(rd_queue_t *q, int maxSize, int elementSize, void *dataArray);

int rd_enqueue(rd_queue_t *q, void *element);
int rd_dequeue(rd_queue_t *q, void *element);
int isQueueEmpty(rd_queue_t *q);


#endif