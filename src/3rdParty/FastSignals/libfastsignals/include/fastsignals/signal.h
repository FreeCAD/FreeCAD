#pragma once

#include "combiners.h"
#include "connection.h"
#include "function.h"
#include "signal_impl.h"
#include "type_traits.h"
#include <type_traits>

#if defined(_MSC_VER)
#	include "msvc_autolink.h"
#endif

namespace is::signals
{
template <class Signature, template <class T> class Combiner = optional_last_value>
class signal;

struct advanced_tag
{
};

/// Signal allows to fire events to many subscribers (slots).
/// In other words, it implements one-to-many relation between event and listeners.
/// Signal implements observable object from Observable pattern.
template <class Return, class... Arguments, template <class T> class Combiner>
class signal<Return(Arguments...), Combiner> : private not_directly_callable
{
public:
	using signature_type = Return(signal_arg_t<Arguments>...);
	using slot_type = function<signature_type>;
	using combiner_type = Combiner<Return>;
	using result_type = typename combiner_type::result_type;

	signal()
		: m_slots(std::make_shared<detail::signal_impl>())
	{
	}

	/// No copy construction
	signal(const signal&) = delete;

	/// Moves signal from other. Any operations on other except destruction, move, and swap are invalid
	signal(signal&& other) = default;

	/// No copy assignment
	signal& operator=(const signal&) = delete;

	/// Moves signal from other. Any operations on other except destruction, move, and swap are invalid
	signal& operator=(signal&& other) = default;

	/**
	 * connect(slot) method subscribes slot to signal emission event.
	 * Each time you call signal as functor, all slots are also called with given arguments.
	 * @returns connection - object which manages signal-slot connection lifetime
	 */
	connection connect(slot_type slot)
	{
		const uint64_t id = m_slots->add(slot.release());
		return connection(m_slots, id);
	}

	/**
	 * connect(slot, advanced_tag) method subscribes slot to signal emission event with the ability to temporarily block slot execution
	 * Each time you call signal as functor, all non-blocked slots are also called with given arguments.
	 * You can temporarily block slot execution using shared_connection_block
	 * @returns advanced_connection - object which manages signal-slot connection lifetime
	 */
	advanced_connection connect(slot_type slot, advanced_tag)
	{
		static_assert(std::is_void_v<Return>, "Advanced connect can only be used with slots returning void (implementation limitation)");
		auto conn_impl = std::make_shared<advanced_connection::advanced_connection_impl>();
		slot_type slot_impl = [this, slot, weak_conn_impl = std::weak_ptr(conn_impl)](signal_arg_t<Arguments>... args) {
			auto conn_impl = weak_conn_impl.lock();
			if (!conn_impl || !conn_impl->is_blocked())
			{
				slot(args...);
			}
		};
		auto conn = connect(std::move(slot_impl));
		return advanced_connection(std::move(conn), std::move(conn_impl));
	}

	/**
	 * disconnect_all_slots() method disconnects all slots from signal emission event.
	 */
	void disconnect_all_slots() noexcept
	{
		m_slots->remove_all();
	}

	/**
	 * num_slots() method returns number of slots attached to this singal
	 */
	[[nodiscard]] std::size_t num_slots() const noexcept
	{
		return m_slots->count();
	}

	/**
	 * empty() method returns true if signal has any slots attached
	 */
	[[nodiscard]] bool empty() const noexcept
	{
		return m_slots->count() == 0;
	}

	/**
	 * operator(args...) calls all slots connected to this signal.
	 * Logically, it fires signal emission event.
	 */
	result_type operator()(signal_arg_t<Arguments>... args) const
	{
		return detail::signal_impl_ptr(m_slots)->invoke<combiner_type, result_type, signature_type, signal_arg_t<Arguments>...>(args...);
	}

	void swap(signal& other) noexcept
	{
		m_slots.swap(other.m_slots);
	}

	/**
	 * Allows using signals as slots for another signal
	 */
	operator slot_type() const noexcept
	{
		return [weakSlots = detail::signal_impl_weak_ptr(m_slots)](signal_arg_t<Arguments>... args) {
			if (auto slots = weakSlots.lock())
			{
				return slots->invoke<combiner_type, result_type, signature_type, signal_arg_t<Arguments>...>(args...);
			}
		};
	}

private:
	detail::signal_impl_ptr m_slots;
};

} // namespace is::signals

namespace std
{

// free swap function, findable by ADL
template <class Signature, template <class T> class Combiner>
void swap(
	::is::signals::signal<Signature, Combiner>& sig1,
	::is::signals::signal<Signature, Combiner>& sig2)
{
	sig1.swap(sig2);
}

} // namespace std
