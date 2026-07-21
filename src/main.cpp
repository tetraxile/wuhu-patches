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

#include "Library/Effect/EffectSystemInfo.h"
#include "Library/LiveActor/ActorActionFunction.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorCollisionFunction.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorMovementFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/Matrix/MatrixUtil.h"
#include "Library/Nature/NatureUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Project/SaveData/SaveDataSequenceRead.h"
#include "Project/SaveData/SaveDataSequenceWrite.h"

#include "MapObj/AnagramAlphabetCharacter.h"
#include "MapObj/CapMessageShowInfo.h"
#include "MapObj/Radish.h"
#include "MapObj/ShineTowerRocket.h"
#include "Scene/StageSceneStateWorldMap.h"
#include "System/GameDataFunction.h"
#include "System/GameProgressData.h"
#include "Util/NpcEventFlowUtil.h"
#include "Util/SensorMsgFunction.h"

#include "pe/Hacks/FSHacks.h"

extern "C" void _ZN15RadiconCarPatchC1EPP14IUsePlayerHack();

static void initHeap() {
    nn::fs::MountSdCardForDebug("sd");

    pe::applyRomFSPatches(sead::HeapMgr::getRootHeap(0));
}

constexpr int WuhuSkiesWorldId = 2;

// HkTrampoline worldMapAppear = [](TrampolineStatic(), StageSceneStateWorldMap* worldMap) -> void {
//     // prevent the game from attempting to unlock a new world if we've reached wuhu skies
//     if (GameDataFunction::getCurrentWorldId(worldMap->getHost()) == WuhuSkiesWorldId)
//         worldMap->field_150 = false;

//     orig(worldMap);
// };

HkReplace<u32, GameProgressData*> calcNextLockedWorldNumForWorldMapHook = [](GameProgressData* thisPtr) -> u32 {
    return (thisPtr->getUnlockWorldNum() >= 3) ? 0 : 1;
};

HkReplace<u32, GameProgressData*> calcNextLockedWorldIdForWorldMapHook = [](GameProgressData* thisPtr) -> u32 {
    return thisPtr->getUnlockWorldNum();
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

namespace al {
    bool calcFindFluidSurface(sead::Vector3f* surfacePos, sead::Vector3f* surfaceNormal, const al::LiveActor* actor, const sead::Vector3f& trans, const sead::Vector3f& gravity, const char* type, bool isFlat, bool isDisplacement, f32 radius);
}

bool calcFindWaterSurfaceFlat(sead::Vector3f* surfacePos, sead::Vector3f* surfaceNormal,
    const al::LiveActor* actor, const sead::Vector3f& trans,
    const sead::Vector3f& gravity, f32 radius) {
    if (!surfacePos || !surfaceNormal)
        return false;
    return al::calcFindFluidSurface(surfacePos, surfaceNormal, actor, trans, gravity, "Water", true, false, radius);
}

HkTrampoline anagramAlphabetCharacterInit = [](TrampolineStatic(), al::LiveActor* actor, const al::ActorInitInfo& info) -> void {
    orig(actor, info);
    al::createAndSetColliderSpecialPurpose(actor, "MoveLimit");
};

HkReplace<void, Radish*> radishExeThrow = [](Radish* thisPtr) -> void {
    if (al::isFirstStep(thisPtr)) {
        al::invalidateClipping(thisPtr);
        al::hideSilhouetteModelIfShow(thisPtr);
        thisPtr->mCollidedWallSensor = nullptr;
    }

    al::tryAddRippleMiddle(thisPtr);
    if (al::isInWaterPos(thisPtr, al::getTrans(thisPtr))) {
        al::scaleVelocityHV(thisPtr, 0.92f, 0.85f);
        al::addVelocityToGravity(thisPtr, 0.7f);
    } else {
        al::scaleVelocity(thisPtr, 1.0f);
        al::addVelocityToGravity(thisPtr, 1.6f);
    }

    al::HitSensor* collidedWallSensor = al::tryGetCollidedWallSensor(thisPtr);
    if (collidedWallSensor)
        rs::sendMsgRadishAttack(collidedWallSensor, thisPtr->mAttackSensor);

    al::HitSensor* collidedSensor = al::tryGetCollidedSensor(thisPtr);
    if (collidedSensor && rs::sendMsgRadishReflect(collidedSensor, al::getHitSensor(thisPtr, "Body")))
        thisPtr->mCollidedSensor = collidedSensor;

    bool isOnGround = al::isOnGround(thisPtr, 0);
    if (collidedWallSensor || isOnGround) {
        thisPtr->mCollidedWallSensor = collidedWallSensor;
        ptr nerveAddr = hk::util::lookupSymbol<"$radish_exe_bound">();
        al::setNerve(thisPtr, cast<const al::Nerve*>(nerveAddr));
    }
};

HkReplace<void, Radish*> radishExeBound = [](Radish* thisPtr) -> void {
    if (al::isFirstStep(thisPtr)) {
        al::startAction(thisPtr, "Reaction");
        if (al::isOnGround(thisPtr, 0)) {
            sead::Vector3f normal = al::getOnGroundNormal(thisPtr, 0);
            sead::Vector3f sideDir;
            al::calcSideDir(&sideDir, thisPtr);
            sead::Vector3f trans = al::getTrans(thisPtr);
            al::makeMtxUpSidePos(&thisPtr->_15c, normal, sideDir, trans);
            al::setEffectFollowMtxPtr(thisPtr, "LandSmoke", &thisPtr->_15c);
            al::emitEffect(thisPtr, "LandSmoke", nullptr);
        }
        al::startHitReaction(thisPtr, "バウンド");
    }

    al::tryAddRippleMiddle(thisPtr);
    if (al::isInWaterPos(thisPtr, al::getTrans(thisPtr))) {
        al::scaleVelocityHV(thisPtr, 0.92f, 0.85f);
        al::addVelocityToGravity(thisPtr, 0.7f);
    } else {
        al::scaleVelocity(thisPtr, 1.0f);
        al::addVelocityToGravity(thisPtr, 1.6f);
    }

    bool isOnGround = al::isOnGround(thisPtr, 0);
    al::HitSensor* collidedWallSensor = al::tryGetCollidedWallSensor(thisPtr);
    if (!isOnGround && !collidedWallSensor)
        return;

    if (al::isCollidedFloorCode(thisPtr, "DamageFire"))
        return;

    al::HitSensor* collidedSensor = al::tryGetCollidedSensor(thisPtr);
    if (collidedSensor && rs::sendMsgRadishReflect(collidedSensor, al::getHitSensor(thisPtr, "Body")))
        thisPtr->mCollidedSensor = collidedSensor;

    if (isOnGround) {
        if (al::reboundVelocityFromCollision(thisPtr, 0.3f, 15.0f, 0.3f)) {
            ptr nerveAddr = hk::util::lookupSymbol<"$radish_exe_bound">();
            al::setNerve(thisPtr, cast<const al::Nerve*>(nerveAddr));
        } else {
            al::setVelocityZeroH(thisPtr);
            ptr nerveAddr = hk::util::lookupSymbol<"$radish_exe_land">();
            al::setNerve(thisPtr, cast<const al::Nerve*>(nerveAddr));
        }
        return;
    }

    if (collidedWallSensor && al::reboundVelocityFromCollision(thisPtr, 0.3f, 15.0f, 0.3f) && collidedWallSensor != thisPtr->mCollidedWallSensor) {
        thisPtr->mCollidedWallSensor = collidedWallSensor;
        ptr nerveAddr = hk::util::lookupSymbol<"$radish_exe_bound">();
        al::setNerve(thisPtr, cast<const al::Nerve*>(nerveAddr));
    }
};

extern "C" void hkMain() {
    hk::hook::writeBranchAtSym<"$heap_create_hook">(initHeap);

    pe::installFSHacks();

    // make RC car die when it lands in water
    hk::hook::writeBranchLinkAtSym<"$radicon_car_ctor_bl">(_ZN15RadiconCarPatchC1EPP14IUsePlayerHack);

    if (!hk::ro::getMainModule()->isVersion("100"))
        hk::hook::a64::assemble<"nop">().installAtSym<"$quest_moon_workaround">();

    // worldMapAppear.installAtPtr(&StageSceneStateWorldMap::appear);
    calcNextLockedWorldNumForWorldMapHook.installAtSym<"_ZNK16GameProgressData33calcNextLockedWorldNumForWorldMapEv">();
    calcNextLockedWorldIdForWorldMapHook.installAtSym<"_ZNK16GameProgressData32calcNextLockedWorldIdForWorldMapEi">();

    exeNoStartEarth.installAtPtr(&ShineTowerRocket::exeNoStartEarth);
    hk::hook::writeBranchAtSym<"$skies_scenario_1_disallow_globe">(isWorldSkiesScenario1);

    // fix motorcycle crashing when it lands in water
    hk::hook::writeBranchLinkAtSym<"$water_motorcycle">(calcFindWaterSurfaceFlat);

    // redirect game saves to the sd card
    saveReadHook.installAtPtr(&al::SaveDataSequenceRead::read);
    saveWriteHook.installAtPtr(&al::SaveDataSequenceWrite::write);

    // make letters obey MoveLimit collision
    anagramAlphabetCharacterInit.installAtPtr(&AnagramAlphabetCharacter::init);

    // stop bowser capture from overriding the camera
    hk::hook::a64::assemble<"mov w0, #1">().installAtMainOffset(0x7de58);
    hk::hook::a64::assemble<"nop\nnop">().installAtMainOffset(0x7de80);
    hk::hook::a64::assemble<"nop">().installAtMainOffset(0x7ee94);
    hk::hook::a64::assemble<"nop">().installAtMainOffset(0x7eeac);
    hk::hook::a64::assemble<"nop">().installAtMainOffset(0x7f3d4);

    // allow CameraParam.byml to be used for SphinxQuiz
    hk::hook::writeBranchLinkAtSym<"$sphinx_object_camera">(rs::initEventCameraObject);

    radishExeThrow.installAtPtr(&Radish::exeThrow);
    radishExeBound.installAtPtr(&Radish::exeBound);
}
