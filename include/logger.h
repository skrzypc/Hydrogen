#pragma once

#include <string>
#include <fstream>
#include <format>
#include <source_location>

#include "basicTypes.h"

namespace Hydrogen
{
	enum class eLogLevel : uint8
	{
		Minimal = 0,
		Regular,
		Verbose
	};

	enum class eLogType : uint8
	{
		Info = 0,
		Warning,
		Error
	};

	class Logger
	{
	public:
		static void Initialize();
		static void SetLogLevel(eLogLevel logLevel);

		// Use macros!
		static void Log(eLogType logType, eLogLevel logLevel, std::string_view message);

	private:
		static std::string_view LogTypeToString(eLogType logType);

	private:
		constexpr static inline std::string_view m_logFileName = "Hydrogen.log";
		constexpr static inline bool m_bLogToFile = true;
		constexpr static inline bool m_bLogToConsole = true;

		static std::ofstream m_file;
		static eLogLevel m_currentLogLevel;
	};
}

#ifdef _RELEASE
	#define H2_INFO(logLevel, formatString, ...) ((void)0)
	#define H2_WARNING(logLevel, formatString, ...) ((void)0)
	#define H2_ERROR(logLevel, formatString, ...) ((void)0)
#else
	#define H2_INFO(logLevel, formatString, ...) \
		Hydrogen::Logger::Log(Hydrogen::eLogType::Info, logLevel, \
			std::format(formatString, __VA_ARGS__))

	#define H2_WARNING(logLevel, formatString, ...) \
		Hydrogen::Logger::Log(Hydrogen::eLogType::Warning, logLevel, \
			std::format(formatString, __VA_ARGS__))

	#define H2_ERROR(logLevel, formatString, ...) \
		Hydrogen::Logger::Log(Hydrogen::eLogType::Error, logLevel, \
			std::format(formatString, __VA_ARGS__))
#endif