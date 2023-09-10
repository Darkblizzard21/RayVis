/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers.
#endif

// Win32 / D3D12 / DXGI includes
#include <Config.h>
#include <RayVisDataformat.h>
#include <Renderer.h>
#include <Window.h>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <rayvis-utils/FileSystemUtils.h>
#include <rayvis-utils/Keys.h>
#include <rayvis-utils/Mouse.h>

#include <spdlog/spdlog.h>
#include <wrl/client.h>  // for ComPtr

#include <CLI11/CLI11.hpp>
#include <cassert>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>

using Microsoft::WRL::ComPtr;

class WindowContext : public IWindowProc {
public:
    HWND         hwnd_ = nullptr;
    Renderer     renderContext;
    KeyRegistry* keys;
    Mouse*       mouse;

    core::Configuration config;
    // for pre checks
    core::Configuration config2;

    std::filesystem::path configPath;

    WindowContext(std::filesystem::path& configPath) : configPath(configPath)
    {
        if (std::filesystem::exists(configPath)) {
            config2.LoadJson(std::filesystem::path(configPath));
            Ray::missTolerance = config2.Get<float>("rayvis.volumeData.missTolerance");
        }

        mouse = Mouse::GetGobalInstance();
        keys  = KeyRegistry::GetGobalInstance();
    }

    void Init(HWND hwnd, const optionalRenderArgs& args)
    {
        hwnd_ = hwnd;
        renderContext.SetConfiguration(config.CreateView("rayvis."));

        if (std::filesystem::exists(configPath)) {
            try {
                config.LoadJson(std::filesystem::path(configPath));
            } catch (const core::CoreException& e) {
                spdlog::error("Loading Config failed:\n {}", e.GetFullMessage());
                throw std::runtime_error(e.GetFullMessage());
            }
        }
        config.ResetModified();

        renderContext.Init(hwnd, args);

        config.ResetModified();
    }

    void Destroy()
    {
        try {
            renderContext.Destroy();
        } catch (std::exception e) {
        };

        SaveConfig();
    }

    void AdvanceFrame()
    {
        if (keys->Down(Key::Escape)) {
            PostQuitMessage(0);
        }

        if (renderContext.wantsToSaveConfig) {
            SaveConfig();
            renderContext.wantsToSaveConfig = false;
        }
        if (renderContext.wantsToLoadSource) {
            RequestSourceFromUser();
            renderContext.wantsToLoadConfig = false;
            renderContext.wantsToLoadSource = false;
        } else if (renderContext.wantsToLoadConfig) {
            LoadConfig();
            renderContext.wantsToLoadConfig = false;
        } else if (renderContext.wantsToResetConfig) {
            const auto source  = config.Get<std::string>("rayvis.dumpSource");
            const auto traceId = config.Get<int>("rayvis.traceId");
            const auto width   = config.Get<int>("rayvis.windowWidth");
            const auto heigth  = config.Get<int>("rayvis.windowHeight");
            config             = core::Configuration();
            renderContext.SetConfiguration(config.CreateView("rayvis."));
            config.Set<std::string>("rayvis.dumpSource", source);
            config.Set<int>("rayvis.traceId", traceId);
            config.Set<int>("rayvis.windowWidth", width);
            config.Set<int>("rayvis.windowHeight", heigth);
            renderContext.wantsToResetConfig = false;
            config.ResetModified();
        }

        renderContext.AdvanceFrame();
        keys->AdvanceFrame();
        mouse->AdvanceFrame();

        config.ResetModified();
    }

    void Render()
    {
        try {
            renderContext.Render();
        } catch (core::CoreException e) {
            spdlog::warn(e.GetFullMessage());
        } catch (std::exception e) {
            spdlog::warn(e.what());
        };
    };

    bool WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override
    {
        if (WM_SIZE == message) {
            renderContext.Resize();

            {  // We dont talk about this fix -  it prevents the window from show dead pixels and "it just works"
                static bool moveup;
                RECT        rect;
                ThrowIfFailed(GetWindowRect(hwnd_, &rect));
                int currentWidth  = rect.right - rect.left;
                int currentHeight = rect.bottom - rect.top;
                MoveWindow(hwnd_, rect.left, rect.top + (moveup ? 1 : -1), currentWidth, currentHeight, TRUE);
                moveup = !moveup;
            }
            return true;
        }
        return keys->HandleKeyEvents(message, wParam, lParam) || mouse->HandleKeyEvents(message, wParam, lParam);
    };

    int GetWindowWidthOr(int defaultWidth)
    {
        return config2.HasEntry("rayvis.windowWidth") ? config2.Get<int>("rayvis.windowWidth") : defaultWidth;
    }

    int GetWindowHeightOr(int defaultHeigth)
    {
        return config2.HasEntry("rayvis.windowHeight") ? config2.Get<int>("rayvis.windowHeight") : defaultHeigth;
    }

private:
    void SaveConfig()
    {
        std::filesystem::create_directories(configPath.parent_path());
        std::ofstream outStream(configPath);
        const auto    j = config.ToJson();
        outStream << std::setw(4) << j << std::endl;
        outStream.close();
        spdlog::info("Saved config to {}", configPath.string());
    }

    bool LoadConfig(std::optional<std::string> userConfig = std::nullopt)
    {
        std::string newConfigPath = userConfig.value_or(Lazy{[] {
            HWND consoleWindow = GetConsoleWindow();
            SetForegroundWindow(consoleWindow);
            spdlog::info("INPUT REQUIRED - Enter a new config location");
            std::string input;
            std::getline(std::cin, input);
            return input;
        }});

        std::filesystem::path userPath(newConfigPath);
        if (!std::filesystem::exists(userPath)) {
            spdlog::error("\"{}\" dose not exist!", newConfigPath);
            return false;
        }
        if (!std::filesystem::is_regular_file(userPath)) {
            spdlog::error("\"{}\" is no file!", newConfigPath);
            return false;
        }
        if (userPath.extension() != ".json") {
            spdlog::error("\"{}\" is no json file!", newConfigPath);
            return false;
        }
        try {
            const auto source  = config.Get<std::string>("rayvis.dumpSource");
            const auto traceId = config.Get<int>("rayvis.traceId");
            config.LoadJson(userPath);
            configPath = newConfigPath;
            // Restore source
            config.Set<std::string>("rayvis.dumpSource", source);
            config.Set<int>("rayvis.traceId", traceId);
        } catch (std::runtime_error& e) {
            spdlog::error("config.LoadJson failed to load {}. \n ERROR: {}", newConfigPath, e.what());
            return false;
        }
        return true;
    }

    void RequestSourceFromUser()
    {
        HWND consoleWindow = GetConsoleWindow();
        SetForegroundWindow(consoleWindow);

        std::string sourcePath = "";
        std::string configPath = "";

        spdlog::info("INPUT REQUIRED - Enter a new config location or enter empty string to not change config:");
        std::getline(std::cin, configPath);
        if (configPath != "") {
            const bool success = LoadConfig(configPath);
            if (!success) {
                return;
            }
        } else {
            spdlog::info("Acknowledged: No config change requested.");
        }

        spdlog::info("INPUT REQUIRED - Enter a path to load from:");
        std::getline(std::cin, sourcePath);
        std::filesystem::path userPath(sourcePath);
        if (!std::filesystem::exists(userPath)) {
            spdlog::error("\"{}\" dose not exist!", sourcePath);
            return;
        }
        const bool isDirecotry  = std::filesystem::is_directory(userPath);
        const bool isRayVisFile = userPath.extension() == dataformat::EXTENSION;
        if (!(isDirecotry || isRayVisFile)) {
            spdlog::error("\"{}\" is neither directory nor rayvis file!", sourcePath);
            return;
        }

        bool validPath = false;
        if (isDirecotry) {
            return;
        } else {
            validPath = dataformat::CheckPathForVaildFile(sourcePath);
        }

        if (!validPath) {
            return;
        }

        config.Set<std::string>("rayvis.dumpSource", sourcePath);
    }
};

int main(int argc, char* argv[])
{
    const std::filesystem::path defaultConfigPath = GetExeDirectory() + "\\config.json";
    std::filesystem::path           configPath        = defaultConfigPath;
    std::string                     input             = "";
    std::string                     shaderSource      = "";
    bool                            enableExport      = false;

    CLI::App app{WINDOW_TITEL};

    app.add_option("-i,--input", input, "Input rayvisFile. If not set input from config will be used")
        ->check(CLI::ExistingPath)
        ->check([](const std::string& path) {
            if (std::filesystem::path(path).extension() == dataformat::EXTENSION) {
                return std::string();
            }
            return fmt::format("Path \"{}\" is neither {} file nor directory", path, dataformat::EXTENSION);
        });
    app.add_option("-c,--config-path", configPath, "Config file to use. Alternatively config path to save config to.")
        ->check(CLI::NonexistentPath | CLI::ExistingFile);
    app.add_option("-s,--shader-source", shaderSource, "Path that is used to load shaders.")
        ->check(CLI::ExistingDirectory);

    app.add_flag("--enableExport", enableExport, "Enables export of files. (consumes more memory)");

    CLI11_PARSE(app, argc, argv);

    optionalRenderArgs optArgs  = {};
    // Build flag info and active settings
    bool               anyFlags = false;
    std::string        flagInfo = std::string(WINDOW_TITEL) + " was started with CLI options:";
    if (enableExport) {
        flagInfo += "\n\t--enableExport\t- enables export of files, but consumes more memory!";
        config::EnableFileSave = enableExport;
        anyFlags               = true;
    }
    if (!input.empty()) {
        flagInfo +=
            fmt::format("\n\t--input\t\t- input source overridden to \"{}\" (This will be saved to config)!", input);
        optArgs.source = std::filesystem::absolute(input).string();
        anyFlags       = true;
    }
    if (!shaderSource.empty()) {
        flagInfo += fmt::format(
            "\n\t--shader-source\t- shader source overridden to \"{}\" (This will be saved to config)!", shaderSource);
        optArgs.shaderSource = std::filesystem::absolute(shaderSource).string();
        anyFlags             = true;
    }
    if (configPath != defaultConfigPath) {
        flagInfo += fmt::format("\n\t--config-path\t- config path overridden to \"{}\"", configPath.string());
        configPath = std::filesystem::absolute(configPath).string();
        anyFlags   = true;
    }
    if (!anyFlags) {
        flagInfo = std::string(WINDOW_TITEL) + " was started without any CLI options";
    }
    spdlog::info(flagInfo);

    // Start application

    WindowContext context(configPath);
    WindowArgs    wArgs;
    wArgs.preferredWindowWidth  = context.GetWindowWidthOr(1280);
    wArgs.preferredWindowHeight = context.GetWindowHeightOr(720);
    wArgs.windowTitle           = WINDOW_TITEL;
    wArgs.proc                  = &context;
    HWND window                 = MakeWindow(wArgs);

    context.Init(window, optArgs);
    ShowWindow(window, SW_SHOW);

    const int exitCode = MessageLoop([&]() {
        context.Render();

        context.AdvanceFrame();
    });
    context.Destroy();

    // Return this part of the WM_QUIT message to Windows.
    return exitCode;
}