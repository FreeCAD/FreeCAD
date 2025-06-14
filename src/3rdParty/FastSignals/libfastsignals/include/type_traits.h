#pragma once

namespace is::signals
{
namespace detail
{

template <typename T>
struct signal_arg
{
	using type = const T&;
};

template <typename U>
struct signal_arg<U&>
{
	using type = U&;
};
} // namespace detail

template <typename T>
using signal_arg_t = typename detail::signal_arg<T>::type;

} // namespace is::signals
