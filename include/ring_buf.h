#ifndef RING_BUF_H
#define RING_BUF_H

#include <stdbool.h>

typedef struct
{
    int* arr;
    int capacity;
    int read_index;
    int write_index;
} ring_buf;

ring_buf* ring_buf_init(int capacity);

bool ring_buf_empty(ring_buf* rb);

bool ring_buf_full(ring_buf* rb);

void ring_buf_add(ring_buf* rb, int x);

int ring_buf_remove(ring_buf* rb);

void ring_buf_destroy(ring_buf* rb);

#endif // RING_BUF_H