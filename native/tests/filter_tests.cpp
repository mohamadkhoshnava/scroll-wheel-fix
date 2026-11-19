#include "filter.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int checks = 0;
void check(bool condition, const char* name) {
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
    ++checks;
}

int main() {
    using swf::Decision;
    swf::DirectionGate gate;
    check(gate.process(1, 0) == Decision::pass, "first tick passes");
    check(gate.process(1, .05) == Decision::pass, "same direction passes");
    check(gate.process(-1, .06) == Decision::hold, "single bounce held");
    check(gate.process(1, .08) == Decision::pass, "return discards bounce");
    check(gate.process(-1, .09) == Decision::hold, "burst starts held");
    check(gate.process(-1, .096) == Decision::hold, "two fast ticks do not confirm");
    check(gate.process(1, .11) == Decision::pass, "two-tick bounce rejected");
    check(gate.process(-1, .15) == Decision::hold, "deliberate reversal starts held");
    check(gate.process(-1, .25) == Decision::confirm, "sustained direction confirms");
    check(gate.process(-1, .28) == Decision::pass, "new direction passes");
    check(gate.process(1, 1) == Decision::pass, "idle resets direction");
    gate.reset();
    check(gate.process(0, 1.1) == Decision::pass, "zero input passes");
    check(gate.process(-1, 1.11) == Decision::pass, "zero did not choose direction");

    swf::DirectionGate strong(.140);
    strong.process(-1, 2);
    strong.process(1, 2.03);
    check(strong.process(1, 2.13) == Decision::hold, "strong holds 100ms");
    check(strong.process(1, 2.20) == Decision::confirm, "strong confirms 170ms");
    strong.setWindow(.080);
    check(strong.process(-1, 2.21) == Decision::pass, "mode change resets stale state");

    swf::BurstFilter<int> filter;
    check(filter.process(1, 3.0, 120) == std::vector<int>{120}, "normal payload retained");
    check(filter.process(-1, 3.01, -240).empty(), "noisy payload buffered");
    check(filter.process(-1, 3.02, -120).empty(), "second noisy payload buffered");
    check(filter.process(1, 3.04, 120) == std::vector<int>{120}, "bounce not replayed");
    check(filter.suppressed == 2, "suppressed counter counts discarded burst");
    filter.process(-1, 3.05, -120);
    check(filter.process(-1, 3.16, -240) == std::vector<int>({-120, -240}),
          "confirmed reversal replays exact deltas in order");
    check(filter.pendingCount() == 0, "confirmed buffer is cleared");
    filter.process(1, 3.17, 120);
    filter.reset();
    check(filter.pendingCount() == 0 && filter.suppressed == 3, "reset discards stale reverse tick");

    swf::BurstFilter<int> bounded;
    bounded.process(1, 5, 120);
    for (int i = 0; i < 200; ++i) bounded.process(-1, 5.01, -120);
    check(bounded.pendingCount() == 128 && bounded.suppressed == 72, "burst buffer is bounded");
    bounded.reset();
    check(bounded.pendingCount() == 0 && bounded.suppressed == 200, "reset accounts for full buffer");
    std::cout << "All " << checks << " C++ checks passed; no OS input hook or device access used.\n";
}
