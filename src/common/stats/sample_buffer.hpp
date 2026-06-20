#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/mman.h>

namespace ts::stats {
template <typename T, typename formatter>
void dump(const std::vector<T>& vec,
          const std::string& header,
          const std::string& filepath,
          formatter&& format_row) {

    std::ofstream out(filepath);
    if (!out) {
        std::cerr << "failed to open " << filepath << "\n";
        return;
    }
    // Write header
    out << header << "\n";
    for (const auto& timestamp : vec) {
        out << format_row(timestamp) << "\n";
    }
}

template <typename T>
void pre_faulting(std::vector<T>& vec, std::int64_t stride, std::size_t total_msgs) {
    if (stride < 0) {
        throw std::invalid_argument("stride must be set and greater than 0.");
    } else if (stride == 0) {
        // Throughput mode
    } else {
        if ((stride & (stride - 1)) != 0) { throw std::invalid_argument("stride must be pow of 2."); }
        // resize() will write every single byte which is slow
        vec.reserve(total_msgs / stride + 1);
        // NOTE: large vec -> malloc uses mmap -> page-aligned -> madvise succeeds;
        //       small vec -> heap allocation -> 16-byte aligned → madvise returns EINVAL -> fallback runs;
        if (madvise(vec.data(), vec.capacity() * sizeof(T), MADV_POPULATE_WRITE) != 0) {
            // fallback
            volatile char* p = reinterpret_cast<char*>(vec.data());
            std::size_t total_bytes = vec.capacity() * sizeof(T);
            auto PAGESIZE = sysconf(_SC_PAGESIZE);
            for (std::size_t i = 0; i < total_bytes; i += PAGESIZE) {
                p[i] = 0;
            }
        } // madvise
    }
}

inline bool should_sample(std::uint64_t seq, std::int64_t stride) {
    return stride > 0 && (seq & (stride - 1)) == 0;
}
}
