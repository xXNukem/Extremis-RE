#pragma once
#include <array>
#include <string_view>
#include <string>
#include <cstdint>

namespace messages {
enum class Id { PortalLocked, DoorUnlocked, CardRejected, AlreadyDisabled, EmptyPanel, MapBlocked, MapUnavailable, SelfDestruction };
std::string_view text(Id id);

// Original five-entry, duplicate-suppressing queue and 800-tick lifetime.
// Full text is shown immediately until the DOS reveal cadence is recovered.
class Queue {
public:
    void post(Id id);
    void postDestruction(std::int32_t remaining);
    void clear();
    void update(double seconds, bool active);
    std::array<std::string_view,5> visible() const;
private:
    void post(Id id,std::string_view value);
    struct Entry { Id id=Id::PortalLocked; std::string text; double age = 0; };
    std::array<Entry,5> entries_{};
    unsigned size_ = 0;
};
}
