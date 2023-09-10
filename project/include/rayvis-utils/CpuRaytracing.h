/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include "rayvis-utils/MathTypes.h"

inline bool HitAABB(const Float2& minMaxT, const float& tMax)
{
    const bool missed     = minMaxT.y < minMaxT.x;
    const bool outOfReach = tMax <= minMaxT.x;
    const bool behind     = minMaxT.y < 0;
    return !missed && !outOfReach && !behind;
}

inline Float2 IntersectAABB(const Float3& rayOrigin, const Float3& rayDir, const Float3& boxMin, const Float3& boxMax)
{
    assert(linalg::length2(rayDir) != 0.0);

    const auto dirFraction = Float3(1.f) / rayDir;
    const auto tLower      = (boxMin - rayOrigin) * dirFraction;
    const auto tUpper      = (boxMax - rayOrigin) * dirFraction;
    const auto t1          = linalg::min(tLower, tUpper);
    const auto t2          = linalg::max(tLower, tUpper);
    const auto tNear       = linalg::maxelem(t1);
    const auto tFar        = linalg::minelem(t2);
    return Float2(tNear, tFar);
}