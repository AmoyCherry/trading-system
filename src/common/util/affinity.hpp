#pragma once

#include <sched.h>
#include <unistd.h>
#include <iostream>

namespace ts::util {

inline void pin_thread_to_cpu(int cpu_core) {
    // Determine the number of available cores to avoid invalid assignments
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_core < 0 || cpu_core >= num_cores) {
        std::cerr << "Error: invalid cpu_core out of range (0-" << (num_cores - 1) << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set); // Clear the cpuset
    CPU_SET(cpu_core, &cpu_set); // Add the requested CPU to the cpuset

    // 0 means the calling process/thread (which is the main thread here)
    if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) != 0) {
        perror("Failed to set CPU affinity");
        std::exit(EXIT_FAILURE);
    }
}

}
