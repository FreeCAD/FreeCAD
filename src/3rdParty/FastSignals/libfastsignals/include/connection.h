#pragma once

#include "signal_impl.h"

namespace is::signals
{

// Connection keeps link between signal and slot and can disconnect them.
// Disconnect operation is thread-safe: any thread can disconnect while
//  slots called on other thread.
// This class itself is not thread-safe: you can't use the same connection
//  object from different threads at the same time.
class connection
{
public:
	connection() noexcept;
	explicit connection(detail::signal_impl_weak_ptr storage, uint64_t id) noexcept;
	connection(const connection& other) noexcept;
	connection& operator=(const connection& other) noexcept;
	connection(connection&& other) noexcept;
	connection& operator=(connection&& other) noexcept;

	bool connected() const noexcept;
	void disconnect() noexcept;

protected:
	detail::signal_impl_weak_ptr m_storage;
	uint64_t m_id = 0;
};

// Connection class that supports blocking callback execution
class advanced_connection : public connection
{
public:
	struct advanced_connection_impl
	{
		void block() noexcept;
		void unblock() noexcept;
		bool is_blocked() const noexcept;

	private:
		std::atomic<int> m_blockCounter = ATOMIC_VAR_INIT(0);
	};
	using impl_ptr = std::shared_ptr<advanced_connection_impl>;

	advanced_connection() noexcept;
	explicit advanced_connection(connection&& conn, impl_ptr&& impl) noexcept;
	advanced_connection(const advanced_connection&) noexcept;
	advanced_connection& operator=(const advanced_connection&) noexcept;
	advanced_connection(advanced_connection&& other) noexcept;
	advanced_connection& operator=(advanced_connection&& other) noexcept;

protected:
	impl_ptr m_impl;
};

// Blocks advanced connection, so its callback will not be executed
class shared_connection_block
{
public:
	shared_connection_block(const advanced_connection& connection = advanced_connection(), bool initially_blocked = true) noexcept;
	shared_connection_block(const shared_connection_block& other) noexcept;
	shared_connection_block(shared_connection_block&& other) noexcept;
	shared_connection_block& operator=(const shared_connection_block& other) noexcept;
	shared_connection_block& operator=(shared_connection_block&& other) noexcept;
	~shared_connection_block();

	void block() noexcept;
	void unblock() noexcept;
	bool blocking() const noexcept;

private:
	void increment_if_blocked() const noexcept;

	std::weak_ptr<advanced_connection::advanced_connection_impl> m_connection;
	std::atomic<bool> m_blocked = ATOMIC_VAR_INIT(false);
};

// Scoped connection keeps link between signal and slot and disconnects them in destructor.
// Scoped connection is movable, but not copyable.
class scoped_connection : public connection
{
public:
	scoped_connection() noexcept;
	scoped_connection(const connection& conn) noexcept;
	scoped_connection(connection&& conn) noexcept;
	scoped_connection(const advanced_connection& conn) = delete;
	scoped_connection(advanced_connection&& conn) noexcept = delete;
	scoped_connection(const scoped_connection&) = delete;
	scoped_connection& operator=(const scoped_connection&) = delete;
	scoped_connection(scoped_connection&& other) noexcept;
	scoped_connection& operator=(scoped_connection&& other) noexcept;
	~scoped_connection();

	connection release() noexcept;
};

// scoped connection for advanced connections
class advanced_scoped_connection : public advanced_connection
{
public:
	advanced_scoped_connection() noexcept;
	advanced_scoped_connection(const advanced_connection& conn) noexcept;
	advanced_scoped_connection(advanced_connection&& conn) noexcept;
	advanced_scoped_connection(const advanced_scoped_connection&) = delete;
	advanced_scoped_connection& operator=(const advanced_scoped_connection&) = delete;
	advanced_scoped_connection(advanced_scoped_connection&& other) noexcept;
	advanced_scoped_connection& operator=(advanced_scoped_connection&& other) noexcept;
	~advanced_scoped_connection();

	advanced_connection release() noexcept;
};

} // namespace is::signals
