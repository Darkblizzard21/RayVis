/* Copyright (c) 2023 Pirmin Pfeifer */
#include "RayTrace.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

float Ray::missTolerance = 0.1f;

Float2 RayTrace::MinMaxTHit()
{
    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::min();
    for (const auto& ray : rays) {
        min = std::min(ray.tHit, min);
        max = std::max(ray.tHit, max);
    }
    return Float2(min, max);
}

bool RayTrace::Save(const char* filename, bool overrideFile) const
{
    auto path = std::filesystem::path(filename);
    assert(path.extension() == RAY_TRACE_EXTENSION);
    if (std::filesystem::exists(path)) {
        if (overrideFile) {
            std::remove(filename);
        } else {
            spdlog::warn("RayTrace Saving: file dose exist");
            return false;
        }
    }

    auto stream = rdf::Stream::CreateFile(filename);
    auto writer = rdf::ChunkFileWriter(stream);

    const auto res = Save(writer);
    writer.Close();
    return res;
}

bool RayTrace::Save(rdf::ChunkFileWriter& writer) const
{
    // Header
    auto header    = RayTraceHeader();
    header.traceId = traceId;

    writer.BeginChunk(RAY_TRACE_CHUNK_ID, sizeof(RayTraceHeader), &header, rdfCompressionZstd, RAY_TRACE_VERSION);

    for (size_t i = 0; i < rays.size(); i++) {
        writer.AppendToChunk(rays[i]);
    }

    writer.EndChunk();
    return true;
}

bool RayTrace::LoadFrom(const char* filename, RayTrace& target, const size_t chunkIdx)
{
    auto path = std::filesystem::path(filename);
    assert(path.extension() == RAY_TRACE_EXTENSION);
    if (!std::filesystem::exists(path)) {
        spdlog::warn("RayTrace Loading: file dose not exist");
        return false;
    }

    auto chunkfile = rdf::ChunkFile(filename);
    return LoadFrom(chunkfile, target, chunkIdx);
}

bool RayTrace::LoadFrom(rdf::ChunkFile& chunkfile, RayTrace& target, const size_t chunkIdx)
{
    if (chunkfile.GetChunkCount(RAY_TRACE_CHUNK_ID) <= chunkIdx) {
        spdlog::warn(fmt::format("RayTrace Loading: missing chunk with RAY_TRACE_CHUNK_ID at index {}", chunkIdx));
        return false;
    }
    const auto headerSize   = chunkfile.GetChunkHeaderSize(RAY_TRACE_CHUNK_ID, chunkIdx);
    const auto chunkVersion = chunkfile.GetChunkVersion(RAY_TRACE_CHUNK_ID, chunkIdx);
    if (headerSize != sizeof(RayTraceHeader)) {
        spdlog::warn("RayTrace Loading: Wrong header size");
        return false;
    }
    if (chunkVersion != RAY_TRACE_VERSION) {
        spdlog::warn("RayTrace Loading: Wrong RAY_TRACE_VERSION");
        return false;
    }

    RayTraceHeader header;
    chunkfile.ReadChunkHeaderToBuffer(RAY_TRACE_CHUNK_ID, chunkIdx, &header);
    target.traceId = header.traceId;

    bool loadedSucessfull = false;
    chunkfile.ReadChunkData(
        RAY_TRACE_CHUNK_ID, chunkIdx, [&target, &loadedSucessfull](int64_t dataSize, const void* data) {
            if (dataSize % sizeof(Ray) != 0) {
                spdlog::warn("RayTrace Loading: Chunk data dose not contain rays in the correct format");
                return;
            }
            const size_t        rayCount = dataSize / sizeof(Ray);
            const std::uint8_t* ptr      = reinterpret_cast<const std::uint8_t*>(data);
            for (size_t i = 0; i < rayCount; i++) {
                Ray                 ray;
                const std::uint8_t* ptrOffset = ptr + i * sizeof(Ray);
                std::memcpy(&ray, ptrOffset, sizeof(Ray));
                target.rays.push_back(ray);
            }

            loadedSucessfull = true;
        });
    return loadedSucessfull;
}

void RayTrace::DumpStartEndPointsToCSV(std::string path) const
{
    std::ofstream wf(path);
    wf << "sx,sy,sz,ex,ey,ez\n";
    for (const auto& ray : rays) {
        const auto rayStart = ray.origin + ray.direction * ray.tMin;
        const auto rayEnd   = ray.origin + ray.direction * ray.tHitOrTMax();

        auto f = fmt::format("{},{},{},{},{},{}\n", rayStart.x, rayStart.y, rayStart.z, rayEnd.x, rayEnd.y, rayEnd.z);
        wf << f;
    }
    spdlog::info("Finished csv dump for {} rays", rays.size());
}

void RayTrace::ScaleBy(const float& scale)
{
    for (auto& ray : rays) {
        ray.origin = ray.origin * scale;
        ray.tMin   = ray.tMin * scale;
        ray.tMax   = ray.tMax * scale;
        if (0 <= ray.tHit) {
            ray.tHit = ray.tHit * scale;
        }
    }
}
