#pragma once
#include "startup_credits.h"

// Owns the campaign handoff, including the alive-player menu after FSEQ.
class VictorySession {
public:
    template<class Playback, class Menu>
    StartupCreditsResult update(bool terminalDefeated, Playback play, Menu menu) {
        if (!terminalDefeated || processed_) return StartupCreditsResult::Continue;
        processed_=true; // Latch before entering the blocking player.
        const auto result=play();
        if (result==StartupCreditsResult::Continue) menu();
        return result;
    }
    bool processed() const { return processed_; }
    void reset() { processed_=false; }
private:
    bool processed_=false;
};
