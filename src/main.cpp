#include "hk/container/FixedString.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/hook/a64/Assembler.h"

#include <cstring>
#include <nn/fs.h>

#include <agl/common/aglDrawContext.h>
#include <sead/heap/seadExpHeap.h>
#include <sead/heap/seadHeapMgr.h>
#include <sead/filedevice/seadFileDevice.h>
#include <sead/filedevice/nin/seadNinSaveFileDeviceNin.h>

#include "Library/Nerve/NerveUtil.h"
#include "Library/Base/StringUtil.h"

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

HkTrampoline saveWriteHook = [](TrampolineStatic(), void* thisPtr, sead::FileDevice* device, const char* name) -> s32 {
    nn::fs::DirectoryEntryType type;
    if (nn::fs::GetEntryType(&type, "sd:/smo/WuhuKingdom/save").IsFailure() || type != nn::fs::DirectoryEntryType_Directory) {
        nn::fs::CreateDirectory("sd:/smo/");
        nn::fs::CreateDirectory("sd:/smo/WuhuKingdom/");
        nn::fs::CreateDirectory("sd:/smo/WuhuKingdom/save/");
    }
    hk::FixedString<50> newPath("smo/WuhuKingdom/save/");
    newPath.append(name);
    sead::NinSaveFileDevice newDevice("sd");
    return orig(thisPtr, &newDevice, newPath.cstr());
};

HkTrampoline saveReadHook = [](TrampolineStatic(), void* thisPtr, sead::FileDevice* device, const char* name, void* flags) -> s32 {
    hk::FixedString<50> newPath("smo/WuhuKingdom/save/");
    newPath.append(name);
    sead::NinSaveFileDevice newDevice("sd");
    return orig(thisPtr, &newDevice, newPath.cstr(), flags);
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

    saveReadHook.installAtSym<"_ZN2al20SaveDataSequenceRead4readEPN4sead10FileDeviceEPKcPj">();
    saveWriteHook.installAtSym<"_ZN2al21SaveDataSequenceWrite5writeEPN4sead10FileDeviceEPKc">();
}
