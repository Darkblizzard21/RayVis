/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers.
#endif

#include <Windows.h>
#include <rayvis-utils/MathTypes.h>

#include <memory>

#include "rayvis-utils/DataStructures.h"
#include <set>

// https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
enum class MouseButtons : uint32_t {
    Left   = 0x0001,
    Middle = 0x0010,
    Right  = 0x0002,
};

class IMouseEventSubscriber {
public:
    virtual void HandleUpEvent(MouseButtons key)                     = 0;
    virtual void HandleDownEvent(MouseButtons key)                   = 0;
    virtual void HandleMoveEvent(const Int2& pos, const Int2& delta) = 0;
};

class Mouse {
public:
    static Mouse* GetGobalInstance();
    // clears up and down events
    void          AdvanceFrame();

    bool Up(MouseButtons key) const;
    bool Pressed(MouseButtons key) const;
    bool Down(MouseButtons key) const;

    Int2 Position();
    Int2 DeltaPosition();

    bool HandleKeyEvents(UINT message, WPARAM wParam, LPARAM lParam);

    void Subscribe(IMouseEventSubscriber* subscriber);
    void Unsubscribe(IMouseEventSubscriber* subscriber);

private:
    bool HandleDownEvent(MouseButtons key);
    bool HandleUpEvent(MouseButtons key);

    static std::unique_ptr<Mouse> GLOBAL_INSTANCE;

    std::set<IMouseEventSubscriber*> subscribers_;

    bool8 up_;
    bool8 pressed_;
    bool8 down_;

    Int2 position_;
    Int2 deltaPosition_;

    int16_t scroll_;
};