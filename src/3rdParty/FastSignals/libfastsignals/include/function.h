#pragma once

#include "function_detail.h"

namespace is::signals
{
// Derive your class from not_directly_callable to prevent function from wrapping it using its template constructor
// Useful if your class provides custom operator for casting to function
struct not_directly_callable
{
};

template <class Fn, class Function, class Return, class... Arguments>
using enable_if_callable_t = typename std::enable_if_t<
	!std::is_same_v<std::decay_t<Fn>, Function> && !std::is_base_of_v<not_directly_callable, std::decay_t<Fn>> && std::is_same_v<std::invoke_result_t<Fn, Arguments...>, Return>>;

template <class Signature>
class function;

// Compact function class - causes minimal code bloat when compiled.
// Replaces std::function in this library.
template <class Return, class... Arguments>
class function<Return(Arguments...)>
{
public:
	function() = default;

	function(const function& other) = default;
	function(function&& other) noexcept = default;
	function& operator=(const function& other) = default;
	function& operator=(function&& other) noexcept = default;

	template <class Fn, typename = enable_if_callable_t<Fn, function<Return(Arguments...)>, Return, Arguments...>>
	function(Fn&& function) noexcept(detail::is_noexcept_packed_function_init<Fn, Return, Arguments...>)
	{
		m_packed.init<Fn, Return, Arguments...>(std::forward<Fn>(function));
	}

	Return operator()(Arguments&&... args) const
	{
		auto& proxy = m_packed.get<Return(Arguments...)>();
		return proxy(std::forward<Arguments>(args)...);
	}

	detail::packed_function release() noexcept
	{
		return std::move(m_packed);
	}

private:
	detail::packed_function m_packed;
};

} // namespace is::signals
