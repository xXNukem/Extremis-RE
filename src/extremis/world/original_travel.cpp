#include "extremis/world/original_travel.h"

namespace interaction::original {

const Route* route(unsigned lift, unsigned row) {
    return lift<lifts.size() && row<4 ? &lifts[lift].routes[(~row)&3] : nullptr;
}
bool admitted(const Route& r, unsigned card, unsigned zone, unsigned sector) {
    if (!r.exists || card<1 || card>9) return false;
    if (card==9 || (r.zone==zone && r.sector==sector)) return true;
    const auto& p=permissions[card-1];
    for (unsigned i=0;i<p.count;++i)
        if (p.destinations[i].zone==r.zone && p.destinations[i].sector==r.sector) return true;
    return false;
}

} // namespace interaction::original
