/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Renderer.h"

#include <RayVisDataformat.h>
#include <Scene.h>
#include <SimpleRayMeshGenerator.h>
#include <imgui.h>
#include <rayvis-utils/BreakAssert.h>
#include <rayvis-utils/Color.h>
#include <rayvis-utils/FileSystemUtils.h>
#include <rayvis-utils/Keys.h>
#include <rayvis-utils/Mouse.h>

#include <spdlog/spdlog.h>

namespace {
    constexpr std::string formatBytes(size_t byteCount)
    {
        if (byteCount <= (1ULL << 14)) {
            return fmt::format("{:>6} bytes", fmt::format("{:6d}", byteCount));
        }
        if (byteCount <= (1ULL << 20)) {
            return fmt::format("{:>6} kB", fmt::format("{:.2f}", static_cast<double>(byteCount) / const_pow<3>(10)));
        }
        if (byteCount <= (1ULL << 30)) {
            return fmt::format("{:>6} MB", fmt::format("{:.2f}", static_cast<double>(byteCount) / const_pow<6>(10)));
        }
        if (byteCount <= (1ULL << 40)) {
            return fmt::format("{:>6} GB", fmt::format("{:.2f}", static_cast<double>(byteCount) / const_pow<9>(10)));
        }
        if (byteCount <= (1ULL << 50)) {
            return fmt::format("{:>6} TB", fmt::format("{:.2f}", static_cast<double>(byteCount) / const_pow<12>(10)));
        }
        if (byteCount <= (1ULL << 60)) {
            return fmt::format("{:>6} PB", fmt::format("{:.2f}", static_cast<double>(byteCount) / const_pow<15>(10)));
        }
        return fmt::format("{:>6} ExaByte", fmt::format("{:.2f}", static_cast<double>(byteCount) / const_pow<18>(10)));
    }
}  // namespace

void Renderer::Init(HWND hwnd, const optionalRenderArgs& args)
{
    if (args.source.has_value()) {
        config_->Set<std::string>("dumpSource", *args.source);
    }
    if (args.shaderSource.has_value()) {
        config_->Set<std::string>("shaders.source", *args.shaderSource);
    }

    this->hwnd = hwnd;
    RECT rect;
    ThrowIfFailed(GetClientRect(hwnd, &rect));
    config_->Set("windowWidth", static_cast<int>(rect.right - rect.left));
    config_->Set("windowHeight", static_cast<int>(rect.bottom - rect.top));

    if (config::EnableDebugLayer) {
        ComPtr<ID3D12Debug1> debug;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
        debug->EnableDebugLayer();
    }

    ComPtr<IDXGIFactory> baseFactory;
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&baseFactory)));

    ComPtr<IDXGIFactory6> factory;
    ThrowIfFailed(baseFactory->QueryInterface(IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter> adapter;
    ThrowIfFailed(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));

    {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(adapter.Get()->GetDesc(&desc))) {
            std::wstring ws(desc.Description);
            std::string  str(ws.begin(), ws.end());
            spdlog::info("Adapter: {} (SharedRAM {} MB; VRAM {} MB)",
                         str,
                         desc.SharedSystemMemory >> 20,
                         desc.DedicatedVideoMemory >> 20);
        };
    }

    ComPtr<ID3D12Device> baseDevice;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&baseDevice)));
    ThrowIfFailed(baseDevice->QueryInterface(IID_PPV_ARGS(&device)));

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 option5 = {};
        ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &option5, sizeof(option5)));
        if (option5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
            spdlog::info("Raytracing support found (Version {})", static_cast<int>(option5.RaytracingTier));
        } else {
            Assert(false);
        }
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
    queueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)));
    queue->SetName(L"RenderQueue");

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.BufferCount           = config::FramesInFlight;
    swapchainDesc.Width                 = config_->Get<int>("windowWidth");
    swapchainDesc.Height                = config_->Get<int>("windowHeight");
    swapchainDesc.Format                = config::BackbufferFormat;
    swapchainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.SampleDesc.Count      = 1;

    ComPtr<IDXGISwapChain1> baseSwapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(queue.Get(),
                                                  hwnd,
                                                  &swapchainDesc,
                                                  nullptr,  // No fullscreen -> One would set the refresh rate here
                                                  nullptr,
                                                  &baseSwapChain));
    ThrowIfFailed(baseSwapChain->QueryInterface(IID_PPV_ARGS(&swapchain)));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    for (size_t i = 0; i < config::FramesInFlight; i++) {
        ThrowIfFailed(device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&commandAllocator[i])));

        ComPtr<ID3D12GraphicsCommandList> baseCommandList;
        ThrowIfFailed(device->CreateCommandList(
            0, queueDesc.Type, commandAllocator[i].Get(), nullptr, IID_PPV_ARGS(&baseCommandList)));
        ThrowIfFailed(baseCommandList->Close());
        ThrowIfFailed(baseCommandList->QueryInterface(IID_PPV_ARGS(&commandList[i])));

        ThrowIfFailed(device->CreateFence(FenceSignalled, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i])));
        fenceEvent[i] = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

        if (!fenceEvent[i]) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    // Create Copy Allocator & command queue
    {
        D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
        copyQueueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        copyQueueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_COPY;
        ThrowIfFailed(device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&copyQueue)));
        copyQueue->SetName(L"CopyQueue");
    }

    descriptorHeap = std::make_unique<DescriptorHeap>(device);

    clock.Reset();
    bvhBuilder.Init(device);

    // Add fallback scene
    fallbackScene = Scene(device);

    LoadScene();

    CreateFrameBuffers();
    InitShaders();

    uiHandler.Init(device, hwnd);
    uiHandler.Register(this);
}

void Renderer::CreateFrameBuffers()
{
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask      = 0;
    heapProperties.VisibleNodeMask       = 0;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment           = 0;
    resourceDesc.Width               = config_->Get<int>("windowWidth");
    resourceDesc.Height              = config_->Get<int>("windowHeight");
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Format              = config::BackbufferFormat;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.SampleDesc.Quality  = 0;
    resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    // Create a copy of the backbuffer, as we cannot render into the backbuffer directly via unordered access
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &resourceDesc,
                                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                  nullptr,
                                                  IID_PPV_ARGS(&output)));

    // Create rayDepthBuffer
    resourceDesc.Format = DXGI_FORMAT_R32_FLOAT;
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &resourceDesc,
                                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                  nullptr,
                                                  IID_PPV_ARGS(&rayDepthBuffer)));
}

void Renderer::InitShaders()
{
    if (!compiler) {
        compiler = std::make_unique<ShaderCompiler>();
        compiler->Init();
    }

    {
        RaytracingShaderData data = {};
        data.bvhBuilder           = &bvhBuilder;
        data.renderTargetUAV      = output.Get();
        data.rayDepthUAV          = rayDepthBuffer.Get();
        if (raytracingShader) {
            raytracingShader->OverrideData(std::move(data));
        } else {
            raytracingShader = std::make_unique<RaytracingShader>(
                device.Get(), compiler.get(), std::move(data), config_->Get<std::string>("shaders.source"));
        }
    }

    {
        VolumeShaderData data = {};
        data.volumeProvider   = vProvider.get();
        data.rayDepthSVR      = rayDepthBuffer.Get();
        data.renderTargetUAV  = output.Get();
        if (volumeShader) {
            volumeShader->OverrideData(std::move(data));
        } else {
            volumeShader = std::make_unique<VolumeShader>(
                device.Get(), compiler.get(), std::move(data), config_->Get<std::string>("shaders.source"));
        }
    }
};

void Renderer::LoadScene(bool dumpsourceChanged)
{
    const auto begin = std::chrono::steady_clock::now();
    if (dumpsourceChanged) {
        spdlog::info("Starting loading scene from \"{}\"", config_->Get<std::string>("dumpSource"));

        traces = loader.Load(config_->Get<std::string>("dumpSource").c_str());
        assert(!traces.empty());
        if (traces.size() <= config_->Get<int32_t>("traceId")) {
            config_->Set<int32_t>("traceId", 0);
        }

        {  // Scene geometry
            scene              = Scene::LoadFrom(device, config_->Get<std::string>("dumpSource"));
            auto colorIterator = color::DefaultPalettIterator();
            scene.OverrideMeshColors([&itr = colorIterator](const Scene::Node* node) {
                Float3 result = *itr;
                ++itr;
                return result * 0.5f;
            });
        }

        const float currentSceneScale = config_->Get<float>("sceneScale.current");
        if (currentSceneScale != 1.0f) {
            for (auto& node : scene.rootNodes) {
                node->matrix = linalg::mul(node->matrix, linalg::scaling_matrix(Float3(currentSceneScale)));
            }
            scene.RecalculateMinMax();

            for (auto& trace : traces) {
                trace.ScaleBy(currentSceneScale);
            }
        }
    } else {
        spdlog::info("Changing trace to {}", config_->Get<int32_t>("traceId"));
    }

    RayTrace* trace = &traces[config_->Get<int32_t>("traceId")];

    // Create Volume Data
    vProvider = std::make_unique<VolumeProvider>(device, trace);

    vProvider->SetFilter(static_cast<RayFilter>(config_->Get<int>("volumeData.filter")));
    vProvider->SetChunkSize(config_->Get<int>("volumeData.chunkSize"));
    vProvider->SetCellSize(config_->Get<float>("volumeData.cellSize"));
    const float                maxT    = config_->Get<float>("volumeData.maxT");
    const std::optional<float> maxTopt = (0 < maxT) ? std::optional(maxT) : std::nullopt;
    vProvider->SetMaxT(maxTopt);

    vProvider->SetMinPointValue(config_->Get<float>("arrows.minVisualizationValue"));
    vProvider->SetMaxPointValue(config_->Get<float>("arrows.maxVisualizationValue"));
    vProvider->SetExcludePointsExeedingLimits(config_->Get<bool>("arrows.excludeExeeding"));
    vProvider->SetPintSampleSize(config_->Get<int>("arrows.sampleSize"));
    vProvider->SetPointScale(config_->Get<float>("arrows.minScale"), config_->Get<float>("arrows.maxScale"));
    vProvider->SetScaleByPointValue(config_->Get<bool>("arrows.scaleByValue"),
                                    config_->Get<bool>("arrows.scaleByValueInverse"));
    Ray::missTolerance = config_->Get<float>("volumeData.missTolerance");

    vpFootprint = vProvider->ComputeData(copyQueue);

    {  // Set up MeshLines

        SimpleRayMeshGenerator::LineDescription desc;
        desc.raytraces   = trace;
        desc.thickness   = config_->Get<float>("rayMesh.thickness");
        desc.rayStride   = config_->Get<int>("rayMesh.stride");
        desc.filter      = static_cast<RayFilter>(config_->Get<int>("rayMesh.filter"));
        const float maxT = config_->Get<float>("rayMesh.maxT");
        if ((0 < maxT)) {
            desc.maxT = maxT;
        }

        rayMesh = SimpleRayMeshGenerator::GenerateLines(device, desc);

        const auto color = config_->Get<glm::vec4>("rayMesh.color");
        rayMesh.OverrideMeshColors(Float3(color.x, color.y, color.z));
    }

    // Set Scene on bvh builder
    SetSceneFor(static_cast<VisualizationMode>(config_->Get<int>("visualizationMode")));

    const auto end     = std::chrono::steady_clock::now();
    const auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.f;
    if (dumpsourceChanged) {
        spdlog::info("Loaded Scene from \"{}\" in {}s", config_->Get<std::string>("dumpSource"), seconds);
    } else {
        spdlog::info("Changing trace to {} in {}s", config_->Get<int32_t>("traceId"), seconds);
    }
}

void Renderer::SetSceneFor(VisualizationMode mode)
{
    std::vector<Scene*> scenesPtrs  = {};
    const auto          shadingMode = static_cast<ShadingMode>(config_->Get<int>("shadingMode"));

    scenesPtrs.push_back(shadingMode == ShadingMode::DoNotRender ? &fallbackScene : &scene);

    if (mode == VisualizationMode::RayMesh) {
        scenesPtrs.push_back(&rayMesh);
    }
    if (mode == VisualizationMode::ArrowPoints) {
        scenesPtrs.push_back(vProvider->GetPointCloud());
    }

    // Rebuild BVH
    WaitForGpuIdle();
    bvhBuilder.SetGeometry(scenesPtrs);
    buildBvh = false;
}

void Renderer::Transition(ComPtr<ID3D12GraphicsCommandList6> c,
                          ComPtr<ID3D12Resource>             resource,
                          D3D12_RESOURCE_STATES              before,
                          D3D12_RESOURCE_STATES              after)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = resource.Get();
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter  = after;
    barrier.Transition.Subresource = 0;
    c->ResourceBarrier(1, &barrier);
}

void Renderer::WaitForFrame(int idx)
{
    // Wait until the GPU releases access to this frames resources
    if (fence[idx]->GetCompletedValue() == FenceUnsignalled) {
        ThrowIfFailed(fence[idx]->SetEventOnCompletion(FenceSignalled, fenceEvent[idx]));
        ::WaitForSingleObject(fenceEvent[idx], INFINITE);
    }
    // Reset fence for this frame
    fence[idx]->Signal(FenceUnsignalled);
}

void Renderer::WaitForGpuIdle()
{
    for (int i = 0; i < config::FramesInFlight; ++i) {
        WaitForFrame(i);
    }

    for (size_t i = 0; i < config::FramesInFlight; i++) {
        fence[i]->Signal(FenceSignalled);
    }
}

void Renderer::RecordCommandList(ComPtr<ID3D12GraphicsCommandList6> c, ComPtr<ID3D12Resource2> backbuffer)
{
    if (IsIconic(hwnd)) {
        return;
    }
    if (!buildBvh) {
        bvhBuilder.BuildBvh(c);
        buildBvh = true;
    }
    vProvider->TranistionToReadable(c);

    const auto width  = config_->Get<int>("windowWidth");
    const auto height = config_->Get<int>("windowHeight");
    Assert(rayDepthBuffer.Get()->GetDesc().Height == height);
    Assert(rayDepthBuffer.Get()->GetDesc().Width == width);
    Assert(output.Get()->GetDesc().Height == height);
    Assert(output.Get()->GetDesc().Width == width);
    Assert(backbuffer.Get()->GetDesc().Height == height);
    Assert(backbuffer.Get()->GetDesc().Width == width);

    RaytracingShaderConstantBuffer rcb = {};
    rcb.camera.toWorld                 = camera.CalcToWorld();
    rcb.camera.tMin                    = camera.GetTMin();
    rcb.camera.tMax                    = camera.GetTMax();
    rcb.camera.fov                     = camera.GetFoVRad();

    rcb.elapsed           = clock.ElapsedTime();
    rcb.viewportWidth     = width;
    rcb.viewportHeight    = height;
    rcb.shaderMode        = config_->Get<int>("shadingMode");
    rcb.visualizationMode = config_->Get<int>("visualizationMode");
    rcb.lightDir          = to3(config_->Get<glm::vec3>("lightDirection"));

    raytracingShader->SetComputeRootDescriptorTable(c.Get(), descriptorHeap.get(), &rcb);
    raytracingShader->Dispatch(c.Get());

    if (config_->Get<int>("visualizationMode") == to_integral(VisualizationMode::VolumeTrace)) {
        Transition(
            c, rayDepthBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource          = output.Get();
        c->ResourceBarrier(1, &barrier);

        VolumeShaderConstantBuffer vcb = {};
        vcb.camera.toWorld             = rcb.camera.toWorld;
        vcb.camera.tMin                = rcb.camera.tMin;
        vcb.camera.tMax                = rcb.camera.tMax;
        vcb.camera.fov                 = rcb.camera.fov;
        vcb.baseTransparency           = config_->Get<float>("volumeShader.baseTransparency");
        vcb.viewportWidth              = width;
        vcb.viewportHeight             = height;
        vcb.samplesPerCell             = config_->Get<int>("volumeShader.samplesPerCell");
        vcb.minValue                   = config_->Get<float>("volumeShader.minValue");
        vcb.maxValue                   = config_->Get<float>("volumeShader.maxValue");
        vcb.b_excludeExceeding         = config_->Get<bool>("volumeShader.excludeExeeding");

        vcb.volume.min        = vpFootprint.minBounds;
        vcb.volume.max        = vpFootprint.maxBounds;
        vcb.volume.chunkCount = vpFootprint.chunkCount;
        vcb.volume.cellSize   = vpFootprint.cellSize;
        vcb.volume.chunkSize  = vpFootprint.chunkSize;

        vcb.elapsed = clock.ElapsedTime();

        vcb.b_enableChunkDebugging = config_->Get<bool>("debug.enableChunks");
        vcb.maxDebugCunkCount =
            std::min(config_->Get<int>("debug.chunkCount"), static_cast<int>(vpFootprint.chunkCount));

        volumeShader->SetComputeRootDescriptorTable(c.Get(), descriptorHeap.get(), &vcb);
        volumeShader->Dispatch(c.Get());

        Transition(
            c, rayDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    Transition(c, output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(c, backbuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
    c->CopyResource(backbuffer.Get(), output.Get());
    Transition(c, output, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Transition(c, backbuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Add Ui
    uiHandler.Render(c.Get(), backbuffer.Get());
    Transition(c, backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void Renderer::Render()
{
    WaitForFrame(frameIndex);

    descriptorHeap->Reset();
    // Reset allocator and command lists to record new commands for this frame
    commandAllocator[frameIndex]->Reset();

    ComPtr<ID3D12GraphicsCommandList6> currentCommandList = commandList[frameIndex];
    ThrowIfFailed(currentCommandList->Reset(commandAllocator[frameIndex].Get(), nullptr));
    ComPtr<ID3D12Resource2> backbuffer;
    ThrowIfFailed(swapchain->GetBuffer(swapchain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&backbuffer)));

    RecordCommandList(currentCommandList, backbuffer);

    ThrowIfFailed(currentCommandList->Close());

    // Submit and present
    ID3D12CommandList* ppCommandLists[] = {currentCommandList.Get()};
    queue->ExecuteCommandLists(1, ppCommandLists);

    swapchain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);

    // The swapchain presents on this queue --> Signal AFTER the present.
    queue->Signal(fence[frameIndex].Get(), FenceSignalled);

    frameIndex = (frameIndex + 1) % config::FramesInFlight;
}

void Renderer::Resize()
{
    RECT rect;
    ThrowIfFailed(GetClientRect(hwnd, &rect));
    int newWidth  = rect.right - rect.left;
    int newHeight = rect.bottom - rect.top;
    if (newWidth == config_->Get<int>("windowWidth") && newHeight == config_->Get<int>("windowHeight")) {
        return;
    }
    if (newWidth <= 0 || newHeight <= 0) {
        spdlog::info("Resize was called with zero value (W:{}|H:{}).", newWidth, newHeight);
        return;
    }

    config_->Set("windowWidth", newWidth);
    config_->Set("windowHeight", newHeight);
    WaitForGpuIdle();
    ThrowIfFailed(
        swapchain->ResizeBuffers(config::FramesInFlight, newWidth, newHeight, config::BackbufferFormat, NULL));

    CreateFrameBuffers();
    InitShaders();
}

void Renderer::AdvanceFrame()
{
    frameCount += 1;
    frameIndex                 = frameCount % config::FramesInFlight;
    const float  maxFPSInverse = 1.0 / config_->Get<float>("maxFPS");
    const double minDelta      = config_->Get<bool>("useMaxFPS") ? maxFPSInverse : 0.0;
    clock.Advance(minDelta);
    raytracingShader->AdvanceFrame();
    volumeShader->AdvanceFrame();
    // Advance Camera
    const auto keys = KeyRegistry::GetGobalInstance();

    if (!uiHandler.IsKeyboardCaptured()) {
        if (keys->Down(Key::Tab)) {
            const auto mode      = increment(static_cast<ShadingMode>(config_->Get<int>("shadingMode")));
            const int  modeAsInt = to_integral(mode);
            config_->SetValue("shadingMode", modeAsInt);
        }
        if (keys->Down(Key::Key_0)) {
            config_->Set<int>("visualizationMode", to_integral(VisualizationMode::None));
        }
        if (keys->Down(Key::Key_1)) {
            config_->Set<int>("visualizationMode", to_integral(VisualizationMode::RayMesh));
        }
        if (keys->Down(Key::Key_2)) {
            config_->Set<int>("visualizationMode", to_integral(VisualizationMode::ArrowPoints));
        }
        if (keys->Down(Key::Key_3)) {
            config_->Set<int>("visualizationMode", to_integral(VisualizationMode::VolumeTrace));
        }

        camera.MoveUp(clock.DeltaTime() * keys->PressedAxisSign(Key::Key_Q, Key::Key_E));
        camera.MoveRight(clock.DeltaTime() * keys->PressedAxisSign(Key::Key_A, Key::Key_D));
        camera.MoveForward(clock.DeltaTime() * keys->PressedAxisSign(Key::Key_S, Key::Key_W));
    }
    if (!uiHandler.IsMouseCaptured()) {
        const auto mouse      = Mouse::GetGobalInstance();
        const auto mouseDelta = mouse->DeltaPosition();
        if (mouse->Pressed(MouseButtons::Left) && (mouseDelta != I2ZERO)) {
            const auto degX = static_cast<float>(mouseDelta.x) / config_->Get<int>("windowWidth") * 360;
            const auto degY = static_cast<float>(mouseDelta.y) / config_->Get<int>("windowHeight") * 360;
            camera.LookRight(degX);
            camera.LookUp(degY);
        }
    }

    if (config_->IsEntryModified("dumpSource") || config_->IsEntryModified("traceId") ||
        config_->IsEntryModified("sceneScale.current"))
    {
        config_->Set<int>("trace_Id", std::clamp(config_->Get<int>("traceId"), 0, static_cast<int>(traces.size() - 1)));
        WaitForGpuIdle();
        LoadScene(config_->IsEntryModified("dumpSource"));
        InitShaders();
    } else {
        bool reInitScene = false;
        if (config_->Get<bool>("recalculateVolume")) {
            WaitForGpuIdle();
            vpFootprint = vProvider->ComputeData(copyQueue);
            config_->Set("recalculateVolume", false);
            reInitScene = true;
        }

        bool newRayMesh = false;
        if (config_->IsAnyEntryModified({"rayMesh.maxT", "rayMesh.thickness", "rayMesh.stride", "rayMesh.filter"})) {
            WaitForGpuIdle();
            SimpleRayMeshGenerator::LineDescription desc;
            desc.raytraces = &traces[config_->Get<int32_t>("traceId")];
            desc.thickness = config_->Get<float>("rayMesh.thickness");
            desc.rayStride = config_->Get<int>("rayMesh.stride");
            desc.filter    = static_cast<RayFilter>(config_->Get<int>("rayMesh.filter"));

            const float maxT = config_->Get<float>("rayMesh.maxT");
            if ((0 < maxT)) {
                desc.maxT = maxT;
            }

            rayMesh    = SimpleRayMeshGenerator::GenerateLines(device, desc);
            newRayMesh = true;
        }
        if (newRayMesh || config_->IsEntryModified("rayMesh.color")) {
            const auto color = config_->Get<glm::vec4>("rayMesh.color");
            rayMesh.OverrideMeshColors(Float3(color.x, color.y, color.z));
            reInitScene = true;
        }

        if (config_->IsEntryModified("visualizationMode") || config_->IsEntryModified("shadingMode")) {
            reInitScene = true;
        }
        if (reInitScene) {
            SetSceneFor(static_cast<VisualizationMode>(config_->Get<int>("visualizationMode")));
        }
    }
    if (config_->IsEntryModified("volumeData.missTolerance")) {
        Ray::missTolerance = config_->Get<float>("volumeData.missTolerance");
        vProvider->MarkDirty();
    }
}

void Renderer::RenderWindow()
{
    if (!ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load (CLI)")) {
                wantsToLoadSource = true;
            }
            if (ImGui::MenuItem("Save As (CLI)", NULL, false, config::EnableFileSave)) {
                HWND consoleWindow = GetConsoleWindow();
                SetForegroundWindow(consoleWindow);

                spdlog::info("PLEASE ENTER A FILE PATH TO SAVE TO:");
                std::string savePath;
                std::getline(std::cin, savePath);
                if (std::filesystem::is_directory(savePath)) {
                    spdlog::info("PLEASE ENTER A NAME FOR THE FILE:");
                    std::string fileName;
                    std::getline(std::cin, fileName);
                    savePath = std::filesystem::path(savePath).concat(fileName).string();
                }
                bool sucess = false;
                try {
                    sucess = dataformat::SaveTo(savePath, traces, &scene);
                } catch (rdf::ApiException& rdfExcep) {
                    spdlog::error("Saving Failed with rdf::ApiException: {}", rdfExcep.what());
                } catch (std::runtime_error& e) {
                    spdlog::error("Saving Failed with runtime_error: {}", e.what());
                }
                if (sucess) {
                    spdlog::info("Saved Data successfully in folder \"{}\"",
                                 std::filesystem::path(savePath).parent_path().string());
                } else {
                    spdlog::warn("Saving Data was not successful");
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Config")) {
            if (ImGui::MenuItem("Load (CLI)")) {
                HWND consoleWindow = GetConsoleWindow();
                SetForegroundWindow(consoleWindow);
                wantsToLoadConfig = true;
            }
            if (ImGui::MenuItem("Save")) {
                wantsToSaveConfig = true;
            }
            if (ImGui::MenuItem("Reset")) {
                wantsToResetConfig = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (RememberingTreeNode("SourceData")) {
        ImGui::Text(fmt::format("DumpSource: {}", config_->Get<std::string>("dumpSource")).c_str());

        const int currentTraceId = config_->Get<int>("traceId");
        if (ImGui::BeginCombo("TraceId", std::to_string(currentTraceId).c_str())) {
            for (int n = 0; n < traces.size(); n++) {
                bool is_selected = (currentTraceId == n);
                if (ImGui::Selectable(std::to_string(n).c_str(), is_selected)) {
                    config_->Set<int>("traceId", n);
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        const float currentScale = config_->Get<float>("sceneScale.current");
        ImGui::Text("CurrentSceneScale: %.3f", currentScale);

        float uiScale = config_->Get<float>("sceneScale.ui");
        auto  params =
            std::get<core::ConfigurationEntry::FloatParameters>(config_->GetEntry("sceneScale.ui").GetParameters());
        ImGui::PushItemWidth(120);
        if (ImGui::DragFloat(
                "SceneScale ", &uiScale, 0.2f, params.min, params.max, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            config_->Set<float>("sceneScale.ui", uiScale);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Apply") && (uiScale != currentScale)) {
            const float relativeScale = uiScale / currentScale;

            for (auto& node : scene.rootNodes) {
                node->matrix = linalg::mul(node->matrix, linalg::scaling_matrix(Float3(relativeScale)));
            }
            scene.RecalculateMinMax();

            for (auto& trace : traces) {
                trace.ScaleBy(relativeScale);
            }

            config_->Set<float>("sceneScale.current", uiScale);
        }

        ImGui::TreePop();
    }

    if (RememberingTreeNode("Volume Data", false)) {
        if (vProvider->IsDirty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "(dirty)");
        }
        if (RememberingTreeNode("Statistics", false)) {
            ImGui::Text(fmt::format("Max VoxelRayCount: {:>6d}", vProvider->MaxRays()).c_str());
            ImGui::Text(fmt::format("Chunk Count:       {:>6d}", vpFootprint.chunkCount).c_str());
            const auto chunkSizeCpu =
                cube(vpFootprint.chunkSize) * (sizeof(VolumetricSampler::rdType) + sizeof(Float3)) +
                sizeof(VolumetricSampler::ChunkData);
            ImGui::Text(fmt::format("Chunk Size CPU:    {}", formatBytes(chunkSizeCpu)).c_str());
            const auto chunkSizeGpu = cube(vpFootprint.chunkSize + 2) * sizeof(VolumetricSampler::rdType);
            ImGui::Text(fmt::format("Chunk Size GPU:    {}", formatBytes(chunkSizeGpu)).c_str());
            ImGui::Text(
                fmt::format("Volume Size CPU:   {}", formatBytes(chunkSizeCpu * vpFootprint.chunkCount)).c_str());
            ImGui::Text(
                fmt::format("Volume Size GPU:   {}", formatBytes(chunkSizeGpu * vpFootprint.chunkCount)).c_str());

            ImGui::Spacing();
            const auto threads = std::max(std::thread::hardware_concurrency(), 1U);
            const auto recalcCpuChunkSize =
                cube(vProvider->ChunkSize()) * (sizeof(VolumetricSampler::rdType) + sizeof(Float3) + sizeof(Double3)) +
                sizeof(VolumetricSampler::ChunkData);
            ImGui::Text("Recalculation (estimates):");
            ImGui::Text(fmt::format("Thread count:      {:>6d}", threads).c_str());
            ImGui::Text(fmt::format("Chunk Memory Size: {}", formatBytes(recalcCpuChunkSize)).c_str());
            ImGui::Text(fmt::format("Total Memory Size: {}", formatBytes(recalcCpuChunkSize * threads)).c_str());

            ImGui::TreePop();
        }
        {  // volumeData.filter
            const RayFilter currentFilter = static_cast<RayFilter>(config_->Get<int>("volumeData.filter"));
            auto            params        = std::get<core::ConfigurationEntry::IntParameters>(
                config_->GetEntry("volumeData.filter").GetParameters());
            if (ImGui::BeginCombo("Filter", display_name(currentFilter).c_str())) {
                for (int n = params.min; n <= params.max; n++) {
                    const auto filter      = static_cast<RayFilter>(n);
                    bool       is_selected = currentFilter == filter;
                    if (ImGui::Selectable(display_name(filter).c_str(), is_selected)) {
                        config_->Set<int>("volumeData.filter", n);
                        vProvider->SetFilter(filter);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        {  // volumeData.missTolerance
            float tolerance = config_->Get<float>("volumeData.missTolerance");
            auto  params    = std::get<core::ConfigurationEntry::FloatParameters>(
                config_->GetEntry("volumeData.missTolerance").GetParameters());

            if (ImGui::DragFloat("Miss detection tolerance",
                                 &tolerance,
                                 0.1f,
                                 params.min,
                                 params.max,
                                 "%.3f",
                                 ImGuiSliderFlags_AlwaysClamp))
            {
                config_->SetValue("volumeData.missTolerance", tolerance);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text(
                    "Ray miss detection checks if tHit of the ray data is greater or equal to tMax of the data.");
                ImGui::Text("In Some data sets the \"missing\" rays have hit values slightly smaller than tMax.");
                ImGui::Text("The miss detection tolerance offsets the detection barrier.");
                ImGui::EndTooltip();
            }
        }

        {  // volumeData.chunkSize
            int        chunkSize = config_->Get<int>("volumeData.chunkSize");
            const auto params    = std::get<core::ConfigurationEntry::IntParameters>(
                config_->GetEntry("volumeData.chunkSize").GetParameters());

            if (ImGui::DragInt(
                    "ChunkSize", &chunkSize, 2.f, params.min, params.max, "%d", ImGuiSliderFlags_AlwaysClamp))
            {
                config_->Set<int>("volumeData.chunkSize", chunkSize);
                vProvider->SetChunkSize(chunkSize);
                config_->Set<int>("arrows.sampleSize", vProvider->PointSampleSize());
            }
        }

        {  // volumeData.cellSize
            float cellSize = config_->Get<float>("volumeData.cellSize");
            auto  params   = std::get<core::ConfigurationEntry::FloatParameters>(
                config_->GetEntry("volumeData.cellSize").GetParameters());

            if (ImGui::DragFloat(
                    "VoxelSize", &cellSize, 1.f, params.min, params.max, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            {
                config_->SetValue("volumeData.cellSize", cellSize);
                vProvider->SetCellSize(cellSize);
                config_->SetValue("arrows.maxScale", vProvider->MaxPointScale());
            }
        }

        float maxT    = config_->Get<float>("volumeData.maxT");
        bool  useMaxT = 0 < maxT;
        if (ImGui::Checkbox("Limit maxT", &useMaxT)) {  // MaxT
            const float newMaxT = -maxT;
            config_->Set("volumeData.maxT", newMaxT);
            const std::optional<float> maxTopt = (0 < newMaxT) ? std::optional(newMaxT) : std::nullopt;
            vProvider->SetMaxT(maxTopt);
        } else if (useMaxT) {
            auto params = std::get<core::ConfigurationEntry::FloatParameters>(
                config_->GetEntry("volumeData.maxT").GetParameters());
            ImGui::SameLine();
            if (ImGui::InputFloat("MaxT", &maxT, 200.f, 1600.f, "%.0f")) {
                maxT = std::abs(maxT);
                if (maxT == 0) {
                    maxT = 200.f;
                }
                config_->Set("volumeData.maxT", maxT);
                vProvider->SetMaxT(maxT);
            }
        }

        // Recalculate Button
        if (ImGui::Button("Recalculate")) {
            config_->SetValue("recalculateVolume", true);
        }
        ImGui::TreePop();
    } else {
        if (vProvider->IsDirty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "(dirty)");
        }
    }

    if (RememberingTreeNode("Visualization")) {
        bool                                                           refreshOpen = false;
        std::array<bool, to_integral(VisualizationMode::_MODE_COUNT_)> open;

        // Visualization modes
        const VisualizationMode currentVisualizationMode =
            static_cast<VisualizationMode>(config_->Get<int>("visualizationMode"));
        if (ImGui::BeginCombo("VisualizationMode (VM)", display_name(currentVisualizationMode).c_str())) {
            for (int n = 0; n < to_integral(VisualizationMode::_MODE_COUNT_); n++) {
                bool is_selected = (to_integral(currentVisualizationMode) == n);
                open[n]          = false;
                if (ImGui::Selectable(display_name(static_cast<VisualizationMode>(n)).c_str(), is_selected)) {
                    config_->Set<int>("visualizationMode", n);
                    open[n]     = true;
                    refreshOpen = true;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (RememberingTreeNode("VM-RayMesh", open[to_integral(VisualizationMode::RayMesh)], refreshOpen)) {
            {  // volumeData.filter
                const RayFilter currentFilter = static_cast<RayFilter>(config_->Get<int>("rayMesh.filter"));
                auto            params        = std::get<core::ConfigurationEntry::IntParameters>(
                    config_->GetEntry("rayMesh.filter").GetParameters());
                if (ImGui::BeginCombo("Filter", display_name(currentFilter).c_str())) {
                    for (int n = params.min; n <= params.max; n++) {
                        const auto filter      = static_cast<RayFilter>(n);
                        bool       is_selected = currentFilter == filter;
                        if (ImGui::Selectable(display_name(filter).c_str(), is_selected)) {
                            config_->Set<int>("rayMesh.filter", n);
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Sync to volume filter")) {
                    config_->Set<int>("rayMesh.filter", config_->Get<int>("volumeData.filter"));
                }
            }
            if (RememberingTreeNode("Color", false)) {
                auto color = config_->Get<glm::vec4>("rayMesh.color");
                if (ImGui::ColorPicker3("##Color", &color.x)) {
                    config_->Set("rayMesh.color", color);
                };
                ImGui::TreePop();
            }

            // rayMesh.stride
            int  stride = config_->Get<int>("rayMesh.stride");
            auto params =
                std::get<core::ConfigurationEntry::IntParameters>(config_->GetEntry("rayMesh.stride").GetParameters());

            if (ImGui::SliderInt("Stride", &stride, params.min, params.max, "%d", ImGuiSliderFlags_AlwaysClamp)) {
                config_->SetValue("rayMesh.stride", stride);
            }

            {  // rayMesh.thickness
                float thickness = config_->Get<float>("rayMesh.thickness");
                auto  params    = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("rayMesh.thickness").GetParameters());

                if (ImGui::SliderFloat(
                        "Thickness", &thickness, params.min, params.max, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                {
                    config_->SetValue("rayMesh.thickness", thickness);
                }
            }

            {  // maxT
                float maxT    = config_->Get<float>("rayMesh.maxT");
                bool  useMaxT = 0 < maxT;
                if (ImGui::Checkbox("Limit maxT##rayMesh", &useMaxT)) {  // MaxT
                    const float newMaxT = -maxT;
                    config_->Set("rayMesh.maxT", newMaxT);
                    const std::optional<float> maxTopt = (0 < newMaxT) ? std::optional(newMaxT) : std::nullopt;
                    vProvider->SetMaxT(maxTopt);
                } else if (useMaxT) {
                    auto params = std::get<core::ConfigurationEntry::FloatParameters>(
                        config_->GetEntry("rayMesh.maxT").GetParameters());
                    ImGui::SameLine();
                    if (ImGui::InputFloat("MaxT##rayMesh", &maxT, 200.f, 1600.f, "%.0f")) {
                        maxT = std::abs(maxT);
                        if (maxT == 0) {
                            maxT = 200.f;
                        }
                        config_->Set("rayMesh.maxT", maxT);
                        vProvider->SetMaxT(maxT);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Sync to volume maxT")) {
                    config_->Set<float>("rayMesh.maxT", config_->Get<float>("volumeData.maxT"));
                }
            }

            const auto rayCount     = traces[config_->Get<int32_t>("traceId")].rays.size();
            const auto rayMeshCount = rayCount / stride;
            ImGui::Text(fmt::format("Ray Count:        {:>12}", rayCount).c_str());
            ImGui::Text(fmt::format("Ray Mesh Count:   {:>12}", rayMeshCount).c_str());
            ImGui::Text(fmt::format("Ray Mesh Percent: {:>12.3}%%", ((rayMeshCount * 1.0) / rayCount) * 100.0).c_str());

            ImGui::TreePop();
        }

        if (RememberingTreeNode("VM-VectorField", open[to_integral(VisualizationMode::ArrowPoints)], refreshOpen)) {
            if (vProvider->IsPointCloudDirty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "(dirty)");
            }
            {  // arrows.maxVisualizationValueint
                ImGui::PushItemWidth(150.f);
                float      min       = config_->Get<float>("arrows.minVisualizationValue");
                const auto minParams = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("arrows.minVisualizationValue").GetParameters());
                float      max       = config_->Get<float>("arrows.maxVisualizationValue");
                const auto maxParams = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("arrows.maxVisualizationValue").GetParameters());

                const float maxRays = vProvider->MaxRays();
                bool        tooltip = false;

                bool minChanged = false;
                minChanged |= ImGui::SliderFloat("Min Density",
                                                 &min,
                                                 minParams.min,
                                                 std::min(minParams.max, maxRays),
                                                 minParams.format.c_str(),
                                                 ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                if (ImGui::IsItemHovered()) {
                    tooltip = true;
                }
                ImGui::SameLine();
                minChanged |= ImGui::DragFloat("##Min Density",
                                               &min,
                                               1.f,
                                               minParams.min,
                                               std::min(minParams.max, maxRays),
                                               minParams.format.c_str(),
                                               ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                if (minChanged) {
                    min = std::clamp(min, minParams.min, minParams.max);
                    max = std::max(min + maxParams.min, max);
                }

                bool maxChanged = false;
                maxChanged |= ImGui::SliderFloat("Max Density",
                                                 &max,
                                                 maxParams.min,
                                                 std::min(maxParams.max, maxRays),
                                                 maxParams.format.c_str(),
                                                 ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                if (ImGui::IsItemHovered()) {
                    tooltip = true;
                }
                ImGui::SameLine();
                maxChanged |= ImGui::DragFloat("##Max Density",
                                               &max,
                                               1.f,
                                               maxParams.min,
                                               std::min(maxParams.max, maxRays),
                                               maxParams.format.c_str(),
                                               ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                if (maxChanged) {
                    max = std::clamp(max, maxParams.min, maxParams.max);
                    min = std::min(max - maxParams.min, min);
                }

                if (tooltip) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Min and Max Density");
                    ImGui::Text(fmt::format("\tFor each vector a cube of {} cells gets sampled and averaged.",
                                            config_->Get<int>("arrows.sampleSize"))
                                    .c_str());
                    ImGui::Text(
                        fmt::format("\tEach volume cell holds a value between 0 and {} rays).", vProvider->MaxRays())
                            .c_str());
                    ImGui::EndTooltip();
                }
                if (minChanged || maxChanged) {
                    min = std::clamp(min, minParams.min, minParams.max);
                    max = std::clamp(max, maxParams.min, maxParams.max);
                    config_->Set<float>("arrows.minVisualizationValue", min);
                    config_->Set<float>("arrows.maxVisualizationValue", max);
                    vProvider->SetMinPointValue(min);
                    vProvider->SetMaxPointValue(max);
                }
                ImGui::PopItemWidth();
            }

            {  // arrows.excludeExeeding
                bool excludeExeeding = config_->Get<bool>("arrows.excludeExeeding");
                if (ImGui::Checkbox("Exclude Points outside limits", &excludeExeeding)) {
                    config_->Set<bool>("arrows.excludeExeeding", excludeExeeding);
                    vProvider->SetExcludePointsExeedingLimits(excludeExeeding);
                }
            }

            {  // arrows.sampleSize
                const int sampleSize = config_->Get<int>("arrows.sampleSize");
                if (ImGui::BeginCombo("SampleSize", std::to_string(sampleSize).c_str())) {
                    for (size_t i = 1; i < vProvider->ChunkSize() / 2; i++) {
                        if ((vProvider->ChunkSize() % i) != 0) {
                            continue;
                        }
                        bool is_selected = sampleSize == i;
                        if (ImGui::Selectable(std::to_string(i).c_str(), is_selected)) {
                            config_->Set<int>("arrows.sampleSize", i);
                            vProvider->SetPintSampleSize(i);
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            if (RememberingTreeNode("Scale")) {
                {  // arrows.scaleByValue
                    bool scaleByValue = config_->Get<bool>("arrows.scaleByValue");
                    bool inverse      = config_->Get<bool>("arrows.scaleByValueInverse");
                    bool modified     = false;
                    if (ImGui::Checkbox("ScaleByValue", &scaleByValue)) {
                        modified = true;
                    }
                    if (scaleByValue) {
                        ImGui::SameLine();
                        if (ImGui::Checkbox("Inverse", &inverse)) {
                            modified = true;
                        }
                    }
                    if (modified) {
                        config_->Set<bool>("arrows.scaleByValue", scaleByValue);
                        config_->Set<bool>("arrows.scaleByValueInverse", inverse);
                        vProvider->SetScaleByPointValue(scaleByValue, inverse);
                    }
                }

                // arrows.scale
                float min       = config_->Get<float>("arrows.minScale");
                auto  minParams = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("arrows.minScale").GetParameters());

                float max       = config_->Get<float>("arrows.maxScale");
                auto  maxParams = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("arrows.maxScale").GetParameters());

                bool modified = false;

                if (ImGui::SliderFloat("Min Scale",
                                       &min,
                                       minParams.min,
                                       minParams.max,
                                       "%.3f",
                                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                {
                    min      = std::clamp(min, minParams.min, minParams.max);
                    max      = std::max(min + maxParams.min, max);
                    modified = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Set to minD/maxD")) {
                    float minD = config_->Get<float>("arrows.minVisualizationValue");
                    float maxD = config_->Get<float>("arrows.maxVisualizationValue");
                    min        = (minD / maxD) * max;
                    modified   = true;
                }
                if (ImGui::SliderFloat("Max Scale",
                                       &max,
                                       maxParams.min,
                                       maxParams.max,
                                       "%.3f",
                                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                {
                    max      = std::clamp(max, maxParams.min, maxParams.max);
                    min      = std::min(max - maxParams.min, min);
                    modified = true;
                }
                if (modified) {
                    min = std::clamp(min, minParams.min, minParams.max);
                    max = std::clamp(max, maxParams.min, maxParams.max);
                    config_->Set<float>("arrows.minScale", min);
                    config_->Set<float>("arrows.maxScale", max);
                    vProvider->SetPointScale(min, max);
                }

                ImGui::TreePop();
            }

            // Recalculate Button
            if (ImGui::Button("Recalculate PointCloud")) {
                config_->SetValue("recalculateVolume", true);
            }
            ImGui::TreePop();
        } else {
            if (vProvider->IsPointCloudDirty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "(dirty)");
            }
        }

        if (RememberingTreeNode("VM-VolumeTrace", open[to_integral(VisualizationMode::VolumeTrace)], refreshOpen)) {
            {  // volumeShader.baseTransparency
                float baseTransparency = config_->Get<float>("volumeShader.baseTransparency");
                auto  params           = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("volumeShader.baseTransparency").GetParameters());

                if (ImGui::SliderFloat("BaseTransparency",
                                       &baseTransparency,
                                       params.min,
                                       params.max,
                                       "%.3f",
                                       ImGuiSliderFlags_AlwaysClamp))
                {
                    config_->SetValue("volumeShader.baseTransparency", baseTransparency);
                }
            }

            {  // volumeShader.samplesPerCell
                int  stride = config_->Get<int>("volumeShader.samplesPerCell");
                auto params = std::get<core::ConfigurationEntry::IntParameters>(
                    config_->GetEntry("volumeShader.samplesPerCell").GetParameters());

                if (ImGui::SliderInt(
                        "SamplesPerVoxel", &stride, params.min, params.max, "%d", ImGuiSliderFlags_AlwaysClamp))
                {
                    config_->SetValue("volumeShader.samplesPerCell", stride);
                }
            }

            if (RememberingTreeNode("Accumulated Density")) {
                // volumeShader.minValue volumeShader.maxValue
                float min       = config_->Get<float>("volumeShader.minValue");
                auto  minParams = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("volumeShader.minValue").GetParameters());

                float max       = config_->Get<float>("volumeShader.maxValue");
                auto  maxParams = std::get<core::ConfigurationEntry::FloatParameters>(
                    config_->GetEntry("volumeShader.maxValue").GetParameters());

                bool modified = false;
                bool tooltip  = false;
                if (ImGui::SliderFloat("Min AccDensity",
                                       &min,
                                       minParams.min,
                                       minParams.max,
                                       minParams.format.c_str(),
                                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                {
                    min      = std::clamp(min, minParams.min, minParams.max);
                    max      = std::max(min + maxParams.min, max);
                    modified = true;
                }
                if (ImGui::IsItemHovered()) {
                    tooltip = true;
                }
                ImGui::SameLine();
                ImGui::Text(fmt::format("({} Rays)", static_cast<uint32_t>(min * vProvider->MaxRays())).c_str());
                if (ImGui::IsItemHovered()) {
                    tooltip = true;
                }

                if (ImGui::SliderFloat("Max AccDensity",
                                       &max,
                                       maxParams.min,
                                       maxParams.max,
                                       maxParams.format.c_str(),
                                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                {
                    max      = std::clamp(max, maxParams.min, maxParams.max);
                    min      = std::min(max - maxParams.min, min);
                    modified = true;
                }
                if (ImGui::IsItemHovered()) {
                    tooltip = true;
                }
                ImGui::SameLine();
                ImGui::Text(fmt::format("({} Rays)", static_cast<uint32_t>(max * vProvider->MaxRays())).c_str());
                if (ImGui::IsItemHovered()) {
                    tooltip = true;
                }

                if (tooltip) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Min and Max Accumulated Volume Density");
                    ImGui::Text("\tVolume values get accumulated a long the primary view ray.");
                    ImGui::Text(
                        fmt::format("\tEach volume cell holds a value between 0.f (= 0 rays) and 1.f (= {} rays).",
                                    vProvider->MaxRays())
                            .c_str());
                    ImGui::EndTooltip();
                }

                if (modified) {
                    min = std::clamp(min, minParams.min, minParams.max);
                    max = std::clamp(max, maxParams.min, maxParams.max);
                    config_->Set<float>("volumeShader.minValue", static_cast<float>(min));
                    config_->Set<float>("volumeShader.maxValue", static_cast<float>(max));
                }

                // volumeShader.excludeExeeding
                bool excludeExeeding = config_->Get<bool>("volumeShader.excludeExeeding");
                if (ImGui::Checkbox("Exclude Values exceeding than Max AccDensity", &excludeExeeding)) {
                    config_->Set<bool>("volumeShader.excludeExeeding", excludeExeeding);
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
            if (RememberingTreeNode("Debug", false)) {
                bool enableDebugView = config_->Get<bool>("debug.enableChunks");
                if (ImGui::Checkbox("EnableDebugVeiw", &enableDebugView)) {
                    config_->Set<bool>("debug.enableChunks", enableDebugView);
                }
                int  count  = config_->Get<int>("debug.chunkCount");
                auto params = std::get<core::ConfigurationEntry::IntParameters>(
                    config_->GetEntry("debug.chunkCount").GetParameters());

                if (ImGui::SliderInt(
                        "MaxDebugChunkCount", &count, params.min, params.max, "%d", ImGuiSliderFlags_AlwaysClamp))
                {
                    config_->SetValue("debug.chunkCount", count);
                }
                ImGui::Text(fmt::format("Current ChunkCount: {}", vpFootprint.chunkCount).c_str());
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    if (RememberingTreeNode("Rendering")) {
        // ShadingMode
        const ShadingMode currendMode = static_cast<ShadingMode>(config_->Get<int>("shadingMode"));
        if (ImGui::BeginCombo("Shading Mode", display_name(currendMode).c_str())) {
            for (int n = -1; n < to_integral(ShadingMode::_MODE_COUNT_); n++) {
                bool is_selected = (to_integral(currendMode) == n);
                if (ImGui::Selectable(display_name(static_cast<ShadingMode>(n)).c_str(), is_selected)) {
                    config_->Set<int>("shadingMode", n);
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Text("Current FPS: %.1f", 1.0 / clock.DeltaTime());

        {  // maxFPS
            bool useLimit = config_->Get<bool>("useMaxFPS");
            if (ImGui::Checkbox("FPS Limit", &useLimit)) {
                config_->Set("useMaxFPS", useLimit);
            } else if (useLimit) {
                float maxValue = config_->Get<float>("maxFPS");
                auto  params =
                    std::get<core::ConfigurationEntry::FloatParameters>(config_->GetEntry("maxFPS").GetParameters());

                ImGui::SameLine();
                if (ImGui::SliderFloat(
                        "##FPS Limit Slider", &maxValue, params.min, params.max, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                {
                    config_->SetValue("maxFPS", maxValue);
                }
            }
        }

        {  // LightDir
            glm::vec3 ligthDir = config_->Get<glm::vec3>("lightDirection");
            auto      params   = std::get<core::ConfigurationEntry::FloatParameters>(
                config_->GetEntry("lightDirection").GetParameters());
            if (ImGui::SliderFloat3(
                    "LightDirection", &ligthDir.x, params.min, params.max, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            {
                config_->Set("lightDirection", ligthDir);
            }
            ImGui::SameLine();
            if (ImGui::Button("Normalize")) {
                Float3 l3 = to3(ligthDir);
                l3        = linalg::normalize(l3);
                config_->Set("lightDirection", glm::vec3(l3.x, l3.y, l3.z));
            }
        }

        if (RememberingTreeNode("Camera")) {
            const auto            pos       = camera.GetPosition();
            constexpr const char* posFormat = "{: 6.1f}";
            ImGui::Text(fmt::format("Position: x: {:>8} y: {:>8} z: {:>8}",
                                    fmt::format(posFormat, pos.x),
                                    fmt::format(posFormat, pos.y),
                                    fmt::format(posFormat, pos.z))
                            .c_str());
            const auto            fwd           = camera.GetForwardG();
            constexpr const char* forwardFormat = "{: .4f}";
            ImGui::Text(fmt::format("Forward:  x: {:>8} y: {:>8} z: {:>8}",
                                    fmt::format(forwardFormat, fwd.x),
                                    fmt::format(forwardFormat, fwd.y),
                                    fmt::format(forwardFormat, fwd.z))
                            .c_str());

            bool invert = config_->Get<bool>("camera.invert");
            if (ImGui::Checkbox("Invert", &invert)) {
                config_->Set<bool>("camera.invert", invert);
            }

            {  // camera.speed
                float speed  = camera.GetSpeed();
                auto  params = camera.GetConfigParameters("speed");

                if (ImGui::DragFloat("Speed",
                                     &speed,
                                     100.f,
                                     params.min,
                                     params.max,
                                     "%.3f",
                                     ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                {
                    camera.SetSpeed(speed);
                }
            }

            {  // camera.t
                ImGui::PushItemWidth(150.f);
                float min       = camera.GetTMin();
                auto  minParams = camera.GetConfigParameters("minT");

                float max       = camera.GetTMax();
                auto  maxParams = camera.GetConfigParameters("maxT");

                const bool minSlider = ImGui::SliderFloat("T Min",
                                                          &min,
                                                          minParams.min,
                                                          minParams.max,
                                                          "%.3f",
                                                          ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                ImGui::SameLine();
                const bool minInput = ImGui::DragFloat("##TMinDragFloat",
                                                       &min,
                                                       50.f,
                                                       minParams.min,
                                                       minParams.max,
                                                       "%.3f",
                                                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                if (minSlider || minInput) {
                    min = std::clamp(min, minParams.min, minParams.max);
                    max = std::max(min + 1, max);
                }

                const bool maxSlider = ImGui::SliderFloat("T Max",
                                                          &max,
                                                          maxParams.min,
                                                          maxParams.max,
                                                          "%.3f",
                                                          ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                ImGui::SameLine();
                const bool maxInput = ImGui::DragFloat("##TMmaxDragFloat",
                                                       &max,
                                                       500.f,
                                                       maxParams.min,
                                                       maxParams.max,
                                                       "%.3f",
                                                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);

                if (maxSlider || maxInput) {
                    max = std::clamp(max, maxParams.min, maxParams.max);
                    min = std::min(max - 1, min);
                }
                if (minSlider || minInput || maxSlider || maxInput) {
                    min = std::clamp(min, minParams.min, minParams.max);
                    max = std::clamp(max, maxParams.min, maxParams.max);
                    camera.SetTMin(min);
                    camera.SetTMax(max);
                }
                ImGui::PopItemWidth();
            }
            {  // camera.fov
                float fov    = camera.GetFoV();
                auto  params = camera.GetConfigParameters("fov");

                if (ImGui::DragFloat("FoV", &fov, 1.0f, params.min, params.max, "%.0f", ImGuiSliderFlags_AlwaysClamp)) {
                    config_->SetValue("camera.fov", fov);
                    camera.SetFoV(fov);
                }
            }
            constexpr auto fMinV         = std::numeric_limits<float>::min();
            const auto     focusCameraOn = [&camera = camera](const Float3& min, const Float3& max) {
                const Float3 center = (max - min) * 0.5f + min;
                const Float2 hit    = IntersectAABB(center, camera.GetForwardG(), min, max);
                const Float3 eye    = center + camera.GetForwardG() * hit.x;
                camera.SetPosition(eye);
            };

            if (ImGui::Button("Focus on geometry")) {
                scene.RecalculateMinMax();
                focusCameraOn(scene.MinTransformed(), scene.MaxTransformed());
            }
            ImGui::SameLine();
            if (ImGui::Button("Focus on visualization")) {
                focusCameraOn(vpFootprint.minBounds, vpFootprint.maxBounds);
            }
            ImGui::SameLine();
            if (ImGui::Button("Recalculate Up")) {
                camera.RecalculateUp();
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
    ImGui::End();
}

bool Renderer::RememberingTreeNode(const std::string& label, bool defaultOpen, bool forceDefault)
{
    const auto entryKey = "ImGui.TreeNodeOpenState." + label;
    if (!config_->HasEntry(entryKey)) {
        config_->Register(entryKey, defaultOpen, label);
    }
    const bool previousState = config_->Get<bool>(entryKey);
    if (forceDefault) {
        ImGui::SetNextItemOpen(defaultOpen);
    } else {
        ImGui::SetNextItemOpen(previousState, ImGuiCond_Once);
    }
    const bool currentState = ImGui::TreeNode(label.c_str());
    if (currentState != previousState) {
        config_->Set<bool>(entryKey, currentState);
    }
    return currentState;
}

void Renderer::SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration)
{
    camera.SetConfiguration(configuration->CreateView("camera."));
    loader.SetConfiguration(configuration->CreateView("rayloader."));

    core::ConfigurationEntry::IntParameters          intParams   = {};
    core::ConfigurationEntry::FloatParameters        floatParams = {};
    std::vector<core::ConfigurationEntry::Validator> validators  = {};

    configuration->RegisterDirectory("shaders.source",
                                     GetExeDirectory() + "\\shaders",
                                     "Shader Source",
                                     "Source folder containing the shaders.",
                                     {},
                                     false,
                                     core::IConfiguration::OverwritePolicy::Always);
    validators.clear();
    validators.push_back(core::ConfigurationEntry::Validator::ExistingPath);
    configuration->Register("dumpSource",
                            GetExeDirectory() + "\\defaultScene.rayvis",
                            "Dump Source",
                            "BVH & Rayhistory V2 dump folder.",
                            validators);
    const core::ConfigurationEntry::IntParameters traceIdParams = {0, std::numeric_limits<int32_t>::max()};
    configuration->Register("traceId", 0, "Trace Id", "Id of rayTrace that is visualized", traceIdParams);

    floatParams.min = 0.1f;
    floatParams.max = 100.f;
    configuration->Register("sceneScale.current", 1.f, "Current Scene Scale", "", floatParams);
    configuration->Register("sceneScale.ui", 1.f, " Ui Scene Scale", "", floatParams);

    {  // volumeData
        floatParams.min = 0.0f;
        floatParams.max = 10.f;
        configuration->Register("volumeData.missTolerance",
                                0.1f,
                                " miss tolerance",
                                "tolerance when identifying rays as miss",
                                floatParams);

        intParams.min = 1;
        intParams.max = to_integral(RayFilter::IncludeAllRays);
        configuration->Register("volumeData.filter",
                                to_integral(RayFilter::IncludeHitRays),
                                "Ray filter",
                                "Ray filter for volume sampling",
                                intParams);

        intParams.min = 1 << 4;
        intParams.max = 1 << 9;
        configuration->Register(
            "volumeData.chunkSize", 128, "Volume Chunk Size", "Number of cells pro volume chunks", intParams);
        floatParams.min = 0.01f;
        floatParams.max = 1 << 10;
        configuration->Register(
            "volumeData.cellSize", 100.f, "Volume cell size", "Size of the single cell in world space", floatParams);
        floatParams.max = std::numeric_limits<float>::max();
        floatParams.min = -floatParams.max;
        configuration->Register("volumeData.maxT",
                                50000,
                                "maxT for volume Sampling",
                                "maxT for volume Sampling (negative Values get treated as no limit)",
                                floatParams);
    }

    intParams.min = 0;
    intParams.max = to_integral(VisualizationMode::_MODE_COUNT_) - 1;
    configuration->Register("visualizationMode",
                            to_integral(VisualizationMode::VolumeTrace),
                            "Visualization Mode",
                            "Visualization Mode as integer",
                            intParams);

    intParams.min = 1;
    intParams.max = to_integral(RayFilter::IncludeAllRays);
    configuration->Register("rayMesh.filter",
                            to_integral(RayFilter::IncludeHitRays),
                            "Ray filter",
                            "Ray filter for ray mesh generatiton",
                            intParams);
    intParams.min = 1;
    intParams.max = 512;
    configuration->Register("rayMesh.stride", 50, "Ray Mesh Stride", "Distance betweeen visualized rays.", intParams);
    floatParams.min = 0.05f;
    floatParams.max = 10.f;
    configuration->Register("rayMesh.thickness", 1.f, "Ray Mesh Thickness", "Thickness of rays", floatParams);
    glm::vec4 defaultRayColor = {1, 0, 0, 1};
    configuration->RegisterColor("rayMesh.color", defaultRayColor, "RayMesh Color", "Color of raymesh");
    floatParams.max = std::numeric_limits<float>::max();
    floatParams.min = -floatParams.max;
    configuration->Register("rayMesh.maxT", 50000, "maxT for rayMesh", "maxT for rayMesh", floatParams);

    floatParams.format = "%0.1f";
    floatParams.min    = 0.f;
    floatParams.max    = std::numeric_limits<VolumetricSampler::rdType>::max() - 1;
    configuration->Register(
        "arrows.minVisualizationValue", 0.f, "Arrow PointCloud Min visualization Value", "", floatParams);
    floatParams.min = 0.01f;
    floatParams.max = std::numeric_limits<VolumetricSampler::rdType>::max();
    configuration->Register(
        "arrows.maxVisualizationValue", 100.f, "Arrow PointCloud Max visualization Value", "", floatParams);

    configuration->Register(
        "arrows.excludeExeeding", false, "Arrows Exclude Exceeding", "exclude points exceeding the max value");

    configuration->Register("arrows.sampleSize",
                            2,
                            "Arrow PointCloud SampleSize",
                            "number of cells to sample per arrow in each direction",
                            {1, std::numeric_limits<int32_t>::max()});

    floatParams.format = "%0.3f";
    floatParams.min    = 0.f;
    floatParams.max    = (1 << 11) - 1;
    configuration->Register(
        "arrows.minScale", 0.f, "Arrow PointCloud Scale min", "Scale of the pointcloud arrows", floatParams);
    floatParams.min = 0.005f;
    floatParams.max = 1 << 11;
    configuration->Register(
        "arrows.maxScale", 100.f, "Arrow PointCloud Scale max", "Scale of the pointcloud arrows", floatParams);
    configuration->Register("arrows.scaleByValue", true, "Arrow Scale by Value", "Scale arrow by value");
    configuration->Register(
        "arrows.scaleByValueInverse", false, "Arrow Inverse Scale by Value", "Inverse Scale arrow by value");

    configuration->Register(
        "recalculateVolume", false, "Recalculate Volume", "Sets flag to recalculate volume after next frame");

    {  // VolumeShader
        floatParams.format = "%0.3f";
        floatParams.min    = 0.f;
        floatParams.max    = 1.f;
        configuration->Register("volumeShader.baseTransparency",
                                0.75f,
                                "VS - Base transparency of the volume",
                                "Base transparency of the volume",
                                floatParams);

        intParams.min = 1;
        intParams.max = 24;
        configuration->Register("volumeShader.samplesPerCell",
                                8,
                                "VS - Samples per cell",
                                "Number of samples taken per volume samples.",
                                intParams);

        floatParams.format = "%0.4f";
        floatParams.min    = 0.f;
        floatParams.max    = (1 << 8) - 1;
        configuration->Register("volumeShader.minValue", 0.5f, "Min AccDensity", "", floatParams);
        floatParams.min = 0.001f;
        floatParams.max = 1 << 8;
        configuration->Register("volumeShader.maxValue", 24.f, "Max AccDensity", "", floatParams);
        configuration->Register("volumeShader.excludeExeeding",
                                false,
                                "VS - Exclude Exceeding",
                                "exclude pixels with value higher than maxValue");
    }

    intParams.min = 1;
    intParams.max = 15360;
    configuration->Register("windowWidth", 1280, "Window Width", "Window Width", intParams);
    intParams.max = 8640;
    configuration->Register("windowHeight", 720, "Window Height", "Window Height", intParams);

    intParams.min = -1;
    intParams.max = to_integral(ShadingMode::_MODE_COUNT_) - 1;
    configuration->Register(
        "shadingMode", to_integral(ShadingMode::SmoothShadingSW), "Shading Mode", "Shading Mode as integer", intParams);

    configuration->Register("useMaxFPS", true, "Use Maximum FPS", "Use maximum frame per seconds limit");

    floatParams.format = "%0.3f";
    floatParams.min    = 24;
    floatParams.max    = 244;
    configuration->Register("maxFPS", 75, "Maximum FPS", "Maximum frame per seconds", floatParams);

    floatParams.min = -1.f;
    floatParams.max = 1.f;
    auto light      = linalg::normalize(Float3(0.541504323, -0.0726069361, 0.837556779));
    configuration->Register(
        "lightDirection", glm::vec3(light.x, light.y, light.z), "lightDirection", "lightDirection", floatParams);

    configuration->Register("debug.enableChunks", false, "Enable ChunkDebug", "");
    intParams.min = 1;
    intParams.max = 128;
    configuration->Register("debug.chunkCount", 5, "Debug chunk count", "", intParams);

    config_ = std::move(configuration);

    Ray::missTolerance = config_->Get<float>("volumeData.missTolerance");
    if (vProvider != nullptr) {
        vProvider->MarkDirty();
    }
}

core::IConfiguration& Renderer::GetConfigurationImpl()
{
    return *config_;
}
