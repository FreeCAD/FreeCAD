# Function bind_weak

## Usage

* Use `is::signals::bind_weak` instead of `std::bind` to ensure that nothing happens if method called when binded object already destroyed
* Pass pointer to T class method as first argument, `shared_ptr<T>` or `weak_ptr<T>` as second argument
* Example: `bind_weak(&Document::save(), document, std::placeholders::_1)`, where `document` is a `weak_ptr<Document>` or `shared_ptr<Document>`

## Weak this idiom

The `is::signals::bind_weak(...)` function implements "weak this" idiom. This idiom helps to avoid dangling pointers and memory access wiolations in asynchronous and/or multithreaded programs.

In the following example, we use weak this idiom to avoid using dangling pointer wehn calling `print()` method of the `Enityt`:

```cpp
struct Entity : std::enable_shared_from_this<Entity>
{
    int value = 42;

    void print()
    {
        std::cout << "print called, num = " << value << std::endl;
    }

    std::function<void()> print_later()
    {
        // ! weak this idiom here !
        auto weak_this = weak_from_this();
        return [weak_this] {
            if (auto shared_this = weak_this.lock())
            {
                shared_this->print();
            }
        };
    }
};

int main()
{
    auto entity = std::make_shared<Entity>();
    auto print = entity->print_later();

    // Prints OK.
    print();

    // Prints nothing - last shared_ptr to the Entity destroyed, so `weak_this.lock()` will return nullptr.
    entity = nullptr;
    print();
}
```

## Using bind_weak to avoid signal receiver lifetime issues

In the following example, `Entity::print()` method connected to the signal. Signal emmited once before and once after the `Entity` instance destroyed. However, no memory access violation happens: once `Entity` destoryed, no slot will be called because `bind_weak` doesn't call binded method if it cannot lock `std::weak_ptr` to binded object. The second `event()` expression just does nothing.

```cpp
#include "fastsignals/signal.h"
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
