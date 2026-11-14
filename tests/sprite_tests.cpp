#include "first_person_view.h"
#include "sprite_renderer.h"
#include "sprite_animation.h"
#include "decorative_animation.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

sprites::Image image(std::uint8_t index = 2)
{
    return {{2, 2, std::vector<wall_textures::Rgb>(4, {255, 0, 0})},
            1, 1, std::vector<std::uint8_t>(4, index)};
}

void decoding()
{
    // Nontransparent black and red coexist with transparent index 0 (also red).
    std::array<wall_textures::Rgb, 256> palette{};
    palette[0] = palette[2] = {255, 0, 0};
    palette[3] = {0, 255, 0};
    const std::vector<std::uint8_t> bytes{
        0,0,1,0,4,0,0,0, 2,0,2,0,1,0,1,0, 0,1,2,3};
    const auto bank = sprites::decode(bytes, palette);
    const auto& img = bank.images.at(0);
    check(img.texture.width == 2 && img.texture.height == 2 && img.anchorX == 1 &&
          img.anchorY == 1 && img.texture.pixels[1].r == 0 &&
          img.texture.pixels[2].r == 255 && img.texture.pixels[3].g == 255,
          "SPR dimensions/anchors/row order/palette conversion failed");
    for (int kind = 0; kind < 6; ++kind) {
        auto bad = bytes;
        if (kind == 0) bad.resize(2);
        if (kind == 1) bad[2] = 255;
        if (kind == 2) bad[4] = 255;
        if (kind == 3) bad[8] = 0;
        if (kind == 4) bad.pop_back();
        if (kind == 5) bad[7] = 255;
        bool rejected = false;
        try { sprites::decode(bad, palette); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "Invalid SPR accepted");
    }
    sprites::Scene scene{bank, {{100,0,128,50,0,0x4000,0}},0};
    std::vector<std::uint32_t> pixels(200 * 100, 0x112233ff);
    std::vector<double> depth(pixels.size(), std::numeric_limits<double>::infinity());
    sprites::render(pixels, depth, scene, {}, {200,100,100,128});
    check(pixels[30 * 200 + 80] == 0x112233ff &&
          !std::isfinite(depth[30 * 200 + 80]), "Transparent index wrote color/depth");
    check(pixels[30 * 200 + 110] == 0x000000ff &&
          pixels[60 * 200 + 80] == 0xff0000ff &&
          pixels[60 * 200 + 110] == 0x00ff00ff, "Black/red opacity or axes incorrect");
    scene.objects[0].type = 0x4300;
    std::fill(pixels.begin(),pixels.end(),0x112233ff);
    std::fill(depth.begin(),depth.end(),std::numeric_limits<double>::infinity());
    sprites::render(pixels, depth, scene, {}, {200,100,100,128});
    check(pixels[30 * 200 + 80] == 0x00ff00ff &&
          pixels[60 * 200 + 110] == 0x112233ff, "Original flip flags not sampled correctly");
}

void wallImpactDepth()
{
    // The right half of this billboard would normally disappear behind a
    // slanted wall even though its impact point is on the wall itself.
    sprites::Scene scene;
    scene.bank.images.push_back(image());
    scene.objects.push_back({100,0,128,50,0,0,0});
    constexpr int w=100,h=100;
    const auto makeDepth=[] {
        std::vector<double> values(w*h);
        for (int y=0;y<h;++y) for (int x=0;x<w;++x)
            values[y*w+x]=1.0/(.01+.00004*(x-50));
        return values;
    };
    std::vector<std::uint32_t> pixels(w*h,0x112233ff);
    auto depth=makeDepth();
    sprites::render(pixels,depth,scene,{}, {w,h,100,128});
    check(pixels[50*w+30]==0xff0000ff && pixels[50*w+70]==0x112233ff,
          "Ordinary billboard did not obey oblique wall depth");

    scene.objects[0].wallImpact=true;
    std::fill(pixels.begin(),pixels.end(),0x112233ff);
    depth=makeDepth();
    sprites::render(pixels,depth,scene,{}, {w,h,100,128});
    check(pixels[50*w+30]==0xff0000ff && pixels[50*w+70]==0xff0000ff,
          "Wall impact was still cut in half by oblique wall depth");

    std::fill(pixels.begin(),pixels.end(),0x112233ff);
    depth=makeDepth();
    depth[50*w+70]=40; // A foreground sprite/wall must occlude the impact.
    depth[50*w+73]=300; // A different wall must not receive the decal.
    sprites::render(pixels,depth,scene,{}, {w,h,100,128});
    check(pixels[50*w+70]==0x112233ff && pixels[50*w+73]==0x112233ff &&
          pixels[50*w+60]==0xff0000ff,
          "Wall impact ignored nearer geometry or did not render elsewhere");
}

void placement()
{
    game_lab::Level level{};
    level.objetBank = 1;
    game_lab::Sector sector{};
    sector.gx = 65000; sector.gy = 500;
    sector.vertices = {{1000,25}};
    sector.entitiesA = {{0,0x4008,0,42}, {0,0x410c,256,0}, {0,0x500e,64,0},
                        {0,0x0002,0,0}, {1,0x5002,0,0}, {0,0x50ff,0,0}};
    level.sectors.push_back(sector);
    sprites::Bank bank;
    bank.images.resize(22, image());
    const auto scene = sprites::makeScene(level, bank);
    check(scene.objects.size() == 3 && scene.skipped == 3, "Entity validation failed");
    check(scene.objects[0].x == 66000 && scene.objects[0].y == 525 &&
          scene.objects[0].z == 0 && scene.objects[0].height == 60 &&
          scene.objects[0].param2 == 42 && scene.objects[0].image == 8,
          "Global coordinates, height or original data lost");
    check(scene.objects[1].z == 256 && scene.objects[1].height == 50 &&
          scene.objects[1].type == 0x410c && scene.objects[2].z == 64,
          "Ceiling/vertical placement or flip flag failed");
    check(sprites::imageIndex(0x4002,22) == 2 && sprites::imageIndex(0x5002,22) == 2 &&
          !sprites::imageIndex(0x5020,22), "Image selection by family/low byte failed");
}

void projection()
{
    const auto img = image();
    sprites::Instance object{100,0,128,50,0,0x4000,0};
    const sprites::Projection p{200,100,100,128};
    const auto r = sprites::project(object,img,{},p);
    check(r && r->left == 75 && r->top == 25 && r->width == 50 && r->height == 50,
          "Perspective size/anchor incorrect");
    object.x = 200;
    check(sprites::project(object,img,{},p)->height == 25, "Inverse depth scale failed");
    object.x = -100;
    check(!sprites::project(object,img,{},p), "Sprite behind camera projected");
    object.x = 0;
    check(!sprites::project(object,img,{},p), "Camera-plane division not rejected");
    object.y = 100;
    const auto turned = sprites::project(object,img,{0,0,1.5707963267948966},p);
    check(turned && std::abs(turned->left - 75) < 1e-9 && turned->width == 50,
          "Billboard distorted after camera rotation");
    object = {100,0,0,50,0,0x4000,0};
    const auto ground = sprites::project(object,img,{},p);
    check(ground->top == 153, "Param1 did not place anchor at floor height");
    auto anchored = img;
    anchored.anchorX = anchored.anchorY = 0;
    const auto normal = sprites::project(object,anchored,{},p);
    object.type = 0x4300;
    const auto flipped = sprites::project(object,anchored,{},p);
    check(flipped->left == normal->left - 50 && flipped->top == normal->top - 50,
          "Flipped anchor incorrect");
    object = {100,0,128,50,0,0x4000,0};
    const auto resized = sprites::project(object,img,{}, {400,200,200,128});
    check(resized->width == 100 && resized->left == 150 && resized->top == 50,
          "Window resize changed projection proportions");
}

void spriteDepth()
{
    auto front = image(), back = image();
    front.indices[0] = 0;
    for (auto& c : back.texture.pixels) c = {0,255,0};
    sprites::Scene scene{{{front,back}}, {{100,0,128,50,0,0x4000,0},
                         {200,0,128,100,1,0x4001,0}},0};
    const auto draw = [&]() {
        std::vector<std::uint32_t> pixels(20000, 0);
        std::vector<double> depth(20000, std::numeric_limits<double>::infinity());
        sprites::render(pixels,depth,scene,{}, {200,100,100,128});
        return pixels;
    };
    const auto a = draw();
    std::swap(scene.objects[0],scene.objects[1]);
    check(a == draw() && a[30 * 200 + 80] == 0x00ff00ff &&
          a[60 * 200 + 110] == 0xff0000ff, "Sprite depth/transparency depends on order");
    scene.objects[0].image = 999;
    draw(); // Invalid image is safely omitted.
}

void sceneOcclusion()
{
    game_lab::Level level{};
    game_lab::Sector a{}, b{};
    a.vertices = {{100,200},{100,0}};
    a.walls = {{0,1,0x1001,0}};
    b.vertices = {{100,0},{100,200}};
    b.walls = {{0,1,0x1001,0}};
    level.sectors = {a,b}; level.connectionStates = {0x0400};
    auto doors = doors::closedDoors(level);
    check(doors.size() == 1, "Invalid occlusion fixture");
    sprites::Scene scene{{{image()}}, {{150,100,128,60,0,0x4000,0}},0};
    SDL_Surface* surface = SDL_CreateSurface(160,100,SDL_PIXELFORMAT_RGBA32);
    check(surface != nullptr, "Cannot create sprite test surface");
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    check(renderer != nullptr, "Cannot create sprite test renderer");
    {
        FirstPersonRenderer view(renderer);
        const auto red = [&](const std::vector<doors::Door>* list, Camera camera) {
            camera.postureHeight = 128;
            check(view.draw(level,camera,nullptr,nullptr,list,nullptr,&scene) &&
                  SDL_RenderPresent(renderer), "Sprite integration rendering failed");
            Uint8 r,g,b,alpha;
            check(SDL_ReadSurfacePixel(surface,80,50,&r,&g,&b,&alpha), "Cannot read sprite pixel");
            return r == 255 && g == 0 && b == 0;
        };
        check(red(nullptr,{50,100,0}), "Free portal hid sprite");
        check(!red(&doors,{50,100,0}), "Closed door did not occlude sprite");
        scene.objects[0].x = 50;
        check(!red(&doors,{150,100,3.141592653589793}), "Door reverse face leaked sprite");
        scene.objects[0].x = 150;
        level.connectionStates[doors[0].stateIndex]=30;
        scene.objects[0].y = 50;
        check(!red(&doors,{50,50,0}), "Partial door surface leaked sprite");
        scene.objects[0].y = 150;
        check(red(&doors,{50,150,0}), "Partial door gap hid sprite");
        scene.objects[0].y = 100;
        level.connectionStates[doors[0].stateIndex]=60;
        check(red(&doors,{50,100,0}), "Open door still occluded sprite");
        level.sectors[0].walls[0].type = 1;
        check(!red(nullptr,{50,100,0}), "Solid wall did not occlude sprite");
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
}

void captureOriginal(const game_lab::Level& level, const sprites::Scene& scene,
                     const std::filesystem::path& directory)
{
    const auto materials = wall_textures::load(directory, level.dalleBank, level.palBank);
    const auto planes = wall_textures::loadPlanes(directory,1,level.palBank);
    const auto doors = doors::closedDoors(level);
    const std::filesystem::path output = "build/texture-debug/sprites";
    std::filesystem::create_directories(output);
    SDL_Surface* surface = SDL_CreateSurface(640,400,SDL_PIXELFORMAT_RGBA32);
    check(surface != nullptr,"Cannot create original-scene surface");
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    check(renderer != nullptr,"Cannot create original-scene renderer");
    {
        FirstPersonRenderer view(renderer);
        const std::array<Camera,3> cameras{{{3500,900,1.5707963267948966},
                                          {3700,1200,-2.498091544796509},
                                          {1500,1900,-2.356194490192345}}};
        for (std::size_t i = 0; i < cameras.size(); ++i) {
            check(view.draw(level,cameras[i],nullptr,&materials,&doors,&planes,&scene) &&
                  SDL_RenderPresent(renderer),"Cannot render original sprite scene");
            const auto path = output / ("scene-" + std::to_string(i) + ".bmp");
            check(SDL_SaveBMP(surface,path.string().c_str()),"Cannot save diagnostic scene");
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
}

void fallingDrops()
{
    sprites::Scene scene;
    scene.objetBank = 1;
    for (double z : {0.,64.,128.,192.,256.})
        scene.objects.push_back({1200,1600,z,10,14,0x400e,0});
    scene.objects.push_back({1200,1600,256,50,12,0x410c,0});
    auto partitioned = scene;
    sprites::Animation animation, other;
    animation.update(scene,0.099);
    check(scene.objects[4].z == 256,"Drop moved before tick");
    animation.update(scene,0.001);
    check(scene.objects[0].z == 236 && scene.objects[1].z == 44 &&
          scene.objects[4].z == 236,"Original drop decrement/wrap failed");
    animation.update(scene,0.9);
    for (int i = 0; i < 60; ++i) other.update(partitioned,1.0/60);
    for (std::size_t i = 0; i < scene.objects.size(); ++i)
        check(scene.objects[i].z == partitioned.objects[i].z,"Drop motion depends on FPS");
    check(scene.objects.back().z == 256,"Cable moved with drops");
    const auto before = scene;
    animation.update(scene,6.4);
    for (std::size_t i = 0; i < scene.objects.size(); ++i)
        check(scene.objects[i].z == before.objects[i].z,"Drop cycle did not repeat");
    animation.update(scene,-1);
    animation.update(scene,std::numeric_limits<double>::infinity());
    check(scene.objects[0].z == before.objects[0].z,"Invalid delta changed drop height");
    sprites::Scene initial;
    initial.objetBank = 1;
    initial.objects.push_back({0,0,256,10,14,0x400e,0});
    sprites::Animation largeDelta;
    largeDelta.update(initial,6.4);
    check(initial.objects[0].z == 0,"Large delta lost initial z=256 transition");
}

void fallingDropSelection()
{
    const std::array<std::vector<std::size_t>, 10> falling{{
        {14}, {3}, {5}, {11,15}, {}, {2,5}, {2,23}, {12,14}, {}, {}
    }};
    for (unsigned bank = 0; bank <= 11; ++bank) {
        sprites::Scene scene;
        scene.objetBank = bank;
        for (std::size_t image = 0; image < 44; ++image)
            scene.objects.push_back({1200,1600,20,35,image,
                std::uint16_t(0x4100 | image),123});
        const auto before = scene.objects;
        sprites::Animation animation;
        animation.update(scene,0.1);
        for (std::size_t image = 0; image < scene.objects.size(); ++image) {
            bool selected = false;
            if (bank >= 1 && bank <= falling.size())
                for (const auto candidate : falling[bank-1])
                    if (candidate == image) selected = true;
            check(scene.objects[image].z == (selected ? 0 : 20),
                  "Wrong falling sprite selection or ground transition");
        }
        animation.update(scene,0.1);
        for (std::size_t image = 0; image < scene.objects.size(); ++image) {
            const auto& object = scene.objects[image];
            const auto& original = before[image];
            bool selected = false;
            if (bank >= 1 && bank <= falling.size())
                for (const auto candidate : falling[bank-1])
                    if (candidate == image) selected = true;
            check(object.z == (selected ? 236 : 20),
                  "Wrong falling sprite wrap or unrelated sprite moved");
            check(object.x == original.x && object.y == original.y &&
                  object.height == original.height && object.image == original.image &&
                  object.type == original.type && object.param2 == original.param2,
                  "Falling animation changed unrelated instance properties");
        }
    }
}

void decorativeFrames()
{
    sprites::Scene initial;
    initial.objetBank = 2;
    for (std::size_t image = 0; image < 22; ++image)
        initial.objects.push_back({1200,1600,128,150,image,
                                   std::uint16_t(0x4300 | image),123});
    auto scene = initial;
    sprites::DecorativeAnimation animation;
    check(animation.tick() == 0, "Decorative counter did not start at zero");
    for (unsigned step = 1; step <= 6; ++step) {
        animation.step(scene);
        check(animation.tick() == step, "Decorative counter missed a step");
        for (std::size_t i = 0; i < scene.objects.size(); ++i) {
            const auto& object = scene.objects[i];
            const auto& before = initial.objects[i];
            const auto expected = i >= 9 && i <= 11 ? 9 + (i-9+step/2)%3 : i;
            check(object.image == expected, "OBJET2 sequence/parity or selection failed");
            check(object.x == before.x && object.y == before.y && object.z == before.z &&
                  object.height == before.height && object.type == before.type &&
                  object.param2 == before.param2, "OBJET2 changed instance properties");
        }
    }
    for (unsigned bank = 1; bank <= 10; ++bank) {
        if (bank == 2 || bank == 3 || bank == 4 || bank == 9) continue;
        auto other = initial;
        other.objetBank = bank;
        sprites::DecorativeAnimation clock;
        clock.update(other,1);
        for (std::size_t i = 0; i < other.objects.size(); ++i)
            check(other.objects[i].image == i, "Another OBJET bank was animated");
    }
    auto batch = initial, partitioned = initial;
    sprites::DecorativeAnimation batchClock, frameClock;
    batchClock.update(batch,1);
    for (unsigned frame = 0; frame < 60; ++frame) frameClock.update(partitioned,1.0/60);
    check(batchClock.tick() == 18 && frameClock.tick() == 18, "18 Hz accumulation failed");
    for (std::size_t i = 0; i < batch.objects.size(); ++i)
        check(batch.objects[i].image == partitioned.objects[i].image,
              "OBJET2 animation depends on render FPS");
    auto wrapped = initial;
    sprites::DecorativeAnimation wrapClock;
    wrapClock.update(wrapped,65535 * sprites::DecorativeAnimation::stepSeconds);
    check(wrapClock.tick() == 65535 && wrapped.objects[9].image == 10,
          "Accumulated steps lost OBJET2 advances");
    wrapClock.update(wrapped,sprites::DecorativeAnimation::stepSeconds);
    check(wrapClock.tick() == 0 && wrapped.objects[9].image == 11,
          "16-bit wrap did not advance on even zero");
    wrapClock.step(wrapped);
    check(wrapClock.tick() == 1 && wrapped.objects[9].image == 11,
          "Odd step after wrap advanced OBJET2");
}

void objet3Frames()
{
    // Exact selections for counter values 0..15, including repeated endpoints.
    const std::array<std::size_t,16> sequence{
        15,15,16,16,17,17,18,18,18,18,17,17,16,16,15,15};
    // Saved DOS selection at counter values 0..31; 22 rests across the boundary.
    const std::array<std::size_t,32> pulseSequence{
        22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
        22,23,24,25,26,27,28,27,26,25,24,23,22,22,22,22};
    sprites::Scene initial;
    initial.objetBank = 3;
    for (std::size_t i = 0; i < 34; ++i)
        initial.objects.push_back({1200,1600,128,270,i,std::uint16_t(0x4300|i),123});
    auto scene = initial;
    sprites::DecorativeAnimation clock;
    for (unsigned step = 1; step <= 66; ++step) {
        clock.step(scene);
        for (std::size_t i = 0; i < scene.objects.size(); ++i) {
            const auto& object = scene.objects[i];
            const auto& before = initial.objects[i];
            const auto expected = i >= 15 && i <= 18 ? sequence[step%16]
                : i >= 22 && i <= 28 ? pulseSequence[step%32] : i;
            check(object.image == expected,
                  "OBJET3 sequence, endpoints or selection failed");
            check(object.x == before.x && object.y == before.y && object.z == before.z &&
                  object.height == before.height && object.type == before.type &&
                  object.param2 == before.param2, "OBJET3 changed instance properties");
        }
    }
    auto batch = initial, partitioned = initial;
    sprites::DecorativeAnimation batchClock, frameClock;
    batchClock.update(batch,2);
    for (unsigned frame = 0; frame < 120; ++frame) frameClock.update(partitioned,1.0/60);
    check(batchClock.tick() == 36 && frameClock.tick() == 36, "OBJET3 lost fixed steps");
    for (std::size_t i = 0; i < batch.objects.size(); ++i)
        check(batch.objects[i].image == partitioned.objects[i].image &&
              batch.objects[i].image == (i >= 15 && i <= 18 ? 17
                  : i >= 22 && i <= 28 ? 22 : i),
              "OBJET3 animation depends on render FPS");
    auto wrapped = initial;
    sprites::DecorativeAnimation wrapClock;
    wrapClock.update(wrapped,65535 * sprites::DecorativeAnimation::stepSeconds);
    for (unsigned step = 0; step < 34; ++step) {
        check(wrapped.objects[15].image == sequence[wrapClock.tick()%16],
              "OBJET3 sequence broke at counter wrap");
        for (std::size_t i = 22; i <= 28; ++i)
            check(wrapped.objects[i].image == pulseSequence[wrapClock.tick()%32],
                  "OBJET3 pulse or pause broke at counter wrap");
        wrapClock.step(wrapped);
    }
}

void objet4Frames()
{
    const std::array<std::size_t,8> sequence{26,27,28,29,29,28,27,26};
    sprites::Scene initial;
    initial.objetBank = 4;
    for (std::size_t i = 0; i < 34; ++i)
        initial.objects.push_back({1200,1600,128,150,i,std::uint16_t(0x4300|i),123});
    auto scene = initial;
    sprites::DecorativeAnimation clock;
    for (unsigned step = 1; step <= 18; ++step) {
        clock.step(scene);
        for (std::size_t i = 0; i < scene.objects.size(); ++i) {
            const auto& object = scene.objects[i];
            const auto& before = initial.objects[i];
            check(object.image == (i >= 26 && i <= 29 ? sequence[step%8] : i),
                  "OBJET4 cycle, repeated endpoints or selection failed");
            check(object.x == before.x && object.y == before.y && object.z == before.z &&
                  object.height == before.height && object.type == before.type &&
                  object.param2 == before.param2, "OBJET4 changed instance properties");
        }
    }
    auto batch = initial, partitioned = initial;
    sprites::DecorativeAnimation batchClock, frameClock;
    batchClock.update(batch,1);
    for (unsigned frame = 0; frame < 60; ++frame) frameClock.update(partitioned,1.0/60);
    check(batchClock.tick() == 18 && frameClock.tick() == 18, "OBJET4 lost fixed steps");
    for (std::size_t i = 0; i < batch.objects.size(); ++i)
        check(batch.objects[i].image == partitioned.objects[i].image &&
              batch.objects[i].image == (i >= 26 && i <= 29 ? 28 : i),
              "OBJET4 animation depends on render FPS");
    auto wrapped = initial;
    sprites::DecorativeAnimation wrapClock;
    wrapClock.update(wrapped,65535 * sprites::DecorativeAnimation::stepSeconds);
    for (unsigned step = 0; step < 10; ++step) {
        check(wrapClock.tick() == std::uint16_t(65535u + step), "OBJET4 counter wrap failed");
        for (std::size_t i = 26; i <= 29; ++i)
            check(wrapped.objects[i].image == sequence[wrapClock.tick()%8],
                  "OBJET4 cycle broke at counter wrap");
        wrapClock.step(wrapped);
    }
}

void objet9Frames()
{
    const std::array<std::array<std::size_t,8>,3> sequences{{
        {{13,14,15,16,16,15,14,13}},
        {{19,20,21,22,22,21,20,19}},
        {{23,24,25,26,26,25,24,23}}
    }};
    sprites::Scene initial;
    initial.objetBank = 9;
    for (std::size_t i = 0; i < 38; ++i)
        initial.objects.push_back({1200,1600,128,180,i,std::uint16_t(0x4300|i),123});
    const auto verify = [&](const sprites::Scene& scene, unsigned tick) {
        for (std::size_t i = 0; i < scene.objects.size(); ++i) {
            const auto& object = scene.objects[i];
            const auto& before = initial.objects[i];
            std::size_t expected = i;
            for (const auto& sequence : sequences)
                if (i >= sequence[0] && i <= sequence[3]) expected = sequence[tick%8];
            check(object.image == expected, "OBJET9 sequence, endpoints or selection failed");
            check(object.x == before.x && object.y == before.y && object.z == before.z &&
                  object.height == before.height && object.type == before.type &&
                  object.param2 == before.param2, "OBJET9 changed instance properties");
        }
    };
    auto scene = initial;
    sprites::DecorativeAnimation clock;
    for (unsigned step = 1; step <= 18; ++step) {
        clock.step(scene);
        verify(scene,step);
    }
    auto batch = initial, partitioned = initial;
    sprites::DecorativeAnimation batchClock, frameClock;
    batchClock.update(batch,1);
    for (unsigned frame = 0; frame < 60; ++frame) frameClock.update(partitioned,1.0/60);
    check(batchClock.tick() == 18 && frameClock.tick() == 18, "OBJET9 lost fixed steps");
    verify(batch,18);
    verify(partitioned,18);
    auto wrapped = initial;
    sprites::DecorativeAnimation wrapClock;
    wrapClock.update(wrapped,65535 * sprites::DecorativeAnimation::stepSeconds);
    for (unsigned step = 0; step < 10; ++step) {
        check(wrapClock.tick() == std::uint16_t(65535u + step), "OBJET9 counter wrap failed");
        verify(wrapped,wrapClock.tick());
        wrapClock.step(wrapped);
    }
}

void composedAnimations()
{
    sprites::Scene scene;
    scene.objetBank = 1;
    scene.animateImages = true;
    for (unsigned i = 0; i < 44; ++i) {
        auto img = image(std::uint8_t(i));
        for (auto& color : img.texture.pixels) color = {std::uint8_t(i),0,0};
        scene.bank.images.push_back(std::move(img));
    }
    for (unsigned i : {4u,5u,15u,16u,17u}) {
        auto& img = scene.bank.images[i];
        img.texture.width = img.texture.height = 128;
        img.indices.resize(128*128);
        img.texture.pixels.resize(128*128);
        for (unsigned y = 0; y < 128; ++y) for (unsigned x = 0; x < 128; ++x) {
            img.indices[y*128+x] = std::uint8_t(y);
            img.texture.pixels[y*128+x] = {std::uint8_t(y),0,0};
        }
    }
    // A transparent source must erase the previous patch, including RGB storage.
    scene.bank.images[24].indices[0] = 0;
    scene.bank.images[24].texture.pixels[0] = {0,0,0};
    const auto raw = scene;
    sprites::composeImages(scene,0,false);
    check(scene.bank.images[15].indices[27*128+3] == 22 &&
          scene.bank.images[4].indices == raw.bank.images[4].indices,
          "Initial composition scrolled columns or selected wrong patch");
    const auto initial = scene;
    sprites::DecorativeAnimation clock;
    const auto verify = [&](const sprites::Scene& current, unsigned steps) {
        const auto tick = std::uint16_t(steps);
        const unsigned source = 22 + 2*(tick&3);
        const auto& patch = current.bank.images[15];
        check(patch.indices[27*128+3] == (source == 24 ? 0 : source) &&
              patch.texture.pixels[27*128+3].r == (source == 24 ? 0 : source),
              "Patch phase or transparent overwrite failed");
        const auto& flow = current.bank.images[4];
        // Column x=3: even ticks, rows 11..45. x=4: odd ticks, rows 10..46.
        for (unsigned row = 0; row < 35; ++row) {
            const auto expected = 11 + (row + steps/2)%35;
            check(flow.indices[(11+row)*128+3] == expected &&
                  flow.texture.pixels[(11+row)*128+3].r == expected,
                  "Even column lost cumulative rotation");
        }
        for (unsigned row = 0; row < 37; ++row)
            check(flow.indices[(10+row)*128+4] == 10+(row+(steps+1)/2)%37,
                  "Odd column parity or wrap failed");
        check(flow.indices[10*128+3] == 10 && flow.indices[46*128+3] == 46,
              "Rotation changed pixels outside its span");
    };
    for (unsigned step = 1; step <= 4; ++step) { clock.step(scene); verify(scene,step); }
    auto batch = initial, partitioned = initial;
    sprites::DecorativeAnimation batchClock, frameClock;
    batchClock.update(batch,1);
    for (unsigned frame = 0; frame < 60; ++frame) frameClock.update(partitioned,1.0/60);
    verify(batch,18); verify(partitioned,18);
    auto wrapped = initial;
    sprites::DecorativeAnimation wrapClock;
    wrapClock.update(wrapped,65535 * sprites::DecorativeAnimation::stepSeconds);
    verify(wrapped,65535);
    wrapClock.step(wrapped); verify(wrapped,65536);
    check(wrapClock.tick() == 0, "Composition counter did not wrap");
    scene.objects = {{100,0,128,50,15,0x400f,0},{100,50,128,50,15,0x400f,0}};
    const auto beforeRender = scene.bank;
    std::vector<std::uint32_t> pixels(20000);
    std::vector<double> depth(20000,std::numeric_limits<double>::infinity());
    for (unsigned frame = 0; frame < 3; ++frame)
        sprites::render(pixels,depth,scene,{}, {200,100,100,128});
    for (std::size_t i = 0; i < scene.bank.images.size(); ++i)
        check(scene.bank.images[i].indices == beforeRender.images[i].indices,
              "Rendering advanced composition");
    for (unsigned bank : {6u,10u}) {
        auto unchanged = initial; unchanged.objetBank = bank;
        sprites::DecorativeAnimation other;
        other.update(unchanged,1);
        for (std::size_t i = 0; i < unchanged.bank.images.size(); ++i)
            check(unchanged.bank.images[i].indices == initial.bank.images[i].indices,
                  "OBJET6/10 composition changed");
    }
    auto invalid = initial;
    invalid.bank.images[24].texture.width = 128;
    bool rejected = false;
    try { sprites::composeImages(invalid,1); }
    catch (const std::runtime_error&) { rejected = true; }
    check(rejected,"Oversized patch source accepted");
}

void originalCompositions(const std::filesystem::path& directory)
{
    for (unsigned bank = 1; bank <= 10; ++bank) {
        if (!std::filesystem::exists(directory / ("OBJET"+std::to_string(bank)+".SPR"))) {
            std::cout << "OBJET" << bank << " missing; composition resource check skipped.\n";
            continue;
        }
        sprites::Scene scene;
        scene.bank = sprites::load(directory,bank,1);
        scene.objetBank = bank; scene.animateImages = true;
        sprites::composeImages(scene,0,false);
        sprites::DecorativeAnimation clock;
        // Covers every patch source and both column parities in every bank.
        for (unsigned step = 0; step < 224; ++step) clock.step(scene);
    }
}

void originalPlacement(const std::filesystem::path& lab)
{
    const auto directory = lab.parent_path() / "DATA";
    for (unsigned zone : {0u, 1u}) {
        const auto level = game_lab::loadLevel(lab, zone).level;
        const auto scene = sprites::loadScene(level, directory);
        check(level.objetBank == zone + 1, "Unexpected level 1/2 OBJET bank");
        check(!scene.objects.empty() && scene.skipped == 0, "Original decorations missing");
        if (zone == 0) check(scene.objects.size() == 27, "OBJET1 placement regressed");
        std::size_t index = 0;
        for (const auto& sector : level.sectors) for (const auto& entity : sector.entitiesA) {
            const auto& object = scene.objects.at(index++);
            const auto& vertex = sector.vertices.at(entity.vertex);
            check(object.image == (entity.type & 0xff) && object.type == entity.type &&
                  object.x == double(sector.gx) + vertex.x &&
                  object.y == double(sector.gy) + vertex.y && object.z == entity.param1 &&
                  object.param2 == entity.param2, "Original placement record changed");
            // Exercise each real image through the existing renderer, without walls.
            auto isolated = scene;
            isolated.objects = {object};
            std::vector<std::uint32_t> pixels(160 * 100, 0);
            std::vector<double> depth(pixels.size(), std::numeric_limits<double>::infinity());
            sprites::render(pixels, depth, isolated, {object.x - 300, object.y, 0},
                            {160,100,100,object.z});
            check(std::any_of(depth.begin(), depth.end(), [](double d) { return std::isfinite(d); }),
                  "Original decorative image produced no pixels");
        }
        std::cout << "Level " << zone + 1 << ": OBJET" << level.objetBank << ", "
                  << scene.objects.size() << " decorations placed/rendered, 0 skipped.\n";
    }
    // Only bank headers and resources; do not load the remaining level geometry.
    std::array<bool,11> checked{};
    for (const auto& level : game_lab::loadBankTable(lab)) {
        check(level.objetBank > 0 && level.objetBank < checked.size(), "Invalid OBJET bank");
        if (checked[level.objetBank]) continue;
        checked[level.objetBank] = true;
        const auto scene = sprites::loadScene(level, directory);
        for (std::size_t i = 0; const auto height = sprites::worldHeight(level.objetBank, i); ++i)
            check(i < scene.bank.images.size(), "Height table exceeds sprite bank");
    }
    std::cout << "OBJET bank height bounds and initial composition passed.\n";
}

void original(const std::filesystem::path& lab)
{
    const auto data = game_lab::loadLevel1(lab);
    const auto scene = sprites::loadScene(data.level, lab.parent_path() / "DATA");
    check(scene.bank.images.size() == 44 && scene.objects.size() == 27 && scene.skipped == 0,
          "Original bank/entity count incorrect");
    const auto& lamp = scene.bank.images[7];
    check(lamp.texture.width == 148 && lamp.texture.height == 39 &&
          lamp.anchorX == 73 && lamp.anchorY == 15, "Original lamp dimensions/anchors wrong");
    const auto& drop = scene.bank.images[14];
    check(drop.texture.width == 7 && drop.texture.height == 13 && drop.anchorY == 0,
          "Original drop dimensions/anchor wrong");
    const auto raw = sprites::load(lab.parent_path() / "DATA",1,1);
    for (const auto& patch : std::vector<std::array<int,4>>{
            {15,22,3,27},{15,23,3,51},{16,30,2,21},
            {17,33,15,65},{17,38,15,88},{17,40,15,104}}) {
        const auto& dst = scene.bank.images[patch[0]];
        const auto& src = raw.images[patch[1]];
        for (int y = 0; y < src.texture.height; ++y)
            for (int x = 0; x < src.texture.width; ++x)
                check(dst.indices[(y + patch[3]) * dst.texture.width + x + patch[2]] ==
                      src.indices[y * src.texture.width + x], "Original initial patch differs");
    }
    std::cout << "Original OBJET1: 44 images, 27 instances, 0 skipped.\n";
    captureOriginal(data.level,scene,lab.parent_path() / "DATA");
}
} // namespace

int main(int argc, char* argv[])
{
    try {
        if (argc == 3 && std::string(argv[2]) == "--composition-only") {
            originalCompositions(argv[1]);
            std::cout << "Available OBJET composition bounds passed.\n";
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--falling-only") {
            fallingDrops(); fallingDropSelection();
            std::cout << "OBJET falling selection, displacement and wrap passed.\n";
            return 0;
        }
        decoding(); placement(); projection(); spriteDepth(); wallImpactDepth(); sceneOcclusion(); fallingDrops();
        fallingDropSelection();
        decorativeFrames();
        objet3Frames();
        objet4Frames();
        objet9Frames();
        composedAnimations();
        if (argc == 2) original(argv[1]);
        if (argc == 3 && std::string(argv[2]) == "--placement-only") originalPlacement(argv[1]);
        std::cout << "SPR decoding, anchors, projection, transparency and scene depth passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
