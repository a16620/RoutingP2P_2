#pragma once
#include <chrono>

class IntervalTimer
{
public:
	using ClockType = std::chrono::steady_clock;
	IntervalTimer(const ClockType::duration term) : term(term), start_(ClockType::now()) {}
	bool IsPassed() const {
		auto duration = (ClockType::now() - start_);
		return duration >= term;
	}
	void Reset() {
		start_ = ClockType::now();
	}
	void Set() {
		start_ -= ClockType::duration(term);
	}
private:
	ClockType::time_point start_{};
	const ClockType::duration term;
};