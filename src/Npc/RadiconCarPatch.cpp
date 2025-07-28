#include "game/Npc/RadiconCarPatch.h"

#include <al/Library/LiveActor/ActorActionFunction.h>
#include <al/Library/LiveActor/ActorClippingFunction.h>
#include <al/Library/LiveActor/ActorModelFunction.h>
#include <al/Library/LiveActor/ActorMovementFunction.h>
#include <al/Library/LiveActor/ActorSensorUtil.h>
#include <al/Library/LiveActor/ActorPoseUtil.h>
#include <al/Library/Nature/NatureUtil.h>
#include <al/Library/Nerve/NerveSetupUtil.h>
#include <al/Library/Nerve/NerveUtil.h>

#include "game/Enemy/EnemyStateReset.h"
#include "game/Util/Hack.h"

namespace {
NERVE_IMPL(RadiconCar, Wait);
NERVE_IMPL_(RadiconCar, MoveAuto, Move);

extern const RadiconCarNrvWait NrvWait;
extern const RadiconCarNrvMoveAuto NrvMoveAuto;

NERVE_IMPL(RadiconCarPatch, HackBreak);
NERVE_IMPL(RadiconCarPatch, HackReset);

NERVES_MAKE_STRUCT(RadiconCarPatch, HackBreak, HackReset);
}  // namespace

RadiconCarPatch::RadiconCarPatch(IUsePlayerHack** hackActor) : RadiconCar(hackActor) {}

void RadiconCarPatch::control() {
    if (al::isInWaterPos(this, al::getTrans(this)) && !al::isNerve(this, &NrvRadiconCarPatch.HackBreak) && !al::isNerve(this, &NrvRadiconCarPatch.HackReset)) {
        al::setNerve(this, &NrvRadiconCarPatch.HackBreak);
    }
}

void RadiconCarPatch::exeHackBreak() {
    if (al::isFirstStep(this)) {
        al::tryAddRippleLarge(this);
        al::setVelocityZero(this);
        al::startAction(this, "Disappear");
        al::startHitReaction(this, "消滅");
        al::invalidateClipping(this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvRadiconCarPatch.HackReset);
    }
}

void RadiconCarPatch::exeHackReset() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        al::stopAction(this);
        rs::startReset(this);
        al::invalidateHitSensors(this);
        tryCancelHack();
    }

    if (al::isGreaterEqualStep(this, 2)) {
        al::resetRotatePosition(this, mStateReset->mRot, mStateReset->mPos);
        al::validateClipping(this);
        al::showModelIfHide(this);
        rs::endReset(this);
        al::restartAction(this);
        al::validateHitSensors(this);

        if (mIsTypeRace)
            al::setNerve(this, &NrvWait);
        else
            al::setNerve(this, &NrvMoveAuto);
    }
}
