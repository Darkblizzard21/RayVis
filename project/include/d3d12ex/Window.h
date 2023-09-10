/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <Windows.h>
#include <functional>

class IWindowProc {
public:
    virtual bool WindowProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;
};

struct WindowArgs {
public:
    const char* windowTitle           = nullptr;
    int         preferredWindowWidth  = 0;
    int         preferredWindowHeight = 0;

    IWindowProc* proc = nullptr;

    bool IsValid() const;
};

HWND MakeWindow(const WindowArgs& args);

int MessageLoop(std::function<void()> body);
