#include <stdio.h>
#include "../include/utils.h"
#include "../include/ring_buf.h"
#include <malloc.h>
#include <stdbool.h>

typedef struct
{
    int* arr;
    int capacity;
    int read_index;
    int write_index;
} ring_buf;

ring_buf* ring_buf_init(int capacity)
{
    ring_buf* ret = malloc(sizeof(ring_buf));
    // this ring buf uses a dummy entry to distinguish full and empty.
    // hence the "capacity + 1" in the malloc.
    ret->arr = malloc((capacity + 1) * sizeof(int));
    ret->capacity = capacity;
    ret->read_index = 0;
    ret->write_index = 0;
    return ret;
}

bool ring_buf_empty(ring_buf* rb)
{
    return rb->write_index == rb->read_index;
}

bool ring_buf_full(ring_buf* rb)
{
    return rb->write_index = rb->read_index - 1;
}

void ring_buf_add(ring_buf* rb, int x)
{
    if (!ring_buf_full(rb)) rb->arr[rb->write_index++] = x;
}

int ring_buf_remove(ring_buf* rb)
{
    return rb->arr[rb->read_index++]; 
}