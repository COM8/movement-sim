#include "TickData.hpp"

namespace sim {
TickData::TickData(uint32_t bunchCount) : bunchCount(bunchCount) {}

void TickData::perform_tick() {
    tick++;
    bunchOffset = 0;
}

void TickData::set_bunch_offset(uint32_t bunchOffset) {
    this->bunchOffset = bunchOffset;
}
}  // namespace sim