/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include < d3d12.h>
#include <dxgiformat.h>
#include <minwindef.h>  // for UINT
#include <rayvis-utils/MathTypes.h>


#include <string>
#include <variant>
#include <vector>

#include "d3d12ex/Buffers.h"

class Scene;

class Mesh final {
    friend Scene;

public:
    typedef std::variant<std::span<UINT>, std::span<UINT16>>     IndexBufferVariant;
    typedef std::pair<std::span<Vertex>, IndexBufferVariant>     PrimitivePack;
    typedef std::variant<std::vector<UINT>, std::vector<UINT16>> IndexBufferVariantV;
    typedef std::pair<std::vector<Vertex>, IndexBufferVariantV>  PrimitivePackV;

    Mesh() = default;

    Mesh(ComPtr<ID3D12Device5> device, std::span<Vertex> vertexBuffer, IndexBufferVariant indexBuffer);
    Mesh(ComPtr<ID3D12Device5> device, std::span<PrimitivePack> primitives);
    Mesh(ComPtr<ID3D12Device5> device, std::span<PrimitivePackV> primitives);

    ~Mesh() = default;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS GetBlasInput(
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags =
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
    D3D12_RAYTRACING_GEOMETRY_DESC GetDes(size_t primitiveId, D3D12_GPU_VIRTUAL_ADDRESS transform);

    size_t PrimitiveCount() const;

    size_t VertexByteSize(size_t primitiveId = 0) const;
    size_t IndexByteSize(size_t primitiveId = 0) const;

    size_t VertexCount(size_t primitiveId = 0);
    size_t IndexCount(size_t primitiveId = 0);
    size_t TriangleCount(size_t primitiveId = 0);

    Vertex Min() const;
    Vertex Max() const;

    std::vector<ID3D12Resource*>                         GetPrimtivieNormalBuffers();
    std::vector<std::pair<DXGI_FORMAT, ID3D12Resource*>> GetPrimtivieIndexBuffers();

private:
    void Init(ComPtr<ID3D12Device5>                                                                     device,
              std::span<std::pair<std::span<Vertex>, std::variant<std::span<UINT>, std::span<UINT16>>>> primitives);

    struct Primitive {
        D3D12_RAYTRACING_GEOMETRY_DESC decription_;

        std::shared_ptr<UploadBuffer> indexBuffer_  = nullptr;
        std::shared_ptr<UploadBuffer> vertexBuffer_ = nullptr;
        std::shared_ptr<UploadBuffer> normalBuffer_ = nullptr;

        DXGI_FORMAT vertexFormat_;
        DXGI_FORMAT indexFormat_;

        std::vector<Vertex> vertices_;
        std::vector<UINT>   indecies;

        void Init(ComPtr<ID3D12Device5> device);
    };

    std::vector<Primitive>                      primitives_;
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> descriptions_;

    Vertex minExtends_ = math::Max<float, 3>();
    Vertex maxExtends_ = math::Min<float, 3>();
};