/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <Scene.h>
#include <rayloader/RayTrace.h>

#include <span>
#include <string>

namespace dataformat {
    constexpr const char* EXTENSION = ".rayvis";

    bool SaveTo(std::string path, std::span<const RayTrace>, const Scene*);

    bool CheckPathForVaildFile(std::string path);
}  // namespace dataformat