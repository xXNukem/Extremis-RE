#pragma once

namespace continuation { struct Checkpoint; }
enum class PortMenuChoice { None, Resume, NewGame, Continue, Exit, Error };
struct PortMenuResult {
    PortMenuChoice choice;
    const continuation::Checkpoint* checkpoint;
    PortMenuResult(PortMenuChoice choice, const continuation::Checkpoint* checkpoint = nullptr)
        : choice(choice), checkpoint(checkpoint) {}
};
enum class PortMenuPage { Main, Options, Video, Audio, Controls, Language, Password };

// VIDEO chooses internal software 3D resolution on the active display.
// SDL display modes contribute the physical MAX; the DOS HUD remains unchanged.
struct PortVideoMode { int width=0, height=0; float density=1.f; bool nativeMax=false; bool dosPreset=false; };

