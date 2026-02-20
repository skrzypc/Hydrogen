#pragma once

#include <chrono>

#include <basicTypes.h>

namespace Hydrogen
{
	class Timer
	{
	public:
		using Clock = std::chrono::high_resolution_clock;
		using Timestamp = std::chrono::time_point<Clock>;
		using TimePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;

	public:
		static Timestamp GetTimestamp() noexcept
		{
			return Clock::now();
		}

		static TimePoint GetTime() noexcept
		{
			return std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
		}

		template<typename DurationType = std::chrono::milliseconds>
		static float64 GetTimeDelta(const Timestamp& startTimestamp, const Timestamp& endTimestamp) noexcept
		{
			return std::chrono::duration<float64, typename DurationType::period>(
				endTimestamp - startTimestamp
			).count();
		}

	public:
		Timer() noexcept
		{
			Mark();
		}

		void Mark() noexcept
		{
			m_startTimestamp = GetTimestamp();
		}

		template<typename DurationType = std::chrono::milliseconds>
		float64 Peek() const noexcept
		{
			return GetTimeDelta<DurationType>(m_startTimestamp, GetTimestamp());
		}

		float64 GetMilliseconds() const noexcept { return Peek<std::chrono::milliseconds>(); }
		float64 GetSeconds() const noexcept { return Peek<std::chrono::seconds>(); }
		float64 GetMicroseconds() const noexcept { return Peek<std::chrono::microseconds>(); }
		float64 GetNanoseconds() const noexcept { return Peek<std::chrono::nanoseconds>(); }

	private:
		Timestamp m_startTimestamp;
	};
}