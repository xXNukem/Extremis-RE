#pragma once

namespace app::menu_detail {
inline constexpr float menuLeft=142.f, menuRight=498.f;
inline constexpr float submenuLeft=84.f, submenuRight=556.f;
inline constexpr float firstRow=170.f, rowSpacing=41.f;
inline constexpr float submenuFirstRow=116.f, submenuRowSpacing=46.f, rowHeight=34.f;

float rowTop(unsigned row,unsigned count,bool submenu);

} // namespace app::menu_detail
