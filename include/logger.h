#pragma once

#include <string>
#include <fstream>
#include <format>

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

	enum class eLogLocation : uint8
	{
		Core = 0,
		Renderer,
		Engine
	};

	class Logger
	{
	public:
		static void Initialize();
		static void SetLogLevel(eLogLevel logLevel);

		static void _Log(eLogType logType, eLogLocation logLocation, eLogLevel logLevel, std::string_view message);

	private:
		static std::string_view LogTypeToString(eLogType logType);
		static std::string_view LogLocationToString(eLogLocation logLocation);

	private:
		constexpr static inline std::string_view m_logFileName = "Hydrogen.log";
		constexpr static inline bool m_bLogToFile = true;
		constexpr static inline bool m_bLogToConsole = true;

		static std::ofstream m_file;
		static eLogLevel m_currentLogLevel;
	};
}

#ifdef _RELEASE
	#define H2_INFO(location, logLevel, formatString, ...) ((void)0)
	#define H2_WARNING(location, logLevel, formatString, ...) ((void)0)
	#define H2_ERROR(location, logLevel, formatString, ...) ((void)0)
#else
	#define H2_INFO(location, logLevel, formatString, ...) \
		Hydrogen::Logger::_Log(Hydrogen::eLogType::Info, location, logLevel, \
			std::format(formatString, __VA_ARGS__))

	#define H2_WARNING(location, logLevel, formatString, ...) \
		Hydrogen::Logger::_Log(Hydrogen::eLogType::Warning, location, logLevel, \
			std::format(formatString, __VA_ARGS__))

	#define H2_ERROR(location, logLevel, formatString, ...) \
		Hydrogen::Logger::_Log(Hydrogen::eLogType::Error, location, logLevel, \
			std::format(formatString, __VA_ARGS__))
#endif