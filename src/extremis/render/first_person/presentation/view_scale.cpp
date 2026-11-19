#include "extremis/render/first_person/presentation/view_scale.h"
#include "extremis/ui/original_hud.h"

#include <algorithm>
#include <cmath>

namespace extremis {

double selectedAspect(unsigned choice, int automaticWidth, int automaticHeight)
{
    return choice == 0 ? 4.0 / 3.0 :
           choice == 1 ? 16.0 / 9.0 :
           choice == 2 ? 21.0 / 9.0 : double(automaticWidth) / automaticHeight;
}

int dosViewWidth(double aspect)
{
    return std::clamp(int(std::lround(hud::width * aspect / (4.0 / 3.0))),
                      hud::width, hud::width * 3);
}

} // namespace extremis
