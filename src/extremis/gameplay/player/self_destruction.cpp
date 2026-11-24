#include <cstdint>
#include "extremis/gameplay/player/self_destruction.h"

std::int32_t SelfDestruction::remaining() const
{
    return remaining_;
}

bool SelfDestruction::expired() const
{
    return expired_;
}

bool SelfDestruction::active() const
{
    return active_;
}

void SelfDestruction::activate()
{
    active_=true;
}

void SelfDestruction::deactivate()
{
    active_=false;
}

bool SelfDestruction::update(std::uint16_t ticks, bool enabled)
{
    if(!enabled || !active_ || expired_) return false;
    remaining_-=ticks;
    if(remaining_<0) {remaining_=0;expired_=true;return true;}
    return false;
}

#ifdef EXTREMIS_DEBUG_MENU
void SelfDestruction::debugShorten()
{
    if(!expired_ && remaining_>1600) remaining_=1600;
}
#endif
