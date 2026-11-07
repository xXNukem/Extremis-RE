#include "anm_format.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool b,const char* text) {if(!b) throw std::runtime_error(text);}
void append(std::vector<std::uint8_t>& b,unsigned v) {b.push_back(v&255);b.push_back(v>>8);}
std::vector<std::uint8_t> fixture()
{
    std::vector<std::uint8_t> b(774,0); b[0]='A';b[1]='N';b[2]='M';b[3]='1';b[4]=2;
    std::vector<std::uint8_t> encoded;
    // 64000 spaces, references overlap and wrap the 4096-byte dictionary.
    for(unsigned token=0;token<3556;++token) {
        if(token%8==0) encoded.push_back(0);
        encoded.push_back(0); encoded.push_back(15);
    }
    append(b,unsigned(encoded.size()));b.insert(b.end(),encoded.begin(),encoded.end());
    append(b,3); append(b,10);append(b,2);
    b.resize(b.size()+796,0);b.insert(b.end(),{192,42,43});
    return b;
}
void rejects(const std::vector<std::uint8_t>& b)
{
    bool failed=false;try {anm_format::decode(b);} catch(const std::runtime_error&) {failed=true;}
    check(failed,"Malformed ANM accepted");
}
}
int main(int argc,char** argv)
{
    try {
        const auto bytes=fixture(); const auto film=anm_format::decode(bytes);
        check(film.frames.size()==2 && std::all_of(film.frames[0].begin(),film.frames[0].end(),[](auto p){return p==32;}),"Initial ANM LZSS frame wrong");
        check(film.frames[1][9]==32 && film.frames[1][10]==42 && film.frames[1][11]==43 && film.frames[1][12]==32,"Delta did not preserve previous frame");
        for(std::size_t size:{0u,773u,775u,800u,static_cast<unsigned>(bytes.size()-1)}) rejects({bytes.begin(),bytes.begin()+size});
        auto bad=bytes;bad[3]='2';rejects(bad);bad=bytes;bad[6]=64;rejects(bad);
        bad=bytes;bad.push_back(0);rejects(bad);
        bad=bytes;const auto delta=776+unsigned(bytes[774])+(unsigned(bytes[775])<<8);
        bad[delta+2]=255;bad[delta+3]=255;rejects(bad);
        bad=bytes;bad[4]=3;append(bad,0);
        check(anm_format::decode(bad).frames[1]==anm_format::decode(bad).frames[2],"Empty delta changed frame");
        if(argc==2) {
            const auto real=anm_format::load(argv[1]);check(real.frames.size()==5,"Weapon movie frame count changed");
            check(real.frames.front()!=real.frames.back(),"ANM delta frames never changed");
        }
        std::cout<<"ANM1 full/delta frames, LZSS overlap and malformed bounds passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
