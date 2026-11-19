#include "extremis/gameplay/player/camera.h"

vortex::CameraPose Camera::renderPose() const
{
    return {x,y,angle};
}

double Camera::viewHeight() const
{
    constexpr int bob[] = {0,5,8,9,10,9,8,5,0};
    return postureHeight + bob[bobPhase >> 4];
}
