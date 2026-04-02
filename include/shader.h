#pragma once

#include <wrl.h>
#include <dxcapi.h>
#include <filesystem>
#include <vector>
#include <string>

#include "basicTypes.h"

namespace Hydrogen
{
    enum class eShaderType : uint8
    {
        VS,
        PS,
        CS,
        RT,
    };

    struct ShaderDefine
    {
        std::wstring name{};
        std::wstring value{};
    };

    class Shader
    {
        friend class ShaderCompiler;
    public:
        struct Desc
        {
            std::filesystem::path sourcePath; // relative to shader source dir.
            std::string name;
            std::string entryPoint = "main";
            eShaderType type = eShaderType::VS;
            std::vector<ShaderDefine> defines{};
        };

        Shader(Desc desc)
            : m_desc(std::move(desc))
        {}
		~Shader() = default;
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader(Shader&&) noexcept = default;
		Shader& operator=(Shader&&) noexcept = default;

        bool IsCompiled() const
        { 
            return m_pBinary != nullptr && m_pBinary->GetBufferSize() > 0;
        }

        const void* GetBytecode() const 
        { 
            return m_pBinary ? m_pBinary->GetBufferPointer() : nullptr;
        }

        uint64 GetBytecodeSize() const
        { 
            return m_pBinary ? m_pBinary->GetBufferSize() : 0;
        }

        const Desc& GetDesc() const { return m_desc; }

        const uint128& GetHash() const { return m_hash; }

    private:
        Desc m_desc{};
        Microsoft::WRL::ComPtr<IDxcBlob> m_pBinary = nullptr;

        uint128 m_hash;
    };
}