#pragma once

enum class PortMenuChoice { None, Resume, NewGame, Exit, Error };
enum class PortMenuPage { Main, Options, Video, Audio, Controls, Language };

// VIDEO chooses internal software 3D resolution on the active display.
// SDL display modes contribute the physical MAX; the DOS HUD remains unchanged.
struct PortVideoMode { int width=0, height=0; float density=1.f; bool nativeMax=false; bool dosPreset=false; };

