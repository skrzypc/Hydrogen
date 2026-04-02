#pragma once

#include <wrl.h>
#include <dxcapi.h>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

#include "basicTypes.h"
#include "shader.h"

namespace Hydrogen
{
    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler() = default;
        ShaderCompiler(const ShaderCompiler&) = delete;
        ShaderCompiler& operator=(const ShaderCompiler&) = delete;
        ShaderCompiler(ShaderCompiler&&) noexcept = default;
        ShaderCompiler& operator=(ShaderCompiler&&) noexcept = default;

        bool Compile(Shader& shader);

    private:
        Microsoft::WRL::ComPtr<IDxcUtils> m_pUtils = nullptr;
        Microsoft::WRL::ComPtr<IDxcCompiler3> m_pCompiler = nullptr;
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_pIncludeHandler = nullptr;

        std::unordered_map<eShaderType, std::wstring> m_targetStrings{};

        std::filesystem::path m_shaderSourceDir{};
        std::filesystem::path m_shaderOutputDir{};
    };
}