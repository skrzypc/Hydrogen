#pragma once

#include <Windows.h>
#include <string>
#include <source_location>

#include <logger.h>

namespace Hydrogen
{
	class Verifier
	{
	public:
		// Use macros!
		template<typename ConditionT>
		static bool Verify(ConditionT condition, std::string_view message, const std::source_location& source)
		{
			bool result = CheckCondition(condition);

			if (!result)
			{
				const std::string finalMessage = std::format(
					"Verification failed!\n> {}\n> {}({})\n",
					message,
					source.file_name(),
					source.line()
				);
				Logger::Log(eLogType::Warning, eLogLevel::Minimal, finalMessage);
			}
			return result;
		}

		// Use macros!
		template<typename ConditionT>
		static bool VerifyFatal(ConditionT condition, std::string_view message, const std::source_location& source)
		{
			bool result = CheckCondition(condition);

			if (!result)
			{
				const std::string finalMessage = std::format(
					"Fatal verification failed!\n> {}\n> {}({})\n> Engine will be terminated!",
					message,
					source.file_name(),
					source.line()
				);
				Logger::Log(eLogType::Error, eLogLevel::Minimal, finalMessage);
#ifdef _DEBUG
				__debugbreak();
#endif
				std::exit(EXIT_FAILURE);
			}
			return result;
		}

	private:
		static bool CheckCondition(bool value) noexcept
		{
			return value;
		}

		static bool CheckCondition(HRESULT hr) noexcept
		{
			return SUCCEEDED(hr);
		}
	};
}

#ifdef _RELEASE
	#define H2_VERIFY(condition, formatString, ...) (condition)
	#define H2_VERIFY_FATAL(condition, formatString, ...) (condition)
#else
#define H2_VERIFY(condition, formatString, ...) \
		Hydrogen::Verifier::Verify(condition, std::format(formatString, __VA_ARGS__), \
			std::source_location::current())

#define H2_VERIFY_FATAL(condition, formatString, ...) \
		Hydrogen::Verifier::VerifyFatal(condition, std::format(formatString, __VA_ARGS__), \
			std::source_location::current())
#endif

#define H2_VERIFY_STATIC(condition) static_assert(condition, #condition)
