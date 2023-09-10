/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Window.h"

#include <cassert>

namespace {
    LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        IWindowProc* context = reinterpret_cast<IWindowProc*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (context != nullptr && context->WindowProc(message, wParam, lParam)) {
            return 0;
        }

        if (message == WM_CREATE) {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        if (message == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }
        // Handle any messages the switch statement didn't.
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}  // namespace

bool WindowArgs::IsValid() const
{
    return windowTitle != nullptr && 0 < preferredWindowWidth && 0 < preferredWindowHeight && proc != nullptr;
}

HWND MakeWindow(const WindowArgs& args)
{
    assert(args.IsValid());
    const HINSTANCE hInstance = GetModuleHandleA(NULL);

    WNDCLASSEX windowClass    = {0};
    windowClass.cbSize        = sizeof(WNDCLASSEX);
    windowClass.style         = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = WindowProc;
    windowClass.hInstance     = hInstance;
    windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = args.windowTitle;
    RegisterClassEx(&windowClass);

    RECT windowRect = {0, 0, args.preferredWindowWidth, args.preferredWindowHeight};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindow(windowClass.lpszClassName,
                             args.windowTitle,
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             windowRect.right - windowRect.left,
                             windowRect.bottom - windowRect.top,
                             nullptr,  // We have no parent window.
                             nullptr,  // We aren't using menus.
                             hInstance,
                             static_cast<void*>(args.proc));

    return hwnd;
}

int MessageLoop (std::function<void()> body)
{
    MSG msg = {};

    while (true) {
        // Process any messages in the queue.
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            // Check if quit is requested
            if (msg.message == WM_QUIT) {
                return static_cast<char>(msg.wParam);
            }
            //TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        body();
    }
}
