#pragma once
#include <stdexcept>
#include <cstdlib>

template<size_t N>
class RecyclerBuffer {
	using BYTE = unsigned char;
	using size_t = unsigned int;
	BYTE buffer[N];
	size_t start, end, used;
public:
	const size_t Length = N;
	RecyclerBuffer() : start(0), end(0), used(0) {
		static_assert(N != 0);
	}

	auto push(const char* buf, int sz) {
		if (used + sz > N)
			throw std::overflow_error("RecyclerBuffer push: overflow");

		if (end + sz > N) {
			Arrange();
		}

		memcpy(buffer + end, buf, sz);
		end += sz;
		used += sz;

		return;
	}

	auto poll(char* dst, size_t sz) {
		if (used < sz) {
			throw std::overflow_error("RecyclerBuffer push2param: overflow");
		}

		memcpy(dst, buffer + start, sz);
		start += sz;
		used -= sz;

		return;
	}

	auto Waste(size_t sz) -> void
	{
		if (used < sz) {
			throw std::overflow_error("RecyclerBuffer push2param: overflow");
		}

		start += sz;
		used -= sz;
	}

	auto Used() const {
		return used;
	}

	void Clear()
	{
		start = end = used = 0;
	}
private:
	auto Arrange() {
		if (!start)
			return;
		if (!used) {
			start = 0;
			end = 0;
			return;
		}

		
		if (used <= N/4)
		{
			BYTE tmpB[N / 4];
			memcpy(tmpB, buffer + start, used);
			memcpy(buffer, tmpB, used);;
			start = 0;
			end = used;
		}
		else if (used <= N/2)
		{
			BYTE tmpB[N / 2];
			memcpy(tmpB, buffer + start, used);
			memcpy(buffer, tmpB, used);
			start = 0;
			end = used;
		}
		else
		{
			BYTE tmpB[N];
			memcpy(tmpB, buffer + start, used);
			memcpy(buffer, tmpB, used);
			start = 0;
			end = used;
		}
		return;
	}
};

template <class T, int N>
class SequentArrayList {
	T arr[N];
	int last;
public:
	SequentArrayList() {
		last = -1;
	}

	auto& getArray() const {
		return arr;
	}

	void push_front(T& e) {
		if (last + 1 == N)
			throw std::overflow_error("SequentArrayList push_front&: overflow");
		arr[last + 1] = arr[0];
		arr[0] = e;
		++last;
	}

	void push_front(T&& e) {
		if (last + 1 == N)
			throw std::overflow_error("SequentArrayList push_front&: overflow");

		arr[last + 1] = move(arr[0]);
		arr[0] = move(e);
		++last;
	}

	void push(T& e) {
		if (last + 1 == N)
			throw std::overflow_error("SequentArrayList push&: overflow");
		arr[last + 1] = e;
		++last;
	}

	void push(T&& e) {
		if (last + 1 == N)
			throw std::overflow_error("SequentArrayList push&&: overflow");
		arr[last + 1] = std::move(e);
		++last;
	}

	void pop(int n) {
		if (last == -1)
			throw std::out_of_range("SequentArrayList pop: poped empty");
		arr[n] = arr[last];
		--last;
	}

	T& at(int n) {
		if (n >= N)
			throw std::out_of_range("SequentArrayList at: invalid indexing");
		return arr[n];
	}

	int size() const {
		return last + 1;
	}
};