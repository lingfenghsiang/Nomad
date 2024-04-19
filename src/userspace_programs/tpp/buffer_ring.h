#ifndef _BUFFER_RING_H
#define _BUFFER_RING_H
#include <stdint.h>
struct head_tail {
  volatile uint64_t head;
  volatile uint64_t tail;
};

struct ring_queue {
  struct head_tail prod;
  struct head_tail cons;
};
#ifdef __cplusplus
extern "C" {
#endif
// Notion: The capacity of the ring queue must be the power of 2!
uint32_t ring_queque_produce_begin(struct ring_queue *buffer, uint32_t capacity,
                                   uint32_t n, uint64_t *head, uint64_t *next,
                                   uint32_t *free_space);

void ring_queque_produce_end(struct ring_queue *buffer, uint64_t *head,
                             uint64_t *next);

uint32_t ring_queque_consume_begin(struct ring_queue *buffer, uint32_t capacity,
                                   uint32_t n, uint64_t *head, uint64_t *next,
                                   uint32_t *available);

void ring_queque_consume_end(struct ring_queue *buffer, uint64_t *head,
                             uint64_t *next);
#ifdef __cplusplus
}
#endif

#endif