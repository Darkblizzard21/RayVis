project(rayvis-utils)
add_library(rayvis-utils STATIC)

TARGET_SOURCES(rayvis-utils
    PRIVATE
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/BreakAssert.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/ChunkedArray3D.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/Clock.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/Color.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/CpuRaytracing.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/DataStructures.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/FastVoxelTraverse.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/FileSystemUtils.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/Keys.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/MathTypes.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/MathUtils.h
    ${RAYVIS_SOURCE_DIR}/include/rayvis-utils/Mouse.h

    src/Clock.cpp
    src/Color.cpp
    src/Keys.cpp
    src/Mouse.cpp
)

target_include_directories(rayvis-utils
    PUBLIC ${RAYVIS_SOURCE_DIR}/include
    PRIVATE ${RAYVIS_SOURCE_DIR}/include/rayvis-utils)

TARGET_LINK_LIBRARIES(rayvis-utils PUBLIC spdlog linalg glm)