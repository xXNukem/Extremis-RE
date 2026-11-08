#include "first_person_view.h"
#include "sprite_renderer.h"
#include "sprite_animation.h"

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
        doors[0].progress = 0.5;
        scene.objects[0].y = 50;
        check(!red(&doors,{50,50,0}), "Partial door surface leaked sprite");
        scene.objects[0].y = 150;
        check(red(&doors,{50,150,0}), "Partial door gap hid sprite");
        scene.objects[0].y = 100;
        doors[0].progress = 1;
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
    initial.objects.push_back({0,0,256,10,14,0x400e,0});
    sprites::Animation largeDelta;
    largeDelta.update(initial,6.4);
    check(initial.objects[0].z == 0,"Large delta lost initial z=256 transition");
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
        decoding(); placement(); projection(); spriteDepth(); sceneOcclusion(); fallingDrops();
        if (argc == 2) original(argv[1]);
        if (argc == 3 && std::string(argv[2]) == "--placement-only") originalPlacement(argv[1]);
        std::cout << "SPR decoding, anchors, projection, transparency and scene depth passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
