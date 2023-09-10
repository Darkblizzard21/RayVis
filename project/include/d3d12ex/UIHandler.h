/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <DescriptorHeap.h>
#include <rayvis-utils/Keys.h>
#include <rayvis-utils/Mouse.h>

class IUiWindow {
public:
    virtual void RenderWindow() = 0;
};

class UIHandler : IMouseEventSubscriber, IKeyEventSubcriber {
public:
    UIHandler() = default;
    ~UIHandler();
    void Init(ComPtr<ID3D12Device5> device, HWND hwnd);
    void Render(ID3D12GraphicsCommandList* commandList, ID3D12Resource* renderTarget);
    void Shutdown();

    void Register(IUiWindow* window);
    void Remove(IUiWindow* window);

    // Returns whether user interface is requesting mouse input
    bool IsMouseCaptured() const;
    // Returns whether user interface is requesting keyboard input
    bool IsKeyboardCaptured() const;

protected:
    void HandleUpEvent(MouseButtons key) override;
    void HandleDownEvent(MouseButtons key) override;
    void HandleMoveEvent(const Int2& pos, const Int2& delta) override;

    void HandleUpEvent(Key key) override;
    void HandleDownEvent(Key key) override;

private:
    bool init_ = false;

    std::unique_ptr<DescriptorHeap> descriptorHeap_;
    ComPtr<ID3D12Device5>           device_;

    std::set<IUiWindow*> windows_;
};