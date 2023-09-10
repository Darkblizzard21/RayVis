/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <d3d12.h>


#include <wrl/client.h>  // for ComPtr

#include "d3d12ex/Buffers.h"

using Microsoft::WRL::ComPtr;

class TextureBuffer {
public:
    TextureBuffer(ComPtr<ID3D12Device5>      device,
                  ComPtr<ID3D12CommandQueue> copyQueue,
                  D3D12_RESOURCE_DIMENSION   dimension,
                  DXGI_FORMAT                format,
                  const void*                data,
                  const uint32_t             width,
                  const uint32_t             height = 0,
                  const uint32_t             depth  = 1);

    bool IsReadable();
    void TranistionToReadable(ComPtr<ID3D12GraphicsCommandList6> c);

    int Dimension();
    int Width();
    int Height();
    int Depth();

    void CreateShaderResourceView(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    static D3D12_DESCRIPTOR_RANGE GetDescriptorRange(UINT tableID, UINT shaderRegister = 0, UINT numDescriptors = 1);

private:
    void                     Init(ComPtr<ID3D12Device5>      device,
                                  ComPtr<ID3D12CommandQueue> copyQueue,
                                  const void*                data,
                                  const uint32_t             width,
                                  const uint32_t             height = 0,
                                  const uint32_t             depth  = 1);
    D3D12_RESOURCE_DIMENSION dimension_;
    DXGI_FORMAT              format_;

    D3D12_RESOURCE_STATES         state_;
    std::unique_ptr<UploadBuffer> uploadBuffer_ = nullptr;
    ComPtr<ID3D12Resource>        textureBuffer_;
};