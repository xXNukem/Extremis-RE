#pragma once

#include "extremis/render/first_person/presentation/video_render_settings.h"

namespace extremis {
double selectedAspect(unsigned choice, int automaticWidth, int automaticHeight);
int dosViewWidth(double aspect);
RequestedVideoSize composedViewSize(int outputWidth, int outputHeight, const VideoRenderSettings& video);
}
