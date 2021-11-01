#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "..\include\ring_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// <summary>
    /// Rb_new the specified buffer_len.
    /// </summary>
    /// <param name="buffer_len">The buffer_len.</param>
    /// <param name="pointer_size">The pointer_size.</param>
    /// <returns></returns>
    /// <version author="André Cachopas" date="05/11/2015" version="1.0" machine="TV-HOMELAND"></version> 
    ring_t* //buffern len is the number of pointer we want allocate, the ptr size is the maximum size of each ptr.
    rb_new(unsigned int buffer_len, unsigned int pointer_size) {
        if (buffer_len == 0  || pointer_size == 0)
            return NULL;

        ring_t* rb = (ring_t*)malloc(sizeof(ring_t));
        if (rb == NULL) 
            return NULL;
        memset(rb, 0, sizeof(ring_t));
        printf("\nInside of the ring buffer, ptrsize= %d , buffer len= %d\n", pointer_size, buffer_len);

        rb->size = buffer_len * pointer_size;
        // increasing one because it starts on zero.
        rb->buffer_start = (uint8_t*)malloc(rb->size);
        memset(rb->buffer_start, 0, rb->size);

        rb->buffer_end = rb->buffer_start + (rb->size -1);
        rb->head = rb->buffer_start;
        rb->tail =  rb->buffer_start;

        return rb;
    }

    /// <summary>
    /// Checks if the rb is full.
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <returns></returns>
    /// <version author="André Cachopas" date="05/11/2015" version="1.0" machine="TV-HOMELAND"></version> 
    int64_t
    rb_is_full(ring_t* rb) {
        if (rb->head == rb->tail  && rb->round_complete == 1)//it should be == instead of <=
            return 1;//true
        else
            return 0;//false
    }

    /// <summary>
    /// check if the buffer is empty
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <returns></returns>
    /// <version author="André Cachopas" date="05/11/2015" version="1.0" machine="TV-HOMELAND"></version> 
    int64_t
    rb_is_empty(ring_t* rb) {
        if (rb->head == rb->tail  && rb->round_complete == 0)
            return 1;//true
        else
            return 0;//false
    }

    /// <summary>
    /// Rb_writes the specified rb.
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <param name="data">The data.</param>
    /// <param name="size">The size.</param>
    /// <returns></returns>
    /// <version author="André Cachopas" date="05/11/2015" version="1.0" machine="TV-HOMELAND"></version> 
    int64_t
    rb_write(ring_t* rb, uint8_t *data, int64_t size) {
        if (rb == NULL || rb->buffer_start == NULL || size <= 0 || data == NULL) {
            return 0;
        }
        if (1 == rb_is_full(rb)) {
            printf("writer: Is full. Read calls:%lld  Write calls:%lld\n", rb->read_calls, rb->write_calls);
            return 0;
        }

        const int64_t available_space = rb_get_free_space(rb);
        if (available_space < size) {
            //printf("writer: no more space. Read calls:%lld  Write calls:%lld\n", rb->read_calls , rb->write_calls);
            //printf("writer: Round:%d - Head:%p - Tail:%p\n", rb->round_complete, rb->head, rb->tail);
            return 0;
        }
        rb->write_calls++;

        if (rb->round_complete == 1) {
            //we already know that we have space, and we can't overlap.
            //printf("writer: round is complete.\n");
            memcpy(rb->head, data, size);
            rb->head = rb->head + size;
            return size;
        }

        const int64_t remaining_free_space_until_end = rb->buffer_end - rb->head;
        int64_t copy_size = size;
        if (remaining_free_space_until_end < size)
            copy_size = remaining_free_space_until_end;

        // copy the data until we reach the end of the buffer.
        memcpy(rb->head, data, copy_size);
        rb->head = (rb->head + copy_size);
        if (rb->head == rb->buffer_end) {
            rb->head = rb->buffer_start;
            //printf("writer: round completed -1!\n");
            rb->round_complete = 1;
        }

        if (copy_size == size)
            return size;
        // copy the remaining data from the start.
        copy_size = size - remaining_free_space_until_end;
        memcpy(rb->head, (data + remaining_free_space_until_end), copy_size);
        rb->head = (rb->head + copy_size);
        if (rb->head == rb->buffer_end) {
            rb->head = rb->buffer_start;
            //printf("writer: round completed -2!\n");
            rb->round_complete = 1;
        }
        return size;
    }

    /// <summary>
    /// Rb_reads the specified rb.
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <param name="size">The size.</param>
    /// <param name="memory">The memory.</param>
    /// <returns></returns>
    /// <version author="André Cachopas" date="05/11/2015" version="1.0" machine="TV-HOMELAND"></version> 
    int64_t
    rb_read(ring_t* rb, uint8_t* memory, int64_t size) {
        if (rb == NULL || rb->buffer_start == NULL) {
            printf("rb_read: rb is null\n");
            return 0;
        }

        if (1 == rb_is_empty(rb)) {
            printf("rb_read:Buffer is empty\n");
            return 0;
        }

        const int64_t size_filled = rb_get_filled_space(rb);
        if (size > size_filled) {
            //printf("rb_read:Nothing data to read from the buffer!\n");
            return 0;
        }
        rb->read_calls++;

        if (rb->round_complete == 0) {
            memcpy(memory, (rb->tail), size);
            rb->tail = rb->tail + size;
            if (rb->tail == rb->buffer_end) {
                rb->tail = rb->buffer_start;
                //printf("rb_reader: resetting round-1!\n");
                rb->round_complete = 0;
            }
            return size;
        } 

        const int64_t remaining_space = rb->buffer_end - rb->tail;
        int64_t copied_size = size;
        if (remaining_space < size) {
            copied_size = remaining_space;
        }
        memcpy(memory, (rb->tail), copied_size);
        rb->tail += copied_size;

        if (rb->tail == rb->buffer_end) {
            rb->tail = rb->buffer_start;
            //printf("rb_reader: resetting round-2!\n");
            rb->round_complete = 0;
        }

        if (copied_size == size)
            return copied_size;

        copied_size = size - remaining_space;
        memcpy((memory+remaining_space), (rb->tail), copied_size);
        rb->tail += copied_size;

        // this shouldn't be needed here.
        if (rb->tail == rb->buffer_end) {
            rb->tail = rb->buffer_start;
            printf("reader: resetting round-3!\n");
            rb->round_complete = 0;
        }

        return size;
    }

    /// <summary>
    /// Rb_free_spaces the specified rb.
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <returns></returns>
    /// <version author="André Cachopas" date="05/11/2015" version="1.0" machine="TV-HOMELAND"></version> 
    int64_t
    rb_get_free_space(ring_t * rb) {
        if (rb == NULL)
            return 0;
        if (rb->round_complete == 1) {
            return (rb->tail - rb->head);
        } else {
            int64_t free_space = (rb->buffer_end - rb->head) + (rb->tail - rb->buffer_start);
            //if they are in the same position it means the the whole buffer is free
            if (free_space == 0)
                free_space = rb->size;

            return free_space;
        }
    }

    /// <summary>
    /// Retrieves the amount of bytes available in the buffer.
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <returns></returns>
    /// <version author="André Cachopas" date="18-Aug-20"  machine="TV-MRROBOT"></version> 
    int64_t
    rb_get_filled_space(ring_t* rb) {
        if (rb == NULL)
            return 0;
        if (rb->round_complete == 1) {
            return (rb->buffer_end - rb->tail) + (rb->head - rb->buffer_start);
        } else {
            return (rb->head - rb->tail);
        }
    }

    /// <summary>
    /// Resets the ring buffer to the start conditions.
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <version author="André Cachopas" date="20-Aug-20"  machine="TV-MRROBOT"></version> 
    void
    rb_reset(ring_t* rb) {
        if (rb == NULL)
            return;

        rb->head = rb->tail = rb->buffer_start;
        rb->round_complete = 0;
    }

    /// <summary>
    /// Rb_releases the specified rb.
    /// </summary>
    /// <param name="rb">The rb.</param>
    /// <version author="André Cachopas" date="05/11/2015" version="1.0" machine="TV-HOMELAND"></version> 
    void
    rb_release(ring_t * rb) {
        if (rb == NULL)
            return;

        free(rb->buffer_start);
        free(rb);
    }

#ifdef __cplusplus
}
#endif