#include "../include/connection.h"

namespace is::signals
{
namespace
{

auto get_advanced_connection_impl(const advanced_connection& connection) noexcept
{
	struct advanced_connection_impl_getter : private advanced_connection
	{
		advanced_connection_impl_getter(const advanced_connection& connection) noexcept
			: advanced_connection(connection)
		{
		}
		using advanced_connection::m_impl;
	};
	return advanced_connection_impl_getter(connection).m_impl;
}

} // namespace

connection::connection(connection&& other) noexcept
	: m_storage(other.m_storage)
	, m_id(other.m_id)
{
	other.m_storage.reset();
	other.m_id = 0;
}

connection::connection(detail::signal_impl_weak_ptr storage, uint64_t id) noexcept
	: m_storage(std::move(storage))
	, m_id(id)
{
}

connection::connection() noexcept = default;

connection::connection(const connection& other) noexcept = default;

connection& connection::operator=(connection&& other) noexcept
{
	m_storage = other.m_storage;
	m_id = other.m_id;
	other.m_storage.reset();
	other.m_id = 0;
	return *this;
}

connection& connection::operator=(const connection& other) noexcept = default;

bool connection::connected() const noexcept
{
	return (m_id != 0);
}

void connection::disconnect() noexcept
{
	if (auto storage = m_storage.lock())
	{
		storage->remove(m_id);
		m_storage.reset();
	}
	m_id = 0;
}

scoped_connection::scoped_connection(connection&& conn) noexcept
	: connection(std::move(conn))
{
}

scoped_connection::scoped_connection(const connection& conn) noexcept
	: connection(conn)
{
}

scoped_connection::scoped_connection() noexcept = default;

scoped_connection::scoped_connection(scoped_connection&& other) noexcept = default;

scoped_connection& scoped_connection::operator=(scoped_connection&& other) noexcept
{
	disconnect();
	static_cast<connection&>(*this) = std::move(other);
	return *this;
}

scoped_connection::~scoped_connection()
{
	disconnect();
}

connection scoped_connection::release() noexcept
{
	connection conn = std::move(static_cast<connection&>(*this));
	return conn;
}

bool advanced_connection::advanced_connection_impl::is_blocked() const noexcept
{
	return m_blockCounter.load(std::memory_order_acquire) != 0;
}

void advanced_connection::advanced_connection_impl::block() noexcept
{
	++m_blockCounter;
}

void advanced_connection::advanced_connection_impl::unblock() noexcept
{
	--m_blockCounter;
}

advanced_connection::advanced_connection() noexcept = default;

advanced_connection::advanced_connection(connection&& conn, impl_ptr&& impl) noexcept
	: connection(std::move(conn))
	, m_impl(std::move(impl))
{
}

advanced_connection::advanced_connection(const advanced_connection&) noexcept = default;

advanced_connection::advanced_connection(advanced_connection&& other) noexcept = default;

advanced_connection& advanced_connection::operator=(const advanced_connection&) noexcept = default;

advanced_connection& advanced_connection::operator=(advanced_connection&& other) noexcept = default;

shared_connection_block::shared_connection_block(const advanced_connection& connection, bool initially_blocked) noexcept
	: m_connection(get_advanced_connection_impl(connection))
{
	if (initially_blocked)
	{
		block();
	}
}

shared_connection_block::shared_connection_block(const shared_connection_block& other) noexcept
	: m_connection(other.m_connection)
	, m_blocked(other.m_blocked.load(std::memory_order_acquire))
{
	increment_if_blocked();
}

shared_connection_block::shared_connection_block(shared_connection_block&& other) noexcept
	: m_connection(other.m_connection)
	, m_blocked(other.m_blocked.load(std::memory_order_acquire))
{
	other.m_connection.reset();
	other.m_blocked.store(false, std::memory_order_release);
}

shared_connection_block& shared_connection_block::operator=(const shared_connection_block& other) noexcept
{
	if (&other != this)
	{
		unblock();
		m_connection = other.m_connection;
		m_blocked = other.m_blocked.load(std::memory_order_acquire);
		increment_if_blocked();
	}
	return *this;
}

shared_connection_block& shared_connection_block::operator=(shared_connection_block&& other) noexcept
{
	if (&other != this)
	{
		unblock();
		m_connection = other.m_connection;
		m_blocked = other.m_blocked.load(std::memory_order_acquire);
		other.m_connection.reset();
		other.m_blocked.store(false, std::memory_order_release);
	}
	return *this;
}

shared_connection_block::~shared_connection_block()
{
	unblock();
}

void shared_connection_block::block() noexcept
{
	bool blocked = false;
	if (m_blocked.compare_exchange_strong(blocked, true, std::memory_order_acq_rel, std::memory_order_relaxed))
	{
		if (auto connection = m_connection.lock())
		{
			connection->block();
		}
	}
}

void shared_connection_block::unblock() noexcept
{
	bool blocked = true;
	if (m_blocked.compare_exchange_strong(blocked, false, std::memory_order_acq_rel, std::memory_order_relaxed))
	{
		if (auto connection = m_connection.lock())
		{
			connection->unblock();
		}
	}
}

bool shared_connection_block::blocking() const noexcept
{
	return m_blocked;
}

void shared_connection_block::increment_if_blocked() const noexcept
{
	if (m_blocked)
	{
		if (auto connection = m_connection.lock())
		{
			connection->block();
		}
	}
}

advanced_scoped_connection::advanced_scoped_connection() noexcept = default;

advanced_scoped_connection::advanced_scoped_connection(const advanced_connection& conn) noexcept
	: advanced_connection(conn)
{
}

advanced_scoped_connection::advanced_scoped_connection(advanced_connection&& conn) noexcept
	: advanced_connection(std::move(conn))
{
}

advanced_scoped_connection::advanced_scoped_connection(advanced_scoped_connection&& other) noexcept = default;

advanced_scoped_connection& advanced_scoped_connection::operator=(advanced_scoped_connection&& other) noexcept = default;

advanced_scoped_connection::~advanced_scoped_connection()
{
	disconnect();
}

advanced_connection advanced_scoped_connection::release() noexcept
{
	advanced_connection conn = std::move(static_cast<advanced_connection&>(*this));
	return conn;
}

} // namespace is::signals
