// Copyright 2018 plutoo
#pragma once

#include "hk/Result.h"
#include "hk/svc/api.h"
#include "hk/svc/types.h"
#include "hk/types.h"
#include "kernel_wait.h"
#include <mutex>

typedef struct WaiterNode WaiterNode;

struct WaiterNode {
    WaitableNode node;
    Waitable* parent;
    hk::svc::Handle thread;
    s32* idx_out;
    s32 idx;
};

struct WaitableMethods {
    bool (*beginWait)(Waitable* ww, WaiterNode* w, u64 cur_tick, u64* next_tick);
    hk::Result (*onTimeout)(Waitable* ww, u64 old_tick);
    hk::Result (*onSignal)(Waitable* ww);
};

static inline void _waitableInitialize(Waitable* ww, const WaitableMethods* vt) {
    ww->vt = vt;
    ww->list.next = &ww->list;
    ww->list.prev = &ww->list;
}

static inline void _waitableSignalAllListeners(Waitable* ww) {
    WaitableNode* node = &ww->list;
    WaitableNode* end = node;

    while (node->next != end) {
        node = node->next;
        WaiterNode* w = (WaiterNode*)node;

        // Try to swap -1 => idx on the waiter thread.
        // If another waitable signals simultaneously only one will win the race and insert its own idx.
        s32 minus_one = -1;
        bool sent_idx = __atomic_compare_exchange_n(
            w->idx_out, &minus_one, w->idx, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

        if (sent_idx)
            hk::svc::CancelSynchronization(w->thread);
    }
}

static inline void _waiterNodeInitialize(
    WaiterNode* w, Waitable* parent, hk::svc::Handle thread,
    s32 idx, s32* idx_out) {
    // Initialize WaiterNode fields
    w->parent = parent;
    w->thread = thread;
    w->idx = idx;
    w->idx_out = idx_out;
}

static inline void _waiterNodeAdd(WaiterNode* w) {
    // Add WaiterNode to the parent's linked list
    w->node.next = w->parent->list.next;
    w->parent->list.next = &w->node;
    w->node.prev = &w->parent->list;
}

static inline void _waiterNodeRemove(WaiterNode* w) {
    auto lock(w->parent->mutex.lockScoped());
    w->node.prev->next = w->node.next;
    w->node.next->prev = w->node.prev;
}
