// Copyright 2017 plutoo
#include "hk/svc/types.h"
#define NX_SERVICE_ASSUME_NON_DOMAIN

#include "hosversion.h"
#include "pm.h"

#define PM_GENERATE_SERVICE_INIT(name)           \
    Service g_pm##name##Srv;                     \
                                                 \
    Service* pm##name##GetServiceSession(void) { \
        return &g_pm##name##Srv;                 \
    }

PM_GENERATE_SERVICE_INIT(dmnt);
PM_GENERATE_SERVICE_INIT(shell);
PM_GENERATE_SERVICE_INIT(info);
PM_GENERATE_SERVICE_INIT(bm);

// pmbm

hk::Result pmbmGetBootMode(PmBootMode* out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "PmBootMode");
    return serviceDispatchOut(&g_pmbmSrv, 0, *out);
}

hk::Result pmbmSetMaintenanceBoot(void) {
    return serviceDispatch(&g_pmbmSrv, 1);
}

static hk::Result _pmCmdInU64OutU64(u64* out, u64 in, Service* srv, u32 cmd_id) {
    return serviceDispatchInOut(srv, cmd_id, in, *out);
}

static hk::Result _pmCmdInU64(u64 in, Service* srv, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, in);
}

static hk::Result _pmCmdVoid(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static hk::Result _pmCmdOutResourceLimitValues(PmResourceLimitValues* out, Service* srv, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

// pmdmnt

hk::Result pmdmntGetJitDebugProcessIdList(u32* out_count, u64* out_pids, size_t max_pids) {
    const u64 cmd_id = hosversionAtLeast(5, 0, 0) ? 0 : 1;
    return serviceDispatchOut(&g_pmdmntSrv, cmd_id, *out_count,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { out_pids, max_pids * sizeof(*out_pids) },
        }, );
}

hk::Result pmdmntStartProcess(u64 pid) {
    return _pmCmdInU64(pid, &g_pmdmntSrv, hosversionAtLeast(5, 0, 0) ? 1 : 2);
}

hk::Result pmdmntGetProcessId(u64* pid_out, u64 program_id) {
    return _pmCmdInU64OutU64(pid_out, program_id, &g_pmdmntSrv, hosversionAtLeast(5, 0, 0) ? 2 : 3);
}

hk::Result pmdmntHookToCreateProcess(Event* out_event, u64 program_id) {
    const u64 cmd_id = hosversionAtLeast(5, 0, 0) ? 3 : 4;
    hk::svc::Handle event = 0;
    hk::Result rc = serviceDispatchIn(&g_pmdmntSrv, cmd_id, program_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event, );
    if (rc.succeeded())
        eventLoadRemote(out_event, event, true);
    return rc;
}

hk::Result pmdmntGetApplicationProcessId(u64* pid_out) {
    const u64 cmd_id = hosversionAtLeast(5, 0, 0) ? 4 : 5;
    return serviceDispatchOut(&g_pmdmntSrv, cmd_id, *pid_out);
}

hk::Result pmdmntHookToCreateApplicationProcess(Event* out_event) {
    const u64 cmd_id = hosversionAtLeast(5, 0, 0) ? 5 : 6;
    hk::svc::Handle event = 0;
    hk::Result rc = serviceDispatch(&g_pmdmntSrv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event, );
    if (rc.succeeded())
        eventLoadRemote(out_event, event, true);
    return rc;
}

hk::Result pmdmntClearHook(u32 which) {
    return serviceDispatchIn(&g_pmdmntSrv, 6, which);
}

hk::Result pmdmntGetProgramId(u64* program_id_out, u64 pid) {
    return _pmCmdInU64OutU64(program_id_out, pid, &g_pmdmntSrv, 7);
}

// pminfo

hk::Result pminfoGetProgramId(u64* program_id_out, u64 pid) {
    return _pmCmdInU64OutU64(program_id_out, pid, &g_pminfoSrv, 0);
}

hk::Result pminfoGetAppletCurrentResourceLimitValues(PmResourceLimitValues* out) {
    return _pmCmdOutResourceLimitValues(out, &g_pminfoSrv, 1);
}

hk::Result pminfoGetAppletPeakResourceLimitValues(PmResourceLimitValues* out) {
    return _pmCmdOutResourceLimitValues(out, &g_pminfoSrv, 2);
}

// pmshell

hk::Result pmshellLaunchProgram(u32 launch_flags, const NcmProgramLocation* location, u64* pid) {
    const struct {
        u32 launch_flags;
        u32 pad;
        NcmProgramLocation location;
    } in = { launch_flags, 0, *location };
    return serviceDispatchInOut(&g_pmshellSrv, 0, in, *pid);
}

hk::Result pmshellTerminateProcess(u64 processID) {
    return _pmCmdInU64(processID, &g_pmshellSrv, 1);
}

hk::Result pmshellTerminateProgram(u64 program_id) {
    return _pmCmdInU64(program_id, &g_pmshellSrv, 2);
}

hk::Result pmshellGetProcessEventHandle(Event* out_event) {
    hk::svc::Handle event = 0;
    hk::Result rc = serviceDispatch(&g_pmshellSrv, 3,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event, );
    if (rc.succeeded())
        eventLoadRemote(out_event, event, true);
    return rc;
}

hk::Result pmshellGetProcessEventInfo(PmProcessEventInfo* out) {
    _Static_assert(sizeof(out->event) == sizeof(u32), "PmProcessEvent");
    return serviceDispatchOut(&g_pmshellSrv, 4, *out);
}

hk::Result pmshellCleanupProcess(u64 pid) {
    return _pmCmdInU64(pid, &g_pmshellSrv, 5);
}

hk::Result pmshellClearJitDebugOccured(u64 pid) {
    return _pmCmdInU64(pid, &g_pmshellSrv, 6);
}

hk::Result pmshellNotifyBootFinished(void) {
    const u64 cmd_id = hosversionAtLeast(5, 0, 0) ? 5 : 7;
    return serviceDispatch(&g_pmshellSrv, cmd_id);
}

hk::Result pmshellGetApplicationProcessIdForShell(u64* pid_out) {
    const u64 cmd_id = hosversionAtLeast(5, 0, 0) ? 6 : 8;
    return serviceDispatchOut(&g_pmshellSrv, cmd_id, *pid_out);
}

hk::Result pmshellBoostSystemMemoryResourceLimit(u64 boost_size) {
    return _pmCmdInU64(boost_size, &g_pmshellSrv, hosversionAtLeast(5, 0, 0) ? 7 : 9);
}

hk::Result pmshellBoostApplicationThreadResourceLimit(void) {
    return _pmCmdVoid(&g_pmshellSrv, 8);
}

hk::Result pmshellBoostSystemThreadResourceLimit(void) {
    return _pmCmdVoid(&g_pmshellSrv, 10);
}
