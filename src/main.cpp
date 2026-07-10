#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/hook/a64/Assembler.h"

#include <cstring>
#include <nn/fs.h>

#include <agl/common/aglDrawContext.h>
#include <sead/heap/seadExpHeap.h>
#include <sead/heap/seadHeapMgr.h>

#include "Scene/StageSceneStateWorldMap.h"
#include "System/GameDataFunction.h"

#include "pe/Hacks/FSHacks.h"

extern "C" void _ZN15RadiconCarPatchC1EPP14IUsePlayerHack();

static void initHeap() {
    nn::fs::MountSdCardForDebug("sd");

    pe::applyRomFSPatches(sead::HeapMgr::getRootHeap(0));
}

constexpr int WuhuSkiesWorldId = 2;
HkTrampoline<void, StageSceneStateWorldMap*> worldMapAppear = hk::hook::trampoline([](StageSceneStateWorldMap* worldMap) {
    // prevent the game from attempting to unlock a new world if we've reached wuhu skies
    if (GameDataFunction::getCurrentWorldId(worldMap->getHost()) == WuhuSkiesWorldId)
        *(reinterpret_cast<u8*>(worldMap) + 0x150) = false;

    worldMapAppear.orig(worldMap);
});

extern "C" void hkMain() {
    hk::hook::writeBranchAtSym<"$heap_create_hook">(initHeap);

    pe::installFSHacks();

    hk::hook::writeBranchLinkAtSym<"$radicon_car_ctor_bl">(_ZN15RadiconCarPatchC1EPP14IUsePlayerHack);

    if (!hk::ro::getMainModule()->isVersion("100"))
        hk::hook::a64::assemble<"nop">().installAtSym<"$quest_moon_workaround">();

    worldMapAppear.installAtSym<"_ZN23StageSceneStateWorldMap6appearEv">();
}
