# CRC++
Easy to use and fast C++ CRC library.

Tired of writing CRC code over and over again? Don't want to include a dozen boost header files just for a little bit of functionality? CRC++ is a portable and extremely lightweight alternative that is incredibly simple, fast, and clean.

### Features

CRC++ supports bit-by-bit and byte-by-byte calculation of full and multipart CRCs. The algorithms used are highly optimized and can even be configured to be branchless (as always, be sure to profile your code to choose the most efficient option). CRC++ is a great option for embedded C++ projects with a need for efficiency.

CRC++ consists of a single header file which can be included in any existing C++ application. No libraries, no boost, no mess, no fuss.

Any CRC width is supported - even CRCs larger than 64 bits, provided there is an integer type large enough to contain it. Trying to compute a 57-bit CRC? Got you covered.

Many common CRCs are provided out-of-the-box, such as CRC-32 (used in PKZip and Ethernet), CRC-XMODEM, and CRC-CCITT.

CRC++ will compile with any reasonably compliant C++03 or C++11 compiler. Compiling with C++11 is recommended, as it allows a number of static computations to be performed at compile-time instead of runtime.

All of the CRC++ code is well-documented. Unit tests are included in the repository (g++ Makefile and Visual Studio 2015 projects included). HTML documentation can also be produced via Doxygen (also included in the repository).

### Comparison

CRC++ boasts one of the fastest and most memory efficient generic CRC implementation available. The below table shows performance comparisons across multiple implementations and platforms.

| Library       | Speed, x64 platform (100 million iterations) | Speed, x86 platform (100 million iterations) |
| ------------- | -------------------------------------------- | -------------------------------------------- |
| CRC++         | 2050 milliseconds                            | 2200 milliseconds                            |
| boost         | 2250 milliseconds                            | 2000 milliseconds                            |
| pycrc         | 2050 milliseconds                            | 2240 milliseconds                            |
| mhash         | 2250 milliseconds                            | 2400 milliseconds                            |

Additionally, CRC++ has the most features of any library and the smallest code footprint:

| Library       | Number of include files | Header-only implemen-tation | Supports byte != 8 bits | Supports arbitrary CRC width | Custom type support | C++11 support | 40+ built-in CRC definitions | Branchless implemen-tation |
| ------------- | ----------------------- | --------------------------- | ----------------------- | ---------------------------- | ------------------- | ------------- | ---------------------------- | -------------------------- |
| CRC++         | 1                       | Yes                         | Yes                     | Yes                          | Yes                 | Yes           | Yes                          | Yes                        |
| boost         | 17                      | Yes                         | Yes                     | Yes                          | Yes                 | Yes           | No                           | No                         |
| pycrc         | 2 per CRC algorithm     | No                          | No                      | No                           | No                  | No            | No                           | No                         |
| mhash         | 6                       | No                          | Yes                     | No                           | No                  | No            | No                           | No                         |

### Usage

Computing a CRC is as simple as the following code:

```cpp
#include "CRC.h" // Only need to include this header file!
                 // No libraries need to be included. No project settings need to be messed with.
				 
#include <iomanip>  // Includes ::std::hex
#include <iostream> // Includes ::std::cout
#include <cstdint>  // Includes ::std::uint32_t

int main(int argc, char ** argv)
{
	const char myString[] = { 'H', 'E', 'L', 'L', 'O', ' ', 'W', 'O', 'R', 'L', 'D' };
	
	std::uint32_t crc = CRC::Calculate(myString, sizeof(myString), CRC::CRC_32());
	
	std::cout << std::hex << crc;
	
	return 0;
}
```

Multi-part CRCs are also supported:

```cpp
int main(int argc, char ** argv)
{
	const char myHelloString[] = { 'H', 'E', 'L', 'L', 'O', ' ' };
	const char myWorldString[] = { 'W', 'O', 'R', 'L', 'D' };
	
	std::uint32_t crc;
	
	crc = CRC::Calculate(myHelloString, sizeof(myHelloString), CRC::CRC_32());
	crc = CRC::Calculate(myWorldString, sizeof(myWorldString), CRC::CRC_32(), crc);
	
	std::cout << std::hex << crc;
	
	return 0;
}
```

This will return the same CRC as the first example.

Both of the above examples compute a CRC bit-by-bit. However, CRC++ also supports lookup tables, as the following example demonstrates:

```cpp
int main(int argc, char ** argv)
{
	const char myHelloString[] = { 'H', 'E', 'L', 'L', 'O', ' ' };
	const char myWorldString[] = { 'W', 'O', 'R', 'L', 'D' };
	
	CRC::Table<std::uint32_t, 32> table(CRC::CRC_32());
	
	std::uint32_t crc;
	
	crc = CRC::Calculate(myHelloString, sizeof(myHelloString), table);
	crc = CRC::Calculate(myWorldString, sizeof(myWorldString), table, crc);
	
	std::cout << std::hex << crc;
	
	return 0;
}
```

Or, if you prefer using the `auto` keyword:

```cpp
int main(int argc, char ** argv)
{
	const char myHelloString[] = { 'H', 'E', 'L', 'L', 'O', ' ' };
	const char myWorldString[] = { 'W', 'O', 'R', 'L', 'D' };
	
	auto table = CRC::CRC_32().MakeTable();
	
	std::uint32_t crc;
	
	crc = CRC::Calculate(myHelloString, sizeof(myHelloString), table);
	crc = CRC::Calculate(myWorldString, sizeof(myWorldString), table, crc);
	
	std::cout << std::hex << crc;
	
	return 0;
}
```

Lookup tables are much faster than computing a CRC bit-by-bit, at the expense of extra memory usage. A lookup table can be reused for as many CRCs as desired until it goes out of scope.

### Configuration

CRC++ can be configured by setting various `#define`s before `#include`-ing the CRC++ header file:

* `#define crcpp_uint8`<br>
Specifies the type used to store CRCs that have a width of 8 bits or less. This type is not used in CRC calculations. Defaults to ::std::uint8_t.
* `#define crcpp_uint16`<br>
Specifies the type used to store CRCs that have a width between 9 and 16 bits (inclusive). This type is not used in CRC calculations. Defaults to ::std::uint16_t.
* `#define crcpp_uint32`<br>
Specifies the type used to store CRCs that have a width between 17 and 32 bits (inclusive). This type is not used in CRC calculations. Defaults to ::std::uint32_t.
* `#define crcpp_uint64`<br>
Specifies the type used to store CRCs that have a width between 33 and 64 bits (inclusive). This type is not used in CRC calculations. Defaults to ::std::uint64_t.
* `#define crcpp_size`<br>
This type is used for loop iteration and function signatures only. Defaults to ::std::size_t.
* `#define CRCPP_USE_NAMESPACE`<br>
Define to place all CRC++ code within the ::CRCPP namespace. Not defined by default.
* `#define CRCPP_BRANCHLESS`<br>
Define to enable a branchless CRC implementation. The branchless implementation uses a single integer multiplication in the bit-by-bit calculation instead of a small conditional. The branchless implementation may be faster on processor architectures which support single-instruction integer multiplication. Not defined by default.
* `#define CRCPP_USE_CPP11`
Define to enables C++11 features (move semantics, constexpr, static_assert, etc.). Not defined by default.
* `#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS`
Define to include definitions for little-used CRCs. Not defined by default.

### License

CRC++ is free to use and provided under a BSD license.
