/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Loader.h"


#include <filesystem>
#include <iostream>

namespace rayloader {
    Loader::Loader() {}

    Loader::Loader(std::unique_ptr<core::IConfiguration>&& configuration)
    {
        SetConfiguration(std::move(configuration));
    }

    std::vector<RayTrace> Loader::Load(const char* filename)
    {
        if (!std::filesystem::exists(filename)) {
            throw std::runtime_error("file dose not exist");
        }

        std::filesystem::path filePath(filename);
        if (filePath.extension() == ".rayvis") {
            return LoadFromRAYVIS(filename);
        }

        throw std::runtime_error("File not supported");
    }

    std::vector<std::string> Loader::GetAvailableConfigurationKeys() const
    {
        return {"useCache"};
    }

    void Loader::SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration)
    {
        configuration->Register("useCache", true, "Use Cache", "Enable caching of rayhistory files");

        if (cache_) {
            cache_->SetConfiguration(configuration->CreateView("cache."));
        } else {
            cache_ = std::make_unique<CacheManager>(configuration->CreateView("cache."));
        }
        config_ = std::move(configuration);
    }

    core::IConfiguration& Loader::GetConfigurationImpl()
    {
        return *config_.get();
    }

    std::vector<RayTrace> Loader::LoadFromRAYVIS(const char* filename)
    {
        const auto begin = std::chrono::steady_clock::now();

        auto path = std::filesystem::path(filename);
        assert(path.extension().string() == ".rayvis");
        if (!std::filesystem::exists(path)) {
            spdlog::error("RayTrace Loading: file dose not exist");
            assert(false);
            return std::vector<RayTrace>();
        }

        std::vector<RayTrace> result   = {};
        bool                  succsess = true;

        auto       chunkfile  = rdf::ChunkFile(filename);
        const auto traceCount = chunkfile.GetChunkCount(RAY_TRACE_CHUNK_ID);

        spdlog::info("RayTrace Loading: started loading {} traces form \"{}\"", traceCount, filename);
        for (size_t i = 0; i < traceCount; i++) {
            RayTrace trace;
            succsess &= RayTrace::LoadFrom(chunkfile, trace, i);
            assert(succsess);
            result.push_back(std::move(trace));
        }
        std::sort(
            result.begin(), result.end(), [](const RayTrace& a, const RayTrace& b) { return a.traceId < b.traceId; });

        const auto end     = std::chrono::steady_clock::now();
        const auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f;
        spdlog::info("RayTrace Loading: Finished Loading {} traces in {}s", traceCount, seconds);

        return result;
    }
}  // namespace rayloader