// Copyright 2018 plutoo

#include "wait.h"
#include "hk/Result.h"
#include "hk/diag/diag.h"
#include "hk/svc/api.h"
#include "hk/svc/types.h"
#include "hk/types.h"
#include "kernel_wait.h"

typedef hk::Result (*WaitImplFunc)(s32* idx_out, const void* objects, s32 num_objects, u64 timeout);

static hk::Result _waitObjectsImpl(s32* idx_out, const Waiter* objects, u32 num_objects, u64 timeout) {
    HK_ASSERT(num_objects <= 0x40);

    hk::svc::Handle own_thread_handle = hk::svc::getTLS()->nnsdk_thread_ptr->handle;
    hk::svc::Handle dummy_handle = own_thread_handle;
    hk::Result rc;

    hk::svc::Handle handles[num_objects];
    u64 cur_tick = hk::svc::getSystemTick();

    s32 triggered_idx = -1;
    u64 waiters_added = 0;
    WaiterNode waiters[num_objects];

    u64 end_tick = UINT64_MAX;
    s32 end_tick_idx = -1;
    size_t i;

    if (timeout != UINT64_MAX)
        end_tick = armNsToTicks(timeout);

    for (i = 0; i < num_objects; i++) {
        const Waiter* obj = &objects[i];
        u64 next_tick;
        bool added;

        switch (obj->type) {
        case WaiterType_Handle:
        case WaiterType_HandleWithClear:
            // Add (real) handle to the array.
            handles[i] = obj->handle;
            break;

        case WaiterType_Waitable:
            // Try to wait on the object. If it doesn't add a listener for this thread then
            // it means the object is signalled and we're already done.
            next_tick = UINT64_MAX;
            _waiterNodeInitialize(&waiters[i], obj->waitable, own_thread_handle, i, &triggered_idx);
            added = obj->waitable->vt->beginWait(obj->waitable, &waiters[i], cur_tick, &next_tick);
            if (!added) {
                *idx_out = i;
                rc = 0;
                goto clean_up;
            }

            // Otherwise, override the user-supplied timeout if the object specified an earlier timeout.
            if (next_tick < end_tick) {
                end_tick = next_tick;
                end_tick_idx = i;
            }

            // Add (fake) handle to the array.
            waiters_added |= 1UL << i;
            handles[i] = dummy_handle;
            break;
        }
    }

    // Do the actual syscall.
    rc = hk::svc::WaitSynchronization(idx_out, handles, num_objects, end_tick == UINT64_MAX ? UINT64_MAX : armTicksToNs(end_tick));

    if (rc.succeeded()) {
        // Wait succeded, so that means an object having a real handle was signalled.
        // Perform autoclear if needed.
        if (objects[*idx_out].type == WaiterType_HandleWithClear) {
            // Try to auto-clear the event. If it is not signalled, the kernel
            // will return an error and thus we need to retry the wait.
            rc = hk::svc::ResetSignal(handles[*idx_out]);
            if (rc.getValue() == 125)
                rc = 118;
        }
    } else if (rc.getValue() == 117) {
        // If we hit the user-supplied timeout, we return the timeout error back to caller.
        if (end_tick_idx == -1)
            goto clean_up;

        // If not, it means an object triggered the timeout; handle it.
        Waitable* w = objects[end_tick_idx].waitable;
        rc = w->vt->onTimeout(w, end_tick + cur_tick);
        if (rc.succeeded())
            *idx_out = end_tick_idx;
    } else if (rc.getValue() == 118) {
        // If no listener filled in its own index, we return the cancelled error back to caller.
        // This only happens if user for some reason manually does a svcCancelSynchronization.
        // Check just in case.
        if (triggered_idx == -1)
            goto clean_up;

        // An object was signalled, handle it.
        Waitable* w = objects[triggered_idx].waitable;
        rc = w->vt->onSignal(w);
        if (rc.succeeded())
            *idx_out = triggered_idx;
    }

clean_up:
    // Remove listeners.
    for (i = 0; i < num_objects; i++)
        if (waiters_added & (1UL << i))
            _waiterNodeRemove(&waiters[i]);

    return rc;
}

static hk::Result _waitLoop(s32* idx_out, const void* objects, s32 num_objects, u64 timeout, WaitImplFunc waitfunc) {
    hk::Result rc;
    bool has_timeout = timeout != UINT64_MAX;
    u64 deadline = 0;

    if (has_timeout)
        deadline = hk::svc::getSystemTick() + armNsToTicks(timeout); // timeout: ns->ticks

    do {
        u64 this_timeout = UINT64_MAX;
        if (has_timeout) {
            s64 remaining = deadline - hk::svc::getSystemTick();
            this_timeout = remaining > 0 ? armTicksToNs(remaining) : 0; // ticks->ns
        }

        rc = waitfunc(idx_out, objects, num_objects, this_timeout);
        if (has_timeout && rc.getValue() == 117)
            break;
    } while (rc.getValue() == 118);

    return rc;
}

hk::Result waitObjects(s32* idx_out, const Waiter* objects, s32 num_objects, u64 timeout) {
    return _waitLoop(idx_out, objects, num_objects, timeout, (WaitImplFunc)_waitObjectsImpl);
}

hk::Result waitHandles(s32* idx_out, const hk::svc::Handle* handles, s32 num_handles, u64 timeout) {
    return _waitLoop(idx_out, handles, num_handles, timeout, (WaitImplFunc)hk::svc::WaitSynchronization);
}
