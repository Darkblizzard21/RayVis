/* Copyright (c) 2023 Pirmin Pfeifer */
#ifdef NDEBUG
#define Assert(expression) ((void)0)
#else
#include <spdlog/spdlog.h>
#define Assert(expression)                                                                          \
    do {                                                                                            \
        if (!!!(expression)) {                                                                      \
            std::string msg = fmt::format("'{}' failed ({}:L{})", #expression, __FILE__, __LINE__); \
            spdlog::error(msg);                 \
            DebugBreak();                                                                           \
            throw std::runtime_error(msg);                                                          \
        }                                                                                           \
    } while (false)
#endif