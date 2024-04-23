/**
 * @file buffer_ring.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-02-13
 * different from normal buffer ring, this one is used in
 * our file system and we use dax driver to do write operations
 *  each time, the writable region must not exceed the boundary
 * of the ring buffer. Therefore a write that crosses the buffer ring
 * boundary will be split into two.
 * @copyright Copyright (c) 2023
 *
 */

#include "buffer_ring.h"

#include <linux/atomic.h>
#include <linux/processor.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>
// #include "kernel_func.h"

#define MPLOCKED "lock ; "

#define ROUNDUP(x, y) ((x + ((y)-1)) & ~((y)-1))

static inline int atomic64_cmpset(volatile uint64_t *dst, uint64_t exp,
                                  uint64_t src) {
  uint8_t res;

  asm volatile(MPLOCKED
               "cmpxchg %[src], %[dst];"
               "sete %[res];"
               : [res] "=a"(res), /* output */
                 [dst] "=m"(*dst)
               : [src] "r"(src), /* input */
                 "a"(exp), "m"(*dst)
               : "memory"); /* no-clobber list */
  return res;
}

static int ring_queue_move_prod_head(struct ring_queue *buffer,
                                     uint32_t capacity, uint32_t n,
                                     uint64_t *old_head, uint64_t *new_head,
                                     uint32_t *free_entries) {
  //   atomic_t *prod_head_ptr = &buffer->prod.head;
  uint32_t max = n;
  int success;
  do {
    n = max;
    *old_head = buffer->prod.head;
    smp_rmb();
    *free_entries = capacity + buffer->cons.tail - *old_head;
    if (n > *free_entries) n = *free_entries;

    if (n == 0) return 0;
    *new_head = *old_head + n;
    success = atomic64_cmpset(&buffer->prod.head, *old_head, *new_head);
    // success = __sync_bool_compare_and_swap()
  } while (success == 0);
  return n;
}

/**
 * @brief the move cons code not tested
 *
 */
static int ring_queue_move_cons_head(struct ring_queue *buffer,
                                     uint32_t capacity, uint32_t n,
                                     uint64_t *old_head, uint64_t *new_head,
                                     uint32_t *entries) {
  uint32_t max = n;
  int success;
  do {
    n = max;
    *old_head = buffer->cons.head;
    smp_rmb();

    *entries = (buffer->prod.tail - *old_head);

    if (n > *entries) n = *entries;
    if (n == 0) return 0;
    *new_head = *old_head + n;
    success = atomic64_cmpset(&buffer->cons.head, *old_head, *new_head);
  } while (success == 0);
  return n;
}

void wait_until_equal_64(volatile uint64_t *addr, uint64_t expected,
                         int memorder) {
  spin_until_cond(__atomic_load_n(addr, __ATOMIC_RELAXED) == expected);
}

void update_tail(struct head_tail *ht, uint64_t old_val, uint64_t new_val,
                 char enqueue) {
  if (enqueue)
    smp_wmb();
  else
    smp_rmb();
  wait_until_equal_64(&ht->tail, old_val, __ATOMIC_RELAXED);
  ht->tail = new_val;
}

/**
 * @brief
 *
 * @param buffer
 * @param capacity
 * @param n
 * @param head
 * @param next
 * @param free_space returns the amount of space after the enqueue operation has
 * finished
 * @return uint32_t
 */
uint32_t ring_queque_produce_begin(struct ring_queue *buffer, uint32_t capacity,
                                   uint32_t n, uint64_t *head, uint64_t *next,
                                   uint32_t *free_space, spinlock_t *lock) {
  uint32_t free_entries, requested_size;
  spin_lock(lock);
  requested_size =
      ring_queue_move_prod_head(buffer, capacity, n, head, next, &free_entries);
  if (free_space) {
    *free_space = free_entries - requested_size;
  }
  spin_unlock(lock);
  return requested_size;
};

void ring_queque_produce_end(struct ring_queue *buffer, uint64_t *head,
                             uint64_t *next, spinlock_t *lock) {
  spin_lock(lock);
  update_tail(&buffer->prod, *head, *next, 1);
  spin_unlock(lock);
};

/**
 * @brief
 *
 * @param buffer
 * @param capacity
 * @param n
 * @param head
 * @param next
 * @param available the number of remaining entries after dequeue is finished
 * @return uint32_t the number of elements requested
 */
uint32_t ring_queque_consume_begin(struct ring_queue *buffer, uint32_t capacity,
                                   uint32_t n, uint64_t *head, uint64_t *next,
                                   uint32_t *available, spinlock_t *lock) {
  uint32_t enqueued_entries, requested_size;
  spin_lock(lock);
  requested_size = ring_queue_move_cons_head(buffer, capacity, n, head, next,
                                             &enqueued_entries);
  if (available) {
    *available = enqueued_entries - requested_size;
  }
  spin_unlock(lock);
  return requested_size;
};

void ring_queque_consume_end(struct ring_queue *buffer, uint64_t *head,
                             uint64_t *next, spinlock_t *lock) {
	spin_lock(lock);
	update_tail(&buffer->cons, *head, *next, 0);
  spin_unlock(lock);
};