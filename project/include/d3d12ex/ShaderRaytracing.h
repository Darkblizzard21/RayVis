/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <Buffers.h>
#include <TextureBuffer.h>
#include <d3d12ex/BvhBuilder.h>
#include <d3d12ex/IShader.h>
#include <rayvis-utils/MathTypes.h>

struct RaytracingShaderData {
    BvhBuilder*     bvhBuilder      = nullptr;  // Bvh builder

    ID3D12Resource* renderTargetUAV = nullptr;
    ID3D12Resource* rayDepthUAV = nullptr;

    bool IsValid();
};

struct RaytracingShaderConstantBuffer {
    struct {
        Matrix4x4 toWorld;
        float     tMin;
        float     tMax;
        float     fov;
    } camera;

    float   elapsed;
    int32_t viewportWidth;
    int32_t viewportHeight;
    int32_t shaderMode;
    int32_t visualizationMode;

    Float3 lightDir;
};

class RaytracingShader final : public IShader<RaytracingShaderConstantBuffer> {
public:
    RaytracingShader(ID3D12Device5* device, ShaderCompiler* compiler, RaytracingShaderData&& data, std::string_view shaderSourceLocation);

    ~RaytracingShader() = default;

    void SetComputeRootDescriptorTable(ID3D12GraphicsCommandList6*     c,
                                       DescriptorHeap*                 descHeap,
                                       RaytracingShaderConstantBuffer* data) override;
    void Dispatch(ID3D12GraphicsCommandList6* c) override;

    void AdvanceFrame() override;
    void OverrideData(RaytracingShaderData&& data);

private:
    std::unique_ptr<ConstantBuffer> constantBuffer = nullptr;
    RaytracingShaderData            resources;
};