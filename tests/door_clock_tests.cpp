#include "door.h"
#include "door_clock.h"

#include <iostream>
#include <stdexcept>

namespace {
using doors::dos::Counter;
using namespace std::chrono_literals;
const Counter::Time origin{};

void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

// First representable nanosecond at/after interrupt n, from the nominal PIT ratio.
Counter::Time interruptAt(std::uint64_t n)
{
    return origin + std::chrono::nanoseconds((n * 7457ULL * 1000000000 + 1193181) / 1193182);
}

void testFractions()
{
    Counter fine(origin), coarse(origin);
    check(fine.value() == 1 && fine.value() == 1, "Initial value / pure query");
    fine.advance(origin);
    fine.advance(interruptAt(1) - 1ns);
    check(fine.value() == 1, "Fraction prematurely produced interrupt");
    fine.advance(interruptAt(1));
    check(fine.value() == 2, "Fraction was discarded");
    fine.advance(interruptAt(1));
    fine.advance(origin); // Reject an out-of-order injected sample without rebasing.
    check(fine.value() == 2, "Repeated/backward timestamp advanced counter");
    for (int ms = 7; ms <= 1000; ++ms) fine.advance(origin + std::chrono::milliseconds(ms));
    coarse.advance(origin + 1s);
    check(fine.value() == 161 && coarse.value() == 161, "Chunking lost PIT fractions");
    fine.advance(interruptAt(161) - 1ns);
    check(fine.value() == 161, "Fraction boundary drift");
    fine.advance(interruptAt(161));
    check(fine.value() == 162, "Long-term fractional remainder lost");
}

void testWrapAndReset()
{
    Counter counter(origin);
    counter.advance(interruptAt(65534));
    check(counter.value() == 65535, "Counter before wrap");
    counter.advance(interruptAt(65535));
    check(counter.value() == 0, "16-bit wrap must not saturate");
    counter.advance(interruptAt(65536));
    check(counter.value() == 1, "Counter after wrap");
    Counter large(origin);
    large.advance(interruptAt(131073));
    check(large.value() == 2, "Multiple wraps in one sample");

    Counter fractional(origin);
    fractional.advance(interruptAt(2) - 1ns);
    check(fractional.value() == 2, "Reset fixture");
    fractional.reset();
    check(fractional.value() == 1, "Reset must write one");
    fractional.advance(interruptAt(2));
    check(fractional.value() == 2, "Reset erased PIT fractional phase");
}

void testInhibition()
{
    Counter counter(origin);
    counter.inhibit(true, interruptAt(1)); // Settle old, uninhibited interval.
    check(counter.value() == 2, "Gate transition discarded preceding interrupt");
    counter.advance(interruptAt(3));
    check(counter.value() == 2, "Inhibited counter advanced");
    counter.inhibit(false, interruptAt(4) - 1ns);
    check(counter.value() == 2, "Resume recovered inhibited interrupts");
    counter.advance(interruptAt(4));
    check(counter.value() == 3, "Inhibition stopped or reset PIT phase");
    counter.inhibit(true, interruptAt(5));
    counter.reset();
    counter.advance(interruptAt(6));
    check(counter.value() == 1, "Reset changed inhibition gate");
    counter.inhibit(false, interruptAt(6));
    counter.advance(interruptAt(7));
    check(counter.value() == 2, "Inhibition failed to resume");
}

void testTable()
{
    Counter counter(origin);
    counter.advance(interruptAt(6)); // Word 7 -> BL 3, odd bit discarded.
    const auto snapshot = counter.value();
    std::vector<std::uint16_t> states{0, 0x400, 0x100, 0x53b, 0x159, 0x45a, 1};
    doors::dos::updateTable(states, snapshot);
    check(states == std::vector<std::uint16_t>{0, 0x400, 0x103, 0x53e, 90, 0x457, 0},
          "All entries must receive the same single increment, retaining lock");
    check(counter.value() == snapshot, "Updating table consumed/reset counter");
    counter.reset();
    const auto saved = states;
    doors::dos::updateTable(states, counter.value());
    check(states == saved && counter.value() == 1, "Zero delta / operation separation");
    states = {0x159, 0x559, 0};
    doors::dos::updateTable(states, 511); // One BL=255 operation, not 255 steps.
    check(states == std::vector<std::uint16_t>{0x158, 0x558, 1}, "Large delta was subdivided");
    doors::dos::updateTable(states, 512); // Low byte after shift is zero.
    check(states == std::vector<std::uint16_t>{0x158, 0x558, 1}, "BL truncation missing");
    states.clear();
    doors::dos::updateTable(states, 65535);
    check(states.empty(), "Empty table changed");
}
} // namespace

int main()
{
    try {
        testFractions();
        testWrapAndReset();
        testInhibition();
        testTable();
        std::cout << "Isolated DOS door clock and table tests passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
