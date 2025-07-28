#include "hk/hook/InstrUtil.h"

#include <nn/fs.h>

#include <sead/heap/seadExpHeap.h>
#include <sead/heap/seadHeapMgr.h>
#include <sead/heap/seadExpHeap.h>
#include <agl/common/aglDrawContext.h>

#include "pe/Hacks/FSHacks.h"

extern "C" void _ZN15RadiconCarPatchC1EPP14IUsePlayerHack();

static void initHeap() {
    nn::fs::MountSdCardForDebug("sd");

    pe::applyRomFSPatches(sead::HeapMgr::getRootHeap(0));
}

extern "C" void hkMain() {
    hk::hook::writeBranchAtSym<"$heap_create_hook">(initHeap);

    pe::installFSHacks();

    hk::hook::writeBranchLinkAtSym<"$radicon_car_ctor_bl">(_ZN15RadiconCarPatchC1EPP14IUsePlayerHack);
}
