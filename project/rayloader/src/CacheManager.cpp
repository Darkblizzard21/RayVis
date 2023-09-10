/* Copyright (c) 2023 Pirmin Pfeifer */
#include "CacheManager.h"

#include <rayvis-utils/FileSystemUtils.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <nlohmann/json.hpp>
using path = std::filesystem::path;
using json = nlohmann::json;

namespace {
    const char* CHACHE_MANIFEST_FILE_NAME = "cache.manifest";
}

namespace rayloader {
    rayloader::CacheManager::CacheManager(std::unique_ptr<core::IConfiguration>&& configuration)
    {
        SetConfiguration(std::move(configuration));
        LoadCacheManifest();
    }

    CacheManager::~CacheManager()
    {
        SaveCacheManifest(lastCacheDir);
    }

    bool CacheManager::TryLoad(const char* filename, RayTrace& target)
    {
        ValidateCacheMainfest();

        path filePath(filename);

        const auto cacheEntryIt = cacheManifest_.find(filename);
        if (cacheEntryIt == cacheManifest_.end()) {
            spdlog::warn("CacheManger: no cache entry found");
            return false;
        }
        CacheEntry entry = cacheEntryIt->second;
        if (entry.hash != std::filesystem::hash_value(filePath) || entry.size != std::filesystem::file_size(filePath)) {
            spdlog::warn("CacheManger: cache invalid");
            return false;
        }

        bool result = RayTrace::LoadFrom(GetCachePath(filename).c_str(), target);
        if (result) {
            spdlog::info(std::format("CacheManger: Successfully loaded {} from cache", filename));
        } else {
            std::filesystem::remove(GetCachePath(filename));
            spdlog::info(std::format(
                "CacheManger: Cache was build with old version of CacheManager deleted old cache for {}", filename));
        }
        target.sourcePath = filename;
        return result;
    }

    bool CacheManager::AddCacheEntry(const char* filename, const RayTrace& target)
    {
        ValidateCacheMainfest();

        const auto savePath = GetCachePath(filename);
        std::filesystem::create_directories(path(savePath).parent_path());
        bool result = target.Save(savePath.c_str());
        if (!result) {
            spdlog::warn("CacheManger: could not save RayTrace");
            return false;
        }

        CacheEntry cacheEntry;
        cacheEntry.size          = std::filesystem::file_size(filename);
        cacheEntry.hash          = std::filesystem::hash_value(filename);
        cacheManifest_[filename] = cacheEntry;

        spdlog::info(std::format("CacheManger: Created cache for {}", filename));
        return true;
    }

    std::vector<std::string> CacheManager::GetAvailableConfigurationKeys() const
    {
        return {"directory"};
    }

    void CacheManager::TrySaveManifest()
    {
        SaveCacheManifest(config_->Get<std::string>("directory"));
    }

    void CacheManager::SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration)
    {
        std::string defaultCachePath = GetExeDirectory() + "\\cache";
        std::filesystem::create_directories(defaultCachePath);
        configuration->RegisterDirectory(
            "directory", defaultCachePath, "Cache Directory", "Directory used to store cache files.");
        config_ = std::move(configuration);
        ValidateCacheMainfest();
    }

    core::IConfiguration& CacheManager::GetConfigurationImpl()
    {
        return *config_;
    }

    void CacheManager::LoadCacheManifest()
    {
        auto cacheMainfestPath = path(config_->Get<std::string>("directory") + "/" + CHACHE_MANIFEST_FILE_NAME);

        if (std::filesystem::exists(cacheMainfestPath)) {
            std::ifstream inStream(cacheMainfestPath);
            json          j;
            inStream >> j;

            for (auto& entry : j["registry"]) {
                CacheEntry cacheEntry;
                cacheEntry.size = entry["size"];
                cacheEntry.hash = entry["hash"];
                cacheManifest_.emplace(entry["filename"], cacheEntry);
            }
        }
    }

    void CacheManager::SaveCacheManifest(const std::string& cacheDir)
    {
        if (cacheManifest_.size() == 0) {
            return;
        }

        auto cacheMainfestPath = path(cacheDir + "/" + CHACHE_MANIFEST_FILE_NAME);
        std::filesystem::create_directories(cacheMainfestPath.parent_path());
        std::ofstream outStream(cacheMainfestPath);
        json          j;
        for (auto& [filename, cacheEntry] : cacheManifest_) {
            json entry;
            entry["filename"] = filename;
            entry["size"]     = cacheEntry.size;
            entry["hash"]     = cacheEntry.hash;
            j["registry"].push_back(entry);
        }
        outStream << std::setw(4) << j << std::endl;
    }

    void CacheManager::ValidateCacheMainfest()
    {
        const auto& currentCacheDir = config_->Get<std::string>("directory");
        if (lastCacheDir != currentCacheDir) {
            SaveCacheManifest(lastCacheDir);
            lastCacheDir = currentCacheDir;
            LoadCacheManifest();
        }
    }

    std::string CacheManager::GetCachePath(const char* filename) const
    {
        path filePath(filename);
        return config_->Get<std::string>("directory") + "/" + filePath.stem().string() + RAY_TRACE_EXTENSION;
    }
}  // namespace rayloader
