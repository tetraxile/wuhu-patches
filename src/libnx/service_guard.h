#pragma once

#include <mutex>

#include "hk/os/Mutex.h"
#include "hk/types.h"

typedef struct ServiceGuard {
    hk::os::Mutex mutex;
    u32 refCount;
} ServiceGuard;

inline bool serviceGuardBeginInit(ServiceGuard* g) {
    g->mutex.lock();
    return (g->refCount++) == 0;
}

inline hk::Result serviceGuardEndInit(ServiceGuard* g, hk::Result rc, void (*cleanupFunc)(void)) {
    if (rc.failed()) {
        cleanupFunc();
        --g->refCount;
    }
    g->mutex.unlock();
    return rc;
}

inline void serviceGuardExit(ServiceGuard* g, void (*cleanupFunc)(void)) {
    std::scoped_lock lock(g->mutex);
    if (g->refCount && (--g->refCount) == 0)
        cleanupFunc();
}

#define NX_GENERATE_SERVICE_GUARD_PARAMS(name, _paramdecl, _parampass)      \
                                                                            \
    static ServiceGuard g_##name##Guard;                                    \
    inline hk::Result _##name##Initialize _paramdecl;                       \
    static void _##name##Cleanup(void);                                     \
                                                                            \
    hk::Result name##Initialize _paramdecl {                                \
        hk::Result rc = 0;                                                  \
        if (serviceGuardBeginInit(&g_##name##Guard))                        \
            rc = _##name##Initialize _parampass;                            \
        return serviceGuardEndInit(&g_##name##Guard, rc, _##name##Cleanup); \
    }                                                                       \
                                                                            \
    void name##Exit(void) {                                                 \
        serviceGuardExit(&g_##name##Guard, _##name##Cleanup);               \
    }

#define NX_GENERATE_SERVICE_GUARD(name) NX_GENERATE_SERVICE_GUARD_PARAMS(name, (void), ())
