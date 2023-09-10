/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include "d3d12ex/Config.h"
// Shader compiler includes
#include <atlbase.h>      // Common COM helpers.
#include <dxcapi.h>       // Be sure to link with dxcompiler.lib.

// Slightly modified copy from https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
class ShaderCompiler {
public:
    void Init();

    CComPtr<IDxcBlob> CompileFromFile(const wchar_t* sourceFile, const wchar_t* target) ;

private:
    bool                        initialize = false;
    CComPtr<IDxcUtils>          pUtils;
    CComPtr<IDxcCompiler3>      pCompiler;
    CComPtr<IDxcIncludeHandler> pIncludeHandler;
};