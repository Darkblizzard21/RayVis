/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Buffers.h"

#include <rayvis-utils/MathUtils.h>
#include <spdlog/spdlog.h>

ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Device5> device, int frameCount, size_t size)
    : device_(device), size_(RoundToNextMultiple(size, BufferAllignSize))
{
    if (frameCount <= 0) {
        spdlog::warn("FrameCount {} is invalid. Defaulting to {}", frameCount, config::FramesInFlight);
        frameCount = config::FramesInFlight;
    }
    Allocate(frameCount);
}

void ConstantBuffer::AdvanceFrame()
{
    frameIdx_ = (frameIdx_ + 1) % buffers_.size();
}

void* ConstantBuffer::Map(UINT Subresource, const D3D12_RANGE* pReadRange)
{
    void* mappedPtr;
    ThrowIfFailed(buffers_[frameIdx_]->Map(Subresource, pReadRange, &mappedPtr));
    return mappedPtr;
}

void ConstantBuffer::Unmap(UINT Subresource, const D3D12_RANGE* pWrittenRange)
{
    buffers_[frameIdx_]->Unmap(Subresource, pWrittenRange);
}

D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBuffer::GetDesc()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
    constantBufferViewDesc.BufferLocation                  = buffers_[frameIdx_]->GetGPUVirtualAddress();
    constantBufferViewDesc.SizeInBytes                     = size_;
    return constantBufferViewDesc;
}

D3D12_DESCRIPTOR_RANGE ConstantBuffer::GetDescriptorRange(UINT                        tableID,
                                                          UINT                        shaderRegister,
                                                          D3D12_DESCRIPTOR_RANGE_TYPE type)
{
    D3D12_DESCRIPTOR_RANGE descriptorRange;
    descriptorRange.BaseShaderRegister                = shaderRegister;
    descriptorRange.NumDescriptors                    = 1;
    descriptorRange.OffsetInDescriptorsFromTableStart = tableID;
    descriptorRange.RangeType                         = type;
    descriptorRange.RegisterSpace                     = 0;
    return descriptorRange;
}

void ConstantBuffer::Allocate(int frameCount)
{
    buffers_.resize(frameCount);
    for (size_t i = 0; i < frameCount; i++) {
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type                  = D3D12_HEAP_TYPE_UPLOAD;
        heapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask      = 0;
        heapProperties.VisibleNodeMask       = 0;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment           = 0;
        resourceDesc.Width               = size_;
        resourceDesc.Height              = 1;
        resourceDesc.DepthOrArraySize    = 1;
        resourceDesc.MipLevels           = 1;
        resourceDesc.Format              = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count    = 1;
        resourceDesc.SampleDesc.Quality  = 0;
        resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(device_->CreateCommittedResource(&heapProperties,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &resourceDesc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr,
                                                       IID_PPV_ARGS(&buffers_[i])));
    }
}

UploadBuffer::UploadBuffer(ComPtr<ID3D12Device5> device)
{
    Init(device);
}

UploadBuffer::UploadBuffer(ComPtr<ID3D12Device5> device, UINT64 width)
{
    Init(device);
    Resize(width);
}

void UploadBuffer::Init(ComPtr<ID3D12Device5> device)
{
    device_ = device;
}

void UploadBuffer::Resize(UINT64 width)
{
    assert(width > 0);
    assert(device_ != nullptr);
    if (uploadBuffer_ && (width == uploadBuffer_->GetDesc().Width)) {
        return;
    }
    D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
    uploadHeapProperties.Type                  = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    uploadHeapProperties.CreationNodeMask      = 0;
    uploadHeapProperties.VisibleNodeMask       = 0;

    D3D12_RESOURCE_DESC uploadBufferDesc = {};
    uploadBufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadBufferDesc.Alignment           = 0;
    uploadBufferDesc.Width               = width;
    uploadBufferDesc.Height              = 1;
    uploadBufferDesc.DepthOrArraySize    = 1;
    uploadBufferDesc.MipLevels           = 1;
    uploadBufferDesc.Format              = DXGI_FORMAT_UNKNOWN;
    uploadBufferDesc.SampleDesc.Count    = 1;
    uploadBufferDesc.SampleDesc.Quality  = 0;
    uploadBufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uploadBufferDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(device_->CreateCommittedResource(&uploadHeapProperties,
                                                   D3D12_HEAP_FLAG_NONE,
                                                   &uploadBufferDesc,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ,
                                                   nullptr,
                                                   IID_PPV_ARGS(&uploadBuffer_)));
}

void* UploadBuffer::Map(UINT Subresource, const D3D12_RANGE* pReadRange)
{
    assert(uploadBuffer_ != nullptr);
    void* mappedPtr;
    ThrowIfFailed(uploadBuffer_->Map(Subresource, pReadRange, &mappedPtr));
    return mappedPtr;
}

void UploadBuffer::Unmap(UINT Subresource, const D3D12_RANGE* pWrittenRange)
{
    assert(uploadBuffer_ != nullptr);
    uploadBuffer_->Unmap(Subresource, pWrittenRange);
}

int UploadBuffer::Width()
{
    assert(uploadBuffer_ != nullptr);
    return uploadBuffer_->GetDesc().Width;
}

ID3D12Resource* UploadBuffer::Get()
{
    assert(uploadBuffer_ != nullptr);
    return uploadBuffer_.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::GetGPUVirtualAddress()
{
    return uploadBuffer_->GetGPUVirtualAddress();
}

D3D12_DESCRIPTOR_RANGE UploadBuffer::GetDescriptorRange(UINT tableID, UINT shaderRegister, UINT numDescriptors)
{
    D3D12_DESCRIPTOR_RANGE descriptorRange;
    descriptorRange.BaseShaderRegister                = shaderRegister;
    descriptorRange.NumDescriptors                    = numDescriptors;
    descriptorRange.OffsetInDescriptorsFromTableStart = tableID;
    descriptorRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.RegisterSpace                     = 0;
    return descriptorRange;
}
