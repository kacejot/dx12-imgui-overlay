#pragma once
#include <fstream>
#include <format>

inline std::ofstream g_log("s2_overlay_log.txt", std::ios::out | std::ios::trunc);

#if 1 

#define LOG(fmt, ...)                                                    \
    do {                                                                 \
        if (g_log) {                                                     \
            auto _msg = std::format(fmt, ##__VA_ARGS__);                 \
            g_log << _msg << std::endl;                                  \
            g_log.flush();                                               \
        }                                                                \
    } while (0)

#else

#define LOG(fmt, ...)

#endif