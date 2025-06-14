#include "../include/function_detail.h"
#include <cstddef>
#include <functional>

namespace is::signals::detail
{

packed_function::packed_function(packed_function&& other) noexcept
	: m_proxy(move_proxy_from(std::move(other)))
{
}

packed_function::packed_function(const packed_function& other)
	: m_proxy(clone_proxy_from(other))
{
}

packed_function& packed_function::operator=(packed_function&& other) noexcept
{
	assert(this != &other);
	reset();
	m_proxy = move_proxy_from(std::move(other));
	return *this;
}

base_function_proxy* packed_function::move_proxy_from(packed_function&& other) noexcept
{
	auto proxy = other.m_proxy ? other.m_proxy->move(&m_buffer) : nullptr;
	other.m_proxy = nullptr;
	return proxy;
}

base_function_proxy* packed_function::clone_proxy_from(const packed_function& other)
{
	return other.m_proxy ? other.m_proxy->clone(&m_buffer) : nullptr;
}

packed_function& packed_function::operator=(const packed_function& other)
{
	if (this != &other)
	{
		if (other.is_buffer_allocated() && is_buffer_allocated())
		{
			// "This" and "other" are using SBO. Safe assignment must use copy+move
			*this = packed_function(other);
		}
		else
		{
			// Buffer is used either by "this" or by "other" or not used at all.
			// If this uses buffer then other's proxy is null or allocated on heap, so clone won't overwrite buffer
			// If this uses heap or null then other's proxy can safely use buffer because reset() won't access buffer
			auto newProxy = clone_proxy_from(other);
			reset();
			m_proxy = newProxy;
		}
	}
	return *this;
}

packed_function::~packed_function() noexcept
{
	reset();
}

void packed_function::reset() noexcept
{
	if (m_proxy != nullptr)
	{
		if (is_buffer_allocated())
		{
			m_proxy->~base_function_proxy();
		}
		else
		{
			delete m_proxy;
		}
		m_proxy = nullptr;
	}
}

base_function_proxy& packed_function::unwrap() const
{
	if (m_proxy == nullptr)
	{
		throw std::bad_function_call();
	}
	return *m_proxy;
}

bool packed_function::is_buffer_allocated() const noexcept
{
	return std::less_equal<const void*>()(&m_buffer[0], m_proxy)
		&& std::less<const void*>()(m_proxy, &m_buffer[1]);
}

} // namespace is::signals::detail
