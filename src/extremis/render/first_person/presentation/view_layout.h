#pragma once

struct VideoRenderSettings;

namespace extremis::render_detail {
struct ViewLayout {
    int width, height;
    int projectionReferenceWidth;
    int renderScale;
    bool nativeMax;
};

ViewLayout viewLayout(int outputWidth, int outputHeight, bool withHud,
                      const VideoRenderSettings& video);
} // namespace extremis::render_detail
