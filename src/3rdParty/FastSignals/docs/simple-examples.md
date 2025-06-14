# Simple Examples

>If you are not familar with Boost.Signals2, please read [Boost.Signals2: Connections](https://theboostcpplibraries.com/boost.signals2-connections)

## Example with signal&lt;&gt; and connection

```cpp
// Creates signal and connects 1 slot, calls 2 times, disconnects, calls again.
// Outputs:
//  13
//  17
#include "libfastsignals/signal.h"

using namespace is::signals;

int main()
{
    signal<void(int)> valueChanged;
    connection conn;
    conn = valueChanged.connect([](int value) {
        cout << value << endl;
    });
    valueChanged(13);
    valueChanged(17);
    conn.disconnect();
    valueChanged(42);
}
```

## Example with scoped_connection

```cpp
// Creates signal and connects 1 slot, calls 2 times, calls again after scoped_connection destroyed.
//  - note: scoped_connection closes connection in destructor
// Outputs:
//  13
//  17
#include "libfastsignals/signal.h"

using namespace is::signals;

int main()
{
    signal<void(int)> valueChanged;
    {
        scoped_connection conn;
        conn = valueChanged.connect([](int value) {
            cout << value << endl;
        });
        valueChanged(13);
        valueChanged(17);
    }
    valueChanged(42);
}
```
