
#include <filesystem>

#include "shaderCompiler.h"
#include "config.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"

namespace Hydrogen
{
    ShaderCompiler::ShaderCompiler()
    {
        // Resolve directories
        auto workDir       = std::filesystem::current_path();
        m_shaderSourceDir  = workDir / "shaders";

#if _DEBUG
        m_shaderOutputDir  = workDir / "build" / "x64" / "Debug"   / "shaders";
#else
        m_shaderOutputDir  = workDir / "build" / "x64" / "Release" / "shaders";
#endif

        if (!std::filesystem::exists(m_shaderOutputDir))
        {
            H2_VERIFY_FATAL(
                std::filesystem::create_directories(m_shaderOutputDir),
                "Failed to create shader output directory: {}",
                m_shaderOutputDir.string()
            );
        }

        // Create DXC objects
        H2_VERIFY_FATAL(
            DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_pUtils)),
            "Failed to create IDxcUtils"
        );
        H2_VERIFY_FATAL(
            DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler)),
            "Failed to create IDxcCompiler3"
        );
        H2_VERIFY_FATAL(
            m_pUtils->CreateDefaultIncludeHandler(&m_pIncludeHandler),
            "Failed to create DXC include handler"
        );

        const std::wstring smSuffix = String::Format(L"_{}_{}", Config::ShaderModelVersionMajor, Config::ShaderModelVersionMinor);

        m_targetStrings[eShaderType::VS] = L"vs" + smSuffix;
        m_targetStrings[eShaderType::PS] = L"ps" + smSuffix;
        m_targetStrings[eShaderType::CS] = L"cs" + smSuffix;
        m_targetStrings[eShaderType::RT] = L"lib" + smSuffix;
    }

    bool ShaderCompiler::Compile(Shader& shader)
    {
        const Shader::Desc& desc = shader.GetDesc();

        std::filesystem::path fullSourcePath = m_shaderSourceDir / desc.sourcePath;
        H2_VERIFY_FATAL(
            std::filesystem::exists(fullSourcePath),
            "Shader source not found: {}", fullSourcePath.string()
        );

        Microsoft::WRL::ComPtr<IDxcBlobEncoding> pSource = nullptr;
        H2_VERIFY_FATAL(
            m_pUtils->LoadFile(fullSourcePath.c_str(), nullptr, &pSource),
            "Failed to load shader: {}", fullSourcePath.string()
        );

        DxcBuffer sourceBuffer{
            .Ptr      = pSource->GetBufferPointer(),
            .Size     = pSource->GetBufferSize(),
            .Encoding = DXC_CP_ACP,
        };

        // We store wstrings in `argStorage` so their memory outlives the LPCWSTR ptrs.
        std::vector<std::wstring> argStorage{};
        argStorage.reserve(32);

        std::vector<LPCWSTR> argPtrs;
        argPtrs.reserve(32);

        auto addArg = [&](std::wstring s) 
            {
                argPtrs.push_back(argStorage.emplace_back(std::move(s)).c_str());
            };

        // Source name (for error messages and PIX)
        addArg(String::ToWide(desc.name));

        // Entry point
        addArg(L"-E"); addArg(String::ToWide(desc.entryPoint));

        // Target profile
        addArg(L"-T"); addArg(m_targetStrings.at(desc.type));

        // HLSL 2021
        addArg(L"-HV"); addArg(L"2021");

        // Include paths
        addArg(L"-I"); addArg((m_shaderSourceDir / "include").wstring());

        // Per-shader defines
        for (const ShaderDefine& define : desc.defines)
        {
            addArg(L"-D");
            addArg(define.value.empty() ? define.name : define.name + L"=" + define.value);
        }

        // Output paths
        auto stem   = desc.sourcePath.stem().wstring();
        auto binOut = (m_shaderOutputDir / stem).wstring() + L".bin";
        auto pdbOut = (m_shaderOutputDir / stem).wstring() + L".pdb";

        addArg(L"-Fo"); addArg(binOut);
        addArg(L"-Fd"); addArg(pdbOut);

        // Strip for runtime
        addArg(L"-Qstrip_debug");
        addArg(L"-Qstrip_reflect");

#if _DEBUG
        addArg(L"-Zi");   // Full debug info
        addArg(L"-Od");   // No optimizations
#else
        addArg(L"-O3");
#endif

        // Compile.
        Microsoft::WRL::ComPtr<IDxcResult> pResult = nullptr;
        H2_VERIFY_FATAL(
            m_pCompiler->Compile(
                &sourceBuffer,
                argPtrs.data(),
                static_cast<uint32>(argPtrs.size()),
                m_pIncludeHandler.Get(),
                IID_PPV_ARGS(&pResult)
            ),
            "DXC Compile() call failed for {}", desc.name
        );

        HRESULT status{};
        pResult->GetStatus(&status);
        if (FAILED(status))
        {
            Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors = nullptr;
            if (SUCCEEDED(pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr)) && pErrors && pErrors->GetStringLength() > 0)
            {
                H2_ERROR(eLogLevel::Minimal, "Shader '{}' compilation failed:\n{}", desc.name, pErrors->GetStringPointer());
            }
            return false; // non-fatal: caller decides whether to keep old binary
        }

        // Retrieve binary.
        {
            Microsoft::WRL::ComPtr<IDxcBlobUtf16> pOutName = nullptr;
            H2_VERIFY_FATAL(
                pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader.m_pBinary), &pOutName),
                "Failed to retrieve shader binary for {}", desc.name
            );

            // Write .bin to disk (for preload / debugging)
            if (pOutName && pOutName->GetStringLength() > 0)
            {
                std::ofstream file(pOutName->GetStringPointer(), std::ios::out | std::ios::binary);
                if (file)
                {
                    file.write(
                        static_cast<const char*>(shader.m_pBinary->GetBufferPointer()),
                        static_cast<std::streamsize>(shader.m_pBinary->GetBufferSize())
                    );
                }
            }
        }

#if _DEBUG
        // Retrieve PDB.
        {
            Microsoft::WRL::ComPtr<IDxcBlob> pPdb = nullptr;
            Microsoft::WRL::ComPtr<IDxcBlobUtf16> pPdbName = nullptr;
            if (SUCCEEDED(pResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPdb), &pPdbName)) && pPdb && pPdbName && pPdbName->GetStringLength() > 0)
            {
                std::ofstream file(pPdbName->GetStringPointer(), std::ios::out | std::ios::binary);
                if (file)
                {
                    file.write(
                        static_cast<const char*>(pPdb->GetBufferPointer()),
                        static_cast<std::streamsize>(pPdb->GetBufferSize())
                    );
                }
            }
        }
#endif

        // Hash.
        {
            Microsoft::WRL::ComPtr<IDxcBlob> pHashBlob = nullptr;
            if (H2_VERIFY(pResult->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&pHashBlob), nullptr), "Can't obtain shader hash!") && pHashBlob)
            {
                auto* pDxcHash = reinterpret_cast<DxcShaderHash*>(pHashBlob->GetBufferPointer());
                H2_VERIFY_STATIC(sizeof(shader.m_hash) == sizeof(pDxcHash->HashDigest));
                std::memcpy(&shader.m_hash, pDxcHash->HashDigest, sizeof(shader.m_hash));
            }
        }

        H2_INFO(eLogLevel::Verbose, "Compiled shader '{}'", desc.name);

        return true;
    }
}