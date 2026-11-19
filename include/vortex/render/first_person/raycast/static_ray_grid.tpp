#pragma once

#include <memory>

namespace vortex {

template <typename EnumerateSegments>
StaticRayGrid::StaticRayGrid(EnumerateSegments enumerate, bool force)
{
    build(std::addressof(enumerate), [](void* enumerator, SegmentVisitor visit) {
        (*static_cast<EnumerateSegments*>(enumerator))(SegmentVisitor{visit});
    }, force);
}

} // namespace vortex
