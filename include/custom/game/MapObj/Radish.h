#pragma once

#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include <Library/LiveActor/LiveActor.h>

namespace al {
    class MtxConnector;
    class JointSpringController;
    class FlashingCtrl;
}

class MapObjStatePlayerHold;
class Shine;
class CapTargetInfo;
class PlayerHoldObjTutorialController;

class Radish : public al::LiveActor {
public:
    Radish();

    void init(const al::ActorInitInfo& info) override;
    void initAfterPlacement() override;
    void control() override;
    void updateCollider() override;
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self) override;

    void resetForHide();
    void reset();
    void release(s32);
    void pull();
    bool trySendBoundMsgToSaucePan();

    void exeHideWait();
    void exeHideReaction();
    void exePull();
    void exePullOut();
    void exeWait();
    void exeHold();
    void exeThrow();
    void exeBound();
    void exeLand();
    void exeSaucePanInNoDemo();
    void exeSaucePanIn();
    void exeReaction();
    void exeBreak();
    void exeResetWait();
    void exeReset();

public:
    MapObjStatePlayerHold* mStatePlayerHold;
    bool mIsGold;
    s32 mCoinAppearTimer;
    Shine* mShine;
    sead::Vector3f mInitTrans;
    sead::Quatf mInitQuat;
    al::MtxConnector* mMtxConnector;
    CapTargetInfo* mCapTargetInfo;
    f32 _150;
    bool _154;
    s32 _158;
    sead::Matrix34f _15c;
    al::FlashingCtrl* mFlashingCtrl;
    al::JointSpringController* mJointSpringController;
    PlayerHoldObjTutorialController* mTutorialController;
    al::HitSensor* mAttackSensor;
    void* _1b0;
    void* _1b8;
    al::HitSensor* mCollidedSensor;
    al::HitSensor* mCollidedWallSensor;
};

static_assert(sizeof(Radish) == 0x1d0);
