/* Copyright (c) 2023 Pirmin Pfeifer */
#include "RayVisDataformat.h"

#include <amdrdf.h>

#include <filesystem>
namespace fs = std::filesystem;


#undef CreateFile

bool dataformat::SaveTo(std::string savePath, std::span<const RayTrace> traces, const Scene* scene)
{
    fs::path filePath(savePath);
    if (fs::is_directory(filePath)) {
        return false;
    }
    if (filePath.has_extension()) {
        savePath = savePath.substr(0, savePath.size() - filePath.extension().string().size());
    }
    savePath += EXTENSION;
    filePath            = fs::path(savePath);
    fs::path parentPath = filePath.parent_path();
    fs::create_directories(parentPath);

    auto stream = rdf::Stream::CreateFile(filePath.string().c_str());
    auto writer = rdf::ChunkFileWriter(stream);

    spdlog::info("RAYVIS::SAVING - Started dumping scene to memory.");
    scene->SaveTo(writer);
    spdlog::info("RAYVIS::SAVING - Scene dumped.");

    spdlog::info("RAYVIS::SAVING - Started dumping {} rayTrace(s) to memory.", traces.size());
    bool success = true;
    for (const auto& trace : traces) {
        success &= trace.Save(writer);
    }
    spdlog::info("RAYVIS::SAVING - {} rayTrace(s) dumped.", traces.size());

    if (!success) {
        spdlog::error("RAYVIS::SAVING - failed! Saved file is now corrupted");
        return false;
    }

    spdlog::info("RAYVIS::SAVING - Started saving RAYVIS file to disc.");
    writer.Close();
    spdlog::info("RAYVIS::SAVING - Saved file to disc.");

    return true;
}

bool dataformat::CheckPathForVaildFile(std::string filename)
{
    auto path = std::filesystem::path(filename);
    assert(path.extension().string() == ".rayvis");
    if (!std::filesystem::exists(path)) {
        spdlog::error("RAYVIS_CHECK_PATH: \"{}\" dose not exist", filename);
        return false;
    }
    if (path.extension() != EXTENSION) {
        spdlog::error("RAYVIS_CHECK_PATH: \"{}\" is no ", filename);
        return false;
    }

    const auto chunkfile = rdf::ChunkFile(filename.c_str());
    {  // Check for scene chunk
        if (chunkfile.GetChunkCount(SCENE_CHUNK_ID) == 0) {
            spdlog::error("RAYVIS_CHECK_PATH: A chunk of type {} is required, but not present in \"{}\"",
                          SCENE_CHUNK_ID,
                          filename);
            return false;
        }
        if (1 < chunkfile.GetChunkCount(SCENE_CHUNK_ID)) {
            spdlog::error("RAYVIS_CHECK_PATH: To many chunks of type {} found, only 1 should be present in \"{}\"",
                          SCENE_CHUNK_ID,
                          filename);
            return false;
        }

        const auto chunkVersion = chunkfile.GetChunkVersion(SCENE_CHUNK_ID, 0);
        if (chunkVersion != SCENE_CHUNK_VERSION) {
            spdlog::error("RAYVIS_CHECK_PATH: Chunk of type {} is of Version {} but current required version is {}",
                          SCENE_CHUNK_ID,
                          chunkVersion,
                          SCENE_CHUNK_VERSION);
            return false;
        }

        const auto headerSize = chunkfile.GetChunkHeaderSize(SCENE_CHUNK_ID, 0);
        if (headerSize != sizeof(SceneChunkHeader)) {
            spdlog::error("RAYVIS_CHECK_PATH: Chunk {}: Header size is {} but was expected to be {}",
                          SCENE_CHUNK_ID,
                          headerSize,
                          sizeof(SceneChunkHeader));
            return false;
        }
    }

    {  // Check for trace chunk
        const auto chunkCount = chunkfile.GetChunkCount(RAY_TRACE_CHUNK_ID);
        if (chunkCount == 0) {
            spdlog::error("RAYVIS_CHECK_PATH: A chunk of type {} is required, but not present in \"{}\"",
                          RAY_TRACE_CHUNK_ID,
                          filename);
            return false;
        }

        for (size_t i = 0; i < chunkCount; i++) {
            const auto chunkVersion = chunkfile.GetChunkVersion(RAY_TRACE_CHUNK_ID, i);
            if (chunkVersion != RAY_TRACE_VERSION) {
                spdlog::error("RAYVIS_CHECK_PATH: Chunk {} ID {}: Version is {} but current required version is {}",
                              RAY_TRACE_CHUNK_ID,
                              i,
                              chunkVersion,
                              RAY_TRACE_VERSION);
                return false;
            }

            const auto headerSize = chunkfile.GetChunkHeaderSize(RAY_TRACE_CHUNK_ID, i);
            if (headerSize != sizeof(RayTraceHeader)) {
                spdlog::error("RAYVIS_CHECK_PATH: Chunk {} ID {}: Header size is {} but was expected to be {}",
                              RAY_TRACE_CHUNK_ID,
                              i,
                              headerSize,
                              sizeof(RayTraceHeader));
                return false;
            }
        }
    }

    return true;
}
