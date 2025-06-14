#pragma once
#include <atomic>

namespace is::signals::detail
{

class spin_mutex
{
public:
	spin_mutex() = default;
	spin_mutex(const spin_mutex&) = delete;
	spin_mutex& operator=(const spin_mutex&) = delete;
	spin_mutex(spin_mutex&&) = delete;
	spin_mutex& operator=(spin_mutex&&) = delete;

	inline bool try_lock() noexcept
	{
		return !m_busy.test_and_set(std::memory_order_acquire);
	}

	inline void lock() noexcept
	{
		while (!try_lock())
		{
			/* do nothing */;
		}
	}

	inline void unlock() noexcept
	{
		m_busy.clear(std::memory_order_release);
	}

private:
	std::atomic_flag m_busy = ATOMIC_FLAG_INIT;
};

} // namespace is::signals::detail
