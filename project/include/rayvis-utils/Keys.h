/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers.
#endif

#include <Windows.h>

#include <array>
#include <memory>
#include <optional>
#include <set>

#include "rayvis-utils/DataStructures.h"

// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
enum class Key : uint32_t {
    Tab    = VK_TAB,
    Escape = VK_ESCAPE,
    Back   = VK_BACK,
    Enter  = VK_RETURN,
    Comma  = VK_OEM_COMMA,
    Period = VK_OEM_PERIOD,
    Plus   = VK_OEM_PLUS,
    Minus  = VK_OEM_MINUS,
    Key_0  = 0x30,
    Key_1  = 0x31,
    Key_2  = 0x32,
    Key_3  = 0x33,
    Key_4  = 0x34,
    Key_5  = 0x35,
    Key_6  = 0x36,
    Key_7  = 0x37,
    Key_8  = 0x38,
    Key_9  = 0x39,
    Key_A  = 0x41,
    Key_B  = 0x42,
    Key_C  = 0x43,
    Key_D  = 0x44,
    Key_E  = 0x45,
    Key_F  = 0x46,
    Key_G  = 0x47,
    Key_H  = 0x48,
    Key_I  = 0x49,
    Key_J  = 0x4A,
    Key_K  = 0x4B,
    Key_L  = 0x4C,
    Key_M  = 0x4D,
    Key_N  = 0x4E,
    Key_O  = 0x4F,
    Key_P  = 0x50,
    Key_Q  = 0x51,
    Key_R  = 0x52,
    Key_S  = 0x53,
    Key_T  = 0x54,
    Key_U  = 0x55,
    Key_V  = 0x56,
    Key_W  = 0x57,
    Key_X  = 0x58,
    Key_Y  = 0x59,
    Key_Z  = 0x5A,
};

std::optional<char> to_char(Key k, bool includeEscapeChars = false);

class IKeyEventSubcriber {
public:
    virtual void HandleUpEvent(Key key)   = 0;
    virtual void HandleDownEvent(Key key) = 0;
};

class KeyRegistry {
public:
    static KeyRegistry*     GetGobalInstance();
    static constexpr size_t ARRAY_LENGTH = 4;

    // clears up and down events
    void AdvanceFrame();

    bool Up(Key key) const;
    bool Pressed(Key key) const;
    bool Down(Key key) const;

    int UpAxisSign(Key negative, Key positiv) const;
    int PressedAxisSign(Key negative, Key positiv) const;
    int DownAxisSign(Key negative, Key positiv) const;

    bool HandleKeyEvents(UINT message, WPARAM wParam, LPARAM lParam);

    void Subscribe(IKeyEventSubcriber* subscriber);
    void Unsubscribe(IKeyEventSubcriber* subscriber);

private:
    static std::unique_ptr<KeyRegistry> GLOBAL_INSTANCE;

    std::set<IKeyEventSubcriber*> subscribers_;

    std::array<bool64, ARRAY_LENGTH> up_;
    std::array<bool64, ARRAY_LENGTH> pressed_;
    std::array<bool64, ARRAY_LENGTH> down_;
};