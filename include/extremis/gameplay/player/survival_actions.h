#pragma once

struct SurvivalInput {
    enum class Diagnostic {
        None, Damage, RestoreHealth, AddOxygen, ExhaustOxygen, FillOxygenReserve,
        ResetVitals, EnableDetector
    };

    bool suspendClock = false;
    Diagnostic diagnostic = Diagnostic::None;
};
