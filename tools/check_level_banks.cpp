#include <cstdint>
#include <array>
#include <filesystem>
#include "extremis/resources/game_lab.h"
#include "extremis/resources/texture_bank.h"
#include "extremis/resources/sprite_bank.h"
#include "extremis/gameplay/enemies/enemy_system.h"
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

namespace {
std::array<std::uint16_t, 14> enemyWords(const game_lab::EnemyRecord& record)
{
    return {record.health, record.behavior, record.state, record.phase, record.sector,
            record.x, record.y, record.polygon, record.angle, record.reserved,
            record.sprite, record.elevation, record.homeX, record.homeY};
}

int checkEnemyLoad(const std::filesystem::path& lab)
{
    const auto data = lab.parent_path() / "DATA";
    const auto levels = game_lab::loadBankTable(lab);
    bool ok = true;
    for (const auto& entry : levels) {
        const auto number = entry.zoneIndex + 1;
        const char* stage = "loadLevel";
        try {
            const auto level = game_lab::loadLevel(lab, entry.zoneIndex).level;
            std::cout << "parsed " << number << ' ' << level.enemies.size() << '\n';
            stage = "System::load";
            enemies::System actors;
            actors.load(level, data);
            const auto& stored = actors.records();
            const auto& ids = actors.sceneRecordIds();
            const auto sceneCount = actors.scene().objects.size();
            std::cout << "level " << number << " parsed " << level.enemies.size()
                      << " stored " << stored.size() << " objects " << sceneCount << '\n';
            if (stored.size() != level.enemies.size() || ids.size() != sceneCount) ok = false;
            std::vector<unsigned> instances(stored.size());
            for (auto id : ids) {
                if (id >= instances.size()) {
                    std::cout << "invalid-scene-id " << number << ' ' << id << '\n';
                    ok = false;
                } else {
                    ++instances[id];
                }
            }
            for (std::size_t id = 0; id < level.enemies.size(); ++id) {
                const auto parsed = enemyWords(level.enemies[id]);
                const bool copied = id < stored.size() && enemyWords(stored[id]) == parsed;
                const unsigned count = id < instances.size() ? instances[id] : 0;
                std::cout << "record " << number << ' ' << id << ' ';
                for (auto word : parsed) std::cout << word << ' ';
                std::cout << (copied ? "copied" : "stored-mismatch") << ' ' << count << '\n';
                const unsigned expected = level.enemies[id].health == 0xffff ? 0 : 1;
                if (!copied || count != expected) ok = false;
            }
            for (auto id = level.enemies.size(); id < stored.size(); ++id)
                std::cout << "unexpected-stored-record " << number << ' ' << id << '\n';
        } catch (const std::exception& e) {
            std::cout << "error " << number << ' ' << stage << ' ' << e.what() << '\n';
            ok = false;
        }
    }
    return ok ? 0 : 1;
}
} // namespace

#ifdef _MSC_VER
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    try {
        if (argc == 3 && std::filesystem::path(argv[1]) == "--enemy-load")
            return checkEnemyLoad(argv[2]);
        if (argc != 2) return 2;
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
            check("MONS", [&] { enemies::System system;
                system.load(game_lab::loadLevel(lab,level.zoneIndex).level,data);
                std::cerr << " MONS images/masks=" << system.scene().bank.images.size()
                          << " instances=" << system.scene().objects.size() << '\n';
            });
        }
        std::cout << levels.size() << " levels checked; " << failures << " resource failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
