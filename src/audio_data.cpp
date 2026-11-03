#include "audio_data.h"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace audio {
namespace {
std::uint32_t integer(const std::vector<std::uint8_t>& bytes, std::size_t p, unsigned n)
{
    if (p > bytes.size() || n > bytes.size() - p) throw std::runtime_error("Truncated DSS field");
    std::uint32_t result = 0;
    for (unsigned i = 0; i < n; ++i) result |= std::uint32_t(bytes[p+i]) << (8*i);
    return result;
}

std::size_t sampleSize(const std::vector<std::uint8_t>& bytes, std::size_t start)
{
    if (start > bytes.size() || bytes.size() - start < 44)
        throw std::runtime_error("Truncated DSS header");
    const auto count = integer(bytes,start+30,4);
    if (count == 0 || count > bytes.size() - start - 44)
        throw std::runtime_error("Invalid DSS sample length");
    return std::size_t(44) + count;
}

Sample sampleAt(const std::vector<std::uint8_t>& bytes, std::size_t start)
{
    const auto size = sampleSize(bytes,start);
    if (std::string(bytes.begin()+start,bytes.begin()+start+4) != "DSS1")
        throw std::runtime_error("Unsupported DSS signature");
    Sample sample;
    sample.name.assign(bytes.begin()+start+4,bytes.begin()+start+28);
    while (!sample.name.empty() && (sample.name.back() == ' ' || sample.name.back() == '\0'))
        sample.name.pop_back();
    sample.rate = integer(bytes,start+28,2);
    if (sample.rate != 5000) throw std::runtime_error("Unvalidated DSS sample rate (expected 5000)");
    sample.loopFlag = integer(bytes,start+34,2);
    sample.loopBegin = integer(bytes,start+36,4);
    sample.loopEnd = integer(bytes,start+40,4);
    if (sample.loopFlag && (sample.loopBegin >= sample.loopEnd || sample.loopEnd > size-44))
        throw std::runtime_error("Invalid DSS loop bounds");
    sample.pcm.reserve(size-44);
    for (auto p = start+44; p < start+size; ++p)
        sample.pcm.push_back((int(bytes[p])-128)/128.0f);
    return sample;
}

std::vector<std::uint8_t> read(const std::filesystem::path& path)
{
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file || file.tellg() < 0 || file.tellg() > 128*1024*1024)
        throw std::runtime_error("Cannot read DSS or unsupported size: " + path.string());
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))
        throw std::runtime_error("Cannot read " + path.string());
    return bytes;
}
} // namespace

Sample decodeSample(const std::vector<std::uint8_t>& bytes)
{
    if (sampleSize(bytes,0) != bytes.size()) throw std::runtime_error("Trailing DSS data");
    return sampleAt(bytes,0);
}

Bank decodeBank(const std::vector<std::uint8_t>& bytes)
{
    const auto count = integer(bytes,0,2);
    if (count == 0 || count > bytes.size()/47) throw std::runtime_error("Invalid DSS bank count");
    Bank bank;
    bank.reserve(count);
    std::size_t p = 2;
    for (unsigned i = 0; i < count; ++i) {
        const auto volume = integer(bytes,p,1), continuous = integer(bytes,p+1,1);
        const auto priority = integer(bytes,p+2,1);
        if (volume > 64 || (continuous != 0 && continuous != 255))
            throw std::runtime_error("Unsupported DSS bank metadata");
        p += 3;
        const auto size = sampleSize(bytes,p);
        bank.push_back({sampleAt(bytes,p),std::uint8_t(volume),std::uint8_t(continuous),
                        std::uint8_t(priority)});
        p += size;
    }
    if (p != bytes.size()) throw std::runtime_error("DSS bank count/length mismatch");
    return bank;
}

Sample loadSample(const std::filesystem::path& path) { return decodeSample(read(path)); }
Bank loadBank(const std::filesystem::path& path) { return decodeBank(read(path)); }
} // namespace audio
