#include "door.h"

#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

void testBoundaries()
{
    using namespace doors::dos;
    struct Case { unsigned phase, up0, down0, up1, down1, up3, down3; };
    struct Step { std::uint8_t delta; unsigned up, down; };
    const Case cases[] = {
        {0, 0x100, 0, 0x101, 0, 0x103, 0},
        {59, 0x13b, 59, 0x13c, 58, 0x13e, 56},
        {60, 0x13c, 60, 0x13d, 59, 0x13f, 57},
        {61, 0x13d, 61, 0x13e, 60, 0x140, 58},
        {64, 0x140, 64, 0x141, 63, 0x143, 61},
        {89, 0x159, 89, 90, 88, 90, 86},
        {90, 90, 90, 90, 89, 90, 87},
    };
    for (const auto& c : cases) {
        // Preserve lock and every unrelated high bit, in both directions.
        for (const unsigned flags : {0u, unsigned(locked), 0xfa00u, 0xfe00u}) {
            for (const auto& step : {Step{0, c.up0, c.down0},
                                     Step{1, c.up1, c.down1},
                                     Step{3, c.up3, c.down3}}) {
                check(update(flags | opening | c.phase, step.delta) == (flags | step.up),
                      "Ascending boundary / high bits mismatch");
                check(update(flags | c.phase, step.delta) == (flags | step.down),
                      "Descending boundary / high bits mismatch");
            }
            for (const unsigned direction : {0u, unsigned(opening)}) {
                const auto state = flags | direction | c.phase;
                check(leafVisible(state) == (c.phase < 60), "Leaf visibility threshold");
                check(connected(state) == (direction != 0 || c.phase != 0), "Connectivity");
                check(passageBranchEnabled(state) == (direction != 0 && c.phase > 60),
                      "Passage branch must remain distinct from visibility/connectivity");
            }
        }
    }
}

void testByteArithmetic()
{
    using namespace doors::dos;
    struct Case { std::uint16_t state; std::uint8_t delta; std::uint16_t expected; };
    for (const auto& c : {
        Case{0x159, 38, 90}, Case{0x159, 39, 0x180}, // Signed CMP at 127/128.
        Case{0x159, 167, 0x100}, Case{0x159, 255, 0x158}, // ADD wraps.
        Case{0, 128, 0}, Case{0, 129, 127}, Case{0, 255, 1}, // SF, not borrow.
        Case{90, 219, 127}, Case{90, 218, 0},
        Case{0x180, 0, 0x180}, Case{0x80, 0, 0}}) {
        check(update(c.state, c.delta) == c.expected, "Byte wrap / signed branch mismatch");
        check(update(c.state | locked, c.delta) == (c.expected | locked), "Lock lost");
    }
    check(increment(0) == 0 && increment(1) == 0 && increment(3) == 1 &&
          increment(5) == 2 && increment(511) == 255 && increment(512) == 0 &&
          increment(65535) == 255, "Original counter shift/truncation mismatch");
}

void testCycle()
{
    using namespace doors::dos;
    std::uint16_t state = locked | opening;
    for (unsigned i = 1; i <= 90; ++i) {
        state = update(state, 1);
        check(state == (locked | (i < 90 ? opening : 0) | i), "Ascending cycle");
    }
    for (int i = 89; i >= 0; --i) {
        state = update(state, 1);
        check(state == (locked | i), "Automatic descending cycle");
    }
    check(update(state, 3) == locked, "Closed endpoint");
    state = update(locked | opening | 89, 3);
    check(state == (locked | 90) && update(state, 3) == (locked | 87),
          "Excess at 90 must be discarded, not spent descending");
}
} // namespace

int main()
{
    try {
        testBoundaries();
        testByteArithmetic();
        testCycle();
        std::cout << "DOS door state regressions passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
