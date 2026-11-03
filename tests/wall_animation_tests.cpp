#include "wall_animation.h"
#include "texture_preview.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

bool equal(const wall_textures::Bank& a, const wall_textures::Bank& b)
{
    if (a.textures.size() != b.textures.size()) return false;
    for (std::size_t i = 0; i < a.textures.size(); ++i) {
        const auto& x = a.textures[i];
        const auto& y = b.textures[i];
        if (x.width != y.width || x.height != y.height || x.pixels.size() != y.pixels.size())
            return false;
        for (std::size_t p = 0; p < x.pixels.size(); ++p) {
            if (x.pixels[p].r != y.pixels[p].r || x.pixels[p].g != y.pixels[p].g ||
                x.pixels[p].b != y.pixels[p].b) return false;
        }
    }
    return true;
}

void write(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    check(bool(file), "Cannot write fixture");
}

void fixtures(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    write(directory / "DALLE1.DAL", std::vector<std::uint8_t>(239760, 1));
    std::vector<std::uint8_t> palette(768);
    for (unsigned i = 0; i < 256; ++i) palette[3 * i] = i % 64;
    write(directory / "PAL1.PAL", palette);
    std::vector<std::uint8_t> bytes(84);
    bytes[2] = 20;
    for (unsigned i = 0; i < 20; ++i) {
        const auto offset = bytes.size() - 4;
        for (unsigned b = 0; b < 4; ++b) bytes[4 + i * 4 + b] = (offset >> (8 * b)) & 255;
        const unsigned w = i < 4 ? 38 : i < 12 ? 24 : i < 18 ? 15 : 37;
        const unsigned h = i < 4 ? 31 : i < 18 ? 17 : 3;
        bytes.insert(bytes.end(), {std::uint8_t(w), 0, std::uint8_t(h), 0, 0, 0, 0, 0});
        for (unsigned p = 0; p < w * h; ++p) bytes.push_back((p + 13 * i) % 64);
    }
    check(bytes.size() == 9972, "Invalid synthetic SPR");
    write(directory / "ADALLE.SPR", bytes);
}

void sequences()
{
    for (unsigned i = 0; i < 54; ++i)
        check(wall_animation::isAnimated(i) == (i == 11 || i == 12 || i == 13),
              "Static material identified as animated");
    for (unsigned t = 0; t < 65536; ++t) {
        const auto frame = wall_animation::overlays(t);
        check(frame[0].image == 4 + t % 8 && frame[1].image == 12 + t % 6 &&
              frame[2].image == (t / 32) % 4, "Wrong ADALLE order/duration");
    }
    check(wall_animation::overlays(1)[3].image == 19 &&
          wall_animation::overlays(1)[4].image == 18 &&
          wall_animation::overlays(33)[9].image == 19 &&
          wall_animation::overlays(33)[3].image == 18, "Indicator reversal failed");
    check(wall_animation::overlays(21)[9].image == 19 &&
          wall_animation::overlays(30)[3].image == 18 &&
          wall_animation::overlays(31)[9].image == 19, "Indicator flash sequence failed");
    for (unsigned t = 0; t < 384; ++t) {
        const auto a = wall_animation::overlays(t), b = wall_animation::overlays(t + 384);
        for (unsigned i = 0; i < a.size(); ++i)
            check(a[i].image == b[i].image, "Combined cycle does not repeat");
    }
}

void timing()
{
    wall_animation::Clock a, b;
    check(a.tick() == 0 && !a.advance(0.099), "Wrong initial frame/duration");
    check(a.advance(0.001) && a.tick() == 1, "Tick boundary failed");
    for (int i = 0; i < 60; ++i) b.advance(1.0 / 60);
    a.advance(0.9);
    check(a.tick() == 10 && a.tick() == b.tick(), "FPS-dependent animation");
    a.advance(-1);
    a.advance(std::numeric_limits<double>::quiet_NaN());
    check(a.tick() == 10, "Invalid time advances clock");
    a.advance(6553.6);
    check(a.tick() == 10, "Original 16-bit clock wrap failed");
    a.advance(100);
    check(a.tick() == 1010, "Large delta lost animation steps");
}

void composition(const std::filesystem::path& directory)
{
    auto bank = wall_textures::load(directory, 1, 1);
    const auto base = bank;
    const auto images = wall_textures::loadWallOverlays(directory, 1);
    for (unsigned tick : {0u, 1u, 31u, 32u, 63u, 127u, 384u, 65535u}) {
        auto expected = base;
        for (const auto& overlay : wall_animation::overlays(tick)) {
            const auto& source = images.textures[overlay.image];
            auto& target = expected.textures[overlay.material];
            for (int y = 0; y < source.height; ++y)
                for (int x = 0; x < source.width; ++x)
                    target.pixels[(overlay.y + y) * target.width + overlay.x + x] =
                        source.pixels[y * source.width + x];
        }
        wall_animation::compose(bank, images, tick);
        check(equal(bank, expected), "Overlay orientation/bounds/static pixels changed");
    }
    TexturePreview uninterrupted, switching;
    uninterrupted.load(directory, 1, 1);
    switching.load(directory, 1, 1);
    uninterrupted.update(3.3);
    // Neither updater takes a view mode: the same clock advances while in 2D.
    for (int i = 0; i < 33; ++i) switching.update(0.1);
    check(equal(*uninterrupted.materials(), *switching.materials()),
          "View-independent global clock/partitioned update differs");
    std::filesystem::remove(directory / "ADALLE.SPR");
    switching.load(directory, 1, 1);
    switching.update(10);
    check(equal(*switching.materials(), base), "Missing overlays broke static fallback");
}

void invalidResources(const std::filesystem::path& directory)
{
    fixtures(directory);
    const auto path = directory / "ADALLE.SPR";
    std::ifstream file(path, std::ios::binary);
    const std::vector<std::uint8_t> valid((std::istreambuf_iterator<char>(file)), {});
    for (unsigned kind = 0; kind < 5; ++kind) {
        auto bytes = valid;
        if (kind == 0) bytes[2] = 19;             // Unsupported count.
        if (kind == 1) bytes[4] = 0xff;           // Wrong first offset.
        if (kind == 2) bytes[84] = 0;             // Invalid width.
        if (kind == 3) bytes[11] = 0xff;          // Offset beyond EOF.
        if (kind == 4) bytes.pop_back();         // Truncated image.
        write(path, bytes);
        bool rejected = false;
        try { wall_textures::loadWallOverlays(directory, 1); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "Invalid ADALLE accepted");
    }
}

// Optional, read-only integration check with external resources. Never create
// fixtures or remove files in this branch.
void originalResources(const std::filesystem::path& directory)
{
    TexturePreview preview;
    preview.load(directory, 1, 1);
    check(preview.materials() != nullptr, "Original DALLE failed to load");
    const auto initial = *preview.materials();
    preview.update(0.1);
    check(!equal(initial, *preview.materials()), "Original animation did not advance");
    preview.update(38.3);
    check(equal(initial, *preview.materials()), "Original resource cycle did not repeat");
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc == 3 && std::string(argv[1]) == "--original-data") {
        try {
            originalResources(argv[2]);
            std::cout << "Original resources: global animation and cycle passed (read-only).\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
    if (argc != 2) return 1;
    try {
        sequences();
        timing();
        fixtures(argv[1]);
        composition(argv[1]);
        invalidResources(argv[1]);
        std::cout << "ADALLE sequences, global timing, composition and static fallback passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
