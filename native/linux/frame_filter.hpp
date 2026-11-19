#pragma once
#include <linux/input.h>
#include "filter.hpp"
#include <vector>

namespace swf {
using Frame = std::vector<input_event>;
struct FrameOutput {
    Frame immediate;
    std::vector<Frame> wheel;
};

class FrameFilter {
public:
    explicit FrameFilter(double window = .080) : bursts(window) {}
    FrameOutput process(const Frame& frame, double time) {
        FrameOutput result;
        Frame wheel;
        long long legacy = 0, high = 0;
        bool hasHigh = false, horizontal = false;
        for (const auto& event : frame) {
            if (event.type == EV_SYN) continue;
            if (event.type == EV_REL && event.code == REL_WHEEL) {
                legacy += event.value;
                wheel.push_back(event);
            } else if (event.type == EV_REL && event.code == REL_WHEEL_HI_RES) {
                high += event.value;
                hasHigh = true;
                wheel.push_back(event);
            } else {
                result.immediate.push_back(event);
                if (event.type == EV_REL &&
                    (event.code == REL_HWHEEL || event.code == REL_HWHEEL_HI_RES)) horizontal = true;
            }
        }
        if (wheel.empty()) return result;
        const long long value = hasHigh ? high : legacy;
        const bool contradictory = legacy != 0 && high != 0 && ((legacy > 0) != (high > 0));
        if (horizontal || contradictory || value == 0) {
            bursts.reset();
            result.wheel.push_back(std::move(wheel));
            return result;
        }
        // Low- and high-resolution representations share ONE decision per
        // SYN_REPORT, so paired events never count as two confirmation ticks.
        result.wheel = bursts.process(value > 0 ? 1 : -1, time, std::move(wheel));
        return result;
    }
    BurstFilter<Frame> bursts;
};
} // namespace swf
