/* Copyright (c) 2023 Pirmin Pfeifer */
#include "DescriptorHeap.h"

#include <rayvis-utils/BreakAssert.h>

constexpr UINT RESOURCE_DESCRIPTOR_SIZE = (1 << 15) + (1 << 10);
constexpr UINT RENDERTARGET_DESCRIPTOR_SIZE = 1<< 6;
constexpr UINT SAMPLER_DESCRIPTOR_SIZE = 1 << 6;

DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device5> device, int framesInFlight)
    : device_(device), framesInFlight(framesInFlight)
{
    for (size_t i = 0; i < framesInFlight; i++) {  // Create DescriptorHeaps
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NodeMask                   = 0;
        heapDesc.NumDescriptors             = RESOURCE_DESCRIPTOR_SIZE;
        heapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&resourceViewHeap_[i])));
        resourceViewHeap_[i]->SetName((std::wstring(L"Resource View Heap") + std::to_wstring(i)).c_str());

        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = RENDERTARGET_DESCRIPTOR_SIZE;
        ThrowIfFailed(device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&renderTargetViewHeap_[i])));
        renderTargetViewHeap_[i]->SetName((std::wstring(L"Render Target View Heap") + std::to_wstring(i)).c_str());

        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapDesc.NumDescriptors = SAMPLER_DESCRIPTOR_SIZE;
        ThrowIfFailed(device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&samplerHeap_[i])));
        samplerHeap_[i]->SetName((std::wstring(L"Sampler Heap") + std::to_wstring(i)).c_str());
    }
}

void DescriptorHeap::Reset()
{
    frameIdx_ = (frameIdx_ + 1) % framesInFlight;

    resourceViewCount_     = 0;
    renderTargetViewCount_ = 0;
    samplerCount_          = 0;
}

ID3D12DescriptorHeap* DescriptorHeap::GetResourceHeap()
{
    return resourceViewHeap_[frameIdx_].Get();
}

ID3D12DescriptorHeap* DescriptorHeap::GetSamplerHeap()
{
    return samplerHeap_[frameIdx_].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetResourceView(D3D12_CPU_DESCRIPTOR_HANDLE base, int index)
{
    const UINT incrementSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    /** Bounds check */
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu = resourceViewHeap_[frameIdx_]->GetCPUDescriptorHandleForHeapStart();
    Assert(cpu.ptr <= base.ptr);
    Assert(base.ptr + incrementSize * index < cpu.ptr + incrementSize * RESOURCE_DESCRIPTOR_SIZE);
    return {base.ptr + incrementSize * index};
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetResourceView(D3D12_GPU_DESCRIPTOR_HANDLE base, int index)
{
    const UINT incrementSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    /** Bounds check */
    const D3D12_GPU_DESCRIPTOR_HANDLE gpu = resourceViewHeap_[frameIdx_]->GetGPUDescriptorHandleForHeapStart();
    Assert(gpu.ptr <= base.ptr);
    Assert(base.ptr + incrementSize * index < gpu.ptr + incrementSize * RESOURCE_DESCRIPTOR_SIZE);
    return {base.ptr + incrementSize * index};
}

Descriptor DescriptorHeap::GetResourceView(int index)
{
    Assert(0 <= index);
    Assert(index < RESOURCE_DESCRIPTOR_SIZE);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu = resourceViewHeap_[frameIdx_]->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE gpu = resourceViewHeap_[frameIdx_]->GetGPUDescriptorHandleForHeapStart();
    return {GetResourceView(cpu, index), GetResourceView(gpu, index)};
}

Descriptor DescriptorHeap::AllocateDescriptorTable(int count)
{
    Assert(0 < count);
    Assert(resourceViewCount_ + count <= RESOURCE_DESCRIPTOR_SIZE);
    Descriptor table = GetResourceView(resourceViewCount_);
    resourceViewCount_ += count;
    return table;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE base, int index)
{
    const UINT incrementSize              = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    /** Bounds check */
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu = renderTargetViewHeap_[frameIdx_]->GetCPUDescriptorHandleForHeapStart();
    Assert(cpu.ptr <= base.ptr);
    Assert(base.ptr + incrementSize * index < cpu.ptr + incrementSize * RENDERTARGET_DESCRIPTOR_SIZE);
    return {base.ptr + incrementSize * index};
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetRenderTargetView(int index)
{
    Assert(0 <= index);
    Assert(index < RENDERTARGET_DESCRIPTOR_SIZE);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu = renderTargetViewHeap_[frameIdx_]->GetCPUDescriptorHandleForHeapStart();
    return GetRenderTargetView(cpu, index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::AllocateRenderTargetView(int count)
{
    Assert(0 < count);
    Assert(renderTargetViewCount_ + count <= RENDERTARGET_DESCRIPTOR_SIZE);
    auto handle = GetRenderTargetView(renderTargetViewCount_);
    renderTargetViewCount_ += count;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetSamplerHandle(D3D12_CPU_DESCRIPTOR_HANDLE base, int index)
{
    const UINT incrementSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    /** Bounds check */
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu = samplerHeap_[frameIdx_]->GetCPUDescriptorHandleForHeapStart();
    Assert(cpu.ptr <= base.ptr);
    Assert(base.ptr + incrementSize * index < cpu.ptr + incrementSize * SAMPLER_DESCRIPTOR_SIZE);
    return {base.ptr + incrementSize * index};
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetSamplerHandle(D3D12_GPU_DESCRIPTOR_HANDLE base, int index)
{
    const UINT incrementSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    /** Bounds check */
    const D3D12_GPU_DESCRIPTOR_HANDLE gpu = samplerHeap_[frameIdx_]->GetGPUDescriptorHandleForHeapStart();
    Assert(gpu.ptr <= base.ptr);
    Assert(base.ptr + incrementSize * index < gpu.ptr + incrementSize * SAMPLER_DESCRIPTOR_SIZE);
    return {base.ptr + incrementSize * index};
}

Descriptor DescriptorHeap::GetSamplerHandle(int index)
{
    Assert(0 <= index);
    Assert(index < SAMPLER_DESCRIPTOR_SIZE);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu = samplerHeap_[frameIdx_]->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE gpu = samplerHeap_[frameIdx_]->GetGPUDescriptorHandleForHeapStart();
    return {GetSamplerHandle(cpu, index), GetSamplerHandle(gpu, index)};
}

Descriptor DescriptorHeap::AllocateSamplerTable(int count)
{
    Assert(0 < count);
    Assert(samplerCount_ + count <= SAMPLER_DESCRIPTOR_SIZE);
    auto handle = GetSamplerHandle(samplerCount_);
    samplerCount_ += count;
    return handle;
}
