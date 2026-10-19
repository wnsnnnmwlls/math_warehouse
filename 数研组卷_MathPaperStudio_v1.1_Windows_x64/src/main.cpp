#include "app.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <gdiplus.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, LPWSTR commandLine,
                    int showCommand) {
    (void)previousInstance;
    (void)commandLine;

    using SetDpiAwarenessContextProc = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto setDpiContext = reinterpret_cast<SetDpiAwarenessContextProc>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setDpiContext) {
            setDpiContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
        } else {
            SetProcessDPIAware();
        }
    }

    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok) {
        MessageBoxW(nullptr, L"无法初始化图形界面组件。", L"数研组卷", MB_ICONERROR);
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 1;
    }

    int exitCode = 1;
    {
        App app;
        if (app.Create(instance, showCommand)) {
            exitCode = app.Run();
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    if (SUCCEEDED(comResult)) CoUninitialize();
    return exitCode;
}
