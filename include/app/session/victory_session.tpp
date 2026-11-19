#pragma once

template<class Playback, class Menu>
StartupCreditsResult VictorySession::update(bool terminalDefeated, Playback play, Menu menu) {
    if (!begin(terminalDefeated)) return StartupCreditsResult::Continue;
    const auto result=play();
    if (result==StartupCreditsResult::Continue) menu();
    return result;
}
