#include "hk/gfx/DebugRenderer.h"
#include "hk/hook/Trampoline.h"

#include "agl/common/aglDrawContext.h"

#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/Application.h"
#include "game/System/GameSystem.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/LiveActor/LiveActorKit.h"
#include "al/Library/Player/PlayerHolder.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/System/GameSystemInfo.h"

HkTrampoline<void, al::LiveActor*> marioControl = hk::hook::trampoline([](al::LiveActor* player) -> void {
    if (al::isPadHoldA(-1)) {
        player->getPoseKeeper()->getVelocityPtr()->y = 0;
        al::getTransPtr(player)->y += 20;
    }

    marioControl.orig(player);
});

HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainHook.orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;

    HakoniwaSequence* sequence = static_cast<HakoniwaSequence*>(gameSystem->mSequence);
    if (sequence == nullptr)
        return;

    al::Scene* scene = sequence->mCurrentScene;
    al::LiveActor* player = nullptr;
    if (scene && scene->mLiveActorKit && scene->mLiveActorKit->mPlayerHolder)
        player = scene->mLiveActorKit->mPlayerHolder->tryGetPlayer(0);

    auto* renderer = hk::gfx::DebugRenderer::instance();

    renderer->clear();
    renderer->begin(drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);

    renderer->setGlyphSize(0.45);

    renderer->drawQuad(
        { { 30, 30 }, { 0, 0 }, 0xef000000 },
        { { 300, 30 }, { 1.0, 0 }, 0xef000000 },
        { { 300, 100 }, { 1.0, 1.0 }, 0xef000000 },
        { { 30, 100 }, { 0, 1.0 }, 0xef000000 });

    renderer->setCursor({ 50, 50 });

    if (player) {
        const sead::Vector3f& trans = al::getTrans(player);

        renderer->printf("Pos: %.2f %.2f %.2f\n", trans.x, trans.y, trans.z);
    } else {
        renderer->printf("No player\n");
    }

    renderer->end();
});

extern "C" void hkMain() {
    marioControl.installAtSym<"_ZN19PlayerActorHakoniwa7controlEv">();
    drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();
    hk::gfx::DebugRenderer::instance()->installHooks();
}
