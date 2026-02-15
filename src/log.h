#pragma once
#include <fstream>
#include <format>

inline std::ofstream g_log;
inline bool g_log_enabled = false;

#define LOG(fmt, ...)                                                    \
    do {                                                                 \
        if (g_log_enabled && g_log) {                                    \
            g_log << std::format(fmt, ##__VA_ARGS__) << std::endl;       \
            g_log.flush();                                               \
        }                                                                \
    } while (0)

inline void enable_logging(const char* path) {
    if (!g_log.is_open()) {
        g_log.open(path, std::ios::out | std::ios::trunc);
    }
    g_log_enabled = true;
}

inline void disable_logging() {
    g_log_enabled = false;
}
