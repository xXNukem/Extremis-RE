#include "original_data.h"

#include "extremis/resources/game_lab.h"
#include "extremis/resources/audio_data.h"
#include "extremis/resources/texture_bank.h"

#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <set>
#include <vector>

namespace app {
namespace {
bool required(const std::filesystem::path& directory, const std::string& name, std::string& error)
{
    const auto path=directory/name;
    std::error_code ec;
    if (!std::filesystem::is_symlink(path,ec) && std::filesystem::is_regular_file(path,ec)) {
        std::ifstream file(path,std::ios::binary);
        if (file && file.peek()!=std::char_traits<char>::eof()) return true;
    }
    error="Missing or unreadable original resource: " + path.string();
    return false;
}
}

bool validateOriginalData(const std::filesystem::path& lab, std::string& error)
{
    try {
        const auto directory=lab.parent_path();
        const auto data=directory/"DATA";
        // Check every level with the production parser, including geometry.
        const auto levels=game_lab::loadBankTable(lab);
        if (levels.size()!=30) { error="GAME.LAB does not contain 30 levels"; return false; }
        for (unsigned level=0;level<levels.size();++level) game_lab::loadLevel(lab,level);
        std::error_code ec;
        if (std::filesystem::is_symlink(data,ec) || !std::filesystem::is_directory(data,ec)) {
            error="Missing DATA directory beside " + lab.string();
            return false;
        }
        const auto bank=audio::loadBank(data/"BANK.DSS");
        if (bank.size()!=30) { error="BANK.DSS does not contain 30 effects"; return false; }
        // These names come from the port's actual presentation, UI, cinematic
        // and audio loaders. EXTREMIS.EXE supplies intro text at runtime.
        constexpr const char* fixed[]{
            "CASQUE.SPR","GOUTTE.SPR","ADALLE.SPR","CONCEPT.PAL","CONCEPT.SPR",
            "CONCEPT.ANM","TITRE.ANM","MAP.SPR","BOUSSOLE.SPR","PAL_DI.PAL",
            "TYPO_3X8.BIN","PSEQ1.ANM","PSEQ2_1.VGA","PSEQ2_2.VGA",
            "PSEQ2_3.SPR","PSEQ3.ANM","PSEQ4.ANM","PSEQ6_1.ANM",
            "PSEQ6_2.SPR","PSEQ7_1.VGA","PSEQ7_2.SPR","PSEQ8.ANM",
            "PSEQ10_1.VGA","PSEQ10_2.SPR","PSEQ10_3.SPR","PSEQ11_1.VGA",
            "PSEQ11_2.SPR","PSEQ12_1.VGA","PSEQ12_2.SPR","PSEQ14_1.VGA",
            "PSEQ14_2.VGA","PSEQ14_3.SPR","CON_VUET.SPR","BANK.DSS",
            "BARGE2.DSS","CABINE.DSS","RADAR.DSS","LABO.DSS","ALARME1.DSS",
            "ALARME4.DSS","TOUCHE.DSS","OXYGENE.DSS","EXPLO.DSS",
            "FSEQ1.ANM","FSEQ2_1.VGA","FSEQ2_2.SPR","FSEQ3.ANM",
            "FSEQ4_1.VGA","FSEQ4_2.SPR","FSEQ5.ANM","FSEQ6.ANM",
            "MONSTRE.DSS","VENT.DSS","A_LEVEL.VGA","A_LEVEL.SPR",
            "NAV_INTE.VGA","NAV_HANG.VGA","NAV_SEQ3.ANM","NAV_SEQ4.ANM",
            "NAV_SEQ7.ANM","NAV_SEQ8.ANM","ARR_BASE.ANM","VAIS_EXP.ANM",
            "ALARME3.DSS","DEPART.DSS","BARGE1.DSS","NAV_INST.SPR",
            "NAV_INS2.SPR","NAV_SEQ2.SPR","INBASE.SPR",
            "MORT0.VGA","MORT0.SPR","MORT1.ANM","GAMEOVER.VGA","TYPOOVER.SPR",
            "CON_VUE.VGA","CON_ECR1.VGA","CON_ECR2.VGA","CON_VISA.SPR",
            "A_ARME.ANM","CH_OBJ2.SPR","CH_OBJ3.SPR","CHARME2.SPR",
            "CHARME3.SPR","B_PLAN2.SPR","CHARME.SPR","CH_OBJ4A.SPR",
            "CHARME4A.SPR","INVENT_A.SPR","INVENT_T.SPR","B_PORTE.SPR",
            "B_PLAN3.SPR","CH_LEVEL.SPR","B_CODE.SPR","TYPO_3X5.BIN",
            "BOMBE.SPR"};
        for (const auto* name:fixed) if (!required(data,name,error)) return false;
        for (unsigned weapon=0;weapon<6;++weapon) {
            if (!required(data,"ARME"+std::to_string(weapon)+".SPR",error) ||
                !required(data,"BALLE"+std::to_string(weapon)+".SPR",error)) return false;
        }
        if (!required(directory,"EXTREMIS.EXE",error)) return false;
        std::error_code sizeError;
        if (std::filesystem::file_size(directory/"EXTREMIS.EXE",sizeError)<0x23bb0 || sizeError) {
            error="EXTREMIS.EXE is too small for the original introduction text";
            return false;
        }
        std::ifstream exe(directory/"EXTREMIS.EXE",std::ios::binary);
        constexpr std::array<std::pair<unsigned,unsigned>,6> texts{{
            {0x23906,350},{0x23a68,316}, // Introduction loader.
            {0x22e2c,178},{0x22edf,197},{0x22fa5,197},{0x2306b,214} // Shuttle loader.
        }};
        for (const auto& [offset,length]:texts) {
            std::string text(length,'\0');
            exe.seekg(offset);
            if (!exe.read(text.data(),length) || exe.get()!=0 ||
                text.find('\0')!=std::string::npos) {
                error="EXTREMIS.EXE lacks compatible original introduction or shuttle text";
                return false;
            }
        }
        std::set<std::pair<unsigned,unsigned>> textures;
        for (const auto& level:levels) {
            const auto pal=unsigned(level.palBank), dalle=unsigned(level.dalleBank);
            textures.emplace(dalle,pal);
            for (const auto& name:{"OBJET"+std::to_string(level.objetBank)+".SPR",
                                   "MONS"+std::to_string(level.monsBank)+".SPR",
                                   "MONS"+std::to_string(level.monsBank)+".SPT"})
                if (!required(data,name,error)) return false;
        }
        for (const auto& [dalle,pal]:textures) {
            // Use the production texture and palette parsers to reject a LAB
            // paired with incompatible or corrupt level banks.
            wall_textures::load(data,dalle,pal);
            wall_textures::loadPlanes(data,dalle,pal);
        }
        error.clear();
        return true;
    } catch (const std::exception& e) {
        error=e.what();
        return false;
    }
}

std::filesystem::path findOriginalData(const std::filesystem::path& executableDirectory)
{
    // SDL promises a trailing separator, which makes path::filename() empty.
    const auto base=executableDirectory.filename().empty()
        ? executableDirectory.parent_path() : executableDirectory;
    std::vector<std::filesystem::path> roots{base};
    if ((base.filename()=="MacOS" || base.filename()=="Resources") &&
        base.parent_path().filename()=="Contents" &&
        base.parent_path().parent_path().extension()==".app") {
        // SDL_GetBasePath() defaults to Contents/Resources inside a macOS app.
        // Include the actual executable directory as well as the app's parent.
        if (base.filename()=="Resources")
            roots.push_back(base.parent_path()/"MacOS");
        roots.push_back(base.parent_path().parent_path().parent_path());
    }
    for (const auto& root:roots) {
        std::vector<std::filesystem::path> frontier{root};
        unsigned visited=0;
        for (unsigned depth=0;depth<=4 && !frontier.empty();++depth) {
            std::sort(frontier.begin(),frontier.end());
            std::vector<std::filesystem::path> next;
            bool budgetExceeded=false;
            for (const auto& directory:frontier) {
                if (++visited>1024) { budgetExceeded=true; break; }
                const auto lab=directory/"GAME.LAB";
                std::error_code ec;
                if (!std::filesystem::is_symlink(lab,ec) &&
                    std::filesystem::is_regular_file(lab,ec)) {
                    std::string error;
                    if (validateOriginalData(lab,error)) return lab;
                }
                if (depth==4) continue;
                std::filesystem::directory_iterator it(directory,
                    std::filesystem::directory_options::skip_permission_denied,ec);
                unsigned entries=0;
                std::vector<std::filesystem::path> children;
                for (;!ec && it!=std::filesystem::directory_iterator{} && entries<256;it.increment(ec),++entries) {
                    const auto& entry=*it;
                    if (entry.path().filename().string().front()=='.' || entry.is_symlink(ec)) continue;
                    if (entry.is_directory(ec)) children.push_back(entry.path());
                }
                // If a directory exceeds the bound, skip its children entirely;
                // filesystem iteration order cannot choose which names survive.
                if (!ec && it==std::filesystem::directory_iterator{})
                    next.insert(next.end(),children.begin(),children.end());
            }
            if (budgetExceeded) break;
            frontier=std::move(next);
        }
    }
    return {};
}
}
