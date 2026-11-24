#include "extremis/ui/cinematic_text.h"
#include "extremis/ui/localization.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
    try {
        using namespace localization;
        for(auto language:{Language::Spanish,Language::French}) {
            if(!cinematicById("comm.page.01",language).empty() ||
               !cinematicById("shuttle.script.02",language).empty() ||
               !cinematicById("shuttle.script.04",language).empty())
                throw std::runtime_error("Unverified prose was translated");
            if(!cinematicById("intro.pseq3.body",Language::English).empty())
                throw std::runtime_error("English original must remain external");
        }
        if(argc==2) {
            const std::filesystem::path data=argv[1];
            const auto intro=spr_format::load(data/"CON_VUET.SPR");
            const auto comm=spr_format::load(data/"CON_VISA.SPR");
            const auto shuttle=spr_format::load(data/"NAV_INST.SPR");
            const auto over=spr_format::load(data/"TYPOOVER.SPR");
            const auto credits=spr_format::load(data/"CASQUE.SPR");
            auto map=[](unsigned char c)->int {
                if(c>='0'&&c<='9') return c-'0';
                if(c>='A'&&c<='Z') return c-'A'+10;
                if(c>='a'&&c<='z') return c-'a'+36;
                const std::string p=":-?!',.";
                const auto n=p.find(c);return n==std::string::npos?-1:int(n)+62;
            };
            auto oneBased=[&](unsigned char c){return map(c);};
            std::vector<std::uint8_t> canvas(64000);
            for(auto language:{Language::Spanish,Language::French}) {
                for(auto id:{"intro.pseq3.body","intro.pseq6.body"})
                    if(!drawIndexed(canvas,intro,cinematicById(id,language),1,1,10,10,310,195,11,oneBased,true))
                        throw std::runtime_error(std::string("Intro text does not fit: ")+id);
                for(unsigned page=2;page<=7;++page) {
                    const auto id="comm.page.0"+std::to_string(page);
                    if(!drawIndexed(canvas,comm,cinematicById(id,language),1,1,96,19,256,151,6,map))
                        throw std::runtime_error("Communication does not fit: "+id);
                }
                for(auto id:{"shuttle.script.01","shuttle.script.03"}) {
                    std::string script="-------------------\r  ";
                    const auto translated=cinematicById(id,language);
                    for(std::size_t j=0;j<translated.size();++j) {
                        if(j+2<translated.size() && translated.substr(j,3)==" / ") {script+="\r  ";j+=2;}
                        else script+=translated[j];
                    }
                    script+="\r-------------------";
                    if(!drawIndexed(canvas,shuttle,script,1,1,125,5,310,90,5,map))
                        throw std::runtime_error(std::string("Shuttle text does not fit: ")+id);
                }
                for(unsigned i=1;i<=8;++i) {
                    const auto id="ending.game_over.line.0"+std::to_string(i);
                    const auto line=cinematicById(id,language);
                    if(line.empty()) continue;
                    if(measureIndexed(over,line,map)>320 ||
                       !drawIndexed(canvas,over,line,1,1,0,60+10*int(i-1),320,70+10*int(i-1),10,map))
                        throw std::runtime_error("Game Over line does not fit: "+id);
                }
                if(language==Language::Spanish) for(unsigned i=1;i<=4;++i) {
                    const auto id="intro.who.0"+std::to_string(i);
                    const auto creditMap=[](unsigned char c)->int {
                        if(c>='a'&&c<='z') c=static_cast<unsigned char>(c-'a'+'A');
                        return c>='A'&&c<='Z'?42+c-'A':-1;
                    };
                    if(measureIndexed(credits,cinematicById(id,language),creditMap)>320)
                        throw std::runtime_error("Credit label does not fit: "+id);
                }
            }
        }
        std::cout<<"Cinematic localization and original font bounds passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
