#include "app/session/victory_session.h"

bool VictorySession::begin(bool terminalDefeated)
{
    if (!terminalDefeated || processed_) return false;
    processed_=true; // Latch before entering the blocking player.
    return true;
}

bool VictorySession::processed() const
{
    return processed_;
}

void VictorySession::reset()
{
    processed_=false;
}
