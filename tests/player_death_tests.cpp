#include "extremis/cinematics/player_death.h"
#include "app/session/victory_session.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
#include <limits>

void check(bool b,const char* text) { if(!b) throw std::runtime_error(text); }
using D=player::DeathPresentation;
player::DeathResources fixture()
{
    player::DeathResources r;
    for(auto* a:{&r.health,&r.oxygen,&r.gameOver,&r.menu}) {
        a->palette.fill(0x408020ff);
        a->frames.emplace_back(64000,1);
    }
    r.explosion.palette.fill(0x804020ff);
    for(unsigned i=0;i<60;++i) {
        r.explosion.palette[i+1]=((i+1)<<24)|0x000020ff;
        r.explosion.frames.emplace_back(64000,i+1);
    }
    r.oxygen.frames.emplace_back(64000,2); r.oxygen.palette[2]=0x204080ff;
    r.body.assign(6,{1,1,0,0,{2}}); r.health.palette[2]=0x80ff20ff;
    r.font.assign(76,{1,1,0,0,{2}});
    r.menuFont=r.font;
    r.menuArt.assign(3,{62,43,0,0,std::vector<std::uint8_t>(62*43,0)});
    r.menuLines={"RUN","OPTIONS","EXIT","NEW","CONTINUE","BACK"};
    r.lines={"A","B","","","","","",""};
    return r;
}
// Audit the CPU-driven MORT0 loop without assigning an unsupported frame rate.
void healthSequence()
{
    auto r=fixture();
    r.health.palette.fill(255);
    r.health.palette[1]=0x000000ff;
    // Distinct frame colours; 200-row columns make the upper clip observable.
    for(unsigned i=0;i<6;++i) {
        r.health.palette[2+i]=((i+1)*0x20000000u)|0x0000ffff;
        r.body[i]={1,200,0,0,std::vector<std::uint8_t>(200,2+i)};
    }
    D fast(r),slow(r);
    const std::vector<std::uint32_t> scene(64000,0x408020ff);
    for(auto* d:{&fast,&slow}) {
        d->begin(player::DeathCause::Health,scene);
        for(int i=0;i<26;++i) d->advance(0);
        check(d->stage()==D::Stage::Body && d->takeSoundStart(),"Health entry/audio boundary");
    }
    constexpr int x[]{55,75,95,95,90,55};
    for(unsigned step=0;step<175;++step) {
        check(fast.stage()==D::Stage::Body && slow.stage()==D::Stage::Body,"Premature body exit");
        check(fast.pixels()==slow.pixels(),"Elapsed time skipped CPU-driven body iterations");
        if(step==0) check(fast.pixels()[26*320+55]==0xff0000ff,"MORT0 starts fully red");
        if(step>=25) {
            const auto& pixels=fast.pixels();
            const unsigned image=step%6;
            // Pixel colours are converted through the native six-bit palette.
            const unsigned red=(image+1)*32;
            const unsigned dac=(red*63+127)/255;
            const unsigned expectedRed=step==25 ? (dac*255+31)/63 : red;
            const auto expected=(expectedRed<<24)|0x0000ffff;
            check(pixels[26*320+x[image]]==expected,"Six-frame order/x position or red reveal wrong");
            check(pixels[25*320+x[image]]==255,"Body escaped top clip");
            check(pixels[174*320+x[image]]==255,"Body escaped transfer rectangle");
            const int lastRow=std::min(173,225-int(step));
            check(pixels[lastRow*320+x[image]]==expected,"Body failed one-pixel vertical advance");
            if(lastRow<173) check(pixels[(lastRow+1)*320+x[image]]==255,"Stale body pixels after background restore");
            if(step>25 && x[image]!=x[(step-1)%6])
                check(pixels[26*320+x[(step-1)%6]]==255,"Previous sprite was not erased");
        }
        // Reading a completed frame does not advance it; neither does invalid time.
        const auto pixels=fast.pixels();
        fast.advance(-1);
        check(fast.pixels()==pixels,"Invalid time advanced the cinematic");
        fast.advance(0); slow.advance(10);
    }
    check(fast.stage()==D::Stage::Fade && slow.stage()==D::Stage::Fade,"Exactly 175 body draws required");
    check(!fast.takeSoundStop(),"Death audio stopped before the health fade");
}
void run(player::DeathResources r, bool save)
{
    if(save) for(bool oxygen:{false,true}) {
        const auto& movie=oxygen?r.oxygen:r.health;
        const auto& indices=movie.frames[oxygen?movie.frames.size()/2:0];
        std::vector<std::uint32_t> pixels; for(auto index:indices) pixels.push_back(movie.palette[index]);
        auto* s=SDL_CreateSurfaceFrom(320,200,SDL_PIXELFORMAT_RGBA8888,pixels.data(),1280);
        check(s && SDL_SaveBMP(s,oxygen?"/tmp/extremis-oxygen.bmp":"/tmp/extremis-health.bmp"),"Resource preview failed");
        SDL_DestroySurface(s);
    }
    for(auto cause:{player::DeathCause::Health,player::DeathCause::Oxygen,player::DeathCause::SelfDestruction}) {
        D d(r); const std::vector<std::uint32_t> scene(64000,0x408020ff);
        d.begin(cause,scene);
        d.begin(cause,scene);
        check(d.stage()==D::Stage::Tint && !d.takeSoundStart(),"Death must start once before sound");
        d.advance(std::numeric_limits<double>::quiet_NaN());
        for(int i=0;i<26;++i) d.advance(0);
        check(d.takeSoundStart() && !d.takeSoundStart(),"One-shot audio transition");
        if(cause==player::DeathCause::Health) {
            check(d.stage()==D::Stage::Body,"Missing health body sequence");
            for(int i=0;i<175;++i) d.advance(0);
            check(d.stage()==D::Stage::Fade,"Health scroll length");
        } else {
            check(d.stage()==D::Stage::MovieReveal,"Missing oxygen movie");
            for(int i=0;i<26;++i) d.advance(0);
            check(d.stage()==D::Stage::Movie,"Movie reveal length");
            const bool explosion=cause==player::DeathCause::SelfDestruction;
            const auto& movie=explosion?r.explosion:r.oxygen;
            const int threshold=explosion?11:25;
            d.advance(threshold*7457.0/1193182.0);
            check(d.pixels()[0]==movie.palette[movie.frames[0][0]],"Movie advanced before strict tick threshold");
            for(unsigned i=0;i<movie.frames.size();++i) {
                check(d.pixels()[0]==movie.palette[movie.frames[i][0]],"Movie skipped/reordered frames");
                d.advance((threshold+1)*7457.0/1193182.0);
            }
            check(d.stage()==D::Stage::Hold,"Movie frame count");
            d.advance(160*7457.0/1193182.0);
            check(d.stage()==D::Stage::Hold,"Hold must exceed 160 ticks");
            d.advance(2*7457.0/1193182.0);
            check(d.stage()==D::Stage::Fade,"Death movie hold failed");
            if(explosion) check(d.takeSoundStop() && !d.takeSoundStop(),"Explosion sound must stop BEFORE fade");
        }
        for(unsigned i=0;i<(cause==player::DeathCause::Health?21:51);++i) d.advance(0);
        check(d.stage()==D::Stage::GameOverReveal &&
              d.takeSoundStop()==(cause!=player::DeathCause::SelfDestruction) && !d.takeSoundStop(),"Audio stop/game-over transition");
        for(int i=0;i<26;++i) d.advance(0);
        for(int i=0;i<1000 && d.stage()==D::Stage::Text;++i) d.advance(10*7457.0/1193182.0,false,true);
        check(d.stage()==D::Stage::Wait,"Text failed to complete");
        if(save) {
            auto* s=SDL_CreateSurfaceFrom(320,200,SDL_PIXELFORMAT_RGBA8888,const_cast<std::uint32_t*>(d.pixels().data()),1280);
            check(s && SDL_SaveBMP(s,"/tmp/extremis-gameover.bmp"),"Preview failed"); SDL_DestroySurface(s);
        }
        d.advance(100,true,true);
        check(d.stage()==D::Stage::Wait,"Held key skipped release gate");
        d.advance(0,false,false);
        if(cause==player::DeathCause::Health) d.advance(0,true,true);
        else {
            d.advance(1600*7457.0/1193182.0);
            check(d.stage()==D::Stage::Wait,"Game-over timeout must exceed 1600 ticks");
            d.advance(2*7457.0/1193182.0);
        }
        check(d.stage()==D::Stage::GameOverFade,"Fresh key/timeout failed to dismiss");
        for(int i=0;i<101;++i) d.advance(0);
        check(d.stage()==D::Stage::Finished && d.pixels()[0]==255,"Final fade failed");
        d.advance(0);
        for(int i=0;i<51;++i) d.advance(0);
        check(d.stage()==D::Stage::Menu,"Missing return to native menu");
        check(d.menuInput(0,true)==D::Request::None,"RUN must open submenu");
        check(d.menuInput(0,true)==D::Request::Restart,"New game request missing");
        d.menuInput(2,false); d.menuInput(0,true);
        check(d.menuInput(2,true)==D::Request::Exit,"Exit request missing");
        d.reset(); check(!d.active(),"Reset failed");
        d.begin(cause,scene); check(d.stage()==D::Stage::Tint,"Restart failed");
    }
}
int main(int argc,char** argv)
{
    try {
        D menu(fixture()); VictorySession victory; unsigned plays=0;
        auto play=[&]() { ++plays; return StartupCreditsResult::Continue; };
        auto enter=[&]() { menu.beginMenu(); };
        victory.update(false,play,enter);
        check(plays==0 && !menu.active(),"Premature victory handoff");
        victory.update(true,play,enter);
        check(plays==1 && menu.stage()==D::Stage::MenuReveal,"Victory did not enter native menu");
        for(int i=0;i<51;++i) menu.advance(0);
        victory.update(true,play,enter);
        check(plays==1 && menu.stage()==D::Stage::Menu,"Victory replayed or reset menu");
        check(menu.menuInput(0,true)==D::Request::None && menu.menuInput(0,true)==D::Request::Restart,"Victory menu cannot restart");
        victory.reset(); menu.reset();
        victory.update(true,[&]() { ++plays; return StartupCreditsResult::Quit; },enter);
        check(plays==2 && !menu.active(),"Quit entered menu");
        victory.reset();
        victory.update(true,[]() { return StartupCreditsResult::Error; },enter);
        check(!menu.active(),"Failed FSEQ entered menu");
        {
            auto r=fixture();D d(r);std::vector<std::uint32_t> black(64000,255);
            d.begin(player::DeathCause::SelfDestruction,black);
            check(d.pixels()[0]==255,"Explosion tint must begin at white amount zero");
            for(unsigned i=0;i<25;++i)d.advance(0);
            check(d.pixels()[0]==0xffffffff,"Explosion tint must reach full white");
            auto missing=r;missing.explosion={};D invalid(std::move(missing));bool failed=false;
            try {invalid.begin(player::DeathCause::SelfDestruction,black);} catch(const std::runtime_error&) {failed=true;}
            check(failed && !invalid.active(),"Missing explosion resources left partial death state");
            invalid.begin(player::DeathCause::Health,black);
            check(invalid.active(),"Missing explosion disabled ordinary health death");
        }
        healthSequence();
        run(fixture(),false);
        if(argc>1) {
            auto r=player::loadDeathResources(argv[1]);
            D loading(r);loading.loadExplosion(argv[1]);
            r.explosion=anm_format::load(std::filesystem::path(argv[1])/"VAIS_EXP.ANM");
            check(r.explosion.frames.size()==60,"Original explosion frame count");
            std::cout<<"Original resources: "<<r.body.size()<<" body sprites, "<<r.oxygen.frames.size()<<" oxygen frames, "<<r.font.size()<<" glyphs\n";
            run(std::move(r),argc>2);
        }
        std::cout<<"Death sequences, audio events, timing boundaries, release gate and reset passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
