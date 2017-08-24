/*
 * (c) 2017 Jan Ruh <jan.ruh@student.kit.edu>
 *
 * This file is distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "ringbuffer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


#define RINGBUFFER_HEAD(ringbuffer) ({ \
        &ringbuffer->data[ringbuffer->offset_head]; \
        })

#define RINGBUFFER_TAIL(ringbuffer) ({ \
        &ringbuffer->data[ringbuffer->offset_tail]; \
        })

static inline void ringbuffer_lock(ringbuffer_t *ringbuffer)
{
    pthread_mutex_lock(&ringbuffer->mutex);
}

static inline void ringbuffer_unlock(ringbuffer_t *ringbuffer)
{
    pthread_mutex_unlock(&ringbuffer->mutex);
}

void ringbuffer_dbg_print(ringbuffer_t ringbuffer)
{
    printf("ringbuffer {\n"
          "\tdata = 0x%lx,\n"
          "\tsize = %d byte,\n"
          "\toffset head = %ld,\n"
          "\toffset tail = %ld,\n"
          "\tmutex state = 0x%x}\n",
          (unsigned long) ringbuffer.data,
          ringbuffer.size,
          (unsigned long) ringbuffer.offset_head,
          (unsigned long) ringbuffer.offset_tail,
          ringbuffer.mutex.__m_lock.__status);
}

ringbuffer_t *ringbuffer_create(unsigned char *buffer,
                                unsigned size)
{
    ringbuffer_t *ringbuffer = (ringbuffer_t*) buffer;

    ringbuffer->size = size - sizeof(ringbuffer_t);
    ringbuffer->offset_head = 0;
    ringbuffer->offset_tail = 0;

    pthread_mutex_init(&ringbuffer->mutex, NULL);

    return ringbuffer;
}

unsigned ringbuffer_write(unsigned char *data,
                          unsigned size,
                          ringbuffer_t *ringbuffer)
{
    int ret;
    unsigned free;
    unsigned contiguous;
    unsigned cutoff = 0;

    ringbuffer_lock(ringbuffer);

    if (ringbuffer->offset_head == ringbuffer->offset_tail
            && ringbuffer->offset_head == 0) {
        // first write
        free = ringbuffer->size;
        contiguous = free;

        ringbuffer->offset_head = 0;
        ringbuffer->offset_tail = 0;
    } else if (ringbuffer->offset_head < ringbuffer->offset_tail) {
        contiguous = ringbuffer->size - ringbuffer->offset_tail;
        cutoff = ringbuffer->offset_head;
        free = contiguous + cutoff;
    } else {
        free = ringbuffer->offset_head - ringbuffer->offset_tail;
        contiguous = free;
    }

    ringbuffer_unlock(ringbuffer);

    if (free == 0) {
        return 0;
    } else if (size > free) {
        fprintf(stdout, "Warning: only %u bytes free.\n", free);
        size = free;
    }

    ringbuffer_lock(ringbuffer);

    if (cutoff == 0) {
        memcpy(RINGBUFFER_TAIL(ringbuffer), data, size);

        ringbuffer->offset_tail += size;

        ret = size;
    } else {
        memcpy(RINGBUFFER_TAIL(ringbuffer), data, contiguous);

        ringbuffer->offset_tail = 0;

        ret = contiguous + ringbuffer_write(data + contiguous, cutoff, ringbuffer);
    }

    ringbuffer_unlock(ringbuffer);

    return ret;
}

int ringbuffer_read(unsigned char *buffer,
                    unsigned size,
                    ringbuffer_t *ringbuffer)
{
    int ret;

    if (size > ringbuffer->size) {
        fprintf(stderr, "Error: cannot read %u byte, ringbuffer is only"
                        " of size %u byte\n", size, ringbuffer->size);
        return RB_ERROR_CANT_READ;
    }

    ringbuffer_lock(ringbuffer);

    if (ringbuffer->offset_head < ringbuffer->offset_tail) {
        size = ringbuffer->offset_head + size <= ringbuffer->offset_tail ?
            size : ringbuffer->offset_tail - ringbuffer->offset_head;
        memcpy(buffer, RINGBUFFER_HEAD(ringbuffer), size);

        ringbuffer->offset_head += size;
        ringbuffer->offset_tail =
            ringbuffer->offset_head < ringbuffer->offset_tail ?
            ringbuffer->offset_tail : ringbuffer->offset_head;

        ret = size;
    } else if (ringbuffer->offset_head > ringbuffer->offset_tail
            && ringbuffer->offset_head + size <= ringbuffer->size) {
        
        memcpy(buffer, RINGBUFFER_HEAD(ringbuffer), size);

        ringbuffer->offset_head =
            ringbuffer->offset_head + size < ringbuffer->size ?
            ringbuffer->offset_head + size : 0;

        ret = size;
    } else if (ringbuffer->offset_head > ringbuffer->offset_tail
            && ringbuffer->offset_head + size > ringbuffer->size) {
        unsigned tmp_size = ringbuffer->size - ringbuffer->offset_head;
        memcpy(buffer, RINGBUFFER_HEAD(ringbuffer), tmp_size);
        
        ringbuffer->offset_head = 0;

        ret = tmp_size + ringbuffer_read(buffer + tmp_size,
                size - tmp_size, ringbuffer);
    } else {
        ret = 0;
    }

    ringbuffer_unlock(ringbuffer);

    return ret;
}
