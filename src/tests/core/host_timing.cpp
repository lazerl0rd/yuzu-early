// Copyright 2016 Dolphin Emulator Project / 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <catch2/catch.hpp>

#include <array>
#include <bitset>
#include <cstdlib>
#include <memory>
#include <string>

#include "common/file_util.h"
#include "core/core.h"
#include "core/host_timing.h"

// Numbers are chosen randomly to make sure the correct one is given.
static constexpr std::array<u64, 5> CB_IDS{{42, 144, 93, 1026, UINT64_C(0xFFFF7FFFF7FFFF)}};
static constexpr int MAX_SLICE_LENGTH = 10000; // Copied from CoreTiming internals
static constexpr std::array<u64, 5> calls_order{{2, 0, 1, 4, 3}};
static std::array<s64, 5> delays{};

static std::bitset<CB_IDS.size()> callbacks_ran_flags;
static u64 expected_callback = 0;
static s64 lateness = 0;

template <unsigned int IDX>
void HostCallbackTemplate(u64 userdata, s64 nanoseconds_late) {
    static_assert(IDX < CB_IDS.size(), "IDX out of range");
    callbacks_ran_flags.set(IDX);
    REQUIRE(CB_IDS[IDX] == userdata);
    REQUIRE(CB_IDS[IDX] == CB_IDS[calls_order[expected_callback]]);
    delays[IDX] = nanoseconds_late;
    ++expected_callback;
}

static u64 callbacks_done = 0;

struct ScopeInit final {
    ScopeInit() {
        core_timing.Initialize();
    }
    ~ScopeInit() {
        core_timing.Shutdown();
    }

    Core::HostTiming::CoreTiming core_timing;
};

TEST_CASE("HostTiming[BasicOrder]", "[core]") {
    ScopeInit guard;
    auto& core_timing = guard.core_timing;
    std::vector<std::shared_ptr<Core::HostTiming::EventType>> events;
    events.resize(5);
    events[0] = Core::HostTiming::CreateEvent("callbackA", HostCallbackTemplate<0>);
    events[1] = Core::HostTiming::CreateEvent("callbackB", HostCallbackTemplate<1>);
    events[2] = Core::HostTiming::CreateEvent("callbackC", HostCallbackTemplate<2>);
    events[3] = Core::HostTiming::CreateEvent("callbackD", HostCallbackTemplate<3>);
    events[4] = Core::HostTiming::CreateEvent("callbackE", HostCallbackTemplate<4>);

    expected_callback = 0;

    core_timing.SyncPause(true);

    u64 one_micro = 1000U;
    for (std::size_t i = 0; i < events.size(); i++) {
        u64 order = calls_order[i];
        core_timing.ScheduleEvent(i * one_micro + 100U, events[order], CB_IDS[order]);
    }
    /// test pause
    REQUIRE(callbacks_ran_flags.none());

    core_timing.Pause(false); // No need to sync

    while (core_timing.HasPendingEvents())
        ;

    REQUIRE(callbacks_ran_flags.all());

    for (std::size_t i = 0; i < delays.size(); i++) {
        const double delay = static_cast<double>(delays[i]);
        const double micro = delay / 1000.0f;
        const double mili = micro / 1000.0f;
        printf("HostTimer Pausing Delay[%zu]: %.3f %.6f\n", i, micro, mili);
    }
}

#pragma optimize("", off)
u64 TestTimerSpeed(Core::HostTiming::CoreTiming& core_timing) {
    u64 start = core_timing.GetGlobalTimeNs().count();
    u64 placebo = 0;
    for (std::size_t i = 0; i < 1000; i++) {
        placebo += core_timing.GetGlobalTimeNs().count();
    }
    u64 end = core_timing.GetGlobalTimeNs().count();
    return (end - start);
}
#pragma optimize("", on)

TEST_CASE("HostTiming[BasicOrderNoPausing]", "[core]") {
    ScopeInit guard;
    auto& core_timing = guard.core_timing;
    std::vector<std::shared_ptr<Core::HostTiming::EventType>> events;
    events.resize(5);
    events[0] = Core::HostTiming::CreateEvent("callbackA", HostCallbackTemplate<0>);
    events[1] = Core::HostTiming::CreateEvent("callbackB", HostCallbackTemplate<1>);
    events[2] = Core::HostTiming::CreateEvent("callbackC", HostCallbackTemplate<2>);
    events[3] = Core::HostTiming::CreateEvent("callbackD", HostCallbackTemplate<3>);
    events[4] = Core::HostTiming::CreateEvent("callbackE", HostCallbackTemplate<4>);

    core_timing.SyncPause(true);
    core_timing.SyncPause(false);

    expected_callback = 0;

    u64 start = core_timing.GetGlobalTimeNs().count();
    u64 one_micro = 1000U;
    for (std::size_t i = 0; i < events.size(); i++) {
        u64 order = calls_order[i];
        core_timing.ScheduleEvent(i * one_micro + 100U, events[order], CB_IDS[order]);
    }
    u64 end = core_timing.GetGlobalTimeNs().count();
    const double scheduling_time = static_cast<double>(end - start);
    const double timer_time = static_cast<double>(TestTimerSpeed(core_timing));

    while (core_timing.HasPendingEvents())
        ;

    REQUIRE(callbacks_ran_flags.all());

    for (std::size_t i = 0; i < delays.size(); i++) {
        const double delay = static_cast<double>(delays[i]);
        const double micro = delay / 1000.0f;
        const double mili = micro / 1000.0f;
        printf("HostTimer No Pausing Delay[%zu]: %.3f %.6f\n", i, micro, mili);
    }

    const double micro = scheduling_time / 1000.0f;
    const double mili = micro / 1000.0f;
    printf("HostTimer No Pausing Scheduling Time: %.3f %.6f\n", micro, mili);
    printf("HostTimer No Pausing Timer Time: %.3f %.6f\n", timer_time / 1000.f,
           timer_time / 1000000.f);
}
