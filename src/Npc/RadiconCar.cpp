#include "hk/types.h"
#include "hk/util/Context.h"

extern "C" {
#define relocation(SYM)                 \
    const extern ptr _relocation_##SYM; \
    const ptr _relocation_##SYM = hk::util::lookupSymbol<"$" #SYM>();

// clang-format off
relocation(radicon_car_name_string)
relocation(radicon_car_effectkeeper_vft)
relocation(radicon_car_audiokeeper_vft)
relocation(radicon_car_sceneobjholder_vft)
relocation(radicon_car_areaobjdirector_vft)
relocation(radicon_car_cameradirector_vft)
relocation(radicon_car_collisiondirector_vft)
relocation(radicon_car_rail_vft)
relocation(radicon_car_ZN4sead8Matrix34IfE5identE)
// clang-format on
}