/* Copyright (c) 2023 Pirmin Pfeifer */
#include "UIHandler.h"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

namespace {
    ImGuiMouseButton TranslateInputButton(const MouseButtons button)
    {
        switch (button) {
        case MouseButtons::Left:
            return ImGuiMouseButton_Left;
        case MouseButtons::Right:
            return ImGuiMouseButton_Right;
        case MouseButtons::Middle:
            return ImGuiMouseButton_Middle;
        default:
            throw "unkown mouse input";
        }
    }

    std::optional<ImGuiKey> TranslateInputKey(const Key key)
    {
        switch (key) {
        case Key::Tab:
            return ImGuiKey_Tab;
        case Key::Escape:
            return ImGuiKey_Escape;
        case Key::Back:
            return ImGuiKey_Backspace;
        case Key::Enter:
            return ImGuiKey_Enter;
        case Key::Comma:
            return ImGuiKey_Comma;
        case Key::Period:
            return ImGuiKey_Period;
        case Key::Minus:
            return ImGuiKey_Minus;
        case Key::Key_0:
            return ImGuiKey_0;
        case Key::Key_1:
            return ImGuiKey_1;
        case Key::Key_2:
            return ImGuiKey_2;
        case Key::Key_3:
            return ImGuiKey_3;
        case Key::Key_4:
            return ImGuiKey_4;
        case Key::Key_5:
            return ImGuiKey_5;
        case Key::Key_6:
            return ImGuiKey_6;
        case Key::Key_7:
            return ImGuiKey_7;
        case Key::Key_8:
            return ImGuiKey_8;
        case Key::Key_9:
            return ImGuiKey_9;
        case Key::Key_A:
            return ImGuiKey_A;
        case Key::Key_B:
            return ImGuiKey_B;
        case Key::Key_C:
            return ImGuiKey_C;
        case Key::Key_D:
            return ImGuiKey_D;
        case Key::Key_E:
            return ImGuiKey_E;
        case Key::Key_F:
            return ImGuiKey_F;
        case Key::Key_G:
            return ImGuiKey_G;
        case Key::Key_H:
            return ImGuiKey_H;
        case Key::Key_I:
            return ImGuiKey_I;
        case Key::Key_J:
            return ImGuiKey_J;
        case Key::Key_K:
            return ImGuiKey_K;
        case Key::Key_L:
            return ImGuiKey_L;
        case Key::Key_M:
            return ImGuiKey_M;
        case Key::Key_N:
            return ImGuiKey_N;
        case Key::Key_O:
            return ImGuiKey_O;
        case Key::Key_P:
            return ImGuiKey_P;
        case Key::Key_Q:
            return ImGuiKey_Q;
        case Key::Key_R:
            return ImGuiKey_R;
        case Key::Key_S:
            return ImGuiKey_S;
        case Key::Key_T:
            return ImGuiKey_T;
        case Key::Key_U:
            return ImGuiKey_U;
        case Key::Key_V:
            return ImGuiKey_V;
        case Key::Key_W:
            return ImGuiKey_W;
        case Key::Key_X:
            return ImGuiKey_X;
        case Key::Key_Y:
            return ImGuiKey_Y;
        case Key::Key_Z:
            return ImGuiKey_Z;
        default:
            break;
        }
        return std::nullopt;
    }
}  // namespace

UIHandler::~UIHandler()
{
    Shutdown();
}

void UIHandler::Init(ComPtr<ID3D12Device5> device, HWND hwnd)
{
    if (init_) {
        return;
    }
    device_         = device;
    descriptorHeap_ = std::make_unique<DescriptorHeap>(device_,1);
    ImGui::CreateContext();

    ImGui_ImplWin32_Init(hwnd);
    const auto desc = descriptorHeap_->GetResourceView(0);
    ImGui_ImplDX12_Init(device_.Get(),
                        config::FramesInFlight,
                        config::BackbufferFormat,
                        descriptorHeap_->GetResourceHeap(),
                        desc.cpu,
                        desc.gpu);

    Mouse::GetGobalInstance()->Subscribe(this);
    KeyRegistry::GetGobalInstance()->Subscribe(this);

    init_ = true;
}

void UIHandler::Render(ID3D12GraphicsCommandList* commandList, ID3D12Resource* renderTarget)
{
    descriptorHeap_->Reset();
    {  // Start new frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    for (auto window : windows_) {
        window->RenderWindow();
    }

    {  // RenderNewFrame
        ImGui::Render();

        const auto renderTargetDescriptor = descriptorHeap_->AllocateRenderTargetView();
        device_->CreateRenderTargetView(renderTarget, nullptr, renderTargetDescriptor);
        commandList->OMSetRenderTargets(1, &renderTargetDescriptor, false, nullptr);

        ID3D12DescriptorHeap* const ppDescriptorHeaps = descriptorHeap_->GetResourceHeap();
        commandList->SetDescriptorHeaps(1, &ppDescriptorHeaps);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }
}

void UIHandler::Shutdown()
{
    if (!init_) {
        return;
    }

    Mouse::GetGobalInstance()->Unsubscribe(this);
    KeyRegistry::GetGobalInstance()->Unsubscribe(this);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    init_ = false;
}

void UIHandler::Register(IUiWindow* window)
{
    windows_.insert(window);
}

void UIHandler::Remove(IUiWindow* window)
{
    auto remove = windows_.find(window);
    if (remove != windows_.end()) {
        windows_.erase(window);
    }
}

bool UIHandler::IsMouseCaptured() const
{
    return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
}

bool UIHandler::IsKeyboardCaptured() const
{
    return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;
}

void UIHandler::HandleUpEvent(MouseButtons key)
{
    auto& io = ImGui::GetIO();
    io.AddMouseButtonEvent(TranslateInputButton(key), false);
}

void UIHandler::HandleDownEvent(MouseButtons key)
{
    auto& io = ImGui::GetIO();
    io.AddMouseButtonEvent(TranslateInputButton(key), true);
}

void UIHandler::HandleMoveEvent(const Int2& pos, const Int2& delta)
{
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(pos.x, pos.y);
}

void UIHandler::HandleUpEvent(Key key)
{
    auto event = TranslateInputKey(key);
    if (event.has_value()) {
        auto& io = ImGui::GetIO();
        io.AddKeyEvent(*event, false);
    }
}

void UIHandler::HandleDownEvent(Key key)
{
    auto& io = ImGui::GetIO();

    auto event = TranslateInputKey(key);
    if (event.has_value()) {
        io.AddKeyEvent(*event, true);
    }

    auto charEvent = to_char(key);
    if (charEvent.has_value()) {
        io.AddInputCharacter(*charEvent);
    }
}
