#include "hk/hook/InstrUtil.h"

#include <sead/heap/seadExpHeap.h>
#include <agl/common/aglDrawContext.h>


extern "C" void _ZN15RadiconCarPatchC1EPP14IUsePlayerHack();

extern "C" void hkMain() {
    hk::hook::writeBranchLinkAtSym<"$radicon_car_ctor_bl">(_ZN15RadiconCarPatchC1EPP14IUsePlayerHack);
}
