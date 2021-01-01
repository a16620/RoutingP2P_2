#pragma once
#include <chrono>

class IntervalTimer
{
	using ClockType = std::chrono::steady_clock;
public:
	IntervalTimer(const long term) : term(term), start_(ClockType::now()) {}
	bool IsPassed() const {
		auto duration = (ClockType::now() - start_);
		auto d_ = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
		return d_ >= term;
	}
	void Reset() {
		start_ = ClockType::now();
	}
private:
	ClockType::time_point start_{};
	const long term;
};