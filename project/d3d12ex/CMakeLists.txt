PROJECT(d3d12ex)
add_executable(d3d12ex)

TARGET_SOURCES(d3d12ex
    PRIVATE
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/Buffers.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/BvhBuilder.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/Camera.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/Config.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/DescriptorHeap.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/IShader.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/Mesh.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/RayVisDataformat.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/Renderer.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/RenderingModes.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/Scene.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/ShaderCompiler.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/ShaderRaytracing.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/ShaderVolumeRendering.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/SimpleRayMeshGenerator.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/TextureBuffer.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/UIHandler.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/VolumeProvider.h
    ${RAYVIS_SOURCE_DIR}/include/d3d12ex/Window.h
    
    src/Buffers.cpp
    src/BvhBuilder.cpp
    src/Camera.cpp
    src/Config.cpp
    src/DescriptorHeap.cpp
    src/main.cpp
    src/Mesh.cpp
    src/RayVisDataformat.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/ShaderCompiler.cpp
    src/ShaderRaytracing.cpp
    src/ShaderVolumeRendering.cpp
    src/SimpleRayMeshGenerator.cpp
    src/TextureBuffer.cpp
    src/UIHandler.cpp
    src/VolumeProvider.cpp
    src/Window.cpp)

target_include_directories(d3d12ex
    PUBLIC ${RAYVIS_SOURCE_DIR}/include
    PRIVATE ${RAYVIS_SOURCE_DIR}/include/d3d12ex)




TARGET_LINK_LIBRARIES(d3d12ex 
    PUBLIC
    amdrdf
    d3d12
    dxgi
    dxcompiler
    imgui
    spdlog
    cli11
    configuration
    rayvis-utils
    rayloader
    json)

set_target_properties(d3d12ex PROPERTIES OUTPUT_NAME "RayVis")

# copy necessary dlls and shaders
add_custom_command(TARGET d3d12ex PRE_BUILD 
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:d3d12ex>/shaders)
add_custom_command(TARGET d3d12ex PRE_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders"
        $<TARGET_FILE_DIR:d3d12ex>/shaders)
add_custom_command(TARGET d3d12ex PRE_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/libs"
        $<TARGET_FILE_DIR:d3d12ex>)


install(TARGETS d3d12ex DESTINATION bin)