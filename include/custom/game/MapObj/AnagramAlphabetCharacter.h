#pragma once

#include "Library/LiveActor/LiveActor.h"

class HackerJudgeNormalFall;
class HackerJudgeStartRun;
class CapTargetInfo;
class AnagramAlphabet;
class IUsePlayerHack;
class CapTargetParts;
class PlayerHackStartShaderCtrl;
class EnemyStateReset;

class AnagramAlphabetCharacter : public al::LiveActor {
public:
    AnagramAlphabetCharacter(const char* name);

    void init(const al::ActorInitInfo& info) override;
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self) override;
    void control() override;

    void setComplete();
    void killCapTarget();

    void exeWait();
    void exeWaitHack();
    void exeWaitHackStart();
    void exeHackStart();
    void exeHackWait();
    void exeHackMove();
    void exeHackFall();
    void exeHackEnd();
    void exeHackGoal();
    void exeSet();
    void exeComplete();
    void exeReset();

    inline bool isHack();
    inline void endHack();
    inline void endHackDir(const sead::Vector3f& dir);

public:
    CapTargetInfo* mCapTargetInfo = nullptr;
    sead::Matrix34f* mPoseMatrix = nullptr;
    AnagramAlphabet* mParent = nullptr;
    IUsePlayerHack* mHackerParent = nullptr;
    CapTargetParts* mCapTargetParts = nullptr;
    HackerJudgeNormalFall* mHackerJudgeNormalFall = nullptr;
    HackerJudgeStartRun* mHackerJudgeStartRun = nullptr;
    PlayerHackStartShaderCtrl* mPlayerHackStartShaderCtrl = nullptr;
    s32 mSwingTimer = 0;
    EnemyStateReset* mStateReset = nullptr;
    sead::Quatf mResetQuat = sead::Quatf::unit;
    sead::Vector3f mResetTrans = sead::Vector3f::zero;
};
