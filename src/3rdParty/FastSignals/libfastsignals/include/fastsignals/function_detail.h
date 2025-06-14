#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace is::signals::detail
{
/// Buffer for callable object in-place construction,
/// helps to implement Small Buffer Optimization.
static constexpr size_t inplace_buffer_size = (sizeof(int) == sizeof(void*) ? 8 : 6) * sizeof(void*);

/// Structure that has size enough to keep type "T" including vtable.
template <class T>
struct type_container
{
	T data;
};

/// Type that has enough space to keep type "T" (including vtable) with suitable alignment.
using function_buffer_t = std::aligned_storage_t<inplace_buffer_size>;

/// Constantly is true if callable fits function buffer, false otherwise.
template <class T>
inline constexpr bool fits_inplace_buffer = (sizeof(type_container<T>) <= inplace_buffer_size);

// clang-format off
/// Constantly is true if callable fits function buffer and can be safely moved, false otherwise
template <class T>
inline constexpr bool can_use_inplace_buffer = 
	fits_inplace_buffer<T> && 
	std::is_nothrow_move_constructible_v<T>;
// clang format on

/// Type that is suitable to keep copy of callable object.
///  - equal to pointer-to-function if Callable is pointer-to-function
///  - otherwise removes const/volatile and references to allow copying callable.
template <class Callable>
using callable_copy_t = std::conditional_t<std::is_function_v<std::remove_reference_t<Callable>>,
	Callable,
	std::remove_cv_t<std::remove_reference_t<Callable>>>;

class base_function_proxy
{
public:
	virtual ~base_function_proxy() = default;
	virtual base_function_proxy* clone(void* buffer) const = 0;
	virtual base_function_proxy* move(void* buffer) noexcept = 0;
};

template <class Signature>
class function_proxy;

template <class Return, class... Arguments>
class function_proxy<Return(Arguments...)> : public base_function_proxy
{
public:
	virtual Return operator()(Arguments&&...) = 0;
};

template <class Callable, class Return, class... Arguments>
class function_proxy_impl final : public function_proxy<Return(Arguments...)>
{
public:
	// If you see this error, probably your function returns value and you're trying to
	//  connect it to `signal<void(...)>`. Just remove return value from callback.
	static_assert(std::is_same_v<std::invoke_result_t<Callable, Arguments...>, Return>,
		"cannot construct function<> class from callable object with different return type");

	template <class FunctionObject>
	explicit function_proxy_impl(FunctionObject&& function)
		: m_callable(std::forward<FunctionObject>(function))
	{
	}

	Return operator()(Arguments&&... args) final
	{
		return m_callable(std::forward<Arguments>(args)...);
	}

	base_function_proxy* clone(void* buffer) const final
	{
		if constexpr (can_use_inplace_buffer<function_proxy_impl>)
		{
			return new (buffer) function_proxy_impl(*this);
		}
		else
		{
			(void)buffer;
			return new function_proxy_impl(*this);
		}
	}

	base_function_proxy* move(void* buffer) noexcept final
	{
		if constexpr (can_use_inplace_buffer<function_proxy_impl>)
		{
			base_function_proxy* moved = new (buffer) function_proxy_impl(std::move(*this));
			this->~function_proxy_impl();
			return moved;
		}
		else
		{
			(void)buffer;
			return this;
		}
	}

private:
	callable_copy_t<Callable> m_callable;
};

template <class Fn, class Return, class... Arguments>
inline constexpr bool is_noexcept_packed_function_init = can_use_inplace_buffer<function_proxy_impl<Fn, Return, Arguments...>>;

class packed_function final
{
public:
	packed_function() = default;
	packed_function(packed_function&& other) noexcept;
	packed_function(const packed_function& other);
	packed_function& operator=(packed_function&& other) noexcept;
	packed_function& operator=(const packed_function& other);
	~packed_function() noexcept;

	// Initializes packed function.
	// Cannot be called without reset().
	template <class Callable, class Return, class... Arguments>
	void init(Callable&& function) noexcept(is_noexcept_packed_function_init<Callable, Return, Arguments...>)
	{
		using proxy_t = function_proxy_impl<Callable, Return, Arguments...>;

		assert(m_proxy == nullptr);
		if constexpr (can_use_inplace_buffer<proxy_t>)
		{
			m_proxy = new (&m_buffer) proxy_t{ std::forward<Callable>(function) };
		}
		else
		{
			m_proxy = new proxy_t{ std::forward<Callable>(function) };
		}
	}

	template <class Signature>
	function_proxy<Signature>& get() const
	{
		return static_cast<function_proxy<Signature>&>(unwrap());
	}

	void reset() noexcept;

private:
	base_function_proxy* move_proxy_from(packed_function&& other) noexcept;
	base_function_proxy* clone_proxy_from(const packed_function &other);
	base_function_proxy& unwrap() const;
	bool is_buffer_allocated() const noexcept;

	function_buffer_t m_buffer[1] = {};
	base_function_proxy* m_proxy = nullptr;
};

} // namespace is::signals::detail
