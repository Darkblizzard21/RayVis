/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <d3d12.h>
#include <wrl/client.h>  // for ComPtr

#include <span>
#include <vector>

#include "Config.h"

using Microsoft::WRL::ComPtr;

constexpr int BufferAllignSize = 256;

class ConstantBuffer {
public:
    ConstantBuffer(ComPtr<ID3D12Device5> device, int frameCount, size_t size = config::ConstantBufferSizeBytes);

    void  AdvanceFrame();
    void* Map(UINT Subresource = 0, const D3D12_RANGE* pReadRange = nullptr);
    void  Unmap(UINT Subresource = 0, const D3D12_RANGE* pWrittenRange = nullptr);

    D3D12_CONSTANT_BUFFER_VIEW_DESC GetDesc();
    static D3D12_DESCRIPTOR_RANGE   GetDescriptorRange(
          UINT                        tableID,
          UINT                        shaderRegister = 0,
          D3D12_DESCRIPTOR_RANGE_TYPE type           = D3D12_DESCRIPTOR_RANGE_TYPE_CBV);

private:
    void Allocate(int frameCount);

    int          frameIdx_ = 0;
    const size_t size_;

    ComPtr<ID3D12Device5>               device_;
    std::vector<ComPtr<ID3D12Resource>> buffers_;
};

class UploadBuffer {
public:
    UploadBuffer() = default;
    UploadBuffer(ComPtr<ID3D12Device5> device);
    UploadBuffer(ComPtr<ID3D12Device5> device, UINT64 width);

    void Init(ComPtr<ID3D12Device5> device);
    void Resize(UINT64 width);

    void* Map(UINT Subresource = 0, const D3D12_RANGE* pReadRange = nullptr);
    void  Unmap(UINT Subresource = 0, const D3D12_RANGE* pWrittenRange = nullptr);

    template <typename T>
    void Map(std::span<T> toMap, UINT Subresource = 0, const D3D12_RANGE* pReadRange = nullptr)
    {
        const auto size = toMap.size() * sizeof(T);
        Resize(size);
        T* mappedPtr = static_cast<T*>(Map(Subresource, pReadRange));
        std::memcpy(mappedPtr, toMap.data(), size);
        Unmap();
    }

    template <typename T>
    void Map(const T& toMap, UINT Subresource = 0, const D3D12_RANGE* pReadRange = nullptr)
    {
        const auto size = sizeof(T);
        Resize(size);
        T* mappedPtr = static_cast<T*>(Map(Subresource, pReadRange));
        std::memcpy(mappedPtr, &toMap, size);
        Unmap();
    }

    int                           Width();
    ID3D12Resource*               Get();
    D3D12_GPU_VIRTUAL_ADDRESS     GetGPUVirtualAddress();
    static D3D12_DESCRIPTOR_RANGE GetDescriptorRange(UINT tableID, UINT shaderRegister = 0, UINT numDescriptors = 1);

private:
    ComPtr<ID3D12Device5>  device_       = nullptr;
    ComPtr<ID3D12Resource> uploadBuffer_ = nullptr;
};
