#pragma once
#include <array>
#include <string_view>

namespace messages {
enum class Id { PortalLocked, DoorUnlocked, CardRejected, AlreadyDisabled, EmptyPanel };
std::string_view text(Id id);

// Original five-entry, duplicate-suppressing queue and 800-tick lifetime.
// Full text is shown immediately until the DOS reveal cadence is recovered.
class Queue {
public:
    void post(Id id);
    void clear();
    void update(double seconds, bool active);
    std::array<std::string_view,5> visible() const;
private:
    struct Entry { std::string_view text; double age = 0; };
    std::array<Entry,5> entries_{};
    unsigned size_ = 0;
};
}
