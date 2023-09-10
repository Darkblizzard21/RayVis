/* Copyright (c) 2023 Pirmin Pfeifer */ 
#pragma once
#include <DescriptorHeap.h>
#include <d3d12.h>
#include <wrl/client.h>  // for ComPtr

#include <span>

#include "d3d12ex/Buffers.h"
#include "d3d12ex/Scene.h"

using Microsoft::WRL::ComPtr;

class BvhBuilder {
public:
    BvhBuilder() = default;
    BvhBuilder(ComPtr<ID3D12Device5> device);
    void Init(ComPtr<ID3D12Device5> device);
    void SetGeometry(std::span<Scene> scenes);
    void SetGeometry(std::span<Scene*> scenes);
    void SetGeometry(Scene& mesh);
    void SetGeometry(Mesh& mesh);
    void SetGeometry(Mesh* mesh);

    D3D12_SHADER_RESOURCE_VIEW_DESC GetTlasViewDesc();
    void             CreateInstanceColorSRV(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    void             CreateInstanceGeometryMappingSRV(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    const Descriptor CreatePrimitiveNormalsDescriptorArray(ComPtr<ID3D12Device5> device, DescriptorHeap* descHeap);
    const Descriptor CreatePrimitiveIndeciesDescriptorArray(ComPtr<ID3D12Device5> device, DescriptorHeap* descHeap);

    void BuildBvh(ComPtr<ID3D12GraphicsCommandList6> c);

private:
    struct InstanceInfo {
        D3D12_RAYTRACING_INSTANCE_DESC desc;
        int32_t                        meshId;
        Float3                         color;
    };

    std::vector<InstanceInfo> GenerateInstanceDesc(Scene::Node* node,
                                                   size_t       sceneId,
                                                   size_t       blasOffset,
                                                   Matrix4x4    currentTranform = linalg::identity);

    bool              geometryInitalize_ = false;

    ComPtr<ID3D12Device5>               device_ = nullptr;
    std::vector<ComPtr<ID3D12Resource>> blas_;
    ComPtr<ID3D12Resource>              tlas_;
    ComPtr<ID3D12Resource>              scratchBuffer_;
    UploadBuffer                        blasInstances_;

    UploadBuffer                                         instanceColors_;
    UploadBuffer                                         InstanceGeometryMapping_;
    std::vector<ID3D12Resource*>                         primitiveNormalBuffer_;
    std::vector<std::pair<DXGI_FORMAT, ID3D12Resource*>> primitiveIndexBuffer_;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS              topLevelInputs_    = {};
    std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> bottomLevelInputs_ = {};
};