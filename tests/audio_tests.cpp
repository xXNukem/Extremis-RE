#include "extremis/resources/audio_data.h"
#include "app/startup/port_menu.h"
#include "extremis/audio/audio_mixer.h"
#include "platform/sdl/audio/audio_player.h"
#include "extremis/audio/game_audio.h"
#include "extremis/world/door.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
void put(std::vector<std::uint8_t>& b, std::size_t p, unsigned value, unsigned size)
{
    for (unsigned i = 0; i < size; ++i) b[p+i] = (value >> (8*i)) & 255;
}
std::vector<std::uint8_t> sample()
{
    std::vector<std::uint8_t> b(44+128,255);
    b[0]='D'; b[1]='S'; b[2]='S'; b[3]='1';
    std::fill(b.begin()+4,b.begin()+28,' ');
    b[4]='x';
    put(b,28,5000,2); put(b,30,128,4); put(b,34,0,2);
    put(b,36,0,4); put(b,40,0,4);
    b[44]=0; b[45]=128; b[46]=255;
    return b;
}
std::vector<std::uint8_t> bankBytes(unsigned count = 2, std::uint8_t continuous = 0)
{
    const auto s = sample();
    std::vector<std::uint8_t> b{std::uint8_t(count & 255),std::uint8_t(count >> 8)};
    for (unsigned i=0; i<count; ++i) {
        b.insert(b.end(),{64,continuous,1}); b.insert(b.end(),s.begin(),s.end());
    }
    return b;
}

void writeBank(const std::filesystem::path& path, unsigned count = 2,
               std::uint8_t continuous = 0)
{
    std::filesystem::create_directories(path.parent_path());
    const auto bytes = bankBytes(count,continuous);
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    check(bool(file), "Cannot write audio fixture");
}
void decoding()
{
    const auto s = audio::decodeSample(sample());
    check(s.name=="x" && s.rate==5000 && s.pcm.size()==128 && s.pcm[0]==-1 &&
          s.pcm[1]==0 && s.pcm[2]==127/128.0f,"DSS header/unsigned PCM conversion failed");
    check(audio::decodeBank(bankBytes()).size()==2,"Bank traversal failed");
    for (int kind=0; kind<7; ++kind) {
        auto b = sample();
        if (kind==0) b.resize(43);
        if (kind==1) b[0]='?';
        if (kind==2) put(b,30,129,4);
        if (kind==3) put(b,28,0,2);
        if (kind==4) b.push_back(0);
        if (kind==5) { put(b,34,65535,2); put(b,36,80,4); put(b,40,70,4); }
        if (kind==6) { put(b,34,65535,2); put(b,40,129,4); }
        bool failed=false;
        try { audio::decodeSample(b); } catch (const std::runtime_error&) { failed=true; }
        check(failed,"Invalid DSS sample accepted");
    }
    for (int kind=0; kind<5; ++kind) {
        auto b=bankBytes();
        if (kind==0) b[0]=3;
        if (kind==1) b[2]=65;
        if (kind==2) b[3]=1;
        if (kind==3) b.pop_back();
        if (kind==4) b.push_back(0);
        bool failed=false;
        try { audio::decodeBank(b); } catch (const std::runtime_error&) { failed=true; }
        check(failed,"Invalid DSS bank accepted");
    }
}
void mixing()
{
    auto bank = audio::decodeBank(bankBytes());
    vortex::audio::Mixer mixer(audio::mixerConfig);
    mixer.setSamples(audio::mixerSamples(bank));
    float frames[512]{};
    check(!mixer.play(99),"Invalid sound ID accepted");
    for (int i=0;i<4;++i) check(mixer.play(0),"Simultaneous voice rejected");
    check(!mixer.play(0) && mixer.active()==4,"Four-voice limit failed");
    mixer.render(frames,128);
    check(mixer.active()==0 && std::abs(frames[64]-0.6f*127/128)<1e-6,
          "Voice completion or normalized mix failed");
    check(std::all_of(std::begin(frames),std::end(frames),[](float x){return std::abs(x)<=1;}),
          "Mixer clipped/distorted");
    mixer.render(frames,16);
    check(std::all_of(frames,frames+16,[](float x){return x==0;}),"Finished effect not silent");
    check(mixer.play(1,audio::playbackOptions(true,audio::Playback::Native)),"Continuous request failed");
    mixer.render(frames,512);
    check(mixer.active()==1,"Continuous effect ended");
    mixer.stop(1); mixer.render(frames,32);
    check(mixer.active()==0 && frames[31]==0,"Release ramp failed");
    bank[0].sample.loopFlag=65535;
    bank[0].sample.loopBegin=16; bank[0].sample.loopEnd=64;
    for (std::size_t i=0;i<128;++i) bank[0].sample.pcm[i]=float(i)/128;
    mixer.setSamples(audio::mixerSamples(bank));
    mixer.play(0); mixer.render(frames,256);
    check(std::abs(frames[128]-0.6f/4*16/128)<1e-6 &&
          std::abs(frames[176]-frames[128])<1e-6,"Original loop end/start semantics failed");
    mixer.clear(); mixer.setVolume(0); mixer.play(0); mixer.render(frames,256);
    check(std::all_of(frames,frames+256,[](float x){return x==0;}),"Configurable volume failed");
}

audio::Effect effect(std::size_t frames)
{
    audio::Effect effect;
    effect.sample.pcm.resize(frames);
    for (std::size_t i = 0; i < frames; ++i)
        effect.sample.pcm[i] = 0.1f + 0.8f * float(i) / float(frames);
    return effect;
}

std::vector<float> capture(vortex::audio::Mixer& mixer, std::size_t frames)
{
    std::vector<float> output(frames);
    const std::array<std::size_t,5> sizes{1,31,2,128,513};
    std::size_t offset = 0, block = 0;
    while (offset < frames) {
        const auto count = std::min(sizes[block++ % sizes.size()], frames - offset);
        mixer.render(output.data() + offset, count);
        offset += count;
    }
    return output;
}

void exactCompletion()
{
    for (std::size_t length : {1,2,31,32,33,127,128,129,511,512,513,3700,5001}) {
        const audio::Bank bank{effect(length)};
        vortex::audio::Mixer mixer(audio::mixerConfig);
        mixer.setSamples(audio::mixerSamples(bank));
        check(mixer.play(0), "One-shot rejected");
        const auto first = capture(mixer, length + 97);
        for (std::size_t i = 0; i < first.size(); ++i) {
            const float ramp = i < length ? std::min({1.0f, float(i+1)/32, float(length-i)/32}) : 0;
            const float expected = i < length ? bank[0].sample.pcm[i] * ramp * 0.6f / 4 : 0;
            check(std::abs(first[i] - expected) < 1e-6,
                  "One-shot source order, ramp or end boundary differs");
        }
        auto stats = *mixer.statistics(0);
        check(stats.requests == 1 && stats.started == 1 && stats.sourceFrames == length &&
              stats.completed == 1 && stats.wraps == 0 && stats.stopped == 0 && mixer.active() == 0,
              "One-shot did not consume exactly its decoded frames");
        check(mixer.play(0), "Consecutive one-shot rejected");
        check(first == capture(mixer, length + 97), "Consecutive effect reused an old cursor/buffer");
        stats = *mixer.statistics(0);
        check(stats.started == 2 && stats.completed == 2 && stats.sourceFrames == 2 * length &&
              stats.wraps == 0, "Consecutive effect consumption differs");
    }
}

void simultaneousEffects()
{
    const audio::Bank bank{effect(33), effect(513)};
    vortex::audio::Mixer mixed(audio::mixerConfig), a(audio::mixerConfig), b(audio::mixerConfig);
    for (auto* mixer : {&mixed, &a, &b}) mixer->setSamples(audio::mixerSamples(bank));
    mixed.play(0); mixed.play(1); a.play(0); b.play(1);
    const auto together = capture(mixed, 700), first = capture(a, 700), second = capture(b, 700);
    for (std::size_t i = 0; i < together.size(); ++i)
        check(std::abs(together[i] - first[i] - second[i]) < 1e-6,
              "Simultaneous effects share a cursor or overwrite buffers");
    check(mixed.statistics(0)->sourceFrames == 33 && mixed.statistics(1)->sourceFrames == 513 &&
          mixed.statistics(0)->completed == 1 && mixed.statistics(1)->completed == 1,
          "Independent effects did not finish at their own length");
}

void loopLifecycle()
{
    auto ambient = effect(128);
    ambient.sample.loopFlag = 65535;
    ambient.sample.loopBegin = 17;
    ambient.sample.loopEnd = 65;
    const audio::Bank bank{ambient, effect(33)};
    vortex::audio::Mixer mixer(audio::mixerConfig);
    mixer.setSamples(audio::mixerSamples(bank));
    check(mixer.play(0), "DSS loop rejected");
    capture(mixer,128);
    check(mixer.statistics(0)->wraps == 1, "Loop skipped full initial payload");
    // Repeated state observations must preserve the cursor; no new play request.
    for (int i = 0; i < 10; ++i) {
        check(mixer.playing(0), "Ambient loop ended between observations");
        capture(mixer,48);
    }
    auto stats = *mixer.statistics(0);
    check(stats.started == 1 && stats.requests == 1 && stats.wraps == 11 &&
          stats.sourceFrames == 608 && stats.completed == 0,
          "Ambient loop restarted or used the wrong repeat interval");
    mixer.play(1);
    mixer.stop(0);
    capture(mixer,64);
    check(!mixer.playing(0) && !mixer.playing(1) &&
          mixer.statistics(0)->stopped == 1 && mixer.statistics(1)->completed == 1,
          "Loop release affected simultaneous one-shot or failed to stop");
    const auto idleFrames = mixer.statistics(0)->sourceFrames;
    const auto silence = capture(mixer,500);
    check(std::all_of(silence.begin(),silence.end(),[](float x){return x == 0;}) &&
          mixer.statistics(0)->sourceFrames == idleFrames, "Stopped loop consumed more PCM");
    mixer.play(0);
    const auto restarted = capture(mixer,128);
    vortex::audio::Mixer fresh(audio::mixerConfig);
    fresh.setSamples(audio::mixerSamples(bank));
    fresh.play(0);
    check(restarted == capture(fresh,128), "Restarted loop retained old cursor/envelope");
    mixer.clear();
    check(mixer.active() == 0, "Clearing an active loop left a voice alive");
}

std::vector<float> converted(const std::vector<float>& input, bool chunks)
{
    const SDL_AudioSpec source{SDL_AUDIO_F32,1,5000}, target{SDL_AUDIO_F32,2,48000};
    SDL_AudioStream* stream = SDL_CreateAudioStream(&source,&target);
    check(stream != nullptr, "Cannot create conversion stream");
    for (std::size_t p = 0; p < input.size();) {
        const auto count = chunks ? std::min<std::size_t>(127,input.size()-p) : input.size();
        check(SDL_PutAudioStreamData(stream,input.data()+p,int(count*sizeof(float))),
              "Cannot queue float source frames");
        p += count;
    }
    check(SDL_FlushAudioStream(stream), "Cannot flush conversion stream");
    const int bytes = SDL_GetAudioStreamAvailable(stream);
    check(bytes > 0 && bytes % (2 * sizeof(float)) == 0, "Invalid converted byte/frame count");
    std::vector<float> result(std::size_t(bytes)/sizeof(float));
    check(SDL_GetAudioStreamData(stream,result.data(),bytes) == bytes, "Incomplete conversion read");
    SDL_DestroyAudioStream(stream);
    return result;
}

void formatConversion()
{
    const audio::Bank bank{effect(3700)};
    vortex::audio::Mixer mixer(audio::mixerConfig);
    mixer.setSamples(audio::mixerSamples(bank));
    mixer.play(0);
    const auto native = capture(mixer,5000);
    const auto whole = converted(native,false), chunks = converted(native,true);
    check(whole.size() == 48000 * 2 && chunks.size() == whole.size(),
          "5000 mono frames did not convert to 48000 stereo frames");
    for (std::size_t i = 0; i < whole.size(); ++i)
        check(std::abs(whole[i]-chunks[i]) < 1e-6, "Resampling depends on input buffer partition");
    for (std::size_t i = 0; i < whole.size(); i += 2) {
        check(whole[i] == whole[i+1], "Mono conversion produced different channels");
        if (i >= 40000 * 2) check(std::abs(whole[i]) < 1e-7,
                                   "Resampling repeated audio after source completion");
    }
}

void waitForCompletion(GameAudio& game, const game_lab::Level& level, const std::vector<doors::Door>& doors, unsigned count)
{
    const auto until = SDL_GetTicks() + 1500;
    while (SDL_GetTicks() < until) {
        game.update(level,doors);
        const auto stats = game.doorStatistics();
        if (stats && stats->completed >= count) return;
        SDL_Delay(5);
    }
    throw std::runtime_error("Door effect did not finish exactly once");
}

void weaponSounds(const std::filesystem::path& directory)
{
    writeBank(directory/"BANK.DSS",30);
    for(unsigned weapon=0;weapon<6;++weapon) {
        GameAudio audio;audio.load(directory);audio.fireWeapon(weapon);
        const auto stats=audio.effectStatistics(weapon+1);
        check(stats && stats->requests==1 && stats->started==1,"Weapon did not play BANK[weapon+1]");
        check(audio.effectStatistics(0)->requests==0,"Loaded weapon used empty sound");
    }
    for(unsigned id:{9u,10u,16u,17u}) {
        GameAudio enemy; enemy.load(directory); enemy.enemyEffect(id);
        const auto stats=enemy.effectStatistics(id);
        check(stats && stats->requests==1 && stats->started==1,"Wrong original enemy effect");
        enemy.enemyEffect(27);
        check(enemy.doorStatistics()->requests==0,"Enemy effect dispatch played an unrelated sound");
    }
    GameAudio empty;empty.load(directory);empty.emptyWeapon();
    check(empty.effectStatistics(0)->requests==1 && empty.effectStatistics(0)->started==1,
          "Empty shot did not play BANK[0]");
}
void doorEvents(const std::filesystem::path& directory)
{
    const auto folder = directory / "door-events";
    writeBank(folder / "BANK.DSS",30,255);
    GameAudio game;
    game.load(folder);
    check(game.doorStatistics().has_value(), "Cannot open dummy door audio device");
    game_lab::Level level; level.connectionStates.resize(15);
    std::vector<doors::Door> doors(15);
    for (unsigned i=0;i<doors.size();++i) doors[i].stateIndex=i;
    game.update(level,doors);
    check(game.doorStatistics()->requests == 0, "Idle doors triggered audio");
    for (auto& state : level.connectionStates) state=0x11e;
    game.update(level,doors,0,false);
    check(game.doorStatistics()->requests==0,"Paused leaf triggered movement audio");
    waitForCompletion(game,level,doors,1);
    for (int i = 0; i < 100; ++i) game.update(level,doors);
    auto stats = *game.doorStatistics();
    check(stats.requests == 1 && stats.started == 1 && stats.sourceFrames == 128 &&
          stats.wraps == 0 && stats.completed == 1, "Movement retriggered a completed door effect");
    for (auto& state : level.connectionStates) state=90;
    game.update(level,doors);
    check(game.doorStatistics()->requests == 1 && game.doorStatistics()->stopped == 0,
          "Endpoint retriggered/truncated the effect");
    for (const auto phase:{60,61,64,89,90}) {
        for (auto& state:level.connectionStates) state=0x100|phase;
        game.update(level,doors);
        check(game.doorStatistics()->requests==1,"Hidden interval triggered leaf audio");
    }
    level.connectionStates[0]=30;
    game.update(level,doors);
    level.connectionStates[0]=0;
    waitForCompletion(game,level,doors,2);
    stats = *game.doorStatistics();
    check(stats.requests == 2 && stats.started == 2 && stats.completed == 2 &&
          stats.sourceFrames == 256 && stats.wraps == 0 && stats.stopped == 0,
          "Consecutive closing effect repeated or was cut off at endpoint");
}

void footstepEvents(const std::filesystem::path& directory)
{
    const auto folder = directory / "step-events";
    writeBank(folder / "BANK.DSS",30,0);
    GameAudio game;
    game.load(folder);
    check(game.effectStatistics(7).has_value(), "Cannot open dummy footstep device");
    game_lab::Level level; level.connectionStates={0};
    const std::vector<doors::Door> idleDoors;
    for (int frame = 0; frame < 100; ++frame) game.update(level,idleDoors,0);
    check(game.effectStatistics(7)->started == 0, "Idle player triggered footsteps");
    game.update(level,idleDoors,1);
    check(game.effectStatistics(7)->started == 1, "Effective movement did not start original ID 7");
    game.update(level,idleDoors,3);
    check(game.effectStatistics(7)->started == 1, "Duplicate cycle events overlapped a step");
    const auto until = SDL_GetTicks() + 1500;
    while (game.effectStatistics(7)->completed < 1 && SDL_GetTicks() < until) {
        game.update(level,idleDoors,0);
        SDL_Delay(5);
    }
    auto step = *game.effectStatistics(7);
    check(step.completed == 1 && step.sourceFrames == 128 && step.wraps == 0,
          "Footstep failed to finish once after stopping movement");
    std::vector<doors::Door> doors(1);
    level.connectionStates[0]=0x11e;
    game.update(level,doors,1); // A new full step and the door cue share the existing mixer.
    check(game.effectStatistics(7)->started == 2 && game.doorStatistics()->started == 1,
          "Footstep suppressed or restarted door audio");
    waitForCompletion(game,level,doors,1);
    const auto end = SDL_GetTicks() + 1500;
    while (game.effectStatistics(7)->completed < 2 && SDL_GetTicks() < end) SDL_Delay(5);
    step = *game.effectStatistics(7);
    check(step.completed == 2 && step.sourceFrames == 256 && step.wraps == 0 &&
          game.doorStatistics()->sourceFrames == 128 && game.doorStatistics()->wraps == 0,
          "Simultaneous footsteps/door did not complete independently");
    for (int frame = 0; frame < 100; ++frame) game.update(level,doors,0);
    check(game.effectStatistics(7)->started == 2 && game.doorStatistics()->started == 1,
          "Finished footsteps or ongoing door movement retriggered audio");
}

void noOverlappingSteps()
{
    const audio::Bank bank{effect(3700),effect(5001)};
    vortex::audio::Mixer mixer(audio::mixerConfig);
    mixer.setSamples(audio::mixerSamples(bank));
    check(!mixer.playing(0), "Idle voice reported active");
    mixer.play(0);
    capture(mixer,100);
    check(mixer.playing(0) && !mixer.playing(1), "Per-effect active query failed");
    // Same check used by GameAudio when another distance threshold is crossed.
    if (!mixer.playing(0)) mixer.play(0);
    mixer.play(1);
    capture(mixer,3700);
    check(!mixer.playing(0) && mixer.playing(1) && mixer.statistics(0)->started == 1,
          "Existing step overlapped/restarted or affected another voice");
    capture(mixer,1401);
    check(!mixer.playing(1), "Long simultaneous effect did not finish");
}
void playback(const std::filesystem::path& directory)
{
    const auto path=directory/"BANK.DSS";
    writeBank(path);
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
    audio::Player player;
    std::string error;
    check(!player.open(directory/"missing.DSS",error) && !error.empty(),"Missing bank accepted");
    check(!player.play(0),"Unavailable device accepted playback");
    for (int i=0;i<3;++i) {
        check(player.open(path,error),"Dummy SDL audio device did not open");
        check(player.play(0),"SDL playback failed");
        const auto until=SDL_GetTicks()+1500;
        while (player.activeVoices() && SDL_GetTicks()<until) SDL_Delay(5);
        check(player.activeVoices()==0,"SDL callback did not consume effect");
        check(player.play(1,true),"SDL continuous request failed");
        player.setVolume(0.25f);
        player.stop(1);
        player.close(); player.close();
        check(player.effectCount() == 0 && player.activeVoices() == 0 &&
              !player.statistics(0) && !player.play(0), "Close retained bank or playable state");
    }
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"in-extremis-unavailable");
    check(!player.open(path,error) && !error.empty(),"Unavailable audio driver not handled");
    check(player.effectCount() == 0, "Device failure retained decoded resources");
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
    check(player.open(path,error), "Could not recover after device failure");
    check(!player.open(directory/"missing.DSS",error) && player.effectCount() == 0,
          "Failed replacement retained previous bank");
    check(player.open(path,error), "Could not reopen after missing replacement");
    check(player.play(1,true), "Could not start loop before destruction");
    // A second owner must retain its own SDL subsystem reference when the first closes.
    audio::Player other;
    check(other.open(path,error), "Second audio owner failed to open");
    player.close();
    check(other.play(0), "Closing first owner invalidated second audio device");
}
void boundedInterface(const audio::Bank& bank, std::size_t id)
{
    vortex::audio::Mixer native(audio::mixerConfig), bounded(audio::mixerConfig);
    native.setSamples(audio::mixerSamples(bank));
    bounded.setSamples(audio::mixerSamples(bank));
    check(bank[id].sample.loopFlag!=0,"Looping motor fixture lost native DSS loop");
    native.play(id); bounded.play(id,audio::playbackOptions(false,audio::Playback::Once));
    const auto count=bank[id].sample.pcm.size();
    capture(native,count*4); const auto output=capture(bounded,count*4);
    check(native.playing(id) && native.statistics(id)->wraps>0,"Native loop was globally disabled");
    const auto stats=*bounded.statistics(id);
    check(!bounded.playing(id) && stats.sourceFrames==count && stats.wraps==0 && stats.completed==1,
          "Bounded interface effect remains active after full payload");
    check(std::all_of(output.begin()+count,output.end(),[](float value){return value==0;}),
          "Interface motor has a trailing loop");
    for(int i=0;i<20;++i) bounded.play(id,audio::playbackOptions(false,audio::Playback::Once));
    check(bounded.active()==1,"Repeated interface requests stacked voices");
    capture(bounded,count+32);
    check(bounded.active()==0,"Repeated open/close left motor running");
}
void originalDoorTail(const audio::Bank& bank)
{
    vortex::audio::Mixer before(audio::mixerConfig), after(audio::mixerConfig);
    before.setSamples(audio::mixerSamples(bank));
    after.setSamples(audio::mixerSamples(bank));
    before.play(27,audio::playbackOptions(true,audio::Playback::Native)); after.play(27);
    const auto repeated = capture(before,5000), corrected = capture(after,5000);
    before.stop(27);
    capture(before,32);
    capture(after,32);
    const auto old = *before.statistics(27), fixed = *after.statistics(27);
    check(old.requests == 1 && old.started == 1 && old.sourceFrames == 5032 && old.wraps == 1,
          "Old forced-repeat defect could not be reproduced independently of SDL");
    check(fixed.requests == 1 && fixed.started == 1 && fixed.sourceFrames == 3700 &&
          fixed.wraps == 0 && fixed.completed == 1 && fixed.stopped == 0,
          "Corrected original effect did not consume exactly 3700 frames");
    check(std::all_of(corrected.begin()+3700,corrected.end(),[](float x){return x==0;}) &&
          std::any_of(repeated.begin()+3700,repeated.end(),[](float x){return x!=0;}),
          "Repeat was not isolated to the forced second pass");
    const std::filesystem::path output="build/audio-debug";
    std::filesystem::create_directories(output);
    std::ofstream report(output/"door-tail-regression.csv");
    report << "model,id,decoded_frames,requests,started,source_frames,wraps,completed,stopped\n";
    const auto row = [&](const char* model,const audio::PlaybackStats& stats) {
        report << model << ",27," << stats.decodedFrames << ',' << stats.requests << ','
               << stats.started << ',' << stats.sourceFrames << ',' << stats.wraps << ','
               << stats.completed << ',' << stats.stopped << '\n';
    };
    row("previous_forced_repeat",old); row("corrected_one_shot",fixed);
    check(bool(report), "Cannot write repeat diagnostic report");
    std::cout << "Door tail: one request; previous consumed=5032 wraps=1; corrected consumed=3700 wraps=0.\n";
}

void masterVolumeOutput(const std::filesystem::path& fixtures,
                        const std::filesystem::path& directory)
{
    bool diskAvailable = false;
    for (int i=0; i<SDL_GetNumAudioDrivers(); ++i)
        diskAvailable |= std::string_view(SDL_GetAudioDriver(i)) == "disk";
    if (!diskAvailable) {
        std::cout << "SDL disk audio driver unavailable; PCM output check skipped.\n";
        return;
    }
    SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER,"disk",SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER,"dummy",SDL_HINT_OVERRIDE);
    check(SDL_InitSubSystem(SDL_INIT_VIDEO),"Cannot initialize video for menu audio test");
    const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("Menu audio regression",640,400,0),SDL_DestroyWindow);
    check(bool(window),"Cannot create audio regression window");
    const std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
        SDL_CreateRenderer(window.get(),"software"),SDL_DestroyRenderer);
    check(bool(renderer),"Cannot create audio regression renderer");

    const auto peak = [&](const std::filesystem::path& raw, const SDL_AudioSpec& spec) {
        std::ifstream input(raw,std::ios::binary);
        const std::vector<Uint8> data{std::istreambuf_iterator<char>(input),{}};
        const SDL_AudioSpec mono{SDL_AUDIO_F32,1,spec.freq};
        Uint8* pcm = nullptr;
        int size = 0;
        check(!data.empty() && SDL_ConvertAudioSamples(&spec,data.data(),int(data.size()),&mono,&pcm,&size),
              "Could not read PCM from the SDL disk device");
        const std::unique_ptr<Uint8, decltype(&SDL_free)> owned(pcm,SDL_free);
        float maximum = 0;
        const auto* values = reinterpret_cast<const float*>(pcm);
        for (int i=0; i<size/int(sizeof(float)); ++i)
            maximum = std::max(maximum,std::abs(values[i]));
        return maximum;
    };

    // Capture the physical device mix, including the separate Port Menu streams.
    for (const bool menu : {false,true}) {
        std::array<float,3> peaks{};
        for (std::size_t i=0; i<peaks.size(); ++i) {
            UserSettings settings;
            settings.masterVolume = static_cast<unsigned>(i)*50;
            const auto file = fixtures / ((menu ? "menu-" : "game-") + std::to_string(i) + ".raw");
            check(SDL_SetHint(SDL_HINT_AUDIO_DISK_OUTPUT_FILE,file.u8string().c_str()),"Cannot set PCM capture path");
            check(SDL_InitSubSystem(SDL_INIT_AUDIO),"Cannot initialize disk audio");
            const SDL_AudioSpec requested{SDL_AUDIO_F32,2,44100};
            const auto device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&requested);
            SDL_AudioSpec spec{};
            check(device && SDL_GetAudioDeviceFormat(device,&spec,nullptr),"Disk device format unavailable");
            if (menu) {
                SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
                for (const auto code : {SDL_SCANCODE_DOWN,SDL_SCANCODE_DOWN,SDL_SCANCODE_RETURN}) {
                    SDL_Event e{};
                    e.type = SDL_EVENT_KEY_DOWN;
                    e.key.windowID = SDL_GetWindowID(window.get());
                    e.key.scancode = code;
                    check(SDL_PushEvent(&e),"Cannot queue menu sound");
                }
                const auto timer = SDL_AddTimer(200,[](void*,SDL_TimerID,Uint32) -> Uint32 {
                    SDL_Event stop{};
                    stop.type = SDL_EVENT_QUIT;
                    SDL_PushEvent(&stop);
                    return 0;
                },nullptr);
                check(timer != 0,"Cannot schedule menu exit");
                check(showPortMenu(window.get(),renderer.get(),directory,false,settings,{}).choice ==
                      PortMenuChoice::Exit,"Menu audio playback did not exit cleanly");
            } else {
                GameAudio audio;
                if (i != 1) audio.setMasterGain(settings.audioGain());
                audio.load(directory);
                check(audio.enabled(),"Game audio did not start");
                if (i == 1) {
                    audio.toggleEffects();
                    audio.setMasterGain(settings.audioGain());
                    check(audio.outputGain() == 0.f,"Classic mute lost priority over Master Volume");
                    audio.toggleEffects();
                    check(audio.outputGain() == .5f,"Unmuting lost the Master Volume");
                }
                audio.interfaceKey();
                const auto until = SDL_GetTicks() + 1500;
                while (SDL_GetTicks() < until && !audio.effectStatistics(28)->completed)
                    SDL_Delay(5);
                check(audio.effectStatistics(28)->completed==1,"Master Volume changed effect completion");
                SDL_Delay(75);
            }
            SDL_CloseAudioDevice(device);
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            peaks[i] = peak(file,spec);
        }
        check(peaks[0] == 0 && peaks[2] > .001f && std::abs(peaks[1]/peaks[2]-.5f) < .03f,
              menu ? "Menu output is not proportional to Master Volume" : "Game output is not proportional to Master Volume");
        std::cout << (menu ? "Menu" : "Game") << " PCM peaks at 0/50/100: "
                  << peaks[0] << ", " << peaks[1] << ", " << peaks[2] << '\n';
    }
    SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER,"dummy",SDL_HINT_OVERRIDE);
}

void originals(const std::filesystem::path& directory)
{
    const auto bank = audio::loadBank(directory/"BANK.DSS");
    check(bank.size()==30 && bank[7].sample.name=="pas" &&
          bank[27].sample.pcm.size()==3700 && bank[27].continuous==255,
          "Original bank IDs/metadata differ");
    int standalone=0;
    for (const auto& entry:std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension()==".DSS" && entry.path().filename()!="BANK.DSS") {
            const auto sample=audio::loadSample(entry.path());
            check(sample.rate==5000 && !sample.pcm.empty(),"Original standalone DSS failed");
            ++standalone;
        }
    }
    check(standalone==14,"Original standalone count changed");
    originalDoorTail(bank); boundedInterface(bank,11);
    vortex::audio::Mixer shared(audio::mixerConfig);
    shared.setSamples(audio::mixerSamples(bank));
    shared.play(26);shared.play(11,audio::playbackOptions(false,audio::Playback::Once));
    capture(shared,12000);
    check(!shared.playing(11) && shared.playing(26) && shared.statistics(26)->stopped==0,
          "Motor completion silenced native death loop");
    std::cout << "Originals: 30 bank entries + 14 standalone samples decoded.\n";
}
} // namespace
int main(int argc,char* argv[])
{
    if (argc<2 || argc>3) return 1;
    try {
        decoding(); mixing();
        { auto fixture=audio::decodeBank(bankBytes()); fixture[0].sample.loopFlag=65535;
          fixture[0].sample.loopBegin=0; fixture[0].sample.loopEnd=127; boundedInterface(fixture,0); } exactCompletion(); simultaneousEffects(); loopLifecycle(); playback(argv[1]);
        formatConversion(); weaponSounds(argv[1]); doorEvents(argv[1]); footstepEvents(argv[1]); noOverlappingSteps();
        const auto volumeFixture=std::filesystem::path(argv[1])/"master-volume";
        writeBank(volumeFixture/"BANK.DSS",30);
        masterVolumeOutput(volumeFixture,argc==3 ? std::filesystem::path(argv[2]) : volumeFixture);
        if (argc==3) originals(argv[2]);
        std::cout << "DSS decoding, PCM, loops, mixing and SDL dummy playback passed.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
