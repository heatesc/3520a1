#include <stdio.h>
#include "../include/utils.h"
#include "../include/ring_buf.h"
#include <malloc.h>
#include <stdbool.h>


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
    return (rb->write_index + 1) % (rb->capacity + 1) == rb->read_index;
}

void ring_buf_add(ring_buf* rb, int x)
{
    if (!ring_buf_full(rb))
    {
        rb->arr[rb->write_index] = x;
        rb->write_index = (rb->write_index + 1) % (rb->capacity + 1);
    }
}

int ring_buf_remove(ring_buf* rb)
{
    int ret = rb->arr[rb->read_index];
    rb->read_index = (rb->read_index + 1) % (rb->capacity + 1);
    return ret;
}

void ring_buf_destroy(ring_buf* rb)
{
    free(rb->arr);
    free(rb);
}