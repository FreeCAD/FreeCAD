#include "catch2/catch.hpp"
#include "libfastsignals/include/bind_weak.h"

using namespace is::signals;

namespace
{
class Testbed
{
public:
	Testbed(unsigned& counter)
		: m_pCounter(&counter)
	{
	}

	void IncrementNonConst()
	{
		++(*m_pCounter);
	}

	void IncrementsConst() const
	{
		++(*m_pCounter);
	}

	int ReflectInt(int value) const
	{
		return value;
	}

private:
	unsigned* m_pCounter = nullptr;
};
} // namespace

TEST_CASE("can bind const methods", "[bind_weak]")
{
	unsigned counter = 0;
	auto pSharedBed = std::make_shared<Testbed>(counter);
	auto boundFn = bind_weak(&Testbed::IncrementNonConst, pSharedBed);
	REQUIRE(counter == 0u);
	boundFn();
	REQUIRE(counter == 1u);
	boundFn();
	REQUIRE(counter == 2u);
	pSharedBed = nullptr;
	boundFn();
	REQUIRE(counter == 2u);
	boundFn();
	REQUIRE(counter == 2u);
}

TEST_CASE("can bind non const methods", "[bind_weak]")
{
	unsigned counter = 0;
	auto pSharedBed = std::make_shared<Testbed>(counter);
	auto boundFn = bind_weak(&Testbed::IncrementsConst, pSharedBed);
	REQUIRE(counter == 0u);
	boundFn();
	REQUIRE(counter == 1u);
	boundFn();
	REQUIRE(counter == 2u);
	pSharedBed = nullptr;
	boundFn();
	REQUIRE(counter == 2u);
	boundFn();
	REQUIRE(counter == 2u);
}

TEST_CASE("can bind method with argument value", "[bind_weak]")
{
	unsigned counter = 0;
	auto pSharedBed = std::make_shared<Testbed>(counter);
	auto boundFn = bind_weak(&Testbed::ReflectInt, pSharedBed, 42);
	REQUIRE(boundFn() == 42);
	REQUIRE(boundFn() == 42);
	pSharedBed = nullptr;
	REQUIRE(boundFn() == 0);
	REQUIRE(boundFn() == 0);
}

TEST_CASE("copies value when bind method with argument const reference value", "[bind_weak]")
{
	unsigned counter = 0;
	auto pSharedBed = std::make_shared<Testbed>(counter);
	auto makeBoundFn = [&]() {
		int value = 15;
		const int& valueRef = value;
		auto result = bind_weak(&Testbed::ReflectInt, pSharedBed, valueRef);
		value = 25;
		return result;
	};

	auto boundFn = makeBoundFn();
	REQUIRE(boundFn(42) == 15);
	REQUIRE(boundFn(42) == 15);
	pSharedBed = nullptr;
	REQUIRE(boundFn(42) == 0);
	REQUIRE(boundFn(42) == 0);
}

TEST_CASE("copies value when bind method with argument reference value", "[bind_weak]")
{
	unsigned counter = 0;
	auto pSharedBed = std::make_shared<Testbed>(counter);
	auto makeBoundFn = [&]() {
		int value = 15;
		int& valueRef = value;
		auto result = bind_weak(&Testbed::ReflectInt, pSharedBed, valueRef);
		valueRef = 25;
		return result;
	};

	auto boundFn = makeBoundFn();
	REQUIRE(boundFn(42) == 15);
	REQUIRE(boundFn(42) == 15);
	pSharedBed = nullptr;
	REQUIRE(boundFn(42) == 0);
	REQUIRE(boundFn(42) == 0);
}

TEST_CASE("can bind method with placeholder", "[bind_weak]")
{
	unsigned counter = 0;
	auto pSharedBed = std::make_shared<Testbed>(counter);
	auto boundFn = bind_weak(&Testbed::ReflectInt, pSharedBed, std::placeholders::_1);
	REQUIRE(boundFn(42) == 42);
	REQUIRE(boundFn(42) == 42);
	pSharedBed = nullptr;
	REQUIRE(boundFn(42) == 0);
	REQUIRE(boundFn(42) == 0);
}
