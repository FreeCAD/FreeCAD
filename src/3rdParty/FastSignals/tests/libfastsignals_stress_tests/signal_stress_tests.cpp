#include "catch2/catch.hpp"
#include <fastsignals/signal.h>
#include <array>
#include <mutex>
#include <random>
#include <vector>
#include <thread>

using namespace fastsignals;

namespace
{
using string_signal = signal<void(std::string)>;
using string_slot = string_signal::slot_type;
using void_signal = signal<void()>;
using void_slot = void_signal::slot_type;

class named_entity
{
public:
	std::string name() const
	{
		std::lock_guard lock(m_nameMutex);
		return m_name;
	}

	void fire_changed(std::string value)
	{
		bool fire = false;
		{
			std::lock_guard lock(m_nameMutex);
			if (m_name != value)
			{
				m_name = std::move(value);
				fire = true;
			}
		}
		if (fire)
		{
			m_nameChanged(value);
		}
	}

	connection on_name_changed(string_slot slot)
	{
		return m_nameChanged.connect(std::move(slot));
	}

private:
	mutable std::mutex m_nameMutex;
	std::string m_name;
	signal<void(std::string)> m_nameChanged;
};

unsigned get_next_seed()
{
	static std::minstd_rand seedEngine(777);
	return seedEngine();
}

size_t get_random_index(size_t size)
{
	thread_local std::minstd_rand disconnectRandomEngine{ get_next_seed() };
	std::uniform_int_distribution<size_t> disconnectIndexDistribution{ 0, size - 1 };

	return disconnectIndexDistribution(disconnectRandomEngine);
}
} // namespace

TEST_CASE("Can work in a few threads", "[signal]")
{
	constexpr unsigned fireThreadCount = 8;
	constexpr unsigned signalsCount = 7;
	constexpr unsigned fireCountPerThread = 100'000;
	constexpr unsigned connectCallsCount = 80'000;
	constexpr unsigned totalRunCount = 10;

	for (unsigned i = 0; i < totalRunCount; ++i)
	{
		std::array<void_signal, signalsCount> signals;

		std::mutex connectionsMutex;
		std::vector<connection> connections;
		connections.reserve(connectCallsCount);

		std::vector<std::thread> threads;

		auto slot = [&] {
			std::lock_guard lock(connectionsMutex);
			if (!connections.empty())
			{
				const size_t index = get_random_index(connections.size());
				connections.at(index).disconnect();
			}
		};

		threads.emplace_back([&] {
			for (unsigned cci = 0; cci < connectCallsCount; ++cci)
			{
				const size_t index = get_random_index(signalsCount);
				connection conn = signals.at(index).connect(slot);
				{
					std::lock_guard lock(connectionsMutex);
					connections.emplace_back(conn);
				}
			}
		});

		for (unsigned fti = 0; fti < fireThreadCount; ++fti)
		{
			threads.emplace_back([&] {
				for (unsigned fi = 0; fi < fireCountPerThread; ++fi)
				{
					const size_t index = get_random_index(signalsCount);
					signals.at(index)();
				}
			});
		}
		for (auto& thread : threads)
		{
			thread.join();
		}
	}
}
