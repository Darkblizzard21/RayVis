/* Copyright (c) 2023 Pirmin Pfeifer */
#include "ShaderVolumeRendering.h"

VolumeShader::VolumeShader(ID3D12Device5*     device,
                           ShaderCompiler*    compiler,
                           VolumeShaderData&& data,
                           std::string_view   shaderSourceLocation)
    : IShader(device, compiler)
{
    OverrideData(std ::move(data));
    // Create Constant buffer
    constantBuffer =
        std::make_unique<ConstantBuffer>(device, config::FramesInFlight, config::ConstantBufferSizeBytes * 2);

    // Compute shader
    auto computeShaderBlob = compiler->CompileFromFile(
        (std::wstring(shaderSourceLocation.begin(), shaderSourceLocation.end()) +
         L"/VolumeRendering.hlsl")
            .data(),
        L"cs_6_5");

    const size_t           descriptorCount                  = 4;
    D3D12_DESCRIPTOR_RANGE descriptorRange[descriptorCount] = {};
    descriptorRange[0]                                      = ConstantBuffer::GetDescriptorRange(0, 0);
    descriptorRange[1]                                      = TextureBuffer::GetDescriptorRange(1, 0);
    descriptorRange[2]                                      = UploadBuffer::GetDescriptorRange(2, 1);

    const size_t outIdx                                       = descriptorCount - 1;
    descriptorRange[outIdx].BaseShaderRegister                = 0;
    descriptorRange[outIdx].NumDescriptors                    = 1;
    descriptorRange[outIdx].OffsetInDescriptorsFromTableStart = outIdx;
    descriptorRange[outIdx].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRange[outIdx].RegisterSpace                     = 0;

    D3D12_ROOT_PARAMETER descriptorTableLayout[3]                = {};
    descriptorTableLayout[0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    descriptorTableLayout[0].DescriptorTable.NumDescriptorRanges = descriptorCount;
    descriptorTableLayout[0].DescriptorTable.pDescriptorRanges   = descriptorRange;
    descriptorTableLayout[0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    auto textureArrayDecriptorRange                              = TextureBuffer::GetDescriptorRange(0, 0, 512);
    textureArrayDecriptorRange.RegisterSpace                     = 1;
    descriptorTableLayout[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    descriptorTableLayout[1].DescriptorTable.NumDescriptorRanges = 1;
    descriptorTableLayout[1].DescriptorTable.pDescriptorRanges   = &textureArrayDecriptorRange;
    descriptorTableLayout[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE samplerDescriptorRange            = {};
    samplerDescriptorRange.BaseShaderRegister                = 0;
    samplerDescriptorRange.NumDescriptors                    = 1;
    samplerDescriptorRange.OffsetInDescriptorsFromTableStart = 0;
    samplerDescriptorRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    samplerDescriptorRange.RegisterSpace                     = 0;

    descriptorTableLayout[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    descriptorTableLayout[2].DescriptorTable.NumDescriptorRanges = 1;
    descriptorTableLayout[2].DescriptorTable.pDescriptorRanges   = &samplerDescriptorRange;
    descriptorTableLayout[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters             = 3;
    rootSignatureDesc.pParameters               = descriptorTableLayout;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(
        0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.CS.BytecodeLength                 = computeShaderBlob->GetBufferSize();
    pipelineDesc.CS.pShaderBytecode                = computeShaderBlob->GetBufferPointer();
    pipelineDesc.pRootSignature                    = rootSignature.Get();
    ThrowIfFailed(device->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&pipeline)));
}

void VolumeShader::SetComputeRootDescriptorTable(ID3D12GraphicsCommandList6* c,
                                                 DescriptorHeap*             descHeap,
                                                 VolumeShaderConstantBuffer* data)
{
    void* mappedPtr = constantBuffer->Map();
    std::memcpy(mappedPtr, data, sizeof(VolumeShaderConstantBuffer));
    constantBuffer->Unmap();

    ID3D12DescriptorHeap* heaps[] = {descHeap->GetResourceHeap(), descHeap->GetSamplerHeap()};
    c->SetDescriptorHeaps(2, heaps);
    c->SetPipelineState(pipeline.Get());
    c->SetComputeRootSignature(rootSignature.Get());

    {  // Fill Descriptor Table
        const Descriptor descriptorTable = descHeap->AllocateDescriptorTable(4);

        // Slot 0
        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = constantBuffer->GetDesc();
        device->CreateConstantBufferView(&constantBufferViewDesc, descHeap->GetResourceView(descriptorTable.cpu, 0));

        // Slot 1 - depth buffer
        D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
        shaderResourceViewDesc.Format                          = DXGI_FORMAT_R32_FLOAT;
        shaderResourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MipLevels             = 1;
        shaderResourceViewDesc.Texture2D.MostDetailedMip       = 0;
        shaderResourceViewDesc.Texture2D.PlaneSlice            = 0;
        shaderResourceViewDesc.Texture2D.ResourceMinLODClamp   = 0;
        shaderResourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(
            resources.rayDepthSVR, &shaderResourceViewDesc, descHeap->GetResourceView(descriptorTable.cpu, 1));

        // Slot 2
        resources.volumeProvider->CreateChunkMinMaxSRV(device, descHeap->GetResourceView(descriptorTable.cpu, 2));

        // Slot 3
        D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc = {};
        unorderedAccessViewDesc.Format                           = config::BackbufferFormat;
        unorderedAccessViewDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
        unorderedAccessViewDesc.Texture2D.MipSlice               = 0;
        unorderedAccessViewDesc.Texture2D.PlaneSlice             = 0;
        device->CreateUnorderedAccessView(resources.renderTargetUAV,
                                          nullptr,
                                          &unorderedAccessViewDesc,
                                          descHeap->GetResourceView(descriptorTable.cpu, 3));

        // Set Table
        c->SetComputeRootDescriptorTable(0, descriptorTable.gpu);

        const auto textureDesciptorTable = resources.volumeProvider->CreateTextureArrayDesciptorArray(device, descHeap);
        c->SetComputeRootDescriptorTable(1, textureDesciptorTable.gpu);

        const Descriptor samplerHandle = descHeap->AllocateSamplerTable();

        D3D12_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter             = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        D3D12_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.MipLODBias     = 0.0f;
        samplerDesc.MaxAnisotropy  = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] =
            samplerDesc.BorderColor[3]                          = 0;
        samplerDesc.MinLOD                                      = 0;
        samplerDesc.MaxLOD                                      = D3D12_FLOAT32_MAX;
        device->CreateSampler(&samplerDesc, samplerHandle.cpu);
        
        c->SetComputeRootDescriptorTable(2, samplerHandle.gpu);
    }
}

void VolumeShader::Dispatch(ID3D12GraphicsCommandList6* c)
{
    const auto description = resources.renderTargetUAV->GetDesc();
    c->Dispatch((description.Width + 7) / 8, (description.Height + 7) / 8, 1);
}

void VolumeShader::AdvanceFrame()
{
    constantBuffer->AdvanceFrame();
}

void VolumeShader::OverrideData(VolumeShaderData&& data)
{
    assert(data.IsValid());
    resources = std::move(data);
}

bool VolumeShaderData::IsValid()
{
    return volumeProvider != nullptr && renderTargetUAV != nullptr && rayDepthSVR != nullptr;
}
