// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/uint128.h"
#include "common/wall_clock.h"

#ifdef ARCHITECTURE_x86_64
#include "common/x64/cpu_detect.h"
#include "common/x64/native_clock.h"
#endif

namespace Common {

using base_timer = std::chrono::steady_clock;
using base_time_point = std::chrono::time_point<base_timer>;

class StandardWallClock : public WallClock {
public:
    StandardWallClock(u64 emulated_cpu_frequency, u64 emulated_clock_frequency)
        : WallClock(emulated_cpu_frequency, emulated_clock_frequency, false) {
        start_time = base_timer::now();
    }

    std::chrono::nanoseconds GetTimeNS() override {
        base_time_point current = base_timer::now();
        auto elapsed = current - start_time;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
    }

    std::chrono::microseconds GetTimeUS() override {
        base_time_point current = base_timer::now();
        auto elapsed = current - start_time;
        return std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    }

    std::chrono::milliseconds GetTimeMS() override {
        base_time_point current = base_timer::now();
        auto elapsed = current - start_time;
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    }

    u64 GetClockCycles() override {
        std::chrono::nanoseconds time_now = GetTimeNS();
        const u128 temporal = Common::Multiply64Into128(time_now.count(), emulated_clock_frequency);
        return Common::Divide128On32(temporal, 1000000000).first;
    }

    u64 GetCPUCycles() override {
        std::chrono::nanoseconds time_now = GetTimeNS();
        const u128 temporal = Common::Multiply64Into128(time_now.count(), emulated_cpu_frequency);
        return Common::Divide128On32(temporal, 1000000000).first;
    }

private:
    base_time_point start_time;
};

#ifdef ARCHITECTURE_x86_64

WallClock* CreateBestMatchingClock(u32 emulated_cpu_frequency, u32 emulated_clock_frequency) {
    const auto& caps = GetCPUCaps();
    u64 rtsc_frequency = 0;
    if (caps.invariant_tsc) {
        if (caps.base_frequency != 0) {
            rtsc_frequency = static_cast<u64>(caps.base_frequency) * 1000000U;
        }
        if (rtsc_frequency == 0) {
            rtsc_frequency = EstimateRDTSCFrequency();
        }
    }
    if (rtsc_frequency == 0) {
        return static_cast<WallClock*>(
            new StandardWallClock(emulated_cpu_frequency, emulated_clock_frequency));
    } else {
        return static_cast<WallClock*>(
            new X64::NativeClock(emulated_cpu_frequency, emulated_clock_frequency, rtsc_frequency));
    }
}

#else

WallClock* CreateBestMatchingClock(u32 emulated_cpu_frequency, u32 emulated_clock_frequency) {
    return static_cast<WallClock*>(
        new StandardWallClock(emulated_cpu_frequency, emulated_clock_frequency));
}

#endif

} // namespace Common
