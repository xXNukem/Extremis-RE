#include "extremis/gameplay/enemies/enemy_system.h"
#include "extremis/resources/sector_coordinates.h"
#include "extremis/render/first_person/camera/camera_start.h"
#include "extremis/render/first_person/presentation/first_person_view.h"
#include "platform/sdl/video/frame_profiler.h"
#include "ray_scene.h"
#include "view_layout.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace extremis::render_detail;
namespace profiling = platform::sdl::video_profiling;
constexpr std::array stages{profiling::Geometry,profiling::Rays,profiling::Walls,
    profiling::Planes,profiling::Sprites,profiling::Hud,profiling::Composition,profiling::Queue};

void require(bool ok)
{
    if (!ok) throw std::runtime_error(SDL_GetError());
}

void printPercentiles(std::vector<double> samples)
{
    std::sort(samples.begin(), samples.end());
    for (double p : {0.50, 0.95})
        std::cout << ',' << samples[std::size_t(std::ceil(p * samples.size())) - 1];
}

// Pick an interior polygon centre whose view has the longest median wall depth.
Camera openView(const game_lab::Level& level, const std::vector<doors::Door>& doors)
{
    Camera best;
    double bestDistance=0;
    for (unsigned index=0; index<level.sectors.size(); ++index) {
        const auto& sector=level.sectors[index];
        for (const auto& polygon:sector.polygons) {
            if (polygon.vertices.empty()) continue;
            Camera pose;
            pose.sector=index;
            for (const auto vertex:polygon.vertices) {
                const auto point=game_lab::worldVertex(sector,vertex);
                pose.x+=point.x;
                pose.y+=point.y;
            }
            pose.x/=polygon.vertices.size();
            pose.y/=polygon.vertices.size();
            if (!collision::insidePolygon({pose.x,pose.y},sector,polygon)) continue;
            const auto scene=prepareRayScene(level,pose,&doors);
            for (int heading=0;heading<8;++heading) {
                pose.angle=heading*pi/4;
                std::array<double,9> depths{};
                for (unsigned x=0;x<depths.size();++x) {
                    const double ray=pose.angle+(double(x)/4-1)*fov/2;
                    const auto hit=vortex::castPreparedSceneRay(scene,{std::cos(ray),std::sin(ray)});
                    depths[x]=std::isfinite(hit.distance)?hit.distance:0;
                }
                std::sort(depths.begin(),depths.end());
                if (depths[4]>bestDistance) {
                    bestDistance=depths[4];
                    best=pose;
                }
            }
        }
    }
    if (!best.sector) throw std::runtime_error("No open view found");
    return best;
}

void measure(SDL_Renderer* renderer, const game_lab::Level& level, const Camera& camera,
             const std::vector<doors::Door>& doors, const wall_textures::Bank& walls,
             const wall_textures::PlaneBank* planes, const sprites::Scene& objects,
             const enemies::System& enemies, const hud::Renderer& hud, const VideoRenderSettings& settings,
             const std::string& preset, const char* view, int frames, bool profile)
{
    profiling::FrameProfiler profiler;
    FirstPersonRenderer firstPerson(renderer, &settings, profile ? &profiler : nullptr);
    firstPerson.setRaycastLevel(level);
    int width=0, height=0, sync=0;
    require(SDL_GetRenderOutputSize(renderer,&width,&height) && SDL_GetRenderVSync(renderer,&sync));
    const auto layout=viewLayout(width,height,true,settings);
    std::array<std::vector<double>, 3 + stages.size()> samples;
    for (auto& values:samples) values.reserve(frames);
    for (int i=-12; i<frames; ++i) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            if (event.type==SDL_EVENT_QUIT || event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                throw std::runtime_error("Benchmark interrupted");
        if (profile) profiler.beginFrame();
        const auto start=SDL_GetTicksNS();
        require(SDL_SetRenderDrawColor(renderer,0,0,0,255) && SDL_RenderClear(renderer));
        require(firstPerson.draw(level,camera,nullptr,&walls,&doors,planes,&objects,&hud,{},
                                 nullptr,nullptr,nullptr,&enemies.scene(),&enemies.effects().objects()));
        const auto queued=SDL_GetTicksNS();
        require(SDL_RenderPresent(renderer));
        const auto presented=SDL_GetTicksNS();
        if (i<0) continue;
        samples[0].push_back((queued-start)*1e-6);
        samples[1].push_back((presented-queued)*1e-6);
        samples[2].push_back((presented-start)*1e-6);
        if (profile)
            for (std::size_t stage=0; stage<stages.size(); ++stage)
                samples[3+stage].push_back(profiler.frameSamplesNs()[stages[stage]]*1e-6);
    }

    // Inspect candidates outside the timed presentation loop.
    StaticRayGrid grid(level);
    const auto scene=prepareRayScene(level,camera,&doors);
    const auto backtrack=grid.backtrack(camera);
    const double plane=std::tan(fov*.5)*layout.width/layout.projectionReferenceWidth;
    const CameraProjection projection{layout.width,layout.height,std::cos(camera.angle),
        std::sin(camera.angle),plane,layout.width/(2*plane),camera.viewHeight()};
    std::size_t candidates=0;
    std::vector<double> distances;
    for (int x=0; x<layout.width; ++x) {
        std::size_t tested=0;
        const auto hit=grid.cast(scene,camera,columnRay(projection,x),backtrack,nullptr,&tested);
        candidates+=tested;
        if (std::isfinite(hit.distance)) distances.push_back(hit.distance);
    }
    std::sort(distances.begin(),distances.end());
    std::cout << unsigned(level.zoneIndex)+1 << ',' << preset << ',' << layout.width << ',' << layout.height
              << ',' << view << ',' << bool(planes) << ',' << settings.preferences.fisheyeEnabled << ',' << sync
              << ',' << scene.walls.size() << ',' << scene.dynamicSurfaces.size()
              << ',' << double(candidates)/layout.width << ',' << grid.valid() << ',' << grid.bytes()
              << ',' << (distances.empty()?0:distances[distances.size()/2])
              << ',' << objects.objects.size() << ',' << enemies.records().size()
              << ',' << enemies.scene().objects.size();
    for (const auto& values:samples) {
        if (!values.empty()) printPercentiles(values);
    }
    std::cout << std::endl;
}
} // namespace

int main(int argc,char** argv)
{
    if (argc!=7) {
        std::cerr << "Usage: level_render_benchmark GAME.LAB software|window|fullscreen LEVEL FRAMES REPEATS throughput|profile|sync\n";
        return 1;
    }
    try {
        const std::string target=argv[2];
        const bool fullscreen=target=="fullscreen";
        const bool windowed=fullscreen || target=="window";
        const int zone=std::stoi(argv[3])-1, frames=std::stoi(argv[4]), repeats=std::stoi(argv[5]);
        const std::string mode=argv[6];
        if ((zone!=0 && zone!=22) || frames<1 || repeats<1)
            throw std::runtime_error("Use level 1 or 23 and positive frame and repeat counts");
        if ((!windowed && target!="software") ||
            (mode!="profile" && mode!="throughput" && mode!="sync"))
            throw std::runtime_error("Unknown benchmark target or measurement mode");
        const bool profile=mode=="profile";
#ifndef EXTREMIS_DEBUG_MENU
        if (profile) throw std::runtime_error("Profile requires a build with EXTREMIS_DEBUG_MENU");
#endif
        require(SDL_Init(windowed?SDL_INIT_VIDEO:SDL_INIT_EVENTS));
        auto* window=windowed?SDL_CreateWindow("Extremis render benchmark",960,600,
            (fullscreen?SDL_WINDOW_FULLSCREEN:0) | SDL_WINDOW_HIGH_PIXEL_DENSITY):nullptr;
        if (windowed) require(window!=nullptr);
        auto* surface=windowed?nullptr:SDL_CreateSurface(1920,1080,SDL_PIXELFORMAT_RGBA8888);
        if (!windowed) require(surface!=nullptr);
        auto* renderer=windowed?SDL_CreateRenderer(window,nullptr):SDL_CreateSoftwareRenderer(surface);
        require(renderer!=nullptr);
        int defaultSync=0;
        require(SDL_GetRenderVSync(renderer,&defaultSync));
        const auto display=window?SDL_GetDisplayForWindow(window):0;
        const auto* displayMode=display?SDL_GetCurrentDisplayMode(display):nullptr;
        int maxW=0,maxH=0;
        if (displayMode) {
            maxW=int(std::lround(displayMode->w*displayMode->pixel_density));
            maxH=int(std::lround(displayMode->h*displayMode->pixel_density));
        }
        int count=0;
        if (auto** modes=display?SDL_GetFullscreenDisplayModes(display,&count):nullptr) {
            for (int i=0;i<count;++i) {
                const int w=int(std::lround(modes[i]->w*modes[i]->pixel_density));
                const int h=int(std::lround(modes[i]->h*modes[i]->pixel_density));
                if (std::int64_t(w)*h>std::int64_t(maxW)*maxH) {maxW=w; maxH=h;}
            }
            SDL_free(modes);
        }
        std::cerr << "SDL " << SDL_GetVersion() << ' ' << SDL_GetRendererName(renderer)
                  << " default_sync=" << defaultSync << " Hz=" << (displayMode?displayMode->refresh_rate:0)
                  << " max=" << maxW << 'x' << maxH << '\n';
        auto data=game_lab::loadLevel(argv[1],zone);
        auto& level=data.level;
        if (zone==22) level.playerStart=game_lab::PlayerStart{1,320,300,0}; // Campaign shuttle arrival.
        auto camera=initialCamera(level);
        const auto directory=std::filesystem::path(argv[1]).parent_path()/"DATA";
        const auto walls=wall_textures::load(directory,level.dalleBank,level.palBank);
        const auto planes=wall_textures::loadPlanes(directory,level.dalleBank,level.palBank);
        const auto objects=sprites::loadScene(level,directory);
        const hud::Renderer hud(hud::load(directory,level.palBank));
        const auto doors=doors::closedDoors(level);
        enemies::System enemies;
        enemies.load(level,directory);
        std::cerr << "Camera " << camera.x << ',' << camera.y << ',' << camera.angle << '\n';
        const auto open=mode=="sync"?camera:openView(level,doors);
        std::cerr << "Open camera " << open.x << ',' << open.y << ',' << open.angle
                  << " sector=" << open.sector.value_or(0) << '\n';
        if (profile) {
            const auto world=collision::staticWorld(level);
            for (int view=0;view<2;++view) {
                const auto& pose=view?open:camera;
                for (int run=0;run<repeats;++run) {
                    auto actors=enemies;
                    player::Vitals vitals;
                    actors.update(level,pose,world,vitals,0.,true);
                    std::vector<double> updates;
                    updates.reserve(frames);
                    for (int i=0;i<frames;++i) {
                        const auto start=SDL_GetTicksNS();
                        actors.update(level,pose,world,vitals,1./60.,true,{},{},doors,&objects);
                        updates.push_back((SDL_GetTicksNS()-start)*1e-6);
                    }
                    std::sort(updates.begin(),updates.end());
                    std::cerr << "Enemy update (ms) " << (view?"open":"arrival") << ' '
                              << updates[(updates.size()-1)/2] << ' '
                              << updates[std::size_t(std::ceil(.95*updates.size()))-1]
                              << " health=" << vitals.health() << '\n';
                }
            }
        }
        std::cout << std::fixed << std::setprecision(4)
                  << "level,preset,world_w,world_h,view,planes,fisheye,vsync,static,dynamic,static_candidates_per_ray,grid,grid_bytes,"
                     "wall_distance_p50,objects,enemies,enemy_sprites,queue_p50,queue_p95,present_p50,present_p95,total_p50,total_p95";
        if (profile) std::cout << ",geometry_p50,geometry_p95,ray_p50,ray_p95,wall_p50,wall_p95,planes_p50,planes_p95,sprite_fish_p50,"
                                  "sprite_fish_p95,hud_p50,hud_p95,compose_p50,compose_p95,upload_queue_p50,upload_queue_p95";
        std::cout << '\n';
        VideoRenderSettings settings;
        if (mode=="sync") {
            settings.preferences.aspectChoice=0;
            for (int i=0;i<repeats;++i) for (int interval:{defaultSync,1,0}) {
                require(SDL_SetRenderVSync(renderer,interval));
                measure(renderer,level,camera,doors,walls,&planes,objects,enemies,hud,settings,
                        "sync","arrival",frames,false);
            }
        } else {
            require(SDL_SetRenderVSync(renderer,0));
            for (int i=0;i<repeats;++i) for (int view=0;view<2;++view) {
                const auto& pose=view?open:camera;
                std::vector<SDL_Point> sizes{{320,200},{1280,720},{1920,1080}};
                if (maxW>0 && maxH>0 && (maxW!=1920 || maxH!=1080)) sizes.push_back({maxW,maxH});
                for (const auto size:sizes) {
                    settings.preferences.aspectChoice=size.x==320?0:3;
                    settings.mode={1,size.x!=320,size.x,size.y,{size.x,size.y}};
                    const auto preset=std::to_string(size.x)+"x"+std::to_string(size.y);
                    for (bool floor:{false,true}) for (bool fish:{false,true}) {
                        settings.preferences.fisheyeEnabled=fish;
                        measure(renderer,level,pose,doors,walls,floor?&planes:nullptr,objects,enemies,hud,
                            settings,preset,view?"open":"arrival",frames,profile);
                    }
                }
            }
        }
        SDL_DestroyRenderer(renderer);
        if (surface) SDL_DestroySurface(surface);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        SDL_Quit();
        return 1;
    }
}
