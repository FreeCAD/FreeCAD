# Why FastSignals?

FastSignals is a C++17 signals/slots implementation which API is compatible with Boost.Signals2.

FastSignals pros:

* Faster than Boost.Signals2
* Has more compact binary code
* Has the same API as Boost.Signals2

FastSignals cons:

* Supports only C++17 compatible compilers: Visual Studio 2017, modern Clang, modern GCC
* Lacks a few rarely used features presented in Boost.Signals2
    * No access to connection from slot with `signal::connect_extended` method
    * No connected object tracking with `slot::track` method
        * Use [bind_weak](bind_weak.md) instead
    * No temporary signal blocking with `shared_connection_block` class
    * Cannot disconnect equivalent slots since no `disconnect(slot)` function overload
    * Any other API difference is a bug - please report it!

See also [Migration from Boost.Signals2](migration-from-boost-signals2.md).

## Benchmark results

Directory `tests/libfastsignals_bench` contains simple benchmark with compares two signal/slot implementations:

* Boost.Signals2
* libfastsignals

Benchmark compairs performance when signal emitted frequently with 0, 1 and 8 active connections. In these cases libfastsignals is 3-6 times faster.

```
*** Results:
measure                 emit_boost   emit_fastsignals
emit_boost/0                  1.00               3.00
emit_boost/1                  1.00               5.76
emit_boost/8                  1.00               3.70
***
```
