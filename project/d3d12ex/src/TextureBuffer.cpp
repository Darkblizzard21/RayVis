/* Copyright (c) 2023 Pirmin Pfeifer */
#include "TextureBuffer.h"

#include "Config.h"
#include <cassert>

namespace {
    constexpr bool Has2D(D3D12_RESOURCE_DIMENSION dimension)
    {
        return 2 < static_cast<int>(dimension);
    }

    constexpr bool Has3D(D3D12_RESOURCE_DIMENSION dimension)
    {
        return 3 < static_cast<int>(dimension);
    }

    constexpr size_t FormatSize(DXGI_FORMAT format)
    {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return 4;
        case DXGI_FORMAT_R8_UNORM:
            return 1;
        case DXGI_FORMAT_R8_UINT:
            return 1;
        default:
            throw "format size is not in list";
        }
    }

    D3D12_SRV_DIMENSION Convert(D3D12_RESOURCE_DIMENSION d)
    {
        switch (d) {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            return D3D12_SRV_DIMENSION_TEXTURE1D;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            return D3D12_SRV_DIMENSION_TEXTURE2D;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            return D3D12_SRV_DIMENSION_TEXTURE3D;
        default:
            throw "D3D12_RESOURCE_DIMENSION is not in list";
        }
    }
}  // namespace

TextureBuffer::TextureBuffer(ComPtr<ID3D12Device5>      device,
                             ComPtr<ID3D12CommandQueue> copyQueue,
                             D3D12_RESOURCE_DIMENSION   dimension,
                             DXGI_FORMAT                format,
                             const void*                data,
                             const uint32_t             width,
                             const uint32_t             height,
                             const uint32_t             depth)
    : format_(format), dimension_(dimension)
{
    Init(device, copyQueue, data, width, height, depth);
}

bool TextureBuffer::IsReadable()
{
    return state_ == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
}

void TextureBuffer::TranistionToReadable(ComPtr<ID3D12GraphicsCommandList6> c)
{
    if (!IsReadable()) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = textureBuffer_.Get();
        barrier.Transition.StateBefore = state_;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = 0;
        c->ResourceBarrier(1, &barrier);
        state_ = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
}

int TextureBuffer::Dimension()
{
    return textureBuffer_->GetDesc().Dimension;
}

int TextureBuffer::Width()
{
    return textureBuffer_->GetDesc().Width;
}

int TextureBuffer::Height()
{
    return textureBuffer_->GetDesc().Height;
}

int TextureBuffer::Depth()
{
    return textureBuffer_->GetDesc().DepthOrArraySize;
}

void TextureBuffer::CreateShaderResourceView(ComPtr<ID3D12Device5> device, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.Format                          = format_;
    shaderResourceViewDesc.ViewDimension                   = Convert(dimension_);
    switch (shaderResourceViewDesc.ViewDimension) {
    case D3D12_SRV_DIMENSION_TEXTURE2D:
        shaderResourceViewDesc.Texture2D.MipLevels           = 1;
        shaderResourceViewDesc.Texture2D.MostDetailedMip     = 0;
        shaderResourceViewDesc.Texture2D.PlaneSlice          = 0;
        shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE3D:
        shaderResourceViewDesc.Texture3D.MipLevels           = 1;
        shaderResourceViewDesc.Texture3D.MostDetailedMip     = 0;
        shaderResourceViewDesc.Texture3D.ResourceMinLODClamp = 0;
        break;
    default:
        throw "dimension not supported";
    }
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    device->CreateShaderResourceView(textureBuffer_.Get(), &shaderResourceViewDesc, handle);
}

D3D12_DESCRIPTOR_RANGE TextureBuffer::GetDescriptorRange(UINT tableID, UINT shaderRegister, UINT numDescriptors)
{
    return UploadBuffer::GetDescriptorRange(tableID, shaderRegister, numDescriptors);
}

void TextureBuffer::Init(ComPtr<ID3D12Device5>      device,
                         ComPtr<ID3D12CommandQueue> copyQueue,
                         const void*                data,
                         const uint32_t             width,
                         const uint32_t             height,
                         const uint32_t             depth)
{
    assert(dimension_ != D3D12_RESOURCE_DIMENSION_UNKNOWN);
    assert(dimension_ != D3D12_RESOURCE_DIMENSION_BUFFER);

    assert(Has2D(dimension_) || height == 0);
    assert(Has3D(dimension_) || depth == 1);

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask      = 0;
    heapProperties.VisibleNodeMask       = 0;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = dimension_;
    resourceDesc.Alignment           = 0;
    resourceDesc.Width               = width;
    resourceDesc.Height              = height;
    resourceDesc.DepthOrArraySize    = depth;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Format              = format_;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.SampleDesc.Quality  = 0;
    resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

    state_ = D3D12_RESOURCE_STATE_COPY_DEST;
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &resourceDesc,
                                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr,
                                                  IID_PPV_ARGS(&textureBuffer_)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    UINT                               numRows;
    UINT64                             rowSizeInBytes;
    UINT64                             totalBytes;
    device->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

    uploadBuffer_         = std::make_unique<UploadBuffer>(device, totalBytes);
    // Initialize buffer with image data
    byte*       mappedPtr = static_cast<byte*>(uploadBuffer_->Map());
    const byte* dataPtr   = reinterpret_cast<const byte*>(data);
    for (int i = 0; i < numRows * depth; ++i) {
        byte*       dstRow = mappedPtr + i * (footprint.Footprint.RowPitch);
        const byte* srcRow = dataPtr + i * rowSizeInBytes;
        std::memcpy(dstRow, srcRow, rowSizeInBytes);
    }
    uploadBuffer_->Unmap();

    ComPtr<ID3D12CommandAllocator>    copyCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> copyCommandList;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copyCommandAllocator)));

    ThrowIfFailed(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_COPY, copyCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&copyCommandList)));

    D3D12_TEXTURE_COPY_LOCATION dest = {};
    dest.pResource                   = textureBuffer_.Get();
    dest.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dest.SubresourceIndex            = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource                   = uploadBuffer_->Get();
    src.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint             = footprint;

    copyCommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

    ThrowIfFailed(copyCommandList->Close());
    ID3D12CommandList* commandLists[] = {copyCommandList.Get()};
    copyQueue->ExecuteCommandLists(1, commandLists);

    ComPtr<ID3D12Fence> myFence;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&myFence)));
    auto fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fenceEvent) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    copyQueue->Signal(myFence.Get(), 1);
    if (myFence->GetCompletedValue() != 1) {
        ThrowIfFailed(myFence->SetEventOnCompletion(1, fenceEvent));
        ::WaitForSingleObject(fenceEvent, INFINITE);
    }
    uploadBuffer_ = nullptr;
}
