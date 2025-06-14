#include "catch2/catch.hpp"
#include "libfastsignals/include/function.h"
#include <array>

using namespace is::signals;

namespace
{
int Abs(int x)
{
	return x >= 0 ? x : -x;
}

int Sum(int a, int b)
{
	return a + b;
}

void InplaceAbs(int& x)
{
	x = Abs(x);
}

std::string GetStringHello()
{
	return "hello";
}

class AbsFunctor
{
public:
	int operator()(int x) const
	{
		return Abs(x);
	}
};

class SumFunctor
{
public:
	int operator()(int a, int b) const
	{
		return Sum(a, b);
	}
};

class InplaceAbsFunctor
{
public:
	void operator()(int& x) /* non-const */
	{
		if (m_calledOnce)
		{
			abort();
		}
		m_calledOnce = true;
		InplaceAbs(x);
	}

private:
	bool m_calledOnce = false;
};

class GetStringFunctor
{
public:
	explicit GetStringFunctor(const std::string& value)
		: m_value(value)
	{
	}

	std::string operator()() /* non-const */
	{
		if (m_calledOnce)
		{
			abort();
		}
		m_calledOnce = true;
		return m_value;
	}

private:
	bool m_calledOnce = false;
	std::string m_value;
};
} // namespace

TEST_CASE("Can use free function with 1 argument", "[function]")
{
	function<int(int)> fn = Abs;
	REQUIRE(fn(10) == 10);
	REQUIRE(fn(-10) == 10);
	REQUIRE(fn(0) == 0);
}

TEST_CASE("Can use free function with 2 arguments", "[function]")
{
	function<int(int, int)> fn = Sum;
	REQUIRE(fn(10, 5) == 15);
	REQUIRE(fn(-10, 0) == -10);
}

TEST_CASE("Can use free function without arguments", "[function]")
{
	function<std::string()> fn = GetStringHello;
	REQUIRE(fn() == "hello");
}

TEST_CASE("Can use free function without return value", "[function]")
{
	function<void(int&)> fn = InplaceAbs;
	int a = -10;
	fn(a);
	REQUIRE(a == 10);
}

TEST_CASE("Can use lambda with 1 argument", "[function]")
{
	function<int(int)> fn = [](int value) {
		return Abs(value);
	};
	REQUIRE(fn(10) == 10);
	REQUIRE(fn(-10) == 10);
	REQUIRE(fn(0) == 0);
}

TEST_CASE("Can use lambda with 2 arguments", "[function]")
{
	function<int(int, int)> fn = [](auto&& a, auto&& b) {
		return Sum(a, b);
	};
	REQUIRE(fn(10, 5) == 15);
	REQUIRE(fn(-10, 0) == -10);
}

TEST_CASE("Can use lambda without arguments", "[function]")
{
	function<std::string()> fn = [] {
		return GetStringHello();
	};
	REQUIRE(fn() == "hello");
}

TEST_CASE("Can use lambda without return value", "[function]")
{
	bool calledOnce = false;
	function<void(int&)> fn = [calledOnce](auto& value) mutable {
		if (calledOnce)
		{
			abort();
		}
		calledOnce = true;
		InplaceAbs(value);
	};
	int a = -10;
	fn(a);
	REQUIRE(a == 10);
}

TEST_CASE("Can use functor with 1 argument", "[function]")
{
	function<int(int)> fn = AbsFunctor();
	REQUIRE(fn(10) == 10);
	REQUIRE(fn(-10) == 10);
	REQUIRE(fn(0) == 0);
}

TEST_CASE("Can use functor with 2 arguments", "[function]")
{
	function<int(int, int)> fn = SumFunctor();
	REQUIRE(fn(10, 5) == 15);
	REQUIRE(fn(-10, 0) == -10);
}

TEST_CASE("Can use functor without arguments", "[function]")
{
	function<std::string()> fn = GetStringFunctor("hello");
	REQUIRE(fn() == "hello");
}

TEST_CASE("Can use functor without return value", "[function]")
{
	function<void(int&)> fn = InplaceAbsFunctor();
	int a = -10;
	fn(a);
	REQUIRE(a == 10);
}

TEST_CASE("Can construct function with cons std::function<>&", "[function]")
{
	using BoolCallback = std::function<void(bool succeed)>;
	bool value = false;
	const BoolCallback& cb = [&value](bool succeed) {
		value = succeed;
	};

	function<void(bool)> fn = cb;
	fn(true);
	REQUIRE(value == true);
	fn(false);
	REQUIRE(value == false);
	fn(true);
	REQUIRE(value == true);
}

TEST_CASE("Can copy function", "[function]")
{
	unsigned calledCount = 0;
	bool value = false;
	function<void(bool)> callback = [&](bool gotValue) {
		++calledCount;
		value = gotValue;
	};
	auto callback2 = callback;
	REQUIRE(calledCount == 0);
	CHECK(!value);
	callback(true);
	REQUIRE(calledCount == 1);
	CHECK(value);
	callback2(false);
	REQUIRE(calledCount == 2);
	CHECK(!value);
}

TEST_CASE("Can move function", "[function]")
{
	bool called = false;
	function<void()> callback = [&] {
		called = true;
	};
	auto callback2(std::move(callback));
	REQUIRE_THROWS(callback());
	REQUIRE(!called);
	callback2();
	REQUIRE(called);
}

TEST_CASE("Works when copying self", "[function]")
{
	bool called = false;
	function<void()> callback = [&] {
		called = true;
	};
	callback = callback;
	callback();
	REQUIRE(called);
}

TEST_CASE("Can release packed function", "[function]")
{
	function<int()> iota = [v = 0]() mutable {
		return v++;
	};
	REQUIRE(iota() == 0);

	auto packedFn = std::move(iota).release();
	REQUIRE_THROWS_AS(iota(), std::bad_function_call);

	auto&& proxy = packedFn.get<int()>();
	REQUIRE(proxy() == 1);
	REQUIRE(proxy() == 2);
}

TEST_CASE("Function copy has its own packed function", "[function]")
{
	function<int()> iota = [v = 0]() mutable {
		return v++;
	};

	REQUIRE(iota() == 0);

	auto iotaCopy(iota);

	REQUIRE(iota() == 1);
	REQUIRE(iota() == 2);

	REQUIRE(iotaCopy() == 1);
	REQUIRE(iotaCopy() == 2);
}

TEST_CASE("can work with callables that have vtable", "[function]")
{
	class Base
	{
	};

	class Interface : public Base
	{
	public:
		virtual ~Interface() = default;
		virtual void operator()() const = 0;
	};
	class Class : public Interface
	{
	public:
		Class(bool* destructorCalled)
			: m_destructorCalled(destructorCalled)
		{
		}

		~Class()
		{
			*m_destructorCalled = true;
		}

		void operator()() const override
		{
		}

		bool* m_destructorCalled = nullptr;
	};
	bool destructorCalled = false;
	{
		function<void()> f = Class(&destructorCalled);
		f();
		auto packed = f.release();
		destructorCalled = false;
	}
	CHECK(destructorCalled);
}

TEST_CASE("can work with callables with virtual inheritance", "[function]")
{
	struct A
	{
		void operator()() const
		{
			m_called = true;
		}

		~A()
		{
			*m_destructorCalled = true;
		}

		mutable bool m_called = false;
		bool* m_destructorCalled = nullptr;
	};
	struct B : public virtual A
	{
	};
	struct C : public virtual A
	{
	};
	struct D : virtual public B
		, virtual public C
	{
		D(bool* destructorCalled)
		{
			m_destructorCalled = destructorCalled;
		}

		using A::operator();
	};
	bool destructorCalled = false;
	{
		function<void()> f = D(&destructorCalled);
		f();
		auto packed = f.release();
		destructorCalled = false;
	}
	CHECK(destructorCalled);
}

TEST_CASE("uses copy constructor if callable's move constructor throws", "[function]")
{
	struct Callable
	{
		Callable() = default;
		Callable(Callable&&)
		{
			throw std::runtime_error("throw");
		}
		Callable(const Callable& other) = default;
		void operator()() const
		{
		}
	};
	Callable c;
	function<void()> f(c);
	auto f2 = std::move(f);
	f2();
	CHECK_THROWS(f());
}

TEST_CASE("uses move constructor if it is noexcept", "[function]")
{
	struct Callable
	{
		Callable() = default;
		Callable(Callable&& other) noexcept = default;
		Callable(const Callable&)
		{
			throw std::runtime_error("throw");
		}
		void operator()() const
		{
		}
	};
	Callable c;
	function<void()> f(std::move(c));
	auto f2 = std::move(f);
	f2();
	CHECK_THROWS(f());
}

TEST_CASE("can copy and move empty function", "[function]")
{
	function<void()> f;
	auto f2 = f;
	auto f3 = std::move(f);
}

TEST_CASE("properly copies callable on assignment", "[function]")
{
	struct Callable
	{
		Callable(int& aliveCounter)
			: m_aliveCounter(&aliveCounter)
		{
			++*m_aliveCounter;
		}
		Callable(const Callable& other)
			: m_aliveCounter(other.m_aliveCounter)
		{
			if (m_aliveCounter)
			{
				++*m_aliveCounter;
			}
		}
		Callable(Callable&& other) noexcept
			: m_aliveCounter(other.m_aliveCounter)
		{
			other.m_aliveCounter = nullptr;
		}
		~Callable()
		{
			if (m_aliveCounter)
			{
				--*m_aliveCounter;
			}
		}
		void operator()() const
		{
		}

		int* m_aliveCounter = nullptr;
	};
	int aliveCounter1 = 0;
	int aliveCounter2 = 0;
	function<void()> f = Callable(aliveCounter1);
	function<void()> f2 = Callable(aliveCounter2);
	CHECK(aliveCounter1 == 1);
	CHECK(aliveCounter2 == 1);
	f = f2;
	CHECK(aliveCounter1 == 0);
	CHECK(aliveCounter2 == 2);
	f = function<void()>();
	f2 = function<void()>();
	CHECK(aliveCounter1 == 0);
	CHECK(aliveCounter2 == 0);
}

TEST_CASE("copy assignment operator provides strong exception safety", "[function]")
{
	struct State
	{
		int callCount = 0;
		bool throwOnCopy = false;
	};
	struct Callable
	{
		Callable(State& state)
			: state(&state)
		{
		}
		void operator()()
		{
			++state->callCount;
		}
		Callable(const Callable& other)
			: state(other.state)
		{
			if (state->throwOnCopy)
			{
				throw std::runtime_error("throw on request");
			}
		}
		State* state = nullptr;
	};
	static_assert(!detail::can_use_inplace_buffer<Callable>);

	State srcState;
	State dstState;

	function<void()> srcFn(Callable{ srcState });
	function<void()> dstFn(Callable{ dstState });

	srcFn();
	dstFn();

	REQUIRE(srcState.callCount == 1);
	REQUIRE(dstState.callCount == 1);

	srcState.throwOnCopy = true;

	REQUIRE_THROWS_AS(dstFn = srcFn, std::runtime_error);

	// srcFn and dstFn must not be emptied even if assignment throws
	REQUIRE_NOTHROW(srcFn());
	REQUIRE_NOTHROW(dstFn());

	// srcFn and dstFn must keep their state
	REQUIRE(srcState.callCount == 2);
	REQUIRE(dstState.callCount == 2);

	// The next copy will succeed
	srcState.throwOnCopy = false;
	REQUIRE_NOTHROW(dstFn = srcFn);

	// Both functions are usable
	REQUIRE_NOTHROW(srcFn());
	REQUIRE_NOTHROW(dstFn());

	// After assignment, dstFn and srcFn refer the same state - srcState
	REQUIRE(srcState.callCount == 4);
	REQUIRE(dstState.callCount == 2);
}

TEST_CASE("assignment of variously allocated functions", "[function]")
{
	int heapCalls = 0;
	auto onHeap = [&heapCalls, largeVar = std::array<std::string, 1000>()]() mutable {
		std::fill(largeVar.begin(), largeVar.end(), "large string to be allocated on heap instead of stack");
		++heapCalls;
	};
	int stackCalls = 0;
	auto onStack = [&stackCalls] {
		++stackCalls;
	};

	static_assert(detail::can_use_inplace_buffer<detail::function_proxy_impl<decltype(onStack), void>>);
	static_assert(!detail::can_use_inplace_buffer<detail::function_proxy_impl<decltype(onHeap), void>>);

	using Fn = function<void()>;
	{
		Fn heap(onHeap);
		Fn stack(onStack);
		heap = stack;
		heap();
		REQUIRE(stackCalls == 1);
		REQUIRE(heapCalls == 0);
	}
	{
		Fn heap(onHeap);
		Fn stack(onStack);
		stack = heap;
		stack();
		REQUIRE(stackCalls == 1);
		REQUIRE(heapCalls == 1);
	}
	{
		Fn heap(onHeap);
		Fn heap1(onHeap);
		heap = heap1;
		heap();
		REQUIRE(stackCalls == 1);
		REQUIRE(heapCalls == 2);
	}
	{
		Fn stack(onStack);
		Fn stack1(onStack);
		stack = stack1;
		stack();
		REQUIRE(stackCalls == 2);
		REQUIRE(heapCalls == 2);
	}
	{
		Fn heap(onHeap);
		Fn empty;
		heap = empty;
		REQUIRE_THROWS(heap());
		REQUIRE(stackCalls == 2);
		REQUIRE(heapCalls == 2);
	}
	{
		Fn stack(onStack);
		Fn empty;
		stack = empty;
		REQUIRE_THROWS(stack());
		REQUIRE(stackCalls == 2);
		REQUIRE(heapCalls == 2);
	}
	{
		Fn empty;
		Fn heap(onHeap);
		empty = heap;
		empty();
		REQUIRE(stackCalls == 2);
		REQUIRE(heapCalls == 3);
	}
	{
		Fn empty;
		Fn stack(onStack);
		empty = stack;
		empty();
		REQUIRE(stackCalls == 3);
		REQUIRE(heapCalls == 3);
	}
	{
		Fn empty;
		Fn empty1;
		empty = empty1;
		REQUIRE_THROWS(empty());
		REQUIRE(stackCalls == 3);
		REQUIRE(heapCalls == 3);
	}
}
