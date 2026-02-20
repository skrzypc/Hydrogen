
#include <Windows.h>

#include "logger.h"
#include "timer.h"

#include "config.h"

namespace Hydrogen
{
	std::ofstream Logger::m_file;
	eLogLevel Logger::m_currentLogLevel = Config::LogLevel;

	void Logger::Initialize()
	{
		//std::lock_guard<std::mutex> lock(m_mutex);

		//if (AllocConsole())
		//{
		//	FILE* fp;
		//	freopen_s(&fp, "CONOUT$", "w", stdout);
		//	freopen_s(&fp, "CONIN$", "r", stdin);
		//	std::cout.sync_with_stdio();
		//}

		m_file.open(m_logFileName.data(), std::ios::out | std::ios::trunc);

		if (!m_file.is_open())
		{
			H2_ERROR(eLogLocation::Core, eLogLevel::Minimal, "Failed to open log file!");
		}
	}

	void Logger::_Log(eLogType logType, eLogLocation logLocation, eLogLevel logLevel, std::string_view message)
	{
		//std::lock_guard<std::mutex> lock(m_mutex);

		if (logLevel > m_currentLogLevel)
		{
			return;
		}

		std::string output = std::format(
			"[{}][{}][{}] {}\n",
			std::format("{:%H:%M:%S}", Timer::GetTime()),
			LogTypeToString(logType),
			LogLocationToString(logLocation),
			message
		);

		if constexpr (m_bLogToFile)
		{
			if (m_file.is_open())
			{
				m_file << output;
				m_file.flush();
			}
		}

		if constexpr (m_bLogToConsole)
		{
			OutputDebugStringA(output.c_str());
		}
	}

	void Logger::SetLogLevel(eLogLevel logLevel)
	{
		//std::lock_guard<std::mutex> lock(m_mutex);

		m_currentLogLevel = logLevel;
	}

	std::string_view Logger::LogTypeToString(eLogType logType)
	{
		switch (logType)
		{
		case eLogType::Info: return "INFO";
		case eLogType::Warning: return "WARNING";
		case eLogType::Error: return "ERROR";
		}
		return "UNKNOWN";
	}

	std::string_view Logger::LogLocationToString(eLogLocation logLocation)
	{
		switch (logLocation)
		{
		case eLogLocation::Engine: return "Engine";
		case eLogLocation::Renderer: return "Renderer";
		case eLogLocation::Core: return "Core";
		}
		return "Unknown";
	}
}