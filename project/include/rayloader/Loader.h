/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <configuration/Configuration.h>
#include <rayloader/CacheManager.h>

namespace rayloader {
    class Loader : public core::IConfigurationComponent {
    public:
        Loader();
        Loader(std::unique_ptr<core::IConfiguration>&& configuration);

        std::vector<RayTrace>    Load(const char* filename);
        std::vector<std::string> GetAvailableConfigurationKeys() const override;

    private:
        void                  SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration) override;
        core::IConfiguration& GetConfigurationImpl() override;
        std::vector<RayTrace> LoadFromRAYVIS(const char* filename);

        std::unique_ptr<core::IConfiguration> config_;
        std::unique_ptr<CacheManager>         cache_;
    };
}  // namespace rayloader