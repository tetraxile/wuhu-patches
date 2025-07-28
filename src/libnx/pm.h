/**
 * @file pm.h
 * @brief Process management (pm*) service IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @author mdbell
 * @copyright libnx Authors
 */
#pragma once

#include "event.h"
#include "hk/types.h"
#include "ncm_types.h"
#include "service.h"

/// LaunchFlag
typedef enum
{
    PmLaunchFlag_None = 0,

    ///< PmLaunchFlag_* should be used on [5.0.0+].
    PmLaunchFlag_SignalOnExit = (1 << 0),
    PmLaunchFlag_SignalOnStart = (1 << 1),
    PmLaunchFlag_SignalOnCrash = (1 << 2),
    PmLaunchFlag_SignalOnDebug = (1 << 3),
    PmLaunchFlag_StartSuspended = (1 << 4),
    PmLaunchFlag_DisableAslr = (1 << 5),

    ///< PmLaunchFlagOld_* should be used on [1.0.0-4.1.0].
    PmLaunchFlagOld_SignalOnExit = (1 << 0),
    PmLaunchFlagOld_StartSuspended = (1 << 1),
    PmLaunchFlagOld_SignalOnCrash = (1 << 2),
    PmLaunchFlagOld_DisableAslr = (1 << 3),
    PmLaunchFlagOld_SignalOnDebug = (1 << 4),
    ///< PmLaunchFlagOld_SignalOnStart is only available on [2.0.0+].
    PmLaunchFlagOld_SignalOnStart = (1 << 5),
} PmLaunchFlag;

/// ProcessEvent
typedef enum
{
    PmProcessEvent_None = 0,
    PmProcessEvent_Exit = 1,
    PmProcessEvent_Start = 2,
    PmProcessEvent_Crash = 3,
    PmProcessEvent_DebugStart = 4,
    PmProcessEvent_DebugBreak = 5,
} PmProcessEvent;

/// ProcessEventInfo
typedef struct {
    PmProcessEvent event;
    u64 process_id;
} PmProcessEventInfo;

/// BootMode
typedef enum
{
    PmBootMode_Normal = 0, ///< Normal
    PmBootMode_Maintenance = 1, ///< Maintenance
    PmBootMode_SafeMode = 2, ///< SafeMode
} PmBootMode;

/// ResourceLimitValues
typedef struct {
    u64 physical_memory;
    u32 thread_count;
    u32 event_count;
    u32 transfer_memory_count;
    u32 session_count;
} PmResourceLimitValues;

/// Initialize pm:dmnt.
hk::Result pmdmntInitialize(void);

/// Exit pm:dmnt.
void pmdmntExit(void);

/// Gets the Service object for the actual pm:dmnt service session.
Service* pmdmntGetServiceSession(void);

/// Initialize pm:info.
hk::Result pminfoInitialize(void);

/// Exit pm:info.
void pminfoExit(void);

/// Gets the Service object for the actual pm:info service session.
Service* pminfoGetServiceSession(void);

/// Initialize pm:shell.
hk::Result pmshellInitialize(void);

/// Exit pm:shell.
void pmshellExit(void);

/// Gets the Service object for the actual pm:shell service session.
Service* pmshellGetServiceSession(void);

/// Initialize pm:bm.
hk::Result pmbmInitialize(void);

/// Exit pm:bm.
void pmbmExit(void);

/// Gets the Service object for the actual pm:bm service session.
Service* pmbmGetServiceSession(void);

/**
 * @brief Gets the \ref PmBootMode.
 * @param[out] out \ref PmBootMode
 */
hk::Result pmbmGetBootMode(PmBootMode* out);

/**
 * @brief Sets the \ref PmBootMode to ::PmBootMode_Maintenance.
 */
hk::Result pmbmSetMaintenanceBoot(void);

hk::Result pmdmntGetJitDebugProcessIdList(u32* out_count, u64* out_pids, size_t max_pids);
hk::Result pmdmntStartProcess(u64 pid);
hk::Result pmdmntGetProcessId(u64* pid_out, u64 program_id);
hk::Result pmdmntHookToCreateProcess(Event* out, u64 program_id);
hk::Result pmdmntGetApplicationProcessId(u64* pid_out);
hk::Result pmdmntHookToCreateApplicationProcess(Event* out);
hk::Result pmdmntClearHook(u32 which);
hk::Result pmdmntGetProgramId(u64* program_id_out, u64 pid);

hk::Result pminfoGetProgramId(u64* program_id_out, u64 pid);
hk::Result pminfoGetAppletCurrentResourceLimitValues(PmResourceLimitValues* out);
hk::Result pminfoGetAppletPeakResourceLimitValues(PmResourceLimitValues* out);

hk::Result pmshellLaunchProgram(u32 launch_flags, const NcmProgramLocation* location, u64* pid);
hk::Result pmshellTerminateProcess(u64 processID);
hk::Result pmshellTerminateProgram(u64 program_id);
hk::Result pmshellGetProcessEventHandle(Event* out); // Autoclear for pmshellProcessEvent is always true.
hk::Result pmshellGetProcessEventInfo(PmProcessEventInfo* out);
hk::Result pmshellCleanupProcess(u64 pid);
hk::Result pmshellClearJitDebugOccured(u64 pid);
hk::Result pmshellNotifyBootFinished(void);
hk::Result pmshellGetApplicationProcessIdForShell(u64* pid_out);
hk::Result pmshellBoostSystemMemoryResourceLimit(u64 boost_size);
hk::Result pmshellBoostApplicationThreadResourceLimit(void);
hk::Result pmshellBoostSystemThreadResourceLimit(void);
