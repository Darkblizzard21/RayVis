/* Copyright (c) 2023 Pirmin Pfeifer */
#include "VolumetricSampler.h"

#include <rayvis-utils/ChunkedArray3D.h>
#include <rayvis-utils/FastVoxelTraverse.h>
#include <rayvis-utils/MathUtils.h>

#include <chrono>
#include <execution>
#include <future>
#include <set>
using namespace std::chrono_literals;

const VolumetricSampler::rdType& VolumetricSampler::ChunkData::RayDenity(const size_t& x,
                                                                         const size_t& y,
                                                                         const size_t& z) const
{
    return rayDensity[x * chunkSize * chunkSize + y * chunkSize + z];
}

const Float3& VolumetricSampler::ChunkData::Directions(const size_t& x, const size_t& y, const size_t& z) const
{
    return directions[x * chunkSize * chunkSize + y * chunkSize + z];
}

void VolumetricSampler::SetFilter(const RayFilter filter)
{
    dirty_ = true;
    if (filter == RayFilter::None) {
        spdlog::warn("VolumetricSampler::SetFilter was set to RayFilter::None");
    }
    filter_ = filter;
}

void VolumetricSampler::SetChunkSize(const size_t chunkSize)
{
    dirty_     = true;
    chunkSize_ = chunkSize;
}

void VolumetricSampler::SetCellSize(float cellSize)
{
    dirty_    = true;
    cellSize_ = cellSize;
}

void VolumetricSampler::SetMaxT(std::optional<float> maxT)
{
    dirty_ = true;
    maxT_  = maxT;
}

void VolumetricSampler::Sample()
{
    std::chrono::steady_clock::time_point absoluteStartTime = std::chrono::steady_clock::now();
    data_.clear();
    const std::vector<Ray> filteredRays =
        filter_ == RayFilter::IncludeAllRays ? trace->rays : [&rays = trace->rays, &filter = filter_]() {
            std::vector<Ray> result;
            for (auto& ray : rays) {
                if (IncludeRay(ray, filter)) {
                    result.push_back(ray);
                }
            }
            spdlog::info("VS::Sampler Preprocessing - filtered rays (Filter: {} Rays: {}/{})",
                         display_name(filter),
                         result.size(),
                         rays.size());
            return result;
        }();

    // Step 0 find ray bounds
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    Float3                                min   = Float3(std::numeric_limits<float>::max());
    Float3                                max   = Float3(std::numeric_limits<float>::lowest());
    for (const auto& ray : filteredRays) {
        const float tMax       = std::min(ray.tHitOrTMax(), maxT_.value_or(ray.tMax));
        const auto  startPoint = ray.origin + ray.direction * ray.tMin;
        const auto  endPoint   = ray.origin + ray.direction * tMax;
        min                    = linalg::min(min, linalg::min(startPoint, endPoint));
        max                    = linalg::max(max, linalg::max(startPoint, endPoint));
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    const auto step0_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f;
    spdlog::info("VS::SamplerStep 0 - calculate ray bounds - finished in {}s", step0_seconds);

    // Step 1 find chunks that are intersected
    begin = std::chrono::steady_clock::now();

    const float                  voxelSize            = chunkSize_ * cellSize_;
    const int32_t                higherLevelChunksize = 128;
    ChunkedArray3D<std::uint8_t> chunks(higherLevelChunksize);
    for (const auto& ray : filteredRays) {
        const float tMax  = std::min(ray.tHitOrTMax(), maxT_.value_or(ray.tMax));
        Double3     start = ray.origin + ray.direction * ray.tMin * 1.0;
        Double3     end   = ray.origin + ray.direction * tMax * 1.0;
        start             = start - min;
        end               = end - min;
        start             = start / voxelSize;
        end               = end / voxelSize;
        VoxelTrace(start, end, [&chunks = chunks](const Int3& voxel) {
            assert((0 <= voxel.x) && (0 <= voxel.y) && (0 <= voxel.z));
            chunks.At(voxel) = 1;
        });
    }
    end                      = std::chrono::steady_clock::now();
    const auto step1_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f;
    spdlog::info("VS::SamplerStep 1 - calculate traversed chunks - finished in {}s", step1_seconds);

    // Step 2 create chunk tasks
    begin = std::chrono::steady_clock::now();

    const size_t dataSize = chunkSize_ * chunkSize_ * chunkSize_;

    for (const auto& data : chunks.GetData()) {
        const Int3& base = data.first * higherLevelChunksize;
        for (size_t i = 0; i < data.second.size(); i++) {
            if (0 < data.second[i]) {
                const Int3 index(i / higherLevelChunksize / higherLevelChunksize,
                                 (i / higherLevelChunksize) % higherLevelChunksize,
                                 i % higherLevelChunksize);

                ChunkData taskData = {};
                taskData.chunkSize = chunkSize_;
                taskData.chunkIdx  = (base + index);
                taskData.min       = Float3(taskData.chunkIdx) * voxelSize + min;
                taskData.max       = taskData.min + Float3(voxelSize);
                taskData.rayDensity.resize(chunkSize_ * chunkSize_ * chunkSize_);
                taskData.directions.resize(chunkSize_ * chunkSize_ * chunkSize_);
                data_.push_back(taskData);
            }
            if (512 <= data_.size()) {
                break;
            }
        }
        if (512 <= data_.size()) {
            spdlog::error("maximum chunks reached (512=. There will be missing chunks");
            break;
        }
    }

    end                      = std::chrono::steady_clock::now();
    const auto step2_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f;
    spdlog::info("VS::SamplerStep 2 - created {} chunk tasks - finished in {}s", data_.size(), step2_seconds);

    // Step 3 execute chunk filing tasks
    begin = std::chrono::steady_clock::now();

    std::atomic_char16_t finishedTasks = 0;
    std::future<void>    computeChunks = std::async(std::launch::async, [&]() {
        std::for_each(
            std::execution::par,
            data_.begin(),
            data_.end(),
            [&chunkSize_    = chunkSize_,
             &rays          = filteredRays,
             &maxT_         = maxT_,
             &cellSize_     = cellSize_,
             &finishedTasks = finishedTasks](ChunkData& task) {
                std::vector<Double3> doubleDirs;
                doubleDirs.resize(chunkSize_ * chunkSize_ * chunkSize_);
                task.rayCount = rays.size();
                for (const auto& ray : rays) {
                    const Float2 minMax = IntersectAABB(ray.origin, ray.direction, task.min, task.max);
                    const float  maxT   = std::min(ray.tHitOrTMax(), maxT_.value_or(ray.tMax));
                    if (!HitAABB(minMax, maxT)) {
                        task.missedRays++;
                        continue;
                    }
                    Double3 start = ray.origin + ray.direction * static_cast<double>(std::max(minMax.x, ray.tMin));
                    Double3 end   = ray.origin + ray.direction * static_cast<double>(std::min(minMax.y, maxT));
                    start         = start - task.min;
                    end           = end - task.min;
                    start         = start / cellSize_;
                    end           = end / cellSize_;

                    const auto assertRoundingError = 0.1;
                    const auto upperAssertLimit    = chunkSize_ + assertRoundingError;
                    assert(-assertRoundingError < start.x && start.x < upperAssertLimit);
                    assert(-assertRoundingError <= start.y && start.y < upperAssertLimit);
                    assert(-assertRoundingError <= start.z && start.z < upperAssertLimit);
                    assert(-assertRoundingError <= end.x && end.x < upperAssertLimit);
                    assert(-assertRoundingError <= end.y && end.y < upperAssertLimit);
                    assert(-assertRoundingError <= end.z && end.z < upperAssertLimit);

                    start = linalg::clamp(start, Double3(0.0), Double3(chunkSize_ - 0.001));
                    end   = linalg::clamp(end, Double3(0.0), Double3(chunkSize_ - 0.001));

                    VoxelTrace(start,
                               end,
                               [&task = task, size = chunkSize_, &doubleDirs = doubleDirs, &dir = ray.direction](
                                   const Int3& voxel) {
                                   assert((0 <= voxel.x) && (0 <= voxel.y) && (0 <= voxel.z));
                                   assert((voxel.x < size) && (voxel.y < size) && (voxel.z < size));
                                   const size_t idx = voxel.x * size * size + voxel.y * size + voxel.z;
                                   if (task.rayDensity[idx] + 1 != std::numeric_limits<rdType>::max()) {
                                       task.rayDensity[idx]++;
                                   }
                                   doubleDirs[idx] += dir;
                               });
                }

                // Normalize Directions
                std::transform(doubleDirs.begin(), doubleDirs.end(), task.directions.begin(), [](Double3& dir) {
                    return Float3(linalg::normalize(dir));
                });

                // Grab max rays
                task.maxRays = std::accumulate(
                    task.rayDensity.begin(), task.rayDensity.end(), 0, [](const rdType& a, const rdType& b) {
                        return std::max(a,b);
                    });

                finishedTasks++;
            });
    });

    size_t lastFinishCount = 0;
    while (computeChunks.wait_for(50ms) != std::future_status::ready) {
        size_t ft = finishedTasks;
        if (lastFinishCount != ft) {
            lastFinishCount = ft;
            spdlog::info(
                "VS::SamplerStep 3 - Computation Update: finished {}/{} chunk tasks", lastFinishCount, data_.size());
        }
    }

    end                      = std::chrono::steady_clock::now();
    const auto step3_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f;
    spdlog::info("VS::SamplerStep 3 - execute {} chunk tasks - finished in {}s", finishedTasks, step3_seconds);

    // calculate min max 
    maxRays_ = 0;
    min_ = Float3(std::numeric_limits<float>::max());
    max_ = Float3(std::numeric_limits<float>::lowest());
    for (const auto& chunk : data_) {
        maxRays_ = std::max(maxRays_, chunk.maxRays);
        min_ = linalg::min(min_, linalg::min(chunk.min, chunk.max));
        max_ = linalg::max(max_, linalg::max(chunk.min, chunk.max));
    }


    // finished

    end = std::chrono::steady_clock::now();
    const auto completeTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - absoluteStartTime).count() / 1000.f;
    spdlog::info("VS::Sampler - finished computation in {}s", completeTime);
    dirty_ = false;
}