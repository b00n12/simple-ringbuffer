#pragma once

#include <pthread.h>

#define RB_ERROR_CANT_READ  -1


typedef struct ringbuffer {
    unsigned size;

    unsigned offset_head;    // offset starting at data to first valid byte
    unsigned offset_tail;    // offset starting at data to first invalid byte

    pthread_mutex_t mutex;

    unsigned char data[];
} ringbuffer_t;

/*
 * Read a specific number of bytes from the ringbuffer into the user defined
 * target buffer.
 *
 * buffer:      target buffer for the read data
 * size:        number of bytes to read at max from the ringbuffer into the
 *              target buffer
 * ringbuffer:  ringbuffer instance to use
 *
 * returns:     < 0 in case of an error, or the number of bytes read from
 *              the ringbuffer
 */
int ringbuffer_read(unsigned char *buffer,
                    unsigned size,
                    ringbuffer_t *ringbuffer);

/*
 * Write a specific number of bytes to the ringbuffer. If number of bytes
 * supposed to be written to ringbuffer exceed the space left  write available
 * number of bytes an print warning to stdout.
 *
 * data:        start address of the data to write to the ringbuffer
 * size:        size of the data to write to the ringbuffer
 * ringbuffer:  ringbuffer instance to use
 *
 * returns:     number of bytes written to ringbuffer
 */
unsigned ringbuffer_write(unsigned char *data,
                          unsigned size,
                          ringbuffer_t *ringbuffer);

/*
 * Prints the state state of the of the ringbuffer structure in a human
 * readable way.
 *
 * ringbuffer: ringbuffer to use
 */
void ringbuffer_dbg_print(ringbuffer_t ringbuffer);

/*
 * Creates and initializes a new ringbuffer in the existing memory area passed
 * as an argument. The size of the new ringbuffer is equivalent to the buffer
 * size minus the size of the ringbuffer structure.
 *
 * buffer:          start address of memory region to use for the ringbuffer
 * size:            size of the memory region to use for the ringbuffer
 * lock, unlock:    function pointer to the implementation of the synchronization
 *                  mechansim
 *
 *
 * returns: new ringbuffer_t instance
 */
ringbuffer_t *ringbuffer_create(unsigned char *buffer,
                                unsigned size);
