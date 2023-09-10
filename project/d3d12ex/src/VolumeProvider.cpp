/* Copyright (c) 2023 Pirmin Pfeifer */
#include "VolumeProvider.h"

#include <rayvis-utils/Color.h>

#include <fstream>
#include <iostream>

namespace {
    std::unique_ptr<Mesh> ArrowCross(ComPtr<ID3D12Device5> device)
    {
        // D:\RayVisThesis\tex\images\ArrowIndexing.png
        //          0
        //         1 2
        //        3   4
        //         | |
        //         5 6

        std::array<Vertex, 7 * 2> vertexBuffer;
        float                     x = 0;
        float                     z = 1;
        vertexBuffer[0]             = Float3(x, 0, z);

        x               = 0.25;
        z               = 0.25;
        vertexBuffer[1] = Float3(x, 0, z);
        vertexBuffer[2] = Float3(-x, 0, z);

        x               = 0.7;
        z               = 0.05;
        vertexBuffer[3] = Float3(x, 0, z);
        vertexBuffer[4] = Float3(-x, 0, z);

        x               = 0.25;
        z               = -1;
        vertexBuffer[5] = Float3(x, 0, z);
        vertexBuffer[6] = Float3(-x, 0, z);

        std::array<UINT16, (5 * 3) * 2> indexBuffer;

        indexBuffer[0] = 0;
        indexBuffer[1] = 1;
        indexBuffer[2] = 3;

        indexBuffer[3] = 0;
        indexBuffer[4] = 2;
        indexBuffer[5] = 1;

        indexBuffer[6] = 0;
        indexBuffer[7] = 4;
        indexBuffer[8] = 2;

        indexBuffer[9]  = 1;
        indexBuffer[10] = 2;
        indexBuffer[11] = 5;

        indexBuffer[12] = 2;
        indexBuffer[13] = 6;
        indexBuffer[14] = 5;

        // Create rotated plane
        for (size_t i = 0; i < 7; i++) {
            const Float3& vertex = vertexBuffer[i];
            vertexBuffer[i + 7]  = Float3(vertex.y, vertex.x, vertex.z);
        }
        for (size_t i = 0; i < (5 * 3); i++) {
            indexBuffer[i + 15] = indexBuffer[i] + 7;
        }

        // Apply uniform scale
        for (size_t i = 0; i < vertexBuffer.size(); i++) {
            vertexBuffer[i] *= 0.5f;
        }

        return std::make_unique<Mesh>(device, vertexBuffer, indexBuffer);
    }

    bool GetPointData(const size_t&                       baseX,
                      const size_t&                       baseY,
                      const size_t&                       baseZ,
                      const size_t&                       pointSize,
                      const VolumetricSampler::ChunkData& data,
                      float&                              outValue,
                      Float3&                             outDir)
    {
        float validDataCount = 0;
        outValue             = 0.f;
        outDir               = Float3(0.f);

        const size_t sX = baseX * pointSize;
        const size_t sY = baseZ * pointSize;
        const size_t sZ = baseY * pointSize;
        for (size_t x = sX; x < (sX + pointSize); x++) {
            for (size_t y = sY; y < (sY + pointSize); y++) {
                for (size_t z = sZ; z < (sZ + pointSize); z++) {
                    if (data.RayDenity(x, y, z) <= 0) {
                        continue;
                    }
                    validDataCount++;
                    outValue += data.RayDenity(x, y, z);
                    outDir += data.Directions(x, y, z);
                }
            }
        }
        if (validDataCount == 0) {
            return false;
        }
        const float dataPoints = pointSize * pointSize * pointSize;
        outValue               = outValue / dataPoints;
        outDir                 = outDir / validDataCount;
        return true;
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
}  // namespace

VolumeProvider::VolumeProvider(ComPtr<ID3D12Device5> deviceIn, RayTrace* trace)
    : device(std::move(deviceIn)), sampler(std::make_unique<VolumetricSampler>(trace))
{
    assert(sampler->ChunkSize() % pointSampleSize_ == 0);

    pointCloudScene_.meshes.push_back(std::move(ArrowCross(device)));
    assert(pointCloudScene_.meshes.size() == 1);

    volumeBounds_.Init(device);
}

VolumeProviderFootPrint VolumeProvider::ComputeData(ComPtr<ID3D12CommandQueue> copyQueue)
{
    if (!dirty_) {
        spdlog::info("VolumeProvider::ComputeData was skipped, because the existing data is not dirty.");
        RecalulatePointCloud();
        return lastFootprint;
    }
    sampler->Sample();

    if (sampler->Data()->size() == 0) {
        HWND consoleWindow = GetConsoleWindow();
        SetForegroundWindow(consoleWindow);
        spdlog::error(
            "Sampler returned with out data (probably filtered rays where empty). Changing miss detection tolerance "
            "can help");
        return VolumeProviderFootPrint();
    }
    const auto     data      = sampler->Data();
    
    
    const auto     begin     = std::chrono::steady_clock::now();
    // Make textures;
    const uint32_t chunkSize = sampler->ChunkSize();
    const double   rayCountScale =
        static_cast<double>(std::numeric_limits<VolumetricSampler::rdType>::max()) / sampler->MaxRays();

    texturesReadable_ = false;
    textures_.clear();
    textures_.reserve(data->size());
    for (const auto& vol : *data) {
        // Create padding outside of volume
        const uint32_t                         chunkSizeWithPadding = chunkSize + 2;
        std::vector<VolumetricSampler::rdType> textureData;
        textureData.resize(chunkSizeWithPadding * chunkSizeWithPadding * chunkSizeWithPadding);

        constexpr float edgeNodeValue = 5.f;
        float           edgeNode      = 1.f;
        const auto      getValue      = [&edgeNode, max = chunkSizeWithPadding - 1](const size_t& i) -> size_t {
            if (i == 0) {
                edgeNode += edgeNodeValue;
                return 1;
            }
            if (i == max) {
                edgeNode += edgeNodeValue;
                return max - 1;
            }
            return i;
        };
        for (size_t x = 0; x < chunkSizeWithPadding; x++) {
            for (size_t y = 0; y < chunkSizeWithPadding; y++) {
                for (size_t z = 0; z < chunkSizeWithPadding; z++) {
                    edgeNode         = 1.f;
                    const size_t rX  = getValue(x) - 1;
                    const size_t rY  = getValue(y) - 1;
                    const size_t rZ  = getValue(z) - 1;
                    const size_t idx = (z)*chunkSizeWithPadding * chunkSizeWithPadding + (y)*chunkSizeWithPadding + (x);
                    VolumetricSampler::rdType value = vol.RayDenity(rX, rY, rZ) * (1.f / edgeNode);

                    // rescale data
                    if (0 < value) {
                        const double scaledValue = value * rayCountScale;
                        value                    = static_cast<VolumetricSampler::rdType>(scaledValue);
                    }
                    textureData[idx] = value;
                }
            }
        }

        static_assert(std::is_unsigned_v<VolumetricSampler::rdType> == true);
        static_assert(std::is_integral_v<VolumetricSampler::rdType> == true);

        constexpr DXGI_FORMAT textureFormat = sizeof(VolumetricSampler::rdType) == 1   ? DXGI_FORMAT_R8_UNORM
                                              : sizeof(VolumetricSampler::rdType) == 2 ? DXGI_FORMAT_R16_UNORM
                                                                                       : DXGI_FORMAT_UNKNOWN;
        static_assert(textureFormat != DXGI_FORMAT_UNKNOWN);

        textures_.push_back(TextureBuffer(device,
                                          copyQueue,
                                          D3D12_RESOURCE_DIMENSION_TEXTURE3D,
                                          textureFormat,
                                          reinterpret_cast<const void*>(textureData.data()),
                                          chunkSizeWithPadding,
                                          chunkSizeWithPadding,
                                          chunkSizeWithPadding));
    }

    
    const auto end = std::chrono::steady_clock::now();
    spdlog::info("Created and uploaded {} chunks as texture in {}s",
                 ChunkCount(),
                 std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f);

    // volumeBounds_ buffer;
    volumeBounds_.Resize(sizeof(Float3) * 2 * data->size());
    Float3* mappedPtr = reinterpret_cast<Float3*>(volumeBounds_.Map());
    for (const auto& vol : *data) {
        *mappedPtr       = vol.min - CellSize();
        *(mappedPtr + 1) = vol.max + CellSize();
        mappedPtr        = mappedPtr + 2;
    }
    volumeBounds_.Unmap();

    dirty_ = false;

    pointCloudDirty_ = true;
    RecalulatePointCloud();

    lastFootprint = {ChunkCount(), ChunkSize() + 2, CellSize(), MinBounds(), MaxBounds()};
    return lastFootprint;
}

void VolumeProvider::RecalulatePointCloud()
{
    if (!pointCloudDirty_) {
        spdlog::info("VolumeProvider::RecalulatePointCloud was skipped, because the existing data is not dirty.");
        return;
    }
    assert(!dirty_);
    const auto begin = std::chrono::steady_clock::now();

    const auto     data      = sampler->Data();
    const uint32_t chunkSize = sampler->ChunkSize();

    int  nodeId   = 0;
    auto rootNode = std::make_unique<Scene::Node>();
    rootNode->id  = nodeId++;

    assert((chunkSize % pointSampleSize_) == 0);
    const size_t pcChunkResolution = chunkSize / pointSampleSize_;
    const auto   arrowMeshPtr      = pointCloudScene_.meshes[0].get();
    [&]() {
        const float  pointScale    = maxPointScale_ - minPointScale_;
        const size_t maxPointCount = 1 << 21;
        size_t       pointCount    = 0;
        for (const auto& vol : *data) {
            for (size_t x = 0; x < pcChunkResolution; x++) {
                for (size_t y = 0; y < pcChunkResolution; y++) {
                    for (size_t z = 0; z < pcChunkResolution; z++) {
                        float  value;
                        Float3 dir;
                        if (!GetPointData(x, y, z, pointSampleSize_, vol, value, dir)) {
                            continue;
                        }

                        // map value
                        value            = (value - minPointValue_) / (maxPointValue_ - minPointValue_);
                        float colorValue = value;

                        if (scaleByPointValue_ && scaleByPointValueInverse_) {
                            value = 1 - value;
                        }
                        if ((scaleByPointValue_ || excludeExceeding_) && value <= 0) {
                            continue;
                        }
                        if (excludeExceeding_ && 1.f < value) {
                            continue;
                        }
                        value = std::clamp(value, 0.f, 1.f);
                        if (!scaleByPointValue_) {
                            value = 1.f;
                        }

                        // Build node
                        auto node          = std::make_shared<Scene::Node>();
                        node->id           = nodeId++;
                        node->mesh         = arrowMeshPtr;
                        node->meshId       = 0;
                        node->instanceMask = InstanceMask::DIRIECTIONAL_POINT_CLOUD;
                        node->meshColor    = color::Plasma(colorValue);

                        // Make matrix
                        const Float3 meshOrigin =
                            vol.min + ((Float3(x, z, y) * Float3(static_cast<float>(pointSampleSize_))) + Float3(0.5)) *
                                          sampler->CellSize();
                        const Float3 scale     = Float3((value * pointScale) + minPointScale_);
                        const Float4 roatation = rotAtoB(F3FORWARD, dir);
                        node->matrix           = transform(meshOrigin, roatation, scale);

                        rootNode->children.push_back(node);

                        if (maxPointCount <= pointCount++) {
                            spdlog::error(
                                "Point cloud calculation stopped because maxPoint limit ({}) has been reached",
                                maxPointCount);
                            return;
                        }
                    }
                }
            }
        }
    }();

    pointCloudScene_.rootNodes.clear();
    pointCloudScene_.rootNodes.push_back(std::move(rootNode));
    pointCloudScene_.RecalculateMinMax();

    const auto end = std::chrono::steady_clock::now();
    spdlog::info("Recalculated PointCloud - {} point in {}s",
                 pointCloudScene_.InstanceCount(),
                 std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f);

    pointCloudDirty_ = false;
}

void VolumeProvider::SetFilter(const RayFilter filter)
{
    dirty_ = true;
    sampler->SetFilter(filter);
}

void VolumeProvider::SetChunkSize(const size_t chunkSize)
{
    dirty_ = true;
    sampler->SetChunkSize(chunkSize);
    while (sampler->ChunkSize() % pointSampleSize_ != 0) {
        pointSampleSize_--;
    }
}

void VolumeProvider::SetCellSize(float cellSize)
{
    dirty_ = true;
    sampler->SetCellSize(cellSize);
    maxPointScale_ = std::min(cellSize, maxPointScale_);
}

void VolumeProvider::SetMaxT(std::optional<float> maxT)
{
    dirty_ = true;
    sampler->SetMaxT(maxT);
}

void VolumeProvider::SetMinPointValue(float minPointValue)
{
    pointCloudDirty_ = true;
    minPointValue_   = minPointValue;
}

void VolumeProvider::SetMaxPointValue(float maxPointValue)
{
    pointCloudDirty_ = true;
    maxPointValue_   = maxPointValue;
}

void VolumeProvider::SetExcludePointsExeedingLimits(bool enable)
{
    pointCloudDirty_  = true;
    excludeExceeding_ = enable;
}

void VolumeProvider::SetPintSampleSize(size_t pointSampleSize)
{
    assert(sampler->ChunkSize() % pointSampleSize == 0);
    pointCloudDirty_ = true;
    pointSampleSize_ = pointSampleSize;
}

void VolumeProvider::SetPointScale(float minPointScale, float maxPointScale)
{
    assert(minPointScale < maxPointScale);
    pointCloudDirty_ = true;

    minPointScale_ = minPointScale;
    maxPointScale_ = maxPointScale;
}

void VolumeProvider::SetScaleByPointValue(bool enable, bool inverse)
{
    pointCloudDirty_          = true;
    scaleByPointValue_        = enable;
    scaleByPointValueInverse_ = inverse;
}

void VolumeProvider::TranistionToReadable(ComPtr<ID3D12GraphicsCommandList6> c)
{
    if (texturesReadable_) {
        return;
    }
    for (auto& tex : textures_) {
        tex.TranistionToReadable(c);
    }
    texturesReadable_ = true;
}

void VolumeProvider::CreateChunkMinMaxSRV(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    CreateFloat3SRV(device, handle, volumeBounds_.Get());
}

const Descriptor VolumeProvider::CreateTextureArrayDesciptorArray(ComPtr<ID3D12Device5> device,
                                                                  DescriptorHeap*       descHeap)
{
    assert(textures_.size() <= 512);
    const Descriptor descriptorTable = descHeap->AllocateDescriptorTable(textures_.size());
    for (size_t i = 0; i < textures_.size(); i++) {
        textures_[i].CreateShaderResourceView(device, descHeap->GetResourceView(descriptorTable.cpu, i));
    }
    return descriptorTable;
}

void VolumeProvider::DumpToCSV(std::string path)
{
    uint64_t      dataPoints = 0;
    std::ofstream dataStream(path + ".csv");
    std::ofstream inverseStream(path + "inverse.csv");
    dataStream << "x,y,z\n";
    inverseStream << "x,y,z\n";
    auto chunkSize     = sampler->ChunkSize();
    auto itmesPerChunk = chunkSize * chunkSize * chunkSize;
    for (const auto& data : *sampler->Data()) {
        const auto extends = data.max - data.min;
        const auto step    = extends / chunkSize;
        for (size_t i = 0; i < itmesPerChunk; i++) {
            const auto ix = i / (chunkSize * chunkSize);
            const auto iy = (i / chunkSize) % chunkSize;
            const auto iz = i % chunkSize;
            const auto x  = data.min.x + ix * step.x;
            const auto y  = data.min.y + iy * step.y;
            const auto z  = data.min.z + iz * step.z;
            auto       f  = fmt::format("{},{},{}\n", x, y, z);
            if (0 < data.rayDensity[i]) {
                dataStream << f;
                dataPoints++;
            } else {
                inverseStream << f;
            }
        }
    }
    spdlog::info("Finished csv dump for {} chunks with resolution {}", dataPoints, ChunkSize());
};
