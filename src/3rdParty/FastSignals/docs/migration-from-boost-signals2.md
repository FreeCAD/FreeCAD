# Migration from Boost.Signals2

This guide helps to migrate large codebase from Boost.Signals2 to `FastSignals` signals/slots library. It helps to solve known migration issues in the right way.

During migrations, you will probably face with following things:

* You code uses `boost::signals2::` namespace and `<boost/signals2.hpp>` header directly
* You code uses third-party headers included implicitly by the `<boost/signals2.hpp>` header

## Reasons migrate from Boost.Signals2 to FastSignals

FastSignals API mostly compatible with Boost.Signals2 - there are differences, and all differences has their reasons explained below.

Comparing to Boost.Signals2, FastSignals has following pros:

* FastSignals is not header-only - so binary code will be more compact
* FastSignals implemented using C++17 with variadic templates, `constexpr if` and other modern metaprogramming techniques - so it compiles faster and, again, binary code will be more compact
* FastSignals probably will faster than Boost.Signals2 for your codebase because with FastSignals you don't pay for things that you don't use, with one exception: you always pay for the multithreading support

## Step 1: Create header with aliases

## Step 2: Rebuild and fix compile errors

### 2.1 Add missing includes

Boost.Signals2 is header-only library. It includes a lot of STL/Boost stuff while FastSignals does not:

```cpp
#include <boost/signals2.hpp>
// Also includes std::map, boost::variant, boost::optional, etc.

// Compiled OK even without `#include <map>`!
std::map CreateMyMap();
```

With FastSignals, you must include headers like `<map>` manually. The following table shows which files should be included explicitly if you see compile erros after migration.

| Class | Header |
|--------------------|:--------------------------------------:|
| std::map | `#include <map>` |
| boost::variant | `#include <boost/variant/variant.hpp>` |
| boost::optional | `#include <boost/optional/optional.hpp>` |
| boost::scoped_ptr | `#include <boost/scoped_ptr.hpp>` |
| boost::noncopyable | `#include <boost/noncopyable.hpp>` |
| boost::bind | `#include <boost/bind.hpp>` |
| boost::function | `#include <boost/function.hpp>` |

If you just want to compile you code, you can add following includes in you `signals.h` header:

```cpp
// WARNING: [libfastsignals] we do not recommend to include following extra headers.
#include <map>
#include <boost/variant/variant.hpp>
#include <boost/optional/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
```

### 2.2 Remove redundant returns for void signals

With Boost.Signals2, following code compiled without any warning:

```cpp
boost::signals2::signal<void()> event;
event.connect([] {
    return true;
});
```

With FastSignals, slot cannot return non-void value when for `signal<void(...)>`. You must fix your code: just remove returns from your slots or add lambdas to wrap slot and ignore it result.

### 2.3 Replace track() and track_foreign() with bind_weak_ptr()

Boost.Signals2 [can track connected objects lifetype](https://www.boost.org/doc/libs/1_55_0/doc/html/signals2/tutorial.html#idp204830936) using `track(...)` and `track_foreign(...)` methods. In the following example `Entity` created with `make_shared`, and `Entity::get_print_slot()` creates slot function which tracks weak pointer to Entity:

```cpp
#include <boost/signals2.hpp>
#include <iostream>
#include <memory>

using VoidSignal = boost::signals2::signal<void()>;
using VoidSlot = VoidSignal::slot_type;

struct Entity : std::enable_shared_from_this<Entity>
{
	int value = 42;

	VoidSlot get_print_slot()
	{
		// Here track() tracks object itself.
		return VoidSlot(std::bind(&Entity::print, this)).track_foreign(shared_from_this());
	}

	void print()
	{
		std::cout << "print called, num = " << value << std::endl;
	}
};

int main()
{
	VoidSignal event;
	auto entity = std::make_shared<Entity>();
	event.connect(entity->get_print_slot());

	// Here slot called - it prints `print called, num = 42`
	event();
	entity = nullptr;

	// This call does nothing.
	event();
}
```

FastSignals uses another approach: `bind_weak` function:

```cpp
#include "fastsignals/bind_weak.h"
#include <iostream>

using VoidSignal = is::signals::signal<void()>;
using VoidSlot = VoidSignal::slot_type;

struct Entity : std::enable_shared_from_this<Entity>
{
	int value = 42;

	VoidSlot get_print_slot()
	{
		// Here is::signals::bind_weak() used instead of std::bind.
		return is::signals::bind_weak(&Entity::print, weak_from_this());
	}

	void print()
	{
		std::cout << "print called, num = " << value << std::endl;
	}
};

int main()
{
	VoidSignal event;
	auto entity = std::make_shared<Entity>();
	event.connect(entity->get_print_slot());

	// Here slot called - it prints `slot called, num = 42`
	event();
	entity = nullptr;

	// Here nothing happens - no exception, no slot call.
	event();
}
```

### FastSignals Differences in Result Combiners

## Step 3: Run Tests

Run all automated tests that you have (unit tests, integration tests, system tests, stress tests, benchmarks, UI tests).

Probably you will see no errors. If you see any, please report an issue.
