#include "anm_format.h"
#include <fstream>
#include <stdexcept>

namespace anm_format {
namespace {
std::uint16_t word(const std::vector<std::uint8_t>& bytes,std::size_t at)
{
    if(at+2>bytes.size()) throw std::runtime_error("Truncated ANM word");
    return bytes[at] | (unsigned(bytes[at+1])<<8);
}
class Lzss {
public:
    Lzss(const std::vector<std::uint8_t>& bytes,std::size_t start,std::size_t end)
        : bytes_(bytes),position_(start),end_(end) { dictionary_.fill(32); }
    std::uint8_t next()
    {
        std::uint8_t value;
        if(remaining_) { value=dictionary_[source_]; source_=(source_+1)&4095; --remaining_; }
        else {
            if(!mask_) { flags_=read(); mask_=128; }
            const bool literal=(flags_&mask_)!=0; mask_>>=1;
            if(literal) value=read();
            else {
                const unsigned low=read(),high=read();
                source_=low|((high&240)<<4); remaining_=(high&15)+3;
                return next();
            }
        }
        dictionary_[write_]=value; write_=(write_+1)&4095;
        return value;
    }
private:
    std::uint8_t read() {
        if(position_>=end_ || position_>=bytes_.size()) throw std::runtime_error("Truncated ANM LZSS stream");
        return bytes_[position_++];
    }
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_,end_;
    std::array<std::uint8_t,4096> dictionary_{};
    unsigned write_=4078,source_=0,remaining_=0,mask_=0,flags_=0;
};
}
Animation decode(const std::vector<std::uint8_t>& bytes)
{
    if(bytes.size()<774 || bytes[0]!='A' || bytes[1]!='N' || bytes[2]!='M' || bytes[3]!='1')
        throw std::runtime_error("Invalid ANM1 header");
    const unsigned count=word(bytes,4);
    if(!count || count>1000) throw std::runtime_error("Invalid ANM frame count");
    Animation result;
    for(unsigned i=0;i<256;++i) {
        std::uint32_t rgba=0;
        for(unsigned c=0;c<3;++c) {
            const unsigned v=bytes[6+3*i+c];
            if(v>63) throw std::runtime_error("Invalid ANM VGA palette");
            rgba=(rgba<<8)|((v*255+31)/63);
        }
        result.palette[i]=(rgba<<8)|255;
    }
    result.palette[0]=255; // Player 0x1477b forces the first palette entry black.
    std::size_t offset=774;
    std::vector<std::uint8_t> frame(64000,0);
    for(unsigned index=0;index<count;++index) {
        const unsigned size=word(bytes,offset); offset+=2;
        const unsigned table=index && size?800:0;
        if(offset+table+size>bytes.size()) throw std::runtime_error("Truncated ANM frame");
        Lzss stream(bytes,offset+table,offset+table+size);
        if(!index) {
            if(!size) throw std::runtime_error("Missing initial ANM frame");
            for(auto& pixel:frame) pixel=stream.next();
        } else if(size) {
            std::size_t target=0;
            for(unsigned row=0;row<200;++row) {
                target+=word(bytes,offset+row*4);
                const unsigned length=word(bytes,offset+row*4+2);
                if(target+length>frame.size()) throw std::runtime_error("ANM delta exceeds framebuffer");
                for(unsigned p=0;p<length;++p) frame[target++]=stream.next();
            }
        }
        offset+=table+size; result.frames.push_back(frame);
    }
    if(offset!=bytes.size()) throw std::runtime_error("Unexpected trailing ANM data");
    return result;
}
Animation load(const std::filesystem::path& path)
{
    std::ifstream file(path,std::ios::binary);
    if(!file) throw std::runtime_error("Cannot open ANM: "+path.string());
    return decode({std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()});
}
}
