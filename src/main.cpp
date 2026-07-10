#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/hook/a64/Assembler.h"

#include <cstring>
#include <nn/fs.h>

#include <agl/common/aglDrawContext.h>
#include <sead/heap/seadExpHeap.h>
#include <sead/heap/seadHeapMgr.h>

#include "Library/Nerve/NerveUtil.h"

#include "Scene/StageSceneStateWorldMap.h"
#include "System/GameDataFunction.h"
#include "MapObj/ShineTowerRocket.h"
#include "MapObj/CapMessageShowInfo.h"

#include "pe/Hacks/FSHacks.h"

extern "C" void _ZN15RadiconCarPatchC1EPP14IUsePlayerHack();

static void initHeap() {
    nn::fs::MountSdCardForDebug("sd");

    pe::applyRomFSPatches(sead::HeapMgr::getRootHeap(0));
}

constexpr int WuhuSkiesWorldId = 2;

HkTrampoline worldMapAppear = [](TrampolineStatic(), StageSceneStateWorldMap* worldMap) -> void {
    // prevent the game from attempting to unlock a new world if we've reached wuhu skies
    if (GameDataFunction::getCurrentWorldId(worldMap->getHost()) == WuhuSkiesWorldId)
        *(reinterpret_cast<u8*>(worldMap) + 0x150) = false;

    orig(worldMap);
};

bool isWorldSkiesScenario1(al::LiveActor* actor) {
    return GameDataFunction::getCurrentWorldId(actor) == WuhuSkiesWorldId && GameDataFunction::getScenarioNo(actor) == 1;
}

HkReplace<void, ShineTowerRocket*> exeNoStartEarth = [](ShineTowerRocket *self) -> void {
    if (!al::isFirstStep(self)) return;

    GameDataHolderAccessor accessor(self);
    if (isWorldSkiesScenario1(self)) {
        rs::showCapMessage(self, "Home_Sky", 90, 0);
    }
};



extern "C" void hkMain() {
    hk::hook::writeBranchAtSym<"$heap_create_hook">(initHeap);

    pe::installFSHacks();

    hk::hook::writeBranchLinkAtSym<"$radicon_car_ctor_bl">(_ZN15RadiconCarPatchC1EPP14IUsePlayerHack);

    if (!hk::ro::getMainModule()->isVersion("100"))
        hk::hook::a64::assemble<"nop">().installAtSym<"$quest_moon_workaround">();

    worldMapAppear.installAtSym<"_ZN23StageSceneStateWorldMap6appearEv">();
    exeNoStartEarth.installAtSym<"_ZN16ShineTowerRocket15exeNoStartEarthEv">();

    hk::hook::writeBranchAtSym<"$is_world_skies_scenario_1">(isWorldSkiesScenario1);
}
