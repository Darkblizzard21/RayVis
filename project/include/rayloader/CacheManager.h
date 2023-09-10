/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <configuration/Configuration.h>
#include <rayloader/RayTrace.h>
#include <rayloader/VolumetricSampler.h>

#include <filesystem>
#include <map>

namespace rayloader {
    class CacheManager : public core::IConfigurationComponent {
    public:
        CacheManager(std::unique_ptr<core::IConfiguration>&& configuration);
        ~CacheManager();

        bool TryLoad(const char* filename, RayTrace& target);
        bool AddCacheEntry(const char* filename, const RayTrace& target);

        std::vector<std::string> GetAvailableConfigurationKeys() const override;

        void TrySaveManifest();
    private:
        void                  SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration) override;
        core::IConfiguration& GetConfigurationImpl() override;

        void LoadCacheManifest();
        void SaveCacheManifest(const std::string& cacheDir);
        void ValidateCacheMainfest();

        std::string GetCachePath(const char* filename) const;

        struct CacheEntry {
            size_t              hash;
            uintmax_t           size;
        };

        std::string                           lastCacheDir = "N/A";
        std::unique_ptr<core::IConfiguration> config_;
        std::map<std::string, CacheEntry>     cacheManifest_;
    };
}  // namespace rayloader