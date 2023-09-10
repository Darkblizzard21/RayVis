/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <rayvis-utils/MathUtils.h>

#include <cstdint>

enum class ShadingMode : int32_t {
    DoNotRender                 = -1,
    SmoothShadingSW             = 0,
    SmoothShadingInstanceColors = 1,
    InstanceColors              = 2,
    Shadows                     = 3,
    Normals                     = 4,
    Barycentry                  = 5,
    _MODE_COUNT_                = 6
};

enum class VisualizationMode : uint32_t {
    None        = 0,
    RayMesh     = 1,
    ArrowPoints = 2,
    VolumeTrace = 3,

    _MODE_COUNT_ = 4
};

constexpr ShadingMode increment(const ShadingMode& mode)
{
    const uint32_t i           = to_integral(mode);
    const auto     incremented = (i + 1) % to_integral(ShadingMode::_MODE_COUNT_);
    return static_cast<ShadingMode>(incremented);
}

template <>
inline std::string display_name<ShadingMode>(ShadingMode e)
{
    switch (e) {
    case ShadingMode::DoNotRender:
        return "DoNotRender";
    case ShadingMode::SmoothShadingSW:
        return "SmoothShadingSW";
    case ShadingMode::SmoothShadingInstanceColors:
        return "SmoothShadingInstanceColors";
    case ShadingMode::InstanceColors:
        return "InstanceColors";
    case ShadingMode::Shadows:
        return "Shadows";
    case ShadingMode::Normals:
        return "Normals";
    case ShadingMode::Barycentry:
        return "Barycentry";
    default:
        spdlog::info("unknown ShadingMode or _ModeCount_");
        break;
    }
    return "N/A";
}

template <>
inline std::string display_name<VisualizationMode>(VisualizationMode e)
{
    switch (e) {
    case VisualizationMode::None:
        return "None";
    case VisualizationMode::RayMesh:
        return "RayMesh";
    case VisualizationMode::ArrowPoints:
        return "VectorField";
    case VisualizationMode::VolumeTrace:
        return "VolumeTrace";
    default:
        spdlog::info("unknown VisualizationMode or _ModeCount_");
        break;
    }
    return "N/A";
}