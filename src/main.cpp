#include "hk/container/FixedString.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/hook/a64/Assembler.h"

#include <cstdio>
#include <cstring>
#include <nn/fs.h>

#include <agl/common/aglDrawContext.h>
#include <sead/filedevice/nin/seadNinSaveFileDeviceNin.h>
#include <sead/filedevice/seadFileDevice.h>
#include <sead/heap/seadExpHeap.h>
#include <sead/heap/seadHeapMgr.h>

#include "Library/Base/StringUtil.h"
#include "Library/Nerve/NerveUtil.h"

#include "MapObj/CapMessageShowInfo.h"
#include "MapObj/ShineTowerRocket.h"
#include "Scene/StageSceneStateWorldMap.h"
#include "System/GameDataFunction.h"

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

HkReplace<void, ShineTowerRocket*> exeNoStartEarth = [](ShineTowerRocket* self) -> void {
    if (!al::isFirstStep(self))
        return;

    GameDataHolderAccessor accessor(self);
    if (isWorldSkiesScenario1(self)) {
        rs::showCapMessage(self, "Home_Sky", 90, 0);
    }
};

void getUsername(hk::FixedString<0x20>* out) {
    char username[0x20] = "default";

    nn::account::Uid uid;
    nn::account::Nickname nickname;
    if (nn::account::GetLastOpenedUser(&uid).IsSuccess() && nn::account::GetNickname(&nickname, uid).IsSuccess()) {
        strncpy(username, nickname.m_Buffer, sizeof(username));
    }

    for (s32 i = 0; i < sizeof(username); i++) {
        const char* disallowedChars = "<>*?:|/\\";
        for (s32 j = 0; j < strlen(disallowedChars); j++)
            if (username[i] == disallowedChars[j])
                username[i] = '_';
    }

    *out = username;
}

HkTrampoline saveWriteHook = [](TrampolineStatic(), void* thisPtr, sead::FileDevice* device, const char* filename) -> s32 {
    hk::FixedString<0x20> username;
    getUsername(&username);

    hk::FixedString<0x40> sdDirPath("sd:/smo/WuhuKingdom/save/");
    sdDirPath.append(username.cstr());
    sdDirPath.append("/");

    hk::FixedString<0x40> path("smo/WuhuKingdom/save/");
    path.append(username.cstr());
    path.append("/");
    path.append(filename);

    hk::FixedString<0x40> sdPath("sd:/");
    sdPath.append(path.cstr());

    nn::fs::DirectoryEntryType type;
    if (nn::fs::GetEntryType(&type, sdDirPath).IsFailure() || type != nn::fs::DirectoryEntryType_Directory) {
        nn::fs::CreateDirectory("sd:/smo/");
        nn::fs::CreateDirectory("sd:/smo/WuhuKingdom/");
        nn::fs::CreateDirectory("sd:/smo/WuhuKingdom/save/");
        nn::fs::CreateDirectory(sdDirPath.cstr());
    }

    sead::NinSaveFileDevice newDevice("sd");
    return orig(thisPtr, &newDevice, path.cstr());
};

HkTrampoline saveReadHook = [](TrampolineStatic(), void* thisPtr, sead::FileDevice* device, const char* name, void* flags) -> s32 {
    hk::FixedString<0x20> username;
    getUsername(&username);

    hk::FixedString<0x40> path("smo/WuhuKingdom/save/");
    path.append(username.cstr());
    path.append("/");
    path.append(name);

    sead::NinSaveFileDevice newDevice("sd");
    return orig(thisPtr, &newDevice, path.cstr(), flags);
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
