/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <DescriptorHeap.h>
#include <Scene.h>
#include <TextureBuffer.h>
#include <rayloader/VolumetricSampler.h>

struct VolumeProviderFootPrint {
    size_t chunkCount = 0;
    size_t chunkSize = 1;
    float  cellSize = 1.f;

    Float3 minBounds = F3ZERO;
    Float3 maxBounds = F3ONE;
};

class VolumeProvider {
public:
    VolumeProvider(ComPtr<ID3D12Device5> device, RayTrace* trace);

    VolumeProviderFootPrint ComputeData(ComPtr<ID3D12CommandQueue> copyQueue);

    void SetFilter(const RayFilter filter);
    void SetChunkSize(const size_t chunkSize);
    void SetCellSize(float cellSize);
    void SetMaxT(std::optional<float> maxT);

    void SetMinPointValue(float minPointValue);
    void SetMaxPointValue(float maxPointValue);
    void SetExcludePointsExeedingLimits(bool enable);
    void SetPintSampleSize(size_t pointSampleSize);
    void SetPointScale(float minPointScale, float maxPointScale);
    void SetScaleByPointValue(bool enable, bool inverse);

    inline bool IsDirty()
    {
        return dirty_;
    }

    inline void MarkDirty() {
        dirty_ = true;
    }

    inline bool IsPointCloudDirty()
    {
        return pointCloudDirty_ || dirty_;
    }

    inline size_t ChunkSize()
    {
        return sampler->ChunkSize();
    }

    inline float CellSize()
    {
        return sampler->CellSize();
    }

    inline std::optional<float> MaxT()
    {
        return sampler->MaxT();
    }

    inline VolumetricSampler::rdType MaxRays()
    {
        return sampler->MaxRays();
    }

    inline Float3 MinBounds()
    {
        return sampler->MinBounds();
    }

    inline Float3 MaxBounds()
    {
        return sampler->MaxBounds();
    }

    inline size_t ChunkCount()
    {
        return sampler->ChunkCount();
    }

    inline size_t PointSampleSize()
    {
        return pointSampleSize_;
    }

    inline float MinPointScale()
    {
        return minPointScale_;
    }

    inline float MaxPointScale()
    {
        return maxPointScale_;
    }

    inline Scene* GetPointCloud()
    {
        return &pointCloudScene_;
    }

    void TranistionToReadable(ComPtr<ID3D12GraphicsCommandList6> c);

    void             CreateChunkMinMaxSRV(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    const Descriptor CreateTextureArrayDesciptorArray(ComPtr<ID3D12Device5> device, DescriptorHeap* descHeap);

    void DumpToCSV(std::string path);

private:
    void                    RecalulatePointCloud();
    VolumeProviderFootPrint lastFootprint;

    ComPtr<ID3D12Device5>                    device;
    const std::unique_ptr<VolumetricSampler> sampler;

    UploadBuffer               volumeBounds_;
    std::vector<TextureBuffer> textures_;

    float  minPointValue_            = 0;
    float  maxPointValue_            = 128;
    bool   excludeExceeding_         = false;
    size_t pointSampleSize_          = 2;
    float  minPointScale_               = 0.f;
    float  maxPointScale_               = 100.f;
    bool   scaleByPointValue_        = false;
    bool   scaleByPointValueInverse_ = false;

    Scene pointCloudScene_;

    bool texturesReadable_ = false;
    bool dirty_            = true;
    bool pointCloudDirty_  = true;
};