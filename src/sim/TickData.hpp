#pragma once

#include <cstdint>

namespace sim {

struct TickData {
    uint32_t tick{0};
    uint32_t bunchCount;
    uint32_t bunchOffset{0};
    uint32_t padding{0};

    explicit TickData(uint32_t bunchCount);
    void perform_tick();
    void set_bunch_offset(uint32_t bunchOffset);
} __attribute__((aligned(16))) __attribute__((__packed__));
}  // namespace sim