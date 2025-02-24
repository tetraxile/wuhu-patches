#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "hk/gfx/DebugRenderer.h"
#include "hk/hook/Trampoline.h"

#include "agl/common/aglDrawContext.h"
#include "al/Library/System/GameSystemInfo.h"
#include "game/System/Application.h"
#include "game/System/GameSystem.h"

HkTrampoline<void, al::LiveActor*> marioControl = hk::hook::trampoline([](al::LiveActor* player) -> void {
    if (al::isPadHoldA(-1)) {
        player->getPoseKeeper()->getVelocityPtr()->y = 0;
        al::getTransPtr(player)->y += 100;
    }

    marioControl.orig(player);
});

HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainHook.orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;

    hk::gfx::DebugRenderer::instance()->clear();
    hk::gfx::DebugRenderer::instance()->begin(drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
    /// hk::gfx::DebugRenderer::instance()->drawTest();
    hk::gfx::DebugRenderer::instance()->drawQuad(
        { { 0.0, 0.0 }, { 0, 0 }, 0xffffffff },
        { { 0.5, 0.0 }, { 1.0, 0 }, 0xffffffff },
        { { 0.5, 1.0 }, { 1.0, 1.0 }, 0xffffffff },
        { { 0.0, 1.0 }, { 0, 1.0 }, 0xffffffff });
    /*hk::gfx::DebugRenderer::instance()->drawTri(
        { { 0.5, 0.0 }, { 0, 0 }, 0xFF00FF00 },
        { { 1.0, 0.0 }, { 0, 0 }, 0x550000FF },
        { { 1.0, 1.0 }, { 0, 0 }, 0xFFFF0000 });*/
    hk::gfx::DebugRenderer::instance()->end();
});

extern "C" void hkMain() {
    marioControl.installAtSym<"_ZN19PlayerActorHakoniwa7controlEv">();
    drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();
    hk::gfx::DebugRenderer::instance()->installHooks();
}
