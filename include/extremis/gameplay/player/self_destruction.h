#pragma once
#include <cstdint>

// EXE [0x60b9] and its expiry latch. Security flags remain owned by ItemSession.
class SelfDestruction {
public:
    static constexpr std::int32_t initialUnits=48000;
    std::int32_t remaining() const;
    bool expired() const;
    bool active() const;
    // Latch the event, without reloading its budget. ItemSession validates flags.
    void activate();
    void deactivate(); // Preserve unused time and any already latched expiry.
    // Receives the existing [0x1428] word, not elapsed seconds or door phases.
    // Returns true only on the first negative subtraction (exact zero is not expiry).
    bool update(std::uint16_t ticks, bool enabled);
#ifdef EXTREMIS_DEBUG_MENU
    void debugShorten();
#endif
private:
    std::int32_t remaining_=initialUnits;
    bool active_=false,expired_=false;
};
