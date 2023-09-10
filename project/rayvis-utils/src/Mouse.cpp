/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Mouse.h"

#include <spdlog/spdlog.h>
#include <windowsx.h>

namespace {
    int32_t GetIdx(MouseButtons key)
    {
        switch (key) {
        case MouseButtons::Left:
            return 0;
        case MouseButtons::Middle:
            return 1;
        case MouseButtons::Right:
            return 2;
        default:
            spdlog::warn("Unknown MouseButton: {}", static_cast<int32_t>(key));
            return 7;
        }
    }
}  // namespace

std::unique_ptr<Mouse> Mouse::GLOBAL_INSTANCE = nullptr;

Mouse* Mouse::GetGobalInstance()
{
    if (GLOBAL_INSTANCE == nullptr) {
        GLOBAL_INSTANCE = std::make_unique<Mouse>();
    }
    return GLOBAL_INSTANCE.get();
}

void Mouse::AdvanceFrame()
{
    up_.SetAll(false);
    down_.SetAll(false);
    deltaPosition_ = {0, 0};
    scroll_        = 0;
}

bool Mouse::Up(MouseButtons key) const
{
    return up_[GetIdx(key)];
}

bool Mouse::Pressed(MouseButtons key) const
{
    return pressed_[GetIdx(key)];
}

bool Mouse::Down(MouseButtons key) const
{
    return down_[GetIdx(key)];
}

Int2 Mouse::Position()
{
    return position_;
}

Int2 Mouse::DeltaPosition()
{
    return deltaPosition_;
}

bool Mouse::HandleKeyEvents(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_LBUTTONDOWN:
        return HandleDownEvent(MouseButtons::Left);
    case WM_LBUTTONUP:
        return HandleUpEvent(MouseButtons::Left);
    case WM_MBUTTONDBLCLK:
        return HandleDownEvent(MouseButtons::Middle);
    case WM_MBUTTONUP:
        return HandleUpEvent(MouseButtons::Middle);
    case WM_RBUTTONDOWN:
        return HandleDownEvent(MouseButtons::Right);
    case WM_RBUTTONUP:
        return HandleUpEvent(MouseButtons::Right);
    case WM_MOUSEMOVE: {
        Int2 newMousePos = Int2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        deltaPosition_   = newMousePos - position_;
        position_        = newMousePos;
        for (auto& sub : subscribers_) {
            sub->HandleMoveEvent(position_, deltaPosition_);
        }
        return true;
    }
    case WM_MOUSEWHEEL: {
        const int16_t scroll = HIWORD(wParam);
        assert(std::abs(scroll) % 120 == 0);
        scroll_ = scroll / 120;
        return true;
    }
    }
    // Event not handle
    return false;
}

void Mouse::Subscribe(IMouseEventSubscriber* subscriber)
{
    subscribers_.insert(subscriber);
}

void Mouse::Unsubscribe(IMouseEventSubscriber* subscriber)
{
    subscribers_.erase(subscriber);
}

bool Mouse::HandleDownEvent(MouseButtons key)
{
    const auto idx = GetIdx(key);
    down_.Set(idx, true);
    pressed_.Set(idx, true);
    for (auto& sub : subscribers_) {
        sub->HandleDownEvent(key);
    }
    return true;
}

bool Mouse::HandleUpEvent(MouseButtons key)
{
    const auto idx = GetIdx(key);
    up_.Set(idx, true);
    pressed_.Set(idx, false);
    for (auto& sub : subscribers_) {
        sub->HandleUpEvent(key);
    }
    return true;
}
