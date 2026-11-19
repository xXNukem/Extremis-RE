#pragma once
#include "extremis/cinematics/playback_result.h"

// Owns the campaign handoff, including the alive-player menu after FSEQ.
class VictorySession {
public:
    template<class Playback, class Menu>
    StartupCreditsResult update(bool terminalDefeated, Playback play, Menu menu);
    bool processed() const;
    void reset();
private:
    bool begin(bool terminalDefeated);
    bool processed_=false;
};

#include "app/session/victory_session.tpp"
