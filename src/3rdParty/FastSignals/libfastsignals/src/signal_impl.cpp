#include "../include/signal_impl.h"
#include <algorithm>
#include <mutex>

namespace is::signals::detail
{

uint64_t signal_impl::add(packed_function fn)
{
	std::lock_guard lock(m_mutex);

	m_functions.emplace_back(std::move(fn));

	try
	{
		m_ids.emplace_back(m_nextId);
	}
	catch (const std::bad_alloc& /*e*/)
	{
		// Remove function since we failed to add its id
		m_functions.pop_back();
		throw;
	}

	return m_nextId++;
}

void signal_impl::remove(uint64_t id) noexcept
{
	std::lock_guard lock(m_mutex);

	// We use binary search because ids array is always sorted.
	auto it = std::lower_bound(m_ids.begin(), m_ids.end(), id);
	if (it != m_ids.end() && *it == id)
	{
		size_t i = std::distance(m_ids.begin(), it);
		m_ids.erase(m_ids.begin() + i);
		m_functions.erase(m_functions.begin() + i);
	}
}

void signal_impl::remove_all() noexcept
{
	std::lock_guard lock(m_mutex);
	m_functions.clear();
	m_ids.clear();
}

bool signal_impl::get_next_slot(packed_function& slot, size_t& expectedIndex, uint64_t& nextId) const
{
	// Slots always arranged by ID, so we can use a simple algorithm which avoids races:
	//  - on each step find first slot with ID >= slotId
	//  - after each call increment slotId

	std::lock_guard lock(m_mutex);

	// Avoid binary search if next slot wasn't moved between mutex locks.
	if (expectedIndex >= m_ids.size() || m_ids[expectedIndex] != nextId)
	{
		auto it = (nextId < m_nextId)
			? std::lower_bound(m_ids.cbegin(), m_ids.cend(), nextId)
			: m_ids.end();
		if (it == m_ids.end())
		{
			return false;
		}
		expectedIndex = std::distance(m_ids.cbegin(), it);
	}

	slot.reset();
	slot = m_functions[expectedIndex];
	nextId = (expectedIndex + 1 < m_ids.size()) ? m_ids[expectedIndex + 1] : m_ids[expectedIndex] + 1;
	++expectedIndex;
	return true;
}

size_t signal_impl::count() const noexcept
{
	std::lock_guard lock(m_mutex);

	return m_functions.size();
}

} // namespace is::signals::detail
