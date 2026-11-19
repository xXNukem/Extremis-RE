#include "vortex/audio/mixer.h"

#include <array>
#include <iostream>

int main()
{
    using namespace vortex::audio;
    Mixer mixer({2, 0, 0.5f});
    mixer.setSamples({
        {{1.0f, 0.5f, -1.0f}, 0.5f, std::nullopt},
        {{0.0f, 0.5f, 1.0f}, 1.0f, Loop{1, 3, 2}}
    });
    if (!mixer.play(0) || !mixer.play(1) || mixer.play(0)) return 1;

    std::array<float, 6> pcm{};
    mixer.render(pcm.data(), pcm.size());
    const std::array<float, 6> expected{0.125f, 0.1875f, 0.0f, 0.25f, 0.125f, 0.25f};
    if (pcm != expected || mixer.active() != 1 || mixer.playing(0)) return 2;

    const auto first = mixer.statistics(0);
    const auto second = mixer.statistics(1);
    if (!first || first->requests != 2 || first->started != 1 ||
        first->completed != 1 || first->sourceFrames != 3 ||
        !second || second->wraps != 3 || second->sourceFrames != pcm.size()) return 3;

    for (float frame : pcm) std::cout << frame << ' ';
    std::cout << '\n';

    mixer.stop(1);
    mixer.render(pcm.data(), 1);
    if (pcm.front() != 0 || mixer.active() != 0 || mixer.statistics(1)->stopped != 1) return 4;

    Mixer faded({1, 5});
    faded.setSamples({{{1.0f}, 1.0f, Loop{0, 1, 1}}});
    if (!faded.play(0)) return 5;
    faded.render(pcm.data(), 5);
    if (pcm[4] != 1.0f) return 6;
    faded.stop(0);
    faded.render(pcm.data(), 5);
    if (pcm[4] != 0 || faded.active() || faded.statistics(0)->stopped != 1) return 7;
}
