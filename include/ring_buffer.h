#pragma once
#include <stdint.h>

typedef struct ring_t
{
    uint8_t *buffer_start;  // Address where the buffer starts
    uint8_t *buffer_end;    // Address where the buffer ends
    uint8_t *head;          // Address where thr writter starts writting
    uint8_t *tail;          // Address where the reader stars

    uint64_t size;          // Size of buffer
    uint64_t read_calls;
    uint64_t write_calls;
    
    char round_complete; //0- false; 1- true
}ring_t;

#ifdef __cplusplus
extern "C" {
#endif

    //buffern len is the number of pointer we want allocate, the ptr size is the maximum size of each ptr.
    ring_t* rb_new(uint32_t buffer_len, uint32_t pointer_size);

    int64_t rb_is_full(ring_t* rb);

    int64_t rb_is_empty(ring_t* rb);

    int64_t rb_write(ring_t* rb, uint8_t *data, int64_t size);

    int64_t rb_read(ring_t* rb, uint8_t* memory, int64_t size);

    int64_t rb_get_free_space(ring_t * rb);

    int64_t rb_get_filled_space(ring_t* rb);

    void rb_reset(ring_t* rb);

    void rb_release(ring_t * rb);


#ifdef __cplusplus
}
#endif

