/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <d3d12.h>
#include <wrl/client.h>  // for ComPtr

#include <tuple>

#include "d3d12ex/Config.h"

using Microsoft::WRL::ComPtr;

struct Descriptor {
    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

class DescriptorHeap {
public:
    DescriptorHeap(ComPtr<ID3D12Device5> device, int framesInFlight = config::FramesInFlight);
    void Reset();

    ID3D12DescriptorHeap* GetResourceHeap();
    ID3D12DescriptorHeap* GetSamplerHeap();

    D3D12_CPU_DESCRIPTOR_HANDLE GetResourceView(D3D12_CPU_DESCRIPTOR_HANDLE base, int index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetResourceView(D3D12_GPU_DESCRIPTOR_HANDLE base, int index);
    Descriptor                  GetResourceView(int index);
    Descriptor                  AllocateDescriptorTable(int count = 1);

    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE base, int index);
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(int index);
    D3D12_CPU_DESCRIPTOR_HANDLE AllocateRenderTargetView(int count = 1);

    D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerHandle(D3D12_CPU_DESCRIPTOR_HANDLE base, int index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHandle(D3D12_GPU_DESCRIPTOR_HANDLE base, int index);
    Descriptor                  GetSamplerHandle(int index);
    Descriptor                  AllocateSamplerTable(int count = 1);

private:
    ComPtr<ID3D12Device5>        device_;
    ComPtr<ID3D12DescriptorHeap> resourceViewHeap_[config::FramesInFlight];
    ComPtr<ID3D12DescriptorHeap> renderTargetViewHeap_[config::FramesInFlight];
    ComPtr<ID3D12DescriptorHeap> samplerHeap_[config::FramesInFlight];

    int resourceViewCount_     = 0;
    int renderTargetViewCount_ = 0;
    int samplerCount_          = 0;

    int       frameIdx_ = 0;
    const int framesInFlight;
};