#include "lmget.h"
#include "service.h"

namespace hk::lm {

    static Service sService;

    void Initialize(Handle service) {
        serviceCreate(&sService, service);
    }

    void StartLogging() {
        serviceDispatch(&sService, 0);
    }

    void StopLogging() {
        serviceDispatch(&sService, 1);
    }

    void GetLog(char* dst, size_t size, s64* outWrittenSize, u32* outDropCount) {
        struct {
            u64 dropCount;
            u64 writtenSize;
        } out;
        serviceDispatchOut(&sService, 2, out,
            .buffer_attrs { .attr0 = SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
            .buffers = { { dst, size } });
        *outWrittenSize = out.writtenSize;
        *outDropCount = out.dropCount;
    }

} // namespace hk::lm
