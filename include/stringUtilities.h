#pragma once

#include <string>
#include <format>
#include <algorithm>
#include <Windows.h>

#include "basicTypes.h"
#include "logger.h"

namespace Hydrogen
{
	namespace String
	{
		inline void ToUpper(std::string& str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		}

		inline std::string ToUpper(std::string_view str)
		{
			std::string result(str);
			ToUpper(result);
			return result;
		}

		inline std::wstring ToWide(const std::string& str)
		{
			if (str.empty())
			{
				return {};
			}

			int32 size = MultiByteToWideChar(
				CP_UTF8,
				MB_ERR_INVALID_CHARS,
				str.data(),
				static_cast<int32>(str.size()),
				nullptr,
				0
			);

			if (size <= 0)
			{
				H2_WARNING(eLogLevel::Regular, "Conversion from UTF8 to wide string failed!");
			}

			std::wstring result(size, 0);

			MultiByteToWideChar(
				CP_UTF8,
				MB_ERR_INVALID_CHARS,
				str.data(),
				static_cast<int32>(str.size()),
				result.data(),
				size
			);

			return result;
		}

		inline std::string ToUTF8(const std::wstring& wstr)
		{
			if (wstr.empty())
			{
				return {};
			}

			int32 size = WideCharToMultiByte(
				CP_UTF8,
				WC_ERR_INVALID_CHARS,
				wstr.data(),
				static_cast<int32>(wstr.size()),
				nullptr,
				0,
				nullptr,
				nullptr
			);

			if (size <= 0)
			{
				H2_WARNING(eLogLevel::Regular, "Conversion from wide to UTF8 string failed!");
			}

			std::string result(size, 0);

			WideCharToMultiByte(
				CP_UTF8,
				WC_ERR_INVALID_CHARS,
				wstr.data(),
				static_cast<int32>(wstr.size()),
				result.data(),
				size,
				nullptr,
				nullptr
			);

			return result;
		}

		template<typename... Args>
		inline std::string Format(std::format_string<Args...> fmt, Args&&... args)
		{
			return std::format(fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		inline std::wstring Format(std::wformat_string<Args...> fmt, Args&&... args)
		{
			return std::format(fmt, std::forward<Args>(args)...);
		}
	}
}