#include "frame_filter.hpp"
#include <cstdlib>
#include <iostream>

void check(bool condition, const char* name) {
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
input_event event(unsigned short type, unsigned short code, int value) {
    input_event result{};
    result.type = type; result.code = code; result.value = value;
    return result;
}
swf::Frame wheel(int sign) {
    return {event(EV_REL, REL_WHEEL, sign), event(EV_REL, REL_WHEEL_HI_RES, sign * 120)};
}
int main() {
    swf::FrameFilter filter;
    auto output = filter.process(wheel(1), 0);
    check(output.wheel.size() == 1 && output.wheel[0].size() == 2, "paired wheel fields pass together");
    auto bounce = wheel(-1);
    bounce.push_back(event(EV_REL, REL_X, 4));
    bounce.push_back(event(EV_KEY, BTN_LEFT, 1));
    output = filter.process(bounce, .01);
    check(output.wheel.empty(), "one reverse frame held");
    check(filter.bursts.pendingCount() == 1, "hi-res pair is one decision, not two");
    check(output.immediate.size() == 2 && output.immediate[0].code == REL_X &&
          output.immediate[1].code == BTN_LEFT, "movement and buttons never buffered");
    output = filter.process(wheel(1), .03);
    check(output.wheel.size() == 1 && filter.bursts.suppressed == 1, "bounce low/high pair discarded together");
    filter.process(wheel(-1), .05);
    output = filter.process(wheel(-1), .15);
    check(output.wheel.size() == 2, "sustained reversal replays both wheel frames");
    check(output.wheel[0][0].value == -1 && output.wheel[0][1].value == -120,
          "exact paired wheel values preserved");
    filter.bursts.reset();
    filter.process(wheel(1), 1);
    auto horizontal = wheel(-1);
    horizontal.push_back(event(EV_REL, REL_HWHEEL, 1));
    output = filter.process(horizontal, 1.01);
    check(output.wheel.size() == 1 && output.immediate.size() == 1, "diagonal/horizontal scrolling bypasses filter");
    filter.process(wheel(1), 2);
    auto odd = swf::Frame{event(EV_REL, REL_WHEEL, 1), event(EV_REL, REL_WHEEL_HI_RES, -120)};
    output = filter.process(odd, 2.01);
    check(output.wheel.size() == 1, "contradictory representations bypass rather than being guessed");
    auto highOnly = swf::Frame{event(EV_REL, REL_WHEEL_HI_RES, -15)};
    output = filter.process(highOnly, 3);
    check(output.wheel.size() == 1 && output.wheel[0][0].value == -15, "fractional hi-res values are preserved");
    output = filter.process({event(EV_REL, REL_X, 1), event(EV_SYN, SYN_REPORT, 0)}, 3.01);
    check(output.immediate.size() == 1 && output.wheel.empty(), "non-wheel frame passes and strips only framing");
    std::cout << "Linux frame regressions passed without opening /dev/input or /dev/uinput.\n";
}
