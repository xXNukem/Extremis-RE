#include "game_messages.h"
#include <algorithm>
#include <cmath>

namespace messages {
std::string_view text(Id id)
{
    switch (id) {
    case Id::PortalLocked: return "ATTENTION:\r\n  IMPOSSIBLE TO OPEN PORTAL!\r\n  DISABLE THE SECURITY  ";
    case Id::DoorUnlocked: return "VALID CARD..... DOOR UNLOCKING!";
    case Id::CardRejected: return "IMPOSSIBLE TO UNLOCK THIS DOOR...";
    case Id::AlreadyDisabled: return "THE SECURITY IS ALREADY DISABLE";
    // English localization of EXE file 0x2193f; original wording is documented.
    case Id::EmptyPanel: return "THIS COMPARTMENT IS EMPTY!";
    }
    return {};
}
void Queue::post(Id id)
{
    const auto value = text(id);
    if (value.empty()) return;
    for (unsigned i = 0; i < size_; ++i) if (entries_[i].text == value) return;
    if (size_ == entries_.size()) {
        std::move(entries_.begin()+1,entries_.end(),entries_.begin());
        --size_;
    }
    entries_[size_++] = {value,0};
}
void Queue::clear() { entries_={}; size_=0; }
void Queue::update(double seconds, bool active)
{
    if (!active || !std::isfinite(seconds) || seconds <= 0) return;
    for (unsigned i = 0; i < size_; ++i) entries_[i].age += std::min(seconds,5.0)*160;
    while (size_ && entries_[0].age >= 800-1e-8) {
        std::move(entries_.begin()+1,entries_.end(),entries_.begin());
        entries_[--size_] = {};
    }
}
std::array<std::string_view,5> Queue::visible() const
{
    std::array<std::string_view,5> result{};
    for (unsigned i = 0; i < size_; ++i) result[i] = entries_[i].text;
    return result;
}
}
