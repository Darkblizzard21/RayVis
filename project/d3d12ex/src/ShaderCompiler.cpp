/* Copyright (c) 2023 Pirmin Pfeifer */
#include "ShaderCompiler.h"

#include <d3d12shader.h>  // Shader reflection.
#include <spdlog/spdlog.h>

void ShaderCompiler::Init()
{
    //
    // Create compiler and utils.
    //
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

    //
    // Create default include handler. (You can create your own...)
    //
    pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
}

CComPtr<IDxcBlob> ShaderCompiler::CompileFromFile(const wchar_t* sourceFile, const wchar_t* target) 

{
    //
    // COMMAND LINE:
    // dxc myshader.hlsl -E main -T ps_6_0 -Zi -D MYDEFINE=1 -Fo myshader.bin -Fd myshader.pdb -Qstrip_reflect
    //
    LPCWSTR pszArgs[] = {
        sourceFile,  // Optional shader source file name for error reporting
        // and for PIX shader source view.
        L"-E",
        L"main",  // Entry point.
        L"-T",
        target,  // Target.
        L"-Zs",  // Enable debug information (slim format)
    };

    //
    // Open source file.
    //
    CComPtr<IDxcBlobEncoding> pSource = nullptr;
    pUtils->LoadFile(sourceFile, nullptr, &pSource);
    DxcBuffer Source;
    Source.Ptr      = pSource->GetBufferPointer();
    Source.Size     = pSource->GetBufferSize();
    Source.Encoding = DXC_CP_ACP;  // Assume BOM says UTF8 or UTF16 or this is ANSI text.

    //
    // Compile it with specified arguments.
    //
    CComPtr<IDxcResult> pResults;
    pCompiler->Compile(&Source,                 // Source buffer.
                       pszArgs,                 // Array of pointers to arguments.
                       _countof(pszArgs),       // Number of arguments.
                       pIncludeHandler,         // User-provided interface to handle #include directives (optional).
                       IID_PPV_ARGS(&pResults)  // Compiler output status, buffer, and errors.
    );

    //
    // Print errors if present.
    //
    CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    ThrowIfFailed(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
    // Note that d3dcompiler would return null if no errors or warnings are present.
    // IDxcCompiler3::Compile will always return an error buffer, but its length
    // will be zero if there are no warnings or errors.
    if (pErrors != nullptr && pErrors->GetStringLength() != 0) {
        spdlog::error("Warnings and Errors: {}", pErrors->GetStringPointer());
    }

    //
    // Quit if the compilation failed.
    //
    HRESULT hrStatus;
    pResults->GetStatus(&hrStatus);
    ThrowIfFailed(hrStatus);

    //
    // Return shader binary.
    //
    CComPtr<IDxcBlob>      pShader     = nullptr;
    CComPtr<IDxcBlobUtf16> pShaderName = nullptr;
    ThrowIfFailed(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName));
    return pShader;
}