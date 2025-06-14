#pragma once

#include "function_detail.h"
#include "spin_mutex.h"
#include <memory>
#include <vector>

namespace is::signals::detail
{

class signal_impl
{
public:
	uint64_t add(packed_function fn);

	void remove(uint64_t id) noexcept;

	void remove_all() noexcept;

	size_t count() const noexcept;

	template <class Combiner, class Result, class Signature, class... Args>
	Result invoke(Args... args) const
	{
		packed_function slot;
		size_t slotIndex = 0;
		uint64_t slotId = 1;

		if constexpr (std::is_same_v<Result, void>)
		{
			while (get_next_slot(slot, slotIndex, slotId))
			{
				slot.get<Signature>()(std::forward<Args>(args)...);
			}
		}
		else
		{
			Combiner combiner;
			while (get_next_slot(slot, slotIndex, slotId))
			{
				combiner(slot.get<Signature>()(std::forward<Args>(args)...));
			}
			return combiner.get_value();
		}
	}

private:
	bool get_next_slot(packed_function& slot, size_t& expectedIndex, uint64_t& nextId) const;

	mutable spin_mutex m_mutex;
	std::vector<packed_function> m_functions;
	std::vector<uint64_t> m_ids;
	uint64_t m_nextId = 1;
};

using signal_impl_ptr = std::shared_ptr<signal_impl>;
using signal_impl_weak_ptr = std::weak_ptr<signal_impl>;

} // namespace is::signals::detail
