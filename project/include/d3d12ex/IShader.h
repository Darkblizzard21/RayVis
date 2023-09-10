/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <d3d12ex/DescriptorHeap.h>
#include <d3d12ex/ShaderCompiler.h>
#include <wrl/client.h>  // for ComPtr
using Microsoft::WRL::ComPtr;

template <class DescriptorData>
class IShader {
public:
    IShader(ID3D12Device5* device, ShaderCompiler* compiler)
        : device(device),
          compiler(compiler){

          };

    virtual ~IShader() = default;

    virtual void SetComputeRootDescriptorTable(ID3D12GraphicsCommandList6* c,
                                               DescriptorHeap*             descHeap,
                                               DescriptorData*             data) = 0;
    virtual void Dispatch(ID3D12GraphicsCommandList6* c)             = 0;

    virtual void AdvanceFrame() = 0;

protected:
    ShaderCompiler* const compiler;
    ID3D12Device5* const  device;

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipeline;
};