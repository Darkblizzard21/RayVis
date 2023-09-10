/* Copyright (c) 2023 Pirmin Pfeifer */
#include "BvhBuilder.h"

#include <rayvis-utils/BreakAssert.h>

#include <numeric>

#include "Config.h"

namespace {
    D3D12_HEAP_PROPERTIES DefaultHeapProperties()
    {
        D3D12_HEAP_PROPERTIES defaultHeapProperties = {};
        defaultHeapProperties.Type                  = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        defaultHeapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
        defaultHeapProperties.CreationNodeMask      = 0;
        defaultHeapProperties.VisibleNodeMask       = 0;
        return defaultHeapProperties;
    }

    // Width needs to be set
    D3D12_RESOURCE_DESC DefaultBufferDescription()
    {
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment           = 0;
        bufferDesc.Width               = 0;
        bufferDesc.Height              = 1;
        bufferDesc.DepthOrArraySize    = 1;
        bufferDesc.MipLevels           = 1;
        bufferDesc.Format              = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count    = 1;
        bufferDesc.SampleDesc.Quality  = 0;
        bufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        return bufferDesc;
    }

    void CreateFloat3SRV(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle, ID3D12Resource* buffer)
    {
        const auto width       = buffer->GetDesc().Width;
        const auto elementSize = sizeof(float) * 3;
        assert((width % elementSize) == 0);

        D3D12_SHADER_RESOURCE_VIEW_DESC bufferViewDes = {};
        bufferViewDes.Format                          = DXGI_FORMAT_R32G32B32_FLOAT;
        bufferViewDes.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        bufferViewDes.ViewDimension                   = D3D12_SRV_DIMENSION_BUFFER;
        bufferViewDes.Buffer.FirstElement             = 0;
        bufferViewDes.Buffer.NumElements              = width / elementSize;
        bufferViewDes.Buffer.StructureByteStride      = 0;
        bufferViewDes.Buffer.Flags                    = D3D12_BUFFER_SRV_FLAG_NONE;

        device->CreateShaderResourceView(buffer, &bufferViewDes, handle);
    }

    void CreateIntSRV(ComPtr<ID3D12Device5>       device,
                      D3D12_CPU_DESCRIPTOR_HANDLE handle,
                      ID3D12Resource*             buffer,
                      DXGI_FORMAT                 format = DXGI_FORMAT_R32_UINT)
    {
        const auto width       = buffer->GetDesc().Width;
        uint32_t   elementSize = 0;
        switch (format) {
        case DXGI_FORMAT_R32_UINT:
            elementSize = sizeof(uint32_t);
            break;
        case DXGI_FORMAT_R16_UINT:
            elementSize = sizeof(uint16_t);
            break;
        default:
            throw "unsupported format";
        }
        assert((width % elementSize) == 0);

        D3D12_SHADER_RESOURCE_VIEW_DESC bufferViewDes = {};
        bufferViewDes.Format                          = format;
        bufferViewDes.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        bufferViewDes.ViewDimension                   = D3D12_SRV_DIMENSION_BUFFER;
        bufferViewDes.Buffer.FirstElement             = 0;
        bufferViewDes.Buffer.NumElements              = width / elementSize;
        bufferViewDes.Buffer.StructureByteStride      = 0;
        bufferViewDes.Buffer.Flags                    = D3D12_BUFFER_SRV_FLAG_NONE;

        device->CreateShaderResourceView(buffer, &bufferViewDes, handle);
    }
}  // namespace

BvhBuilder::BvhBuilder(ComPtr<ID3D12Device5> device)
{
    Init(device);
}

void BvhBuilder::Init(ComPtr<ID3D12Device5> device)
{
    device_ = device;
    blasInstances_.Init(device);

    instanceColors_.Init(device);
    InstanceGeometryMapping_.Init(device);
}

void BvhBuilder::SetGeometry(std::span<Scene> scenes)
{
    std::vector<Scene*> ptr;
    for (auto& scene : scenes) {
        ptr.push_back(&scene);
    }
    SetGeometry(ptr);
};

void BvhBuilder::SetGeometry(std::span<Scene*> scenes)
{
    Assert(device_ != nullptr);

    UINT64                requieredScratchMemmory = 0;
    D3D12_HEAP_PROPERTIES defaultHeapProperties   = DefaultHeapProperties();
    D3D12_RESOURCE_DESC   bufferDesc              = DefaultBufferDescription();

    {  // Prepare Tlas
        topLevelInputs_.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelInputs_.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        topLevelInputs_.Flags       = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        topLevelInputs_.NumDescs    = std::accumulate(
            scenes.begin(), scenes.end(), 0, [](UINT ac, const Scene* f) { return ac + f->InstanceCount(); });
        // topLevelInputs_.Instances is set later

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
        device_->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs_, &topLevelPrebuildInfo);
        Assert(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

        // Create Resource
        bufferDesc.Width = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
        ThrowIfFailed(device_->CreateCommittedResource(&defaultHeapProperties,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &bufferDesc,
                                                       D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                                                       nullptr,
                                                       IID_PPV_ARGS(&tlas_)));

        // Update Scratch Memory Size
        requieredScratchMemmory = std::max(requieredScratchMemmory, topLevelPrebuildInfo.ScratchDataSizeInBytes);
    }

    {  // Build Blas
        const auto meshCount = std::accumulate(
            scenes.begin(), scenes.end(), 0, [](UINT ac, const Scene* f) { return ac + f->meshes.size(); });
        blas_.clear();
        blas_.resize(meshCount);
        bottomLevelInputs_.clear();
        bottomLevelInputs_.resize(meshCount);
        size_t blasId = 0;
        for (const auto scene : scenes) {
            for (size_t i = 0; i < scene->meshes.size(); i++) {
                const auto currentBlasId          = blasId++;
                bottomLevelInputs_[currentBlasId] = scene->meshes[i]->GetBlasInput();

                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
                device_->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs_[currentBlasId],
                                                                        &bottomLevelPrebuildInfo);
                Assert(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

                // Create Resource

                bufferDesc.Width = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
                ThrowIfFailed(device_->CreateCommittedResource(&defaultHeapProperties,
                                                               D3D12_HEAP_FLAG_NONE,
                                                               &bufferDesc,
                                                               D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                                                               nullptr,
                                                               IID_PPV_ARGS(&blas_[currentBlasId])));

                // Update Scratch Memory Size
                requieredScratchMemmory =
                    std::max(requieredScratchMemmory, bottomLevelPrebuildInfo.ScratchDataSizeInBytes);
            }
        }
    }

    {  // Allocate Scratch

        bufferDesc.Width = requieredScratchMemmory;
        ThrowIfFailed(device_->CreateCommittedResource(&defaultHeapProperties,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &bufferDesc,
                                                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                       nullptr,
                                                       IID_PPV_ARGS(&scratchBuffer_)));
    }

    {  // Instance Descriptions & colors
        primitiveNormalBuffer_.clear();
        primitiveIndexBuffer_.clear();
        // Geometry Primitive Mapping & normal buffer collection
        uint32_t              tlasPrimitiveCount = 0;
        std::vector<uint32_t> geomtrieStartIndecies;
        for (const auto& scene : scenes) {
            for (const auto& mesh : scene->meshes) {
                const auto primNormals = mesh->GetPrimtivieNormalBuffers();
                primitiveNormalBuffer_.insert(primitiveNormalBuffer_.end(), primNormals.begin(), primNormals.end());

                const auto primIndecies = mesh->GetPrimtivieIndexBuffers();
                primitiveIndexBuffer_.insert(primitiveIndexBuffer_.end(), primIndecies.begin(), primIndecies.end());

                geomtrieStartIndecies.push_back(tlasPrimitiveCount);
                tlasPrimitiveCount += mesh->PrimitiveCount();
            }
        }

        // Color
        instanceColors_.Resize(topLevelInputs_.NumDescs * sizeof(InstanceInfo::color));
        Float3* mappedColors = reinterpret_cast<Float3*>(instanceColors_.Map());
        // InstanceGeometryMapping
        InstanceGeometryMapping_.Resize(topLevelInputs_.NumDescs * sizeof(int32_t));
        uint32_t* mappedInstanceGeometryMapping = reinterpret_cast<uint32_t*>(InstanceGeometryMapping_.Map());
        // Description
        blasInstances_.Resize(topLevelInputs_.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        D3D12_RAYTRACING_INSTANCE_DESC* mappedDescirptions =
            reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(blasInstances_.Map());

        size_t blasOffset = 0;
        size_t sceneId    = 0;
        for (const auto& scene : scenes) {
            for (const auto& root : scene->rootNodes) {
                std::vector<BvhBuilder::InstanceInfo> instances = GenerateInstanceDesc(root.get(), sceneId, blasOffset);
                for (const auto& info : instances) {
                    // Color
                    *mappedColors = info.color;
                    mappedColors++;
                    // Transform
                    *mappedInstanceGeometryMapping = geomtrieStartIndecies[info.meshId];
                    mappedInstanceGeometryMapping++;
                    // Desc
                    *mappedDescirptions = info.desc;
                    mappedDescirptions++;
                }
            }
            blasOffset += scene->meshes.size();
            sceneId++;
        }
        instanceColors_.Unmap();
        blasInstances_.Unmap();
        InstanceGeometryMapping_.Unmap();
        topLevelInputs_.InstanceDescs = blasInstances_.GetGPUVirtualAddress();
    }

    geometryInitalize_ = true;
}

void BvhBuilder::SetGeometry(Scene& scene)
{
    std::array<Scene*, 1> scenes;
    scenes[0] = &scene;
    SetGeometry(scenes);
}

void BvhBuilder::SetGeometry(Mesh* mesh)
{
    SetGeometry(*mesh);
}

void BvhBuilder::SetGeometry(Mesh& mesh)
{
    throw "implement if needed";
}

D3D12_SHADER_RESOURCE_VIEW_DESC BvhBuilder::GetTlasViewDesc()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC tlasViewDesc          = {};
    tlasViewDesc.Format                                   = DXGI_FORMAT_UNKNOWN;
    tlasViewDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    tlasViewDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    tlasViewDesc.RaytracingAccelerationStructure.Location = tlas_->GetGPUVirtualAddress();
    return tlasViewDesc;
}

void BvhBuilder::CreateInstanceColorSRV(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    CreateFloat3SRV(device, handle, instanceColors_.Get());
}

void BvhBuilder::CreateInstanceGeometryMappingSRV(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    CreateIntSRV(device, handle, InstanceGeometryMapping_.Get());
}

const Descriptor BvhBuilder::CreatePrimitiveNormalsDescriptorArray(ComPtr<ID3D12Device5> device,
                                                                   DescriptorHeap*       descHeap)
{
    Assert(primitiveNormalBuffer_.size() < 16384);
    if (primitiveNormalBuffer_.size() >= 16384) {
        const auto msg = fmt::format(
            "More Primitives than allowed found {} (Max allowed: {})", primitiveNormalBuffer_.size(), 16384);
        spdlog::error(msg);
        throw std::runtime_error(msg);
    }

    const Descriptor descriptorTable = descHeap->AllocateDescriptorTable(primitiveNormalBuffer_.size());
    for (size_t i = 0; i < primitiveNormalBuffer_.size(); i++) {
        CreateFloat3SRV(device, descHeap->GetResourceView(descriptorTable.cpu, i), primitiveNormalBuffer_[i]);
    }
    return descriptorTable;
}

const Descriptor BvhBuilder::CreatePrimitiveIndeciesDescriptorArray(ComPtr<ID3D12Device5> device,
                                                                    DescriptorHeap*       descHeap)
{
    Assert(primitiveIndexBuffer_.size() < 16384);
    if (primitiveIndexBuffer_.size() >= 16384) {
        const auto msg =
            fmt::format("More Primitives than allowed found {} (Max allowed: {})", primitiveIndexBuffer_.size(), 16384);
        spdlog::error(msg);
        throw std::runtime_error(msg);
    }

    const Descriptor descriptorTable = descHeap->AllocateDescriptorTable(primitiveIndexBuffer_.size());
    for (size_t i = 0; i < primitiveIndexBuffer_.size(); i++) {
        CreateIntSRV(device,
                     descHeap->GetResourceView(descriptorTable.cpu, i),
                     primitiveIndexBuffer_[i].second,
                     primitiveIndexBuffer_[i].first);
    }
    return descriptorTable;
}

void BvhBuilder::BuildBvh(ComPtr<ID3D12GraphicsCommandList6> c)
{
    Assert(geometryInitalize_);
    D3D12_RESOURCE_BARRIER computeBarrier = {};
    computeBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    computeBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    computeBarrier.UAV.pResource          = nullptr;

    Assert(bottomLevelInputs_.size() == blas_.size());
    for (size_t i = 0; i < blas_.size(); i++) {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
        bottomLevelBuildDesc.Inputs                                             = bottomLevelInputs_[i];
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer_->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData    = blas_[i]->GetGPUVirtualAddress();

        c->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
        c->ResourceBarrier(1, &computeBarrier);
    }

    // Top Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    topLevelBuildDesc.Inputs                                             = topLevelInputs_;
    topLevelBuildDesc.ScratchAccelerationStructureData                   = scratchBuffer_->GetGPUVirtualAddress();
    topLevelBuildDesc.DestAccelerationStructureData                      = tlas_->GetGPUVirtualAddress();

    c->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    c->ResourceBarrier(1, &computeBarrier);
}

std::vector<BvhBuilder::InstanceInfo> BvhBuilder::GenerateInstanceDesc(Scene::Node* node,
                                                                       size_t       sceneId,
                                                                       size_t       blasOffset,
                                                                       Matrix4x4    currentTranform)
{
    std::vector<InstanceInfo> result;
    currentTranform = mul(currentTranform, node->matrix);
    if (node->mesh.has_value()) {
        InstanceInfo info;
        // Identity transform
        for (size_t i = 0; i < 12; i++) {
            const auto row     = i / 4;
            const auto column  = i % 4;
            const auto rcValue = currentTranform[column][row];

            info.desc.Transform[row][column] = rcValue;
        }
        info.meshId                     = *node->meshId;
        info.desc.InstanceID            = sceneId;
        info.desc.InstanceMask          = node->instanceMask;
        info.desc.AccelerationStructure = blas_[blasOffset + *node->meshId]->GetGPUVirtualAddress();
        info.color                      = node->meshColor.value_or(Float3(1, 0, 1));
        result.push_back(info);
    }
    for (auto child : node->children) {
        auto childResult = GenerateInstanceDesc(child.get(), sceneId, blasOffset, currentTranform);
        result.insert(result.end(), childResult.begin(), childResult.end());
    }
    return result;
}
