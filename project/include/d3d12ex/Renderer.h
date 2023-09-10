/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <Buffers.h>
#include <BvhBuilder.h>
#include <Camera.h>
#include <Config.h>
#include <DescriptorHeap.h>
#include <RenderingModes.h>
#include <Scene.h>
#include <ShaderCompiler.h>
#include <ShaderRaytracing.h>
#include <ShaderVolumeRendering.h>
#include <TextureBuffer.h>
#include <UIHandler.h>
#include <configuration/Configuration.h>
#include <d3d12.h>
#include <d3d12ex/VolumeProvider.h>
#include <dxgi1_6.h>
#include <rayloader/Loader.h>
#include <rayvis-utils/Clock.h>
#include <wrl/client.h>  // for ComPtr

using Microsoft::WRL::ComPtr;

struct optionalRenderArgs {
    std::optional<std::string> source       = std::nullopt;
    std::optional<std::string> shaderSource = std::nullopt;
};

class Renderer : public IUiWindow, public core::IConfigurationComponent {
    static constexpr int FenceSignalled   = 1;
    static constexpr int FenceUnsignalled = 0;

    Clock                 clock;
    Camera                camera;
    rayloader::Loader     loader;
    std::vector<RayTrace> traces;

    HWND                    hwnd = nullptr;
    ComPtr<ID3D12Device5>   device;
    ComPtr<IDXGISwapChain3> swapchain;

    ComPtr<ID3D12CommandQueue>         queue;
    ComPtr<ID3D12CommandAllocator>     commandAllocator[config::FramesInFlight];
    ComPtr<ID3D12GraphicsCommandList6> commandList[config::FramesInFlight];

    ComPtr<ID3D12Fence> fence[config::FramesInFlight];
    HANDLE              fenceEvent[config::FramesInFlight];

    std::unique_ptr<DescriptorHeap> descriptorHeap = nullptr;
    UIHandler                       uiHandler;
    std::unique_ptr<ShaderCompiler> compiler = nullptr;

    ComPtr<ID3D12Resource> output;
    ComPtr<ID3D12Resource> rayDepthBuffer;

    VolumeProviderFootPrint         vpFootprint;
    std::unique_ptr<VolumeProvider> vProvider;
    // Scenes
    Scene                           scene;
    Scene                           rayMesh;
    Scene                           fallbackScene;

    std::unique_ptr<RaytracingShader> raytracingShader = nullptr;
    std::unique_ptr<VolumeShader>     volumeShader     = nullptr;

    bool       buildBvh = false;
    BvhBuilder bvhBuilder;

    ComPtr<ID3D12CommandQueue> copyQueue;
    // Allocated descriptors in the current frames heap
    int                        frameIndex = 0;
    int                        frameCount = 0;

public:
    void Init(HWND hwnd, const optionalRenderArgs& args);
    void CreateFrameBuffers();
    // InitShaders or updates them if already initialized
    void InitShaders();
    void LoadScene(bool dumpsourceChanged = true);
    void SetSceneFor(VisualizationMode mode);

    void Transition(ComPtr<ID3D12GraphicsCommandList6> c,
                    ComPtr<ID3D12Resource>             resource,
                    D3D12_RESOURCE_STATES              before,
                    D3D12_RESOURCE_STATES              after);

    void WaitForFrame(int idx);

    void WaitForGpuIdle();

    void RecordCommandList(ComPtr<ID3D12GraphicsCommandList6> c, ComPtr<ID3D12Resource2> backbuffer);

    void Render();

    void Resize();

    void AdvanceFrame();

    inline void Destroy()
    {
        WaitForGpuIdle();
        uiHandler.Shutdown();
    }

    inline HWND GetWindow()
    {
        return hwnd;
    }

    bool wantsToSaveConfig  = false;
    bool wantsToLoadConfig  = false;
    bool wantsToResetConfig = false;
    bool wantsToLoadSource  = false;

private:
    void RenderWindow() override;
    bool RememberingTreeNode(const std::string& label, bool defaultOpen = true, bool forceDefault = false);

    void                  SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration) override;
    core::IConfiguration& GetConfigurationImpl() override;

    std::unique_ptr<core::IConfiguration> config_;
    std::unique_ptr<core::IConfiguration> rayMeshConfig_;
};