#pragma once

#include "hk/types.h"

namespace hk::lm {

    void Initialize(Handle service);

    void StartLogging();
    void StopLogging();

    void GetLog(char* dst, size_t size, s64* outWrittenSize, u32* outDropCount);

} // namespace hk::lm
