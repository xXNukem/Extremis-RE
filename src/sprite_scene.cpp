#include "sprite_scene.h"
#include "original_sprite_data.h"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sprites {
namespace {
void copyPatch(Image& target, const Image& source, int x, int y)
{
    const auto& src = source.texture;
    auto& dst = target.texture;
    if (x < 0 || y < 0 || x + src.width > dst.width || y + src.height > dst.height ||
        src.width <= 0 || src.height <= 0 || source.indices.size() != src.pixels.size() ||
        target.indices.size() != dst.pixels.size())
        throw std::runtime_error("Invalid original OBJET composition bounds");
    for (int row = 0; row < src.height; ++row) {
        const auto from = std::size_t(row) * src.width;
        const auto to = std::size_t(y + row) * dst.width + x;
        std::copy_n(source.indices.begin() + from, src.width, target.indices.begin() + to);
        std::copy_n(src.pixels.begin() + from, src.width, dst.pixels.begin() + to);
    }
}


}

std::optional<double> worldHeight(unsigned bank, std::size_t image)
{
    if(bank<1 || bank>original::counts.size() || image>=original::counts[bank-1]) return {};
    return original::heights[bank-1][image];
}

void composeImages(Scene& scene, unsigned tick, bool scroll)
{
    if(!scene.animateImages) return;
    auto& images=scene.bank.images;
    // EXE 0x14d1f..0x1517e. Convert its one-based sprite API here only.
    const auto patch=[&](unsigned to,unsigned from,int x,int y) {
        if(to==0 || from==0 || to>images.size() || from>images.size())
            throw std::runtime_error("Invalid original OBJET animation image");
        copyPatch(images[to-1],images[from-1],x,y);
    };
    const auto flow=[&](unsigned target,const auto& table) {
        if(!scroll) return;
        auto& image=images.at(target-1);
        const auto width=image.texture.width, height=image.texture.height;
        for(unsigned col=0;col<table[1];++col) {
            if(((table[1]-col)&1)!=(tick&1)) continue;
            const unsigned x=table[0]+col,y=table[2+2*col],length=table[3+2*col];
            if(x>=unsigned(width) || y+length>=unsigned(height))
                throw std::runtime_error("Invalid original OBJET column rotation");
            const auto first=y*width+x;
            const auto index=image.indices[first]; const auto color=image.texture.pixels[first];
            for(unsigned row=0;row<length;++row) {
                image.indices[first+row*width]=image.indices[first+(row+1)*width];
                image.texture.pixels[first+row*width]=image.texture.pixels[first+(row+1)*width];
            }
            image.indices[first+length*width]=index; image.texture.pixels[first+length*width]=color;
        }
    };
    const unsigned ping4=(tick&7)<4?(tick&7):((~tick)&3);
    const unsigned ping3=(tick&3)==3?1:(tick&3);
    switch(scene.objetBank) {
    case 1:
        flow(5,original::flow1); flow(6,original::flow2);
        patch(16,23+2*(tick&3),3,27); patch(16,24+2*(tick&3),3,51);
        patch(17,31+ping3,2,21);
        patch(18,34+((tick&7)<=4?(tick&7):8-(tick&7)),15,65);
        patch(18,39+(tick&1),15,88); patch(18,41+(tick&3),15,104); break;
    case 2:
        patch(19,22+((tick&2)>>1),6,37);
        patch(20,24+std::min(tick&7,6u),41,0);
        patch(21,31+std::min(tick&31,13u),24,10); break;
    case 3:
        flow(15,original::flow3); patch(14,30+ping3,19,22);
        patch(20,33+(tick&1),11,0); break;
    case 4:
        patch(22,23+((tick&4)>>2),0,6); patch(22,25+((tick&4)>>2),62,6); break;
    case 5: {
        flow(12,original::flow5);
        unsigned phase=tick&31; if(phase>=10) phase=0; if(phase>=6) phase=10-phase;
        patch(13,15+phase,5,60); patch(14,21+tick%7,0,46); break;
    }
    case 7: patch(26,28+ping4,8,34); patch(27,32+ping4,15,55); break;
    case 8:
        patch(21,24+tick%5,2,6); patch(22,29+ping3,4,14);
        flow(23,original::flow8); break;
    case 9: {
        const unsigned phase=(tick&15)<8?(tick&15):((~tick)&7);
        for(int y:{21,30,40,48,57}) patch(18,28+std::min(phase,5u),2,y);
        patch(19,34+std::min(phase,4u),7,36); break;
    }
    default: break; // OBJET6/10 have no OBJET-to-OBJET patch branch.
    }
    // 0x15180 onwards copies ADALLE into wall textures, not decorative sprites.
}

std::optional<std::size_t> imageIndex(std::uint16_t type, std::size_t count)
{
    const auto family = type & 0xf000;
    const std::size_t index = type & 0xff;
    if ((family != 0x4000 && family != 0x5000) || index >= count) return std::nullopt;
    return index;
}

Scene makeScene(const game_lab::Level& level, Bank bank)
{
    Scene scene{std::move(bank), {}, 0};
    scene.objetBank=level.objetBank;
    for (const auto& sector : level.sectors) {
        for (const auto& entity : sector.entitiesA) {
            const auto image = imageIndex(entity.type, scene.bank.images.size());
            const auto height=image?worldHeight(level.objetBank,*image):std::nullopt;
            if (!height || *height==0 ||
                entity.vertex >= sector.vertices.size()) {
                ++scene.skipped;
                continue;
            }
            const auto& vertex = sector.vertices[entity.vertex];
            scene.objects.push_back({double(sector.gx) + vertex.x,
                double(sector.gy) + vertex.y, double(entity.param1), *height,
                *image, entity.type, entity.param2});
        }
    }
    return scene;
}

Scene loadScene(const game_lab::Level& level, const std::filesystem::path& directory)
{
    auto bank = load(directory, level.objetBank, level.palBank);
    auto scene=makeScene(level,std::move(bank));
    scene.animateImages=true;
    composeImages(scene,0,false);
    return scene;
}
} // namespace sprites
