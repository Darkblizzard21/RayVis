/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <rayloader/RayTrace.h>

#include "d3d12ex/Scene.h"

namespace SimpleRayMeshGenerator {
    struct LineDescription {
        RayTrace* raytraces = nullptr;
        float     thickness = 1.0;
        int       rayStride = 4;
        RayFilter filter    = RayFilter::IncludeAllRays;

        Float3 color = Float3(1, 0, 0);

        #undef max
        float maxT = std::numeric_limits<float>::max();
    };

    Scene GenerateLines(ComPtr<ID3D12Device5> device, RayTrace* raytraces);
    Scene GenerateLines(ComPtr<ID3D12Device5> device, const LineDescription& desc);
}  // namespace SimpleRayMeshGenerator