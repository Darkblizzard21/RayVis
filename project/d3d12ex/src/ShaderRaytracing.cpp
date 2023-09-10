/* Copyright (c) 2023 Pirmin Pfeifer */
#include "ShaderRaytracing.h"

RaytracingShader::RaytracingShader(ID3D12Device5*         device,
                                   ShaderCompiler*        compiler,
                                   RaytracingShaderData&& data,
                                   std::string_view       shaderSourceLocation)
    : IShader(device, compiler)
{
    OverrideData(std::move(data));
    // Create Constant buffer
    constantBuffer =
        std::make_unique<ConstantBuffer>(device, config::FramesInFlight, config::ConstantBufferSizeBytes * 2);

    // Compute shader
    auto computeShaderBlob = compiler->CompileFromFile(
        (std::wstring(shaderSourceLocation.begin(), shaderSourceLocation.end()) + L"/Raytracing.hlsl").data(), L"cs_6_5");

    const size_t           descriptorCount                  = 6;
    D3D12_DESCRIPTOR_RANGE descriptorRange[descriptorCount] = {};
    descriptorRange[0]                                      = ConstantBuffer::GetDescriptorRange(0, 0);
    descriptorRange[1]                                      = UploadBuffer::GetDescriptorRange(1, 0);
    descriptorRange[2]                                      = UploadBuffer::GetDescriptorRange(2, 1);
    descriptorRange[3]                                      = UploadBuffer::GetDescriptorRange(3, 2);

    const size_t outIdx                                           = descriptorCount - 2;
    // Output Buffer
    descriptorRange[outIdx].BaseShaderRegister                    = 0;
    descriptorRange[outIdx].NumDescriptors                        = 1;
    descriptorRange[outIdx].OffsetInDescriptorsFromTableStart     = outIdx;
    descriptorRange[outIdx].RangeType                             = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRange[outIdx].RegisterSpace                         = 0;
    // Depth Buffer
    descriptorRange[outIdx + 1].BaseShaderRegister                = 1;
    descriptorRange[outIdx + 1].NumDescriptors                    = 1;
    descriptorRange[outIdx + 1].OffsetInDescriptorsFromTableStart = outIdx + 1;
    descriptorRange[outIdx + 1].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRange[outIdx + 1].RegisterSpace                     = 0;

    D3D12_ROOT_PARAMETER descriptorTableLayout[3]                = {};
    descriptorTableLayout[0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    descriptorTableLayout[0].DescriptorTable.NumDescriptorRanges = descriptorCount;
    descriptorTableLayout[0].DescriptorTable.pDescriptorRanges   = descriptorRange;
    descriptorTableLayout[0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    auto normalsArrayDescriptorRange                             = UploadBuffer::GetDescriptorRange(0, 0, 16384);
    normalsArrayDescriptorRange.RegisterSpace                    = 1;
    descriptorTableLayout[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    descriptorTableLayout[1].DescriptorTable.NumDescriptorRanges = 1;
    descriptorTableLayout[1].DescriptorTable.pDescriptorRanges   = &normalsArrayDescriptorRange;
    descriptorTableLayout[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    auto indeciesArrayDescriptorRange                            = UploadBuffer::GetDescriptorRange(0, 0, 16384);
    indeciesArrayDescriptorRange.RegisterSpace                   = 2;
    descriptorTableLayout[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    descriptorTableLayout[2].DescriptorTable.NumDescriptorRanges = 1;
    descriptorTableLayout[2].DescriptorTable.pDescriptorRanges   = &indeciesArrayDescriptorRange;
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

void RaytracingShader::SetComputeRootDescriptorTable(ID3D12GraphicsCommandList6*     c,
                                                     DescriptorHeap*                 descHeap,
                                                     RaytracingShaderConstantBuffer* data)
{
    void* mappedPtr = constantBuffer->Map();
    std::memcpy(mappedPtr, data, sizeof(RaytracingShaderConstantBuffer));
    constantBuffer->Unmap();

    ID3D12DescriptorHeap* heaps[] = {descHeap->GetResourceHeap()};
    c->SetDescriptorHeaps(1, heaps);
    c->SetPipelineState(pipeline.Get());
    c->SetComputeRootSignature(rootSignature.Get());

    {  // Fill Descriptor Table Space 0
        const Descriptor descriptorTable = descHeap->AllocateDescriptorTable(6);

        // Slot 0
        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = constantBuffer->GetDesc();
        device->CreateConstantBufferView(&constantBufferViewDesc, descHeap->GetResourceView(descriptorTable.cpu, 0));


        // Slot 1
        D3D12_SHADER_RESOURCE_VIEW_DESC tlasViewDesc = resources.bvhBuilder->GetTlasViewDesc();
        device->CreateShaderResourceView(nullptr, &tlasViewDesc, descHeap->GetResourceView(descriptorTable.cpu, 1));

        // Slot 2
        resources.bvhBuilder->CreateInstanceColorSRV(device, descHeap->GetResourceView(descriptorTable.cpu, 2));

        // Slot 3 geometry primitive mapping
        resources.bvhBuilder->CreateInstanceGeometryMappingSRV(device,
                                                               descHeap->GetResourceView(descriptorTable.cpu, 3));

        // Slot 4 - output
        D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc = {};
        unorderedAccessViewDesc.Format                           = config::BackbufferFormat;
        unorderedAccessViewDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
        unorderedAccessViewDesc.Texture2D.MipSlice               = 0;
        unorderedAccessViewDesc.Texture2D.PlaneSlice             = 0;
        device->CreateUnorderedAccessView(resources.renderTargetUAV,
                                          nullptr,
                                          &unorderedAccessViewDesc,
                                          descHeap->GetResourceView(descriptorTable.cpu, 4));
        // Slot 5 - depth
        unorderedAccessViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
        device->CreateUnorderedAccessView(resources.rayDepthUAV,
                                          nullptr,
                                          &unorderedAccessViewDesc,
                                          descHeap->GetResourceView(descriptorTable.cpu, 5));

        // Set Table
        c->SetComputeRootDescriptorTable(0, descriptorTable.gpu);

        // Space 1 Slot 0 primitiveNormals
        const auto primitiveNormalsDescriptorTable =
            resources.bvhBuilder->CreatePrimitiveNormalsDescriptorArray(device, descHeap);
        c->SetComputeRootDescriptorTable(1, primitiveNormalsDescriptorTable.gpu);

        // Space 2 Slot 0 primitiveIndecies
        const auto primitiveIndeciesDescriptorTable =
            resources.bvhBuilder->CreatePrimitiveIndeciesDescriptorArray(device, descHeap);
        c->SetComputeRootDescriptorTable(2, primitiveIndeciesDescriptorTable.gpu);
    }
}

void RaytracingShader::Dispatch(ID3D12GraphicsCommandList6* c)
{
    const auto description = resources.renderTargetUAV->GetDesc();
    c->Dispatch((description.Width + 7) / 8, (description.Height + 7) / 8, 1);
}

void RaytracingShader::AdvanceFrame()
{
    constantBuffer->AdvanceFrame();
}

void RaytracingShader::OverrideData(RaytracingShaderData&& data)
{
    assert(data.IsValid());
    resources = std::move(data);
}

bool RaytracingShaderData::IsValid()
{
    return bvhBuilder != nullptr && renderTargetUAV != nullptr && rayDepthUAV != nullptr;
}
