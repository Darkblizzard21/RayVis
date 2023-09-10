/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once

#include <rayloader/RayTrace.h>
#include <rayvis-utils/CpuRaytracing.h>

class VolumetricSampler {
public:
    typedef uint16_t rdType;

    struct Footprint {
        float  cellSize;
        size_t chunkSize;
    };

    struct ChunkData {
        Int3                chunkIdx;
        rdType              maxRays;
        Float3              min;
        Float3              max;
        std::vector<rdType> rayDensity;
        std::vector<Float3> directions;
        size_t              chunkSize;

        const rdType& RayDenity(const size_t& x, const size_t& y, const size_t& z) const;
        const Float3& Directions(const size_t& x, const size_t& y, const size_t& z) const;

        size_t rayCount   = 0;
        size_t missedRays = 0;
    };

    VolumetricSampler() = default;

    VolumetricSampler(RayTrace*            trace,
                      const size_t         chunkSize = 128,
                      float                cellSize  = 100,
                      std::optional<float> maxT      = 50000)
        : trace(trace), chunkSize_(chunkSize), cellSize_(cellSize), maxT_(maxT){};

    void SetFilter(const RayFilter filter);
    void SetChunkSize(const size_t chunkSize);
    void SetCellSize(float cellSize);
    void SetMaxT(std::optional<float> maxT);

    void Sample();

    inline Footprint GetFootprint()
    {
        return {cellSize_, chunkSize_};
    }

    inline const std::vector<ChunkData>* Data()
    {
        return &data_;
    }

    inline bool IsDirty()
    {
        return dirty_;
    }

    inline RayFilter GetFilter()
    {
        return filter_;
    }

    inline size_t ChunkSize()
    {
        return chunkSize_;
    }

    inline float CellSize()
    {
        return cellSize_;
    }

    inline std::optional<float> MaxT()
    {
        return maxT_;
    }

    inline rdType MaxRays() {
        return maxRays_;
    }

    inline Float3 MinBounds()
    {
        return min_;
    }

    inline Float3 MaxBounds()
    {
        return max_;
    }

    inline size_t ChunkCount()
    {
        return data_.size();
    }

private:
    bool dirty_ = true;

    RayFilter            filter_        = RayFilter::IncludeAllRays;
    size_t               chunkSize_     = 0;
    float                cellSize_      = 0;
    std::optional<float> maxT_          = std::nullopt;
    const RayTrace*      trace;

    rdType maxRays_ = 0;
    Float3 min_;
    Float3 max_;

    std::vector<ChunkData> data_;
};
