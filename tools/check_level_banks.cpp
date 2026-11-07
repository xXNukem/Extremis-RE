#include "game_lab.h"
#include "texture_bank.h"
#include "sprite_bank.h"
#include "enemy_system.h"
#include <iostream>
#include <set>
#include <stdexcept>

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    try {
        const std::filesystem::path lab = argv[1], data = lab.parent_path()/"DATA";
        const auto levels = game_lab::loadBankTable(lab);
        std::set<unsigned> described;
        unsigned failures = 0;
        std::cout << "Nivel PAL DALLE SOL OBJET MONS\n";
        for (const auto& level : levels) {
            std::cout << level.zoneIndex+1 << ' ' << level.palBank << ' ' << level.dalleBank
                      << ' ' << level.dalleBank << ' ' << level.objetBank << ' ' << level.monsBank << '\n';
            const auto check = [&](const char* kind, auto decode) {
                try { decode(); }
                catch (const std::exception& e) {
                    ++failures;
                    std::cerr << "Level " << level.zoneIndex+1 << ' ' << kind << ": " << e.what() << '\n';
                }
            };
            check("DALLE", [&] { const auto b = wall_textures::load(data,level.dalleBank,level.palBank);
                if (described.insert(level.dalleBank).second)
                    std::cerr << "DALLE" << level.dalleBank << ": " << b.textures.size() << " x 60x74; SOL: 2 x 128x128\n";
            });
            check("SOL", [&] { wall_textures::loadPlanes(data,level.dalleBank,level.palBank); });
            check("OBJET", [&] { auto b = sprites::load(data,level.objetBank,level.palBank);
                std::cerr << "Level " << level.zoneIndex+1 << " OBJET images=" << b.images.size(); });
            check("MONS", [&] { enemies::System system; system.load(level,data);
                std::cerr << " MONS images/masks=" << system.scene().bank.images.size() << '\n';
            });
        }
        std::cout << levels.size() << " levels checked; " << failures << " resource failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
