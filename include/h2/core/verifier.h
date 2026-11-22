#pragma once

#include <Windows.h>
#include <string>
#include <source_location>

#include <h2/core/logger.h>

namespace Hydrogen
{
    class Verifier
    {
    public:
        // Use macros!
        template <typename ConditionT>
        static bool Verify(ConditionT condition, std::string_view message, const std::source_location& source)
        {
            bool result = CheckCondition(condition);

            if (!result)
            {
                const std::string finalMessage =
                    std::format("Verification failed!\n> {}\n> {}({})\n", message, source.file_name(), source.line());
                Logger::Log(eLogType::Warning, eLogLevel::Minimal, finalMessage);
            }
            return result;
        }

        // Use macros!
        template <typename ConditionT>
        static bool VerifyFatal(ConditionT condition, std::string_view message, const std::source_location& source)
        {
            bool result = CheckCondition(condition);

            if (!result)
            {
                const std::string finalMessage =
                    std::format("Fatal verification failed!\n> {}\n> {}({})\n> Engine will be terminated!", message,
                                source.file_name(), source.line());
                Logger::Log(eLogType::Error, eLogLevel::Minimal, finalMessage);
#ifdef _DEBUG
                __debugbreak();
#endif
                std::exit(EXIT_FAILURE);
            }
            return result;
        }

        // Converts a condition to its success value (HRESULTs via SUCCEEDED). Used by the macros in every config.
        static bool CheckCondition(bool value) noexcept
        {
            return value;
        }

        static bool CheckCondition(HRESULT hr) noexcept
        {
            return SUCCEEDED(hr);
        }
    };
} // namespace Hydrogen

#ifdef _RELEASE
// No message or logging in release, but the condition is still evaluated and returned as a bool.
#define H2_VERIFY(condition, formatString, ...) Hydrogen::Verifier::CheckCondition(condition)
#define H2_VERIFY_FATAL(condition, formatString, ...) Hydrogen::Verifier::CheckCondition(condition)
#else
#define H2_VERIFY(condition, formatString, ...)                                                                        \
    Hydrogen::Verifier::Verify(condition, std::format(formatString, __VA_ARGS__), std::source_location::current())

#define H2_VERIFY_FATAL(condition, formatString, ...)                                                                  \
    Hydrogen::Verifier::VerifyFatal(condition, std::format(formatString, __VA_ARGS__), std::source_location::current())
#endif

#define H2_VERIFY_STATIC(condition) static_assert(condition, #condition)
