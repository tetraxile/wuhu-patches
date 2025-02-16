#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "hk/hook/Trampoline.h"

HkTrampoline<void, al::LiveActor*> marioControl = hk::hook::trampoline([](al::LiveActor* player) -> void {
    if (al::isPadHoldA(-1)) {
        player->getPoseKeeper()->getVelocityPtr()->y = 0;
        al::getTransPtr(player)->y += 100;
    }

    marioControl.orig(player);
});

extern "C" void hkMain() {
    marioControl.installAtSym<"_ZN19PlayerActorHakoniwa7controlEv">();
}
