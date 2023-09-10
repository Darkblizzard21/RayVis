/* Copyright (c) 2023 Pirmin Pfeifer */

#pragma once
#include <spdlog/spdlog.h>
#include <windows.h>

#include <filesystem>
#include <iostream>
#include <string>

inline std::string GetExeDirectory()
{
    char buffer[MAX_PATH];
    bool sucess = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (sucess) {
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        return std::string(buffer).substr(0, pos);
    }
    spdlog::error("GetModuleFileNameA failed defaulting to path::absolute(.)");
    return std::filesystem::absolute(".").string();
}
