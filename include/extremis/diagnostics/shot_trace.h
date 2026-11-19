#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace extremis::shot_trace {
inline const char* const mode = std::getenv("EXTREMIS_SHOT_TRACE");
inline const bool pixels = mode && mode[0]=='p';
inline thread_local unsigned long long frame = 0;

template<typename... Args>
void event(const char* kind, const char* format, Args... args)
{
    if (!mode) return;
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    std::fprintf(stderr, "SHOT %lld %llu %s ", static_cast<long long>(ns), frame, kind);
    std::fprintf(stderr, format, args...);
    std::fputc('\n', stderr);
}
}
