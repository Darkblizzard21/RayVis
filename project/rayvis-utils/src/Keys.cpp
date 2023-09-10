/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Keys.h"

#include <spdlog/spdlog.h>

namespace {
    std::string BytesAsString(bool64 b)
    {
        return fmt::format("{:064b}", b.RawValue());
    }

    bool Check(Key key, const std::array<bool64, KeyRegistry::ARRAY_LENGTH>& src)
    {
        const uint32_t keyCode    = static_cast<uint32_t>(key);
        const uint32_t outerIndex = keyCode / 64;
        const uint32_t innerIndex = keyCode % 64;
        assert(outerIndex < src.size());
        return src[outerIndex].Check(innerIndex);
    }

    void Set(uint32_t keyCode, std::array<bool64, KeyRegistry::ARRAY_LENGTH>& src, bool value)
    {
        const uint32_t outerIndex = keyCode / 64;
        const uint32_t innerIndex = keyCode % 64;
        assert(outerIndex < src.size());
        src[outerIndex].Set(innerIndex, value);
    }

    int AxisSign(bool negativePressed, bool positivePressed)
    {
        int res = 0;
        if (negativePressed) {
            --res;
        }
        if (positivePressed) {
            ++res;
        }
        return res;
    }
}  // namespace

std::unique_ptr<KeyRegistry> KeyRegistry::GLOBAL_INSTANCE = nullptr;


std::optional<char> to_char(Key k, bool includeEscapeChars)
{
    switch (k) {
    case Key::Tab:
        if (includeEscapeChars) {
            return '\t';
        }
        break;
    case Key::Comma:
        return '-';
    case Key::Period:
        return '.';
    case Key::Plus:
        return '+';
    case Key::Minus:
        return '-';
    case Key::Key_0:
        return '0';
    case Key::Key_1:
        return '1';
    case Key::Key_2:
        return '2';
    case Key::Key_3:
        return '3';
    case Key::Key_4:
        return '4';
    case Key::Key_5:
        return '5';
    case Key::Key_6:
        return '6';
    case Key::Key_7:
        return '7';
    case Key::Key_8:
        return '8';
    case Key::Key_9:
        return '9';
    case Key::Key_A:
        return 'a';
    case Key::Key_B:
        return 'b';
    case Key::Key_C:
        return 'c';
    case Key::Key_D:
        return 'd';
    case Key::Key_E:
        return 'e';
    case Key::Key_F:
        return 'f';
    case Key::Key_G:
        return 'g';
    case Key::Key_H:
        return 'h';
    case Key::Key_I:
        return 'i';
    case Key::Key_J:
        return 'j';
    case Key::Key_K:
        return 'k';
    case Key::Key_L:
        return 'l';
    case Key::Key_M:
        return 'm';
    case Key::Key_N:
        return 'n';
    case Key::Key_O:
        return 'o';
    case Key::Key_P:
        return 'p';
    case Key::Key_Q:
        return 'q';
    case Key::Key_R:
        return 'r';
    case Key::Key_S:
        return 's';
    case Key::Key_T:
        return 't';
    case Key::Key_U:
        return 'u';
    case Key::Key_V:
        return 'v';
    case Key::Key_W:
        return 'w';
    case Key::Key_X:
        return 'x';
    case Key::Key_Y:
        return 'y';
    case Key::Key_Z:
        return 'z';
    default:
        break;
    }
    return std::nullopt;
}

KeyRegistry* KeyRegistry::GetGobalInstance()
{
    if (GLOBAL_INSTANCE == nullptr) {
        GLOBAL_INSTANCE = std::make_unique<KeyRegistry>();
    }
    return GLOBAL_INSTANCE.get();
}

void KeyRegistry::AdvanceFrame()
{
    // spdlog::info("{}-{}", BytesAsString(pressed_[0]), BytesAsString(pressed_[1]));
    for (size_t i = 0; i < ARRAY_LENGTH; i++) {
        up_[i].SetAll(false);
        down_[i].SetAll(false);
    }
}

bool KeyRegistry::Up(Key key) const
{
    return Check(key, up_);
}

bool KeyRegistry::Pressed(Key key) const
{
    return Check(key, pressed_);
}

bool KeyRegistry::Down(Key key) const
{
    return Check(key, down_);
}

int KeyRegistry::UpAxisSign(Key negative, Key positiv) const
{
    return AxisSign(Up(negative), Up(positiv));
}

int KeyRegistry::PressedAxisSign(Key negative, Key positiv) const
{
    return AxisSign(Pressed(negative), Pressed(positiv));
}

int KeyRegistry::DownAxisSign(Key negative, Key positiv) const
{
    return AxisSign(Down(negative), Down(positiv));
}

bool KeyRegistry::HandleKeyEvents(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_KEYDOWN: {
        bool keyPreviouslyPressed = (lParam & (1L << 30));
        if (!keyPreviouslyPressed) {
            Set(static_cast<UINT8>(wParam), down_, true);
            Set(static_cast<UINT8>(wParam), pressed_, true);
            for (auto& sub : subscribers_) {
                sub->HandleDownEvent(static_cast<Key>(wParam));
            }
        }
        return true;
    }
    case WM_KEYUP:
        Set(static_cast<UINT8>(wParam), up_, true);
        Set(static_cast<UINT8>(wParam), pressed_, false);
        for (auto& sub : subscribers_) {
            sub->HandleUpEvent(static_cast<Key>(wParam));
        }
        return true;
    }
    // Event not handle
    return false;
}

void KeyRegistry::Subscribe(IKeyEventSubcriber* subscriber)
{
    subscribers_.insert(subscriber);
}

void KeyRegistry::Unsubscribe(IKeyEventSubcriber* subscriber)
{
    subscribers_.erase(subscriber);
}
