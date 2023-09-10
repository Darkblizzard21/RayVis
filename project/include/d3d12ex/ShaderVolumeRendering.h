/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <Buffers.h>
#include <TextureBuffer.h>
#include <d3d12ex/BvhBuilder.h>
#include <d3d12ex/IShader.h>
#include <d3d12ex/VolumeProvider.h>
#include <rayvis-utils/MathTypes.h>

struct VolumeShaderData {
    VolumeProvider* volumeProvider = nullptr;
    ID3D12Resource* rayDepthSVR    = nullptr;

    ID3D12Resource* renderTargetUAV = nullptr;

    bool IsValid();
};

struct VolumeShaderConstantBuffer {
    struct {
        Matrix4x4 toWorld;
        float     tMin;
        float     tMax;
        float     fov;
    } camera;

    float baseTransparency;

    int32_t viewportWidth;
    int32_t viewportHeight;
    int32_t samplesPerCell;
    int32_t b_excludeExceeding;

    struct {
        Float3 min;
        float  chunkCount;
        Float3 max;
        float  cellSize;
        float  chunkSize;
    } volume;

    float elapsed;
    float minValue;
    float maxValue;

    int32_t b_enableChunkDebugging;
    int32_t maxDebugCunkCount;
};

class VolumeShader final : public IShader<VolumeShaderConstantBuffer> {
public:
    VolumeShader(ID3D12Device5*     device,
                 ShaderCompiler*    compiler,
                 VolumeShaderData&& data,
                 std::string_view   shaderSourceLocation);
    ~VolumeShader() = default;

    void SetComputeRootDescriptorTable(ID3D12GraphicsCommandList6* c,
                                       DescriptorHeap*             descHeap,
                                       VolumeShaderConstantBuffer* data) override;
    void Dispatch(ID3D12GraphicsCommandList6* c) override;

    void AdvanceFrame() override;

    void OverrideData(VolumeShaderData&& data);

private:
    std::unique_ptr<ConstantBuffer> constantBuffer = nullptr;
    VolumeShaderData                resources;
};