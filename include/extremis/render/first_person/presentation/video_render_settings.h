#pragma once

#include "extremis/render/first_person/presentation/requested_video_size.h"

struct VideoPreferences {
    unsigned aspectChoice = 3; // AUTO
    bool fisheyeEnabled = false;
};

struct VideoRenderMode {
    int renderScale = 1;
    bool nativeMax = false;
    int nativeWidth = 0;
    int nativeHeight = 0;
    RequestedVideoSize requestedSize;
};

struct VideoRenderSettings {
    VideoPreferences preferences;
    VideoRenderMode mode;
};
