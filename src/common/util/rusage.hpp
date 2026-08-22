#pragma once

#include <cstring>
#include <stdexcept>
#include <sys/resource.h>

struct RusageCounters {
    long voluntary_ctx_sw = 0;
    long involuntary_ctx_sw = 0;
};

RusageCounters read_rusage_self() {
    rusage ru{};
    if (getrusage(RUSAGE_SELF, &ru) != 0) {
        throw std::runtime_error(std::string("getrusage failed: ") + std::string(strerror(errno)));
    }

    return RusageCounters{
        .voluntary_ctx_sw = ru.ru_nvcsw,
        .involuntary_ctx_sw = ru.ru_nivcsw,
    };
}

RusageCounters diff_rusage(const RusageCounters& begin, const RusageCounters& end) {
    return RusageCounters{
        .voluntary_ctx_sw = end.voluntary_ctx_sw - begin.voluntary_ctx_sw,
        .involuntary_ctx_sw = end.involuntary_ctx_sw - begin.involuntary_ctx_sw,
    };
}

