#pragma once

#include <math/seadMatrix.h>

#include <al/Library/LiveActor/LiveActor.h>

namespace al {
class JointSpringControllerHolder;
}

class RadiconCarStateAutoMove;
class EnemyStateReset;
class IUsePlayerHack;

class RadiconCar : public al::LiveActor {
public:
    RadiconCar(IUsePlayerHack** hackActor);

    void init(const al::ActorInitInfo &info) override;
    void initAfterPlacement() override;
    void makeActorAlive() override;
    void attackSensor(al::HitSensor *self, al::HitSensor *other) override;
    bool receiveMsg(const al::SensorMsg *message, al::HitSensor *other, al::HitSensor *self) override;

    void tryCancelHack() const;

    void exeWait();
    void exeMoveAuto();
    void exeReset();
    void exeBreak();
    void exeReaction();
    void exeAppear();
    void exeStartHackMove();
    void exeMove();

protected:
    RadiconCarStateAutoMove* mStateAutoMove = nullptr;
    EnemyStateReset* mStateReset = nullptr;
    IUsePlayerHack** mHackActor = nullptr;
    al::JointSpringControllerHolder* mJointSpringControllerHolder = nullptr;
    s32 _128 = 0;
    sead::Matrix34f mEffectFollowMtx = sead::Matrix34f::ident;
    bool _15c = false;
    bool mIsTypeRace = false;
};

class RadiconCarPatch : public RadiconCar {
public:
    RadiconCarPatch(IUsePlayerHack** hackActor);

    void control() override;

    void exeHackBreak();
    void exeHackReset();
};
