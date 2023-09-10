/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include < d3d12.h>

#include <format>
#include <iostream>
#include <string>
#define ThrowIfFailed(expression)                                                                         \
    do {                                                                                                  \
        const HRESULT hr = expression;                                                                    \
        if (FAILED(hr)) {                                                                                 \
            const std::string msg = std::format("'{}' failed ({}:L{})", #expression, __FILE__, __LINE__); \
            std::cerr << msg << "\n" << std::flush;                                                       \
            DebugBreak();                                                                                 \
            throw std::runtime_error(msg);                                                                \
        }                                                                                                 \
    } while (false)

constexpr const char* WINDOW_TITEL = "RayVis 1.0";

class config {
public:
    config() = delete;

    static const int         FramesInFlight          = 2;
    static const DXGI_FORMAT BackbufferFormat        = DXGI_FORMAT_R8G8B8A8_UNORM;
    static const int         ConstantBufferSizeBytes = 8192;
    static const bool        EnableDebugLayer        = true;

    static bool EnableFileSave;
};