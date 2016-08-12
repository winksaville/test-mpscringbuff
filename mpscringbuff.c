/**
 * This software is released into the public domain.
 *
 * A MpscRingBuff is a wait free/thread safe multi-producer
 * single consumer ring buffer.
 *
 * The ring buffer has a head and tail, the elements are added
 * to the head removed from the tail.
 */

//#define NDEBUG

#define _DEFAULT_SOURCE

#define DELAY 0

#ifndef NDEBUG
#define COUNT
#endif

#include "mpscringbuff.h"
#include "dpf.h"

#include <sys/types.h>
#include <pthread.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <memory.h>


/**
 * @see mpscringbuff.h
 */
MpscRingBuff_t* initMpscRingBuff(MpscRingBuff_t* pRb, uint32_t size) {
  DPF(LDR "initMpscRingBuff:+pRb=%p size=%d\n", ldr(), pRb, size);
  pRb->add_idx = 0;
  pRb->rmv_idx = 0;
  pRb->size = size;
  pRb->mask = size - 1;
  pRb->msgs_processed = 0;
  if ((size & pRb->mask) != 0) {
    printf(LDR "initMpscRingBuff:-pRb=%p size=%d not power of 2 return NULL\n", ldr(), pRb, size);
    return NULL;
  }
  pRb->count = 0;
  pRb->ring_buffer = malloc(size * sizeof(*pRb->ring_buffer));
  for (uint32_t i = 0; i < pRb->size; i++) {
    pRb->ring_buffer[i].seq = i;
    pRb->ring_buffer[i].msg = NULL;
  }
  if (pRb->ring_buffer == NULL) {
    printf(LDR "initMpscRingBuff:-pRb=%p size=%d could not allocate ring_buffer return NULL\n", ldr(), pRb, size);
    return NULL;
  }
  DPF(LDR "initMpscRingBuff:-pRb=%p size=%d\n", ldr(), pRb, size);
  return pRb;
}

/**
 * @see mpscringbuff.h
 */
uint64_t deinitMpscRingBuff(MpscRingBuff_t* pRb) {
  DPF(LDR "deinitMpscRingBuff:+pRb=%p\n", ldr(), pRb);
  uint64_t msgs_processed = pRb->msgs_processed;
  free(pRb->ring_buffer);
  pRb->ring_buffer = NULL;
  pRb->add_idx = 0;
  pRb->rmv_idx = 0;
  pRb->size = 0;
  pRb->mask = 0;
  pRb->count = 0;
  pRb->msgs_processed = 0;
  DPF(LDR "deinitMpscRingBuff:-pRb=%p\n", ldr(), pRb);
  return msgs_processed;
}


#if USE_ATOMIC_TYPES

/**
 * @see mpscringbuff.h
 */
bool add_non_blocking(MpscRingBuff_t* pRb, Msg_t* pMsg) {
  Cell_t* cell;
  uint32_t pos = pRb->add_idx;

  DPF(LDR "add_non_blocking:+pRb=%p count=%d msg=%p pool=%p arg1=%lu arg2=%lu\n", ldr(), pRb, pRb->count, pMsg, pMsg->pPool, pMsg->arg1, pMsg->arg2);
  DPF(LDR "add_non_blocking: pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), pRb, pRb->count, pos, pRb->rmv_idx);

  while (true) {

#if DELAY != 0
    usleep(DELAY);
#endif

    cell = &pRb->ring_buffer[pos & pRb->mask];
    uint32_t seq = cell->seq;
    int32_t dif = seq - pos;


    DPF(LDR "add_non_blocking: dif=%d seq=%d pos=%d pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), dif, seq, pos, pRb, pRb->count, pos, pRb->rmv_idx);
    if (dif == 0) {
      // Ring buffer is not full because this cell isn't occuppied
      // Claim this entry by incrementing add_idx which will be
      // the next empty entry.

#if DELAY != 0
      usleep(DELAY);
#endif

      if (__atomic_compare_exchange_n((uint32_t*)&pRb->add_idx, &pos, pos + 1, true, __ATOMIC_ACQ_REL, __ATOMIC_RELEASE)) {
        // Won compare/exchange race with other producers
        DPF(LDR "add_non_blocking: WON dif=%d pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), dif, pRb, pRb->count, pos, pRb->rmv_idx);
        break;
      } else {
        // We lost the race and pos has been updated to lastest value
        DPF(LDR "add_non_blocking: LOST 2 dif=%d pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), dif, pRb, pRb->count, pos, pRb->rmv_idx);
      }
    } else if (dif < 0) {
      // Buffer is full
      DPF(LDR "add_non_blocking: FULL dif=%d pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), dif, pRb, pRb->count, pos, pRb->rmv_idx);
      return false;
    } else {
      // Lost before compare/exchange
      DPF(LDR "add_non_blocking: LOST 1 dif=%d pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), dif, pRb, pRb->count, pos, pRb->rmv_idx);
      pos = pRb->add_idx;
    }
  }

  // Update the data
  cell->msg = pMsg;

  // Update the sequence number
  cell->seq = pos + 1;

#ifdef COUNT
  pRb->count += 1;
#endif

  DPF(LDR "add_non_blocking seq=%d pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), pos + 1, pRb, pRb->count, pRb->add_idx, pRb->rmv_idx);
  DPF(LDR "add_non_blocking-pRb=%p count=%d msg=%p pool=%p arg1=%lu arg2=%lu\n", ldr(), pRb, pRb->count, pMsg, pMsg->pPool, pMsg->arg1, pMsg->arg2);

  return true;
}

#else

// Not using __ATOMIC types and streamlined with no comments or debug
bool add_non_blocking(MpscRingBuff_t* pRb, Msg_t* pMsg) {
  Cell_t* cell;
  uint32_t pos = pRb->add_idx;

  while (true) {
    cell = &pRb->ring_buffer[pos & pRb->mask];
    uint32_t seq = __atomic_load_n(&cell->seq, __ATOMIC_ACQUIRE);
    int32_t dif = seq - pos;

    if (dif == 0) {
      if (__atomic_compare_exchange_n((uint32_t*)&pRb->add_idx, &pos, pos + 1, true, __ATOMIC_ACQ_REL, __ATOMIC_RELEASE)) {
        break;
      }
    } else if (dif < 0) {
      return false;
    } else {
      pos = pRb->add_idx;
    }
  }

  cell->msg = pMsg;
  __atomic_store_n(&cell->seq, pos + 1, __ATOMIC_RELEASE);

  return true;
}

#endif

/**
 * @see mpscringbuff.h
 */
void add(MpscRingBuff_t* pRb, Msg_t* pMsg) {
  while (true) {
    if (add_non_blocking(pRb, pMsg)) {
      return;
    } else {
      sched_yield();
    }
  }
}

#if USE_ATOMIC_TYPES

/**
 * @see mpscringbuff.h
 */
Msg_t* rmv_non_stalling(MpscRingBuff_t* pRb) {
  // Same as rmv
  return rmv(pRb);
}

#else

// Not using __ATOMIC types and streamlined with no comments or debug
Msg_t* rmv_non_stalling(MpscRingBuff_t* pRb) {
  return rmv(pRb);
}

#endif

#if USE_ATOMIC_TYPES

/**
 * @see mpscringbuff.h
 */
Msg_t* rmv(MpscRingBuff_t* pRb) {
  Msg_t* msg;
  Cell_t* cell;
  uint32_t pos = pRb->rmv_idx;

  DPF(LDR "rmv_non_stalling:0+pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), pRb, pRb->count, pRb->add_idx, pos);

  cell = &pRb->ring_buffer[pos & pRb->mask];
  uint32_t seq = cell->seq;
  int32_t dif = seq - (pos + 1);

  if (dif < 0) {
    DPF(LDR "rmv_non_stalling:-msg=NULL pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), pRb, pRb->count, pRb->add_idx, pos);
    return NULL;
  }
  
  msg = cell->msg;
  cell->seq = pos + pRb->mask + 1;

  pRb->rmv_idx += 1;
  pRb->msgs_processed += 1;

  DPF(LDR "rmv_non_stalling:-msg=%p pRb=%p count=%d add_idx=%u rmv_idx=%u\n", ldr(), msg, pRb, pRb->count, pRb->rmv_idx, pos);
  return msg;
}

#else

Msg_t* rmv(MpscRingBuff_t* pRb) {
  Msg_t* msg;
  Cell_t* cell;
  uint32_t pos = pRb->rmv_idx;

  cell = &pRb->ring_buffer[pos & pRb->mask];
  uint32_t seq = __atomic_load_n(&cell->seq, __ATOMIC_ACQUIRE);
  int32_t dif = seq - (pos + 1);

  if (dif < 0) {
    return NULL;
  }
  
  msg = cell->msg;
  __atomic_store_n(&cell->seq, pos + pRb->mask + 1, __ATOMIC_RELEASE);

  pRb->rmv_idx += 1;
  pRb->msgs_processed += 1;

  return msg;
}

#endif

/**
 * @see mpscringbuff.h
 */
Msg_t* rmv_no_dbg_on_empty(MpscRingBuff_t* pRb) {
  Cell_t* cell;
  uint32_t pos = pRb->rmv_idx;

  cell = &pRb->ring_buffer[pos & pRb->mask];
  uint32_t seq = cell->seq;
  int32_t dif = seq - (pos + 1);

  if (dif < 0) {
    return NULL;
  } else {
    return rmv(pRb);
  }
}

/**
 * @see mpscringbuff.h
 */
void ret_msg(Msg_t* pMsg) {
  if ((pMsg != NULL) && (pMsg->pPool != NULL)) {
    add(pMsg->pPool, pMsg);
  } else {
    if (pMsg == NULL) {
      DPF(LDR "ret_msg:#No msg msg=%p\n", ldr(), pMsg);
    } else {
      DPF(LDR "ret_msg:#No pool msg=%p pool=%p arg1=%lu arg2=%lu\n",
          ldr(), pMsg, pMsg->pPool, pMsg->arg1, pMsg->arg2);
    }
  }
}

/**
 * @see mpscringbuff.h
 */
void send_rsp_or_ret(Msg_t* msg, uint64_t arg1) {
  if (msg->pRspRb != NULL) {
    MpscRingBuff_t* pRspRb = msg->pRspRb;
    msg->pRspRb = NULL;
    msg->arg1 = arg1;
    DPF(LDR "send_rsp_or_ret: send pRspRb=%p msg=%p pool=%p arg1=%lu arg2=%lu\n",
        ldr(), pRspRb, msg, msg->pPool, msg->arg1, msg->arg2);
    add(pRspRb, msg);
  } else {
    DPF(LDR "send_rsp_or_ret: no RspRb ret msg=%p pool=%p arg1=%lu arg2=%lu\n",
        ldr(), msg, msg->pPool, msg->arg1, msg->arg2);
    ret_msg(msg);
  }
}


