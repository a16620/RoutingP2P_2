/*
	Code from <high performance C++>
*/
#pragma once
#include <cstdlib>
#include <stdexcept>

template <size_t N>
class Arena {
	static constexpr size_t alignment = alignof(std::max_align_t);
public:
	Arena() noexcept : ptr_(buffer_) {}
	Arena(const Arena&) = delete;
	Arena& operator=(const Arena&) = delete;

	auto reset() noexcept { ptr_ = buffer_; }
	static constexpr auto size() noexcept { return N; }
	auto used() const noexcept {
		return static_cast<size_t>(ptr_ - buffer_);
	}

	auto allocate(size_t n) -> char* {
		const auto aligned_n = align_up(n);
		const auto available_bytes = static_cast<decltype(aligned_n)>(buffer_ + N - ptr_);
		if (available_bytes >= aligned_n) {
			char* r = ptr_;
			ptr_ += aligned_n;
			return r;
		}
		throw std::overflow_error("할당 범위를 초과합니다");
	}
	auto deallocate(char* p, size_t n) noexcept -> void {
		if (pointer_in_buffer(p))
		{
			n = align_up(p);
			if (p + n == ptr_) {
				ptr_ = p;
			}
		}
		else
		{
			throw std::invalid_argument("범위 밖의 포인터입니다");
		}
	}
private:
	static auto align_up(size_t n) noexcept -> size_t {
		return (n + (alignment - 1)) & ~(alignment - 1);
	}
	auto pointer_in_buffer(const char* p) const noexcept -> bool {
		return buffer_ <= p && p <= buffer_ + N;
	}
	alignas(alignment) char buffer_[N];
	char* ptr_{};
};

template <class T, size_t N>
struct StaticAlloc
{
	using value_type = T;
	using arena_type = Arena<N>;

	StaticAlloc(const StaticAlloc&) = delete;
	StaticAlloc& operator=(const StaticAlloc&) = delete;

	StaticAlloc(arena_type& arena) noexcept : arena_{ arena } {}

	template <class U>
	StaticAlloc(const StaticAlloc<U, N>& other) noexcept : arena_{ other.arena } {}

	template <class U> struct rebind {
		using other = StaticAlloc<U, N>;
	};

	auto allocate(size_t n) -> T* {
		return reinterpret_cast<T*>(arena_.allocate(n * sizeof(T)));
	}

	auto deallocate(T* p, size_t n) noexcept -> void {
		arena_.deallocate(reinterpret_cast<char*>(p), n * sizeof(T));
	}

	template <class U, size_t M>
	auto operator==(const StaticAlloc<U, M>& other) const noexcept {
		return N == M && std::addressof(arena_) == std::addressof(other.arena_);
	}
	template<class U, size_t M>
	auto operator!=(const StaticAlloc<U, M>& other) const noexcept {
		return !(*this == other);
	}
	template <class U, size_t M> friend struct StaticAlloc;
private:
	arena_type& arena_;
};