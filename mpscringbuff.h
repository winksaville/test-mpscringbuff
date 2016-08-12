/**
 * This software is released into the public domain.
 *
 * A MpscRingBuff is a wait free/thread safe multi-producer
 * single consumer ring buffer.
 *
 * The ring buffer has a head and tail, the elements are added
 * to the head removed from the tail.
 */

#ifndef COM_SAVILLE_MPSCRINGBUFF_H
#define COM_SAVILLE_MPSCRINGBUFF_H

#include <stdbool.h>
#include <stdint.h>

/** mfence instruction */
static inline void mfence(void) {
  __asm__ volatile ("mfence": : :"memory");
}

/** lfence instruction */
static inline void lfence(void) {
  __asm__ volatile ("lfence": : :"memory");
}

/** sfence instruction */
static inline void sfence(void) {
  __asm__ volatile ("sfence": : :"memory");
}

typedef struct MpscRingBuff_t MpscRingBuff_t;
typedef struct Msg_t Msg_t;

#define VOLATILE volatile

#define USE_ATOMIC_TYPES 0

typedef struct Msg_t {
  MpscRingBuff_t* pPool;
  MpscRingBuff_t* pRspRb;
  uint64_t arg1;
  uint64_t arg2;
} Msg_t;

typedef struct Cell_t {
#if USE_ATOMIC_TYPES
  _Atomic(uint32_t) seq;
#else
  uint32_t seq;
#endif
  Msg_t* msg;
} Cell_t;

typedef struct MpscRingBuff_t {
#if USE_ATOMIC_TYPES
  _Atomic(uint32_t) VOLATILE add_idx __attribute__(( aligned (64) ));
  uint32_t VOLATILE rmv_idx __attribute__(( aligned (64) ));
#else
  uint32_t VOLATILE add_idx __attribute__(( aligned (64) ));
  uint32_t VOLATILE rmv_idx __attribute__(( aligned (64) ));
#endif
  uint32_t size;
  uint32_t mask;
  Cell_t* ring_buffer;
  VOLATILE _Atomic(uint32_t) count;
  uint64_t msgs_processed;
} MpscRingBuff_t;

extern _Atomic(uint64_t) gTick;

#define LDR "%6ld %lx  "
#define ldr() ++gTick, pthread_self()

#define CRASH() do { *((volatile uint8_t*)0) = 0; } while(0)


/**
 * Initialize the MpscRingBuff_t, size must be a power of two.
 *
 * @return NULL if an error if size cannot be malloced or is not a power of 2.
 */
extern MpscRingBuff_t* initMpscRingBuff(MpscRingBuff_t* pRb, uint32_t size);

/**
 * Deinitialize the MpscRingBuff_t, assumes the ring buffer is empty.
 *
 * @return number of messages removed.
 */
extern uint64_t deinitMpscRingBuff(MpscRingBuff_t *pQ);

/**
 * Add a Msg_t to the ring buffer. This maybe used by multiple
 * entities on the same or different thread. This will never
 * block as it is a wait free algorithm.
 */
extern void add(MpscRingBuff_t *pQ, Msg_t *pMsg);

/**
 * Add a Msg_t to the ring buffer return true if added false if
 * it would have blocked.
 */
extern bool add_non_blocking(MpscRingBuff_t* pRb, Msg_t* pMsg);

/**
 * Remove a Msg_t from the ring buffer. This maybe used only by
 * a single thread and returns NULL if empty or would
 * have blocked.
 */
extern Msg_t *rmv_non_stalling(MpscRingBuff_t *pQ);

/**
 * Remove a Msg_t from the ring buffer. This maybe used only by
 * a single thread and returns NULL if empty. This may
 * stall if a producer call add and was preempted before
 * finishing.
 */
extern Msg_t *rmv(MpscRingBuff_t *pQ);

/**
 * Remove a Msg_t from the ring buffer DO NOT PRINT DBG output if empty.
 * This maybe used only by a single thread and returns NULL if empty.
 * This may stall if a producer call add and was preempted before
 * finishing.
 */
extern Msg_t *rmv_no_dbg_on_empty(MpscRingBuff_t *pQ);

/**
 * Return the message to its pool.
 */
extern void ret_msg(Msg_t* pMsg);

/**
 * Send a response arg1 if the msg->pRspQ != NULL otherwise ret msg
 */
extern void send_rsp_or_ret(Msg_t* msg, uint64_t arg1);

#endif
