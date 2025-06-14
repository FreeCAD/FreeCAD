#pragma once

#include <optional>

namespace is::signals
{

/**
 * This results combiner reduces results collection into last value of this collection.
 * In other words, it keeps only result of the last slot call.
 */
template <class T>
class optional_last_value
{
public:
	using result_type = std::optional<T>;

	template <class TRef>
	void operator()(TRef&& value)
	{
		m_result = std::forward<TRef>(value);
	}

	result_type get_value() const
	{
		return m_result;
	}

private:
	result_type m_result = {};
};

template <>
class optional_last_value<void>
{
public:
	using result_type = void;
};

} // namespace is::signals
