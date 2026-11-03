#include "level_items.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

void cardsAndWeapon()
{
    player::Inventory inventory;
    std::uint16_t card = 0x3001;
    check(inventory.takeSupply(card) == player::Transfer::Taken && card == 0xffff &&
          inventory.cards()[0] == 1,"First original card slot/packing failed");
    check(inventory.takeSupply(card) == player::Transfer::Empty,"Card collected twice");
    card = 0x4001; inventory.takeSupply(card);
    card = 0x5002; inventory.takeSupply(card);
    check(inventory.cards()[1] == 0x1001 && inventory.cards()[2] == 0x2002,
          "Card family was confused with ammunition");
    card = 0x3003;
    check(inventory.takeSupply(card) == player::Transfer::Capacity && card == 0x3003,
          "Full inventory silently destroyed/replaced a card");
    std::uint16_t weapon = 0x11f4;
    check(inventory.exchangeWeapon(weapon) == player::Transfer::Exchanged &&
          inventory.weapon() == 1 && inventory.ammunition() == 500 && weapon == 0x01f4,
          "Original weapon swap did not leave the old weapon in the panel");
    inventory.exchangeWeapon(weapon);
    check(inventory.weapon() == 0 && weapon == 0x11f4,"Repeated swap duplicated a weapon");
    weapon = 0x7001;
    check(inventory.exchangeWeapon(weapon) == player::Transfer::Unsupported,"Unknown weapon accepted");
    inventory = player::Inventory{};
    check(inventory.cards()[0] == -1 && inventory.weapon() == 0 && inventory.ammunition() == 500,
          "New level retained old inventory");
}

void injectors()
{
    player::Inventory inventory;
    player::Vitals vitals;
    vitals.applyDamage(500);
    std::uint16_t item = 0x2064;
    inventory.takeSupply(item);
    check(item == 0xffff && inventory.injectors()[0] == 100 && vitals.health() == 500,
          "Injector was used immediately or healed by its fill value");
    item = 0x2064; inventory.takeSupply(item);
    item = 0x2064;
    check(inventory.takeSupply(item) == player::Transfer::Capacity && item == 0x2064,
          "More than two full injectors were stored");
    check(inventory.useInjector(vitals) && vitals.health() == 1000 && inventory.injectors()[0] == 0,
          "Injector failed to restore full health and leave an empty container");
    check(inventory.takeSupply(item) == player::Transfer::Exchanged && item == 0x2000 &&
          inventory.injectors()[0] == 100,"Empty injector was not exchanged back into cabinet");
    inventory.useInjector(vitals); inventory.useInjector(vitals);
    check(!inventory.useInjector(vitals),"Empty injector healed player");
    item = 0x2064; inventory.takeSupply(item);
    vitals.applyDamage(1000);
    check(!inventory.useInjector(vitals) && inventory.injectors()[0] == 100,
          "Dead player consumed inventory or revived");
}

void oxygen()
{
    // General verified bottle branch, not a claim of bottle placement in level 1.
    player::Inventory inventory;
    player::Vitals vitals;
    std::uint16_t bottle = 0x1708; // Kind 1, 1800 nominal seconds.
    inventory.takeSupply(bottle);
    check(bottle == 0xffff && inventory.oxygenReserve() == 288000,"Bottle time units decoded incorrectly");
    vitals.update(900,true);
    check(inventory.useOxygen(vitals) && vitals.oxygen() == 288000 && inventory.oxygenReserve() == 144000,
          "Stored oxygen added instead of exchanged");
    bottle = 0x1708;
    check(inventory.takeSupply(bottle) == player::Transfer::Capacity && bottle == 0x1708,
          "Full bottle replacement invented an automatic choice");
    inventory = player::Inventory{};
    bottle = 0x1000; inventory.takeSupply(bottle);
    check(!inventory.useOxygen(vitals) && vitals.alive(),"Empty bottle was usable");
    bottle = 0x1708;
    check(inventory.takeSupply(bottle) == player::Transfer::Exchanged && bottle == 0x1000,
          "Empty stored bottle not returned to world");
    vitals.applyDamage(1000);
    check(!inventory.useOxygen(vitals),"Bottle revived dead player");
}

game_lab::Level fixture()
{
    game_lab::Level level{};
    game_lab::Sector sector{};
    // Positive coordinates keep the original uint16 map format.
    sector.vertices = {{100,200},{100,0}};
    sector.walls = {{0,1,0x3205,0x2064}};
    sector.entitiesA = {{0,0x5004,0,0}}; // Decoration is deliberately untouched.
    level.sectors.push_back(sector);
    return level;
}

bool canCollect(Camera camera, const collision::World& extra = {})
{
    const auto level = fixture();
    items::World world(level);
    player::Inventory inventory;
    return world.collect(camera,true,inventory,extra).transfer == player::Transfer::Taken;
}

void reachAndPersistence()
{
    auto batteryLevel=fixture(); batteryLevel.sectors[0].walls[0].extra=900;
    items::World batteryWorld(batteryLevel); player::Inventory batteryInventory;
    check(batteryWorld.collect({50,100,0},true,batteryInventory,{}).transfer==player::Transfer::Taken &&
          batteryInventory.batteryCharge()==144000 && batteryWorld.panels()[0].contents==0xffff,
          "Original class0 battery panel did not use existing pickup path");
    check(canCollect({50,100,0}) && canCollect({10.001,100,0}),"Valid panel reach rejected");
    check(!canCollect({10,100,0}) && !canCollect({150,100,0}) &&
          !canCollect({50,201,0}) && !canCollect({50,0,0}),"Range, face or endpoints differ");
    check(canCollect({50,100,0.5}) && !canCollect({50,100,0.6}) &&
          !canCollect({50,100,3.141592653589793}),"Facing cone differs from half-degree EXE units");
    const collision::World occluder{{{75,0},{75,200}}};
    check(!canCollect({50,100,0},occluder),"Panel collected through a wall");
    auto level = fixture();
    items::World world(level);
    player::Inventory inventory;
    check(!world.collect({50,100,0},false,inventory,{}).panel,"Dead player collected a panel");
    world.collect({50,100,0},true,inventory,{});
    check(world.panels()[0].contents == 0xffff && level.sectors[0].walls[0].extra == 0x2064 &&
          level.sectors[0].entitiesA[0].type == 0x5004,"World state changed immutable source or decoration");
    check(world.collect({50,100,0},true,inventory,{}).transfer == player::Transfer::Empty,
          "Consumed panel reappeared on subsequent update");
    world = items::World(level);
    inventory = player::Inventory{};
    check(world.collect({50,100,0},true,inventory,{}).transfer == player::Transfer::Taken,
          "Level reset did not restore original panel contents");
}

void originals(const std::filesystem::path& path)
{
    const auto level = game_lab::loadLevel1(path).level;
    items::World world(level);
    check(level.playerStart && level.playerStart->sector==0 && level.playerStart->x==300 &&
          level.playerStart->y==320 && level.playerStart->angle==180,"DOS new-game start differs");
    const collision::Point spawn{double(level.sectors[0].gx)+300,double(level.sectors[0].gy)+320};
    check(spawn.x==3500 && spawn.y==320,"Local-to-global start conversion differs");
    for (const auto& wall:collision::staticWorld(level))
        check(!collision::overlaps(wall,spawn,30),"Original spawn overlaps a static wall");
    check(world.panels().size() == 9 && level.accessCode==90 &&
          level.codeTargets==std::vector<std::uint16_t>({0,1}),"Original panel/code data differs");
    const std::uint16_t values[]{0x3001,0x4001,0x11f4,0x2064};
    const std::size_t sectors[]{3,7,8,8}, walls[]{6,10,6,8};
    player::Inventory inventory;
    const auto obstacles = collision::staticWorld(level);
    std::size_t i = 0;
    for (const auto& panel : world.panels()) {
        if (panel.elevator) {
            check((panel.sector==0 || panel.sector==9) && panel.wall==6,"Elevator panel mapping differs");
            continue;
        }
        if (panel.codeReader) {
            check(panel.sector==8 && panel.wall==7 && panel.contents==1,"Code terminal misclassified");
            continue;
        }
        if (panel.codePanel) {
            check(panel.sector==1 && panel.wall==6,"Initial panel misclassified as a supply");
            continue;
        }
        if (panel.cardReader) {
            check(panel.sector == 7 && panel.wall == 17 && panel.contents == 0x0107,
                  "Original C-card reader target differs");
            continue;
        }
        check(panel.contents == values[i] && panel.sector == sectors[i] && panel.wall == walls[i],
              "Original type/parameter mapping differs");
        const double dx = panel.face.b.x-panel.face.a.x, dy = panel.face.b.y-panel.face.a.y;
        const double length = std::hypot(dx,dy);
        const Camera camera{(panel.face.a.x+panel.face.b.x)/2+dy/length*50,
                            (panel.face.a.y+panel.face.b.y)/2-dx/length*50,std::atan2(dx,-dy)};
        const auto collected = world.collect(camera,true,inventory,obstacles);
        check(collected.panel && (collected.transfer == player::Transfer::Taken ||
                                 collected.transfer == player::Transfer::Exchanged),
              "Original panel inaccessible from its physical front face");
        ++i;
    }
    check(inventory.cards()[0] == 1 && inventory.cards()[1] == 0x1001 &&
          inventory.injectors()[0] == 100 && inventory.weapon() == 1 && inventory.ammunition() == 500,
          "Original level transfers disagree with EXE inventory layout");
    std::cout << "Four actual level-1 panels verified against GAME.LAB.\n";
}
} // namespace

int main(int argc, char* argv[])
{
    try {
        cardsAndWeapon(); injectors(); oxygen(); reachAndPersistence();
        if (argc == 2) originals(argv[1]);
        std::cout << "Original panel items, fixed inventory, recovery, access and persistence passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
