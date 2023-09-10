project(rayloader)
add_library(rayloader STATIC)

TARGET_SOURCES(rayloader
    PRIVATE
    ${RAYVIS_SOURCE_DIR}/include/rayloader/CacheManager.h
    ${RAYVIS_SOURCE_DIR}/include/rayloader/Loader.h
    ${RAYVIS_SOURCE_DIR}/include/rayloader/RayTrace.h
    ${RAYVIS_SOURCE_DIR}/include/rayloader/VolumetricSampler.h

    src/CacheManager.cpp
    src/Loader.cpp
    src/RayTrace.cpp
    src/VolumetricSampler.cpp
)

target_include_directories(rayloader
    PUBLIC ${RAYVIS_SOURCE_DIR}/include
    PRIVATE ${RAYVIS_SOURCE_DIR}/include/rayloader)

TARGET_LINK_LIBRARIES(rayloader PUBLIC spdlog linalg amdrdf configuration)