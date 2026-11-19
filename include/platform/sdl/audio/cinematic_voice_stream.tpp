#pragma once

namespace platform::sdl {

template<class Cue>
void CinematicVoiceStream::cues(const std::vector<Cue>& cues)
{
    if (!lock()) return;
    for (const auto& cue : cues)
        setCue(cue.voice, static_cast<unsigned>(cue.sound), cue.volume);
    unlock();
}

} // namespace platform::sdl
