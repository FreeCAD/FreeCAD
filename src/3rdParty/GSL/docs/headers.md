The Guidelines Support Library (GSL) interface is very lightweight and exposed via a header-only library. This document attempts to document all of the headers and their exposed classes and functions.

Types and functions are exported in the namespace `gsl`.

See [GSL: Guidelines support library](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-gsl)

# <a name="H" />Headers

- [`<algorithms>`](#user-content-H-algorithms)
- [`<assert>`](#user-content-H-assert)
- [`<byte>`](#user-content-H-byte)
- [`<gsl>`](#user-content-H-gsl)
- [`<narrow>`](#user-content-H-narrow)
- [`<pointers>`](#user-content-H-pointers)
- [`<span>`](#user-content-H-span)
- [`<span_ext>`](#user-content-H-span_ext)
- [`<zstring>`](#user-content-H-zstring)
- [`<util>`](#user-content-H-util)

## <a name="H-algorithms" />`<algorithms>`

This header contains some common algorithms that have been wrapped in GSL safety features.

- [`gsl::copy`](#user-content-H-algorithms-copy)

### <a name="H-algorithms-copy" />`gsl::copy`

```cpp
template <class SrcElementType, std::size_t SrcExtent, class DestElementType,
          std::size_t DestExtent>
void copy(span<SrcElementType, SrcExtent> src, span<DestElementType, DestExtent> dest);
```

This function copies the content from the `src` [`span`](#user-content-H-span-span) to the `dest` [`span`](#user-content-H-span-span). It [`Expects`](#user-content-H-assert-expects)
that the destination `span` is at least as large as the source `span`.

## <a name="H-assert" />`<assert>`

This header contains some macros used for contract checking and suppressing code analysis warnings.

See [GSL.assert: Assertions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-assertions)

- [`GSL_SUPPRESS`](#user-content-H-assert-gsl_suppress)
- [`Expects`](#user-content-H-assert-expects)
- [`Ensures`](#user-content-H-assert-ensures)

### <a name="H-assert-gsl_suppress" />`GSL_SUPPRESS`

This macro can be used to suppress a code analysis warning.

The core guidelines request tools that check for the rules to respect suppressing a rule by writing
`[[gsl::suppress(tag)]]` or `[[gsl::suppress(tag, justification: "message")]]`.

Clang does not use exactly that syntax, but requires `tag` to be put in double quotes `[[gsl::suppress("tag")]]`.

For portable code you can use `GSL_SUPPRESS(tag)`.

See [In.force: Enforcement](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#inforce-enforcement).

### <a name="H-assert-expects" />`Expects`

This macro can be used for expressing a precondition. If the precondition is not held, then `std::terminate` will be called.

See [I.6: Prefer `Expects()` for expressing preconditions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i6-prefer-expects-for-expressing-preconditions)

### <a name="H-assert-ensures" />`Ensures`

This macro can be used for expressing a postcondition. If the postcondition is not held, then `std::terminate` will be called.

See [I.8: Prefer `Ensures()` for expressing postconditions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i8-prefer-ensures-for-expressing-postconditions)

## <a name="H-byte" />`<byte>`

This header contains the definition of a byte type, implementing `std::byte` before it was standardized into C++17.

- [`gsl::byte`](#user-content-H-byte-byte)

### <a name="H-byte-byte" />`gsl::byte`

If `GSL_USE_STD_BYTE` is defined to be `1`, then `gsl::byte` will be an alias to `std::byte`.  
If `GSL_USE_STD_BYTE` is defined to be `0`, then `gsl::byte` will be a distinct type that implements the concept of byte.  
If `GSL_USE_STD_BYTE` is not defined, then the header file will check if `std::byte` is available (C\+\+17 or higher). If yes,
`gsl::byte` will be an alias to `std::byte`, otherwise `gsl::byte` will be a distinct type that implements the concept of byte.

&#x26a0; Take care when linking projects that were compiled with different language standards (before C\+\+17 and C\+\+17 or higher).
If you do so, you might want to `#define GSL_USE_STD_BYTE 0` to a fixed value to be sure that both projects use exactly
the same type. Otherwise you might get linker errors.

See [SL.str.5: Use `std::byte` to refer to byte values that do not necessarily represent characters](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rstr-byte)

### Non-member functions

```cpp
template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte& operator<<=(byte& b, IntegerType shift) noexcept;

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte operator<<(byte b, IntegerType shift) noexcept;

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte& operator>>=(byte& b, IntegerType shift) noexcept;

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte operator>>(byte b, IntegerType shift) noexcept;
```

Left or right shift a `byte` by a given number of bits.

```cpp
constexpr byte& operator|=(byte& l, byte r) noexcept;
constexpr byte operator|(byte l, byte r) noexcept;
```

Bitwise "or" of two `byte`s.

```cpp
constexpr byte& operator&=(byte& l, byte r) noexcept;
constexpr byte operator&(byte l, byte r) noexcept;
```

Bitwise "and" of two `byte`s.

```cpp
constexpr byte& operator^=(byte& l, byte r) noexcept;
constexpr byte operator^(byte l, byte r) noexcept;
```

Bitwise xor of two `byte`s.

```cpp
constexpr byte operator~(byte b) noexcept;
```

Bitwise negation of a `byte`. Flips all bits. Zeroes become ones, ones become zeroes.

```cpp
template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr IntegerType to_integer(byte b) noexcept;
```

Convert the given `byte` value to an integral type.

```cpp
template <typename T>
constexpr byte to_byte(T t) noexcept;
```

Convert the given value to a `byte`. The template requires `T` to be an `unsigned char` so that no data loss can occur.
If you want to convert an integer constant to a `byte` you probably want to call `to_byte<integer constant>()`.

```cpp
template <int I>
constexpr byte to_byte() noexcept;
```

Convert the given value `I` to a `byte`. The template requires `I` to be in the valid range 0..255 for a `gsl::byte`.

## <a name="H-gsl" />`<gsl>`

This header is a convenience header that includes all other [GSL headers](#user-content-H).
Since `<narrow>` requires exceptions, it will only be included if exceptions are enabled.

## <a name="H-narrow" />`<narrow>`

This header contains utility functions and classes, for narrowing casts, which require exceptions. The narrowing-related utilities that don't require exceptions are found inside [util](#user-content-H-util).

See [GSL.util: Utilities](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-utilities)

- [`gsl::narrowing_error`](#user-content-H-narrow-narrowing_error)
- [`gsl::narrow`](#user-content-H-narrow-narrow)

### <a name="H-narrow-narrowing_error" />`gsl::narrowing_error`

`gsl::narrowing_error` is the exception thrown by [`gsl::narrow`](#user-content-H-narrow-narrow) when a narrowing conversion fails. It is derived from `std::exception`.

### <a name="H-narrow-narrow" />`gsl::narrow`

`gsl::narrow<T>(x)` is a named cast that does a `static_cast<T>(x)` for narrowing conversions with no signedness promotions.
If the argument `x` cannot be represented in the target type `T`, then the function throws a [`gsl::narrowing_error`](#user-content-H-narrow-narrowing_error) (e.g., `narrow<unsigned>(-42)` and `narrow<char>(300)` throw).

Note: compare [`gsl::narrow_cast`](#user-content-H-util-narrow_cast) in header [util](#user-content-H-util).

See [ES.46: Avoid lossy (narrowing, truncating) arithmetic conversions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-narrowing) and [ES.49: If you must use a cast, use a named cast](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-casts-named)

## <a name="H-pointers" />`<pointers>`

This header contains some pointer types.

See [GSL.view](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-views)

- [`gsl::unique_ptr`](#user-content-H-pointers-unique_ptr)
- [`gsl::shared_ptr`](#user-content-H-pointers-shared_ptr)
- [`gsl::owner`](#user-content-H-pointers-owner)
- [`gsl::not_null`](#user-content-H-pointers-not_null)
- [`gsl::strict_not_null`](#user-content-H-pointers-strict_not_null)

### <a name="H-pointers-unique_ptr" />`gsl::unique_ptr`

`gsl::unique_ptr` is an alias to `std::unique_ptr`.

See [GSL.owner: Ownership pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-ownership)

### <a name="H-pointers-shared_ptr" />`gsl::shared_ptr`

`gsl::shared_ptr` is an alias to `std::shared_ptr`.

See [GSL.owner: Ownership pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-ownership)

### <a name="H-pointers-owner" />`gsl::owner`

`gsl::owner<T>` is designed as a safety mechanism for code that must deal directly with raw pointers that own memory. Ideally such code should be restricted to the implementation of low-level abstractions. `gsl::owner` can also be used as a stepping point in converting legacy code to use more modern RAII constructs such as smart pointers.
`T` must be a pointer type (`std::is_pointer<T>`).

A `gsl::owner<T>` is a typedef to `T`. It adds no runtime overhead whatsoever, as it is purely syntactic and does not add any runtime checks.  Instead, it serves as an annotation for static analysis tools which check for memory safety, and as a code comprehension guide for human readers.

See Enforcement section of [C.31: All resources acquired by a class must be released by the class’s destructor](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-dtor-release).

### <a name="H-pointers-not_null" />`gsl::not_null`

`gsl::not_null<T>` restricts a pointer or smart pointer to only hold non-null values. It has no size overhead over `T`.

The checks for ensuring that the pointer is not null are done in the constructor. There is no overhead when retrieving or dereferencing the checked pointer.
When a nullptr check fails, `std::terminate` is called.

See [F.23: Use a `not_null<T>` to indicate that “null” is not a valid value](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-nullptr)

#### Member Types

```cpp
using element_type = T;
```

The type of the pointer or smart pointer that is managed by this object.

#### Member functions

##### Construct/Copy

```cpp
template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
constexpr not_null(U&& u);

template <typename = std::enable_if_t<!std::is_same<std::nullptr_t, T>::value>>
constexpr not_null(T u);
```

Constructs a `gsl_owner<T>` from a pointer that is convertible to `T` or that is a `T`. It [`Expects`](#user-content-H-assert-expects) that the provided pointer is not `== nullptr`.

```cpp
template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
constexpr not_null(const not_null<U>& other);
```

Constructs a `gsl_owner<T>` from another `gsl_owner` where the other pointer is convertible to `T`. It [`Expects`](#user-content-H-assert-expects) that the provided pointer is not `== nullptr`.

```cpp
not_null(const not_null& other) = default;
not_null& operator=(const not_null& other) = default;
```

Copy construction and assignment.

```cpp
not_null(std::nullptr_t) = delete;
not_null& operator=(std::nullptr_t) = delete;
```

Construction from `std::nullptr_t`  and assignment of `std::nullptr_t` are explicitly deleted.

##### Modifiers

```cpp
not_null& operator++() = delete;
not_null& operator--() = delete;
not_null operator++(int) = delete;
not_null operator--(int) = delete;
not_null& operator+=(std::ptrdiff_t) = delete;
not_null& operator-=(std::ptrdiff_t) = delete;
```

Explicitly deleted operators. Pointers point to single objects ([I.13: Do not pass an array as a single pointer](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-array)), so don't allow these operators.

##### Observers

```cpp
constexpr details::value_or_reference_return_t<T> get() const;
constexpr operator T() const { return get(); }
```

Get the underlying pointer.

```cpp
constexpr decltype(auto) operator->() const { return get(); }
constexpr decltype(auto) operator*() const { return *get(); }
```

Dereference the underlying pointer.

```cpp
void operator[](std::ptrdiff_t) const = delete;
```

Array index operator is explicitly deleted. Pointers point to single objects ([I.13: Do not pass an array as a single pointer](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-array)), so don't allow treating them as an array.

```cpp
void swap(not_null<T>& other) { std::swap(ptr_, other.ptr_); }
```

Swaps contents with another `gsl::not_null` object.

#### Non-member functions

```cpp
template <class T>
auto make_not_null(T&& t) noexcept;
```

Creates a `gsl::not_null` object, deducing the target type from the type of the argument.

```cpp
template <typename T, std::enable_if_t<std::is_move_assignable<T>::value && std::is_move_constructible<T>::value, bool> = true>
void swap(not_null<T>& a, not_null<T>& b);
```

Swaps the contents of two `gsl::not_null` objects.

```cpp
template <class T, class U>
auto operator==(const not_null<T>& lhs,
                const not_null<U>& rhs) noexcept(noexcept(lhs.get() == rhs.get()))
    -> decltype(lhs.get() == rhs.get());
template <class T, class U>
auto operator!=(const not_null<T>& lhs,
                const not_null<U>& rhs) noexcept(noexcept(lhs.get() != rhs.get()))
    -> decltype(lhs.get() != rhs.get());
template <class T, class U>
auto operator<(const not_null<T>& lhs,
               const not_null<U>& rhs) noexcept(noexcept(lhs.get() < rhs.get()))
    -> decltype(lhs.get() < rhs.get());
template <class T, class U>
auto operator<=(const not_null<T>& lhs,
                const not_null<U>& rhs) noexcept(noexcept(lhs.get() <= rhs.get()))
    -> decltype(lhs.get() <= rhs.get());
template <class T, class U>
auto operator>(const not_null<T>& lhs,
               const not_null<U>& rhs) noexcept(noexcept(lhs.get() > rhs.get()))
    -> decltype(lhs.get() > rhs.get());
template <class T, class U>
auto operator>=(const not_null<T>& lhs,
                const not_null<U>& rhs) noexcept(noexcept(lhs.get() >= rhs.get()))
    -> decltype(lhs.get() >= rhs.get());
```

Comparison of pointers that are convertible to each other.

##### Input/Output

```cpp
template <class T>
std::ostream& operator<<(std::ostream& os, const not_null<T>& val);
```

Performs stream output on a `not_null` pointer, invoking `os << val.get()`. This function is only available when `GSL_NO_IOSTREAMS` is not defined.

##### Modifiers

```cpp
template <class T, class U>
std::ptrdiff_t operator-(const not_null<T>&, const not_null<U>&) = delete;
template <class T>
not_null<T> operator-(const not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(const not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(std::ptrdiff_t, const not_null<T>&) = delete;
```

Addition and subtraction are explicitly deleted. Pointers point to single objects ([I.13: Do not pass an array as a single pointer](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-array)), so don't allow these operators.

##### STL integration

```cpp
template <class T>
struct std::hash<gsl::not_null<T>> { ... };
```

Specialization of `std::hash` for `gsl::not_null`.

### <a name="H-pointers-strict_not_null" />`gsl::strict_not_null`

`strict_not_null` is the same as [`not_null`](#user-content-H-pointers-not_null) except that the constructors are `explicit`.

The free function that deduces the target type from the type of the argument and creates a `gsl::strict_not_null` object is `gsl::make_strict_not_null`.

## <a name="H-span" />`<span>`

This header file exports the class `gsl::span`, a bounds-checked implementation of `std::span`.

- [`gsl::span`](#user-content-H-span-span)

### <a name="H-span-span" />`gsl::span`

```cpp
template <class ElementType, std::size_t Extent>
class span;
```

`gsl::span` is a view over memory. It does not own the memory and is only a way to access contiguous sequences of objects.
The extent can be either a fixed size or [`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent).

The `gsl::span` is based on the standardized version of `std::span` which was added to C++20. Originally, the plan was to
deprecate `gsl::span` when `std::span` finished standardization, however that plan changed when the runtime bounds checking
was removed from `std::span`'s design.

The only difference between `gsl::span` and `std::span` is that `gsl::span` strictly enforces runtime bounds checking.
Any violations of the bounds check results in termination of the program.
Like `gsl::span`, `gsl::span`'s iterators also differ from `std::span`'s iterator in that all access operations are bounds checked.

#### Which version of span should I use?

##### Use `gsl::span` if

- you want to guarantee bounds safety in your project.
  - All data accessing operations use bounds checking to ensure you are only accessing valid memory.
- your project uses C++14 or C++17.
  - `std::span` is not available as it was not introduced into the STL until C++20.

##### Use `std::span` if

- your project is C++20 and you need the performance offered by `std::span`.

#### Types

```cpp
using element_type = ElementType;
using value_type = std::remove_cv_t<ElementType>;
using size_type = std::size_t;
using pointer = element_type*;
using const_pointer = const element_type*;
using reference = element_type&;
using const_reference = const element_type&;
using difference_type = std::ptrdiff_t;

using iterator = details::span_iterator<ElementType>;
using reverse_iterator = std::reverse_iterator<iterator>;
```

#### Member functions

```cpp
constexpr span() noexcept;
```

Constructs an empty `span`. This constructor is only available if `Extent` is 0 or [`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent).
`span::data()` will return `nullptr`.

```cpp
constexpr explicit(Extent != gsl::dynamic_extent) span(pointer ptr, size_type count) noexcept;
```

Constructs a `span` from a pointer and a size. If `Extent` is not [`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent),
then the constructor [`Expects`](#user-content-H-assert-expects) that `count == Extent`.

```cpp
constexpr explicit(Extent != gsl::dynamic_extent) span(pointer firstElem, pointer lastElem) noexcept;
```

Constructs a `span` from a pointer to the begin and the end of the data. If `Extent` is not [`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent),
then the constructor [`Expects`](#user-content-H-assert-expects) that `lastElem - firstElem == Extent`.

```cpp
template <std::size_t N>
constexpr span(element_type (&arr)[N]) noexcept;
```

Constructs a `span` from a C style array. This overload is available if `Extent ==`[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent)
or `N == Extent`.

```cpp
template <class T, std::size_t N>
constexpr span(std::array<T, N>& arr) noexcept;

template <class T, std::size_t N>
constexpr span(const std::array<T, N>& arr) noexcept;
```

Constructs a `span` from a `std::array`. These overloads are available if `Extent ==`[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent)
or `N == Extent`, and if the array can be interpreted as a `ElementType` array.

```cpp
template <class Container>
constexpr explicit(Extent != gsl::dynamic_extent) span(Container& cont) noexcept;

template <class Container>
constexpr explicit(Extent != gsl::dynamic_extent) span(const Container& cont) noexcept;
```

Constructs a `span` from a container. These overloads are available if `Extent ==`[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent)
or `N == Extent`, and if the container can be interpreted as a contiguous `ElementType` array.

```cpp
constexpr span(const span& other) noexcept = default;
```

Copy constructor.

```cpp
template <class OtherElementType, std::size_t OtherExtent>
explicit(Extent != gsl::dynamic_extent && OtherExtent == dynamic_extent)
constexpr span(const span<OtherElementType, OtherExtent>& other) noexcept;
```

Constructs a `span` from another `span`. This constructor is available if `OtherExtent == Extent || Extent ==`[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent)` || OtherExtent ==`[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent)
and if `ElementType` and `OtherElementType` are compatible.

If `Extent !=`[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent) and `OtherExtent ==`[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent),
then the constructor [`Expects`](#user-content-H-assert-expects) that `other.size() == Extent`.

```cpp
constexpr span& operator=(const span& other) noexcept = default;
```

Copy assignment

```cpp
template <std::size_t Count>
constexpr span<element_type, Count> first() const noexcept;

constexpr span<element_type, dynamic_extent> first(size_type count) const noexcept;

template <std::size_t Count>
constexpr span<element_type, Count> last() const noexcept;

constexpr span<element_type, dynamic_extent> last(size_type count) const noexcept;
```

Return a subspan of the first/last `Count` elements. [`Expects`](#user-content-H-assert-expects) that `Count` does not exceed the `span`'s size.

```cpp
template <std::size_t offset, std::size_t count = dynamic_extent>
constexpr auto subspan() const noexcept;

constexpr span<element_type, dynamic_extent>
subspan(size_type offset, size_type count = dynamic_extent) const noexcept;
```

Return a subspan starting at `offset` and having size `count`. [`Expects`](#user-content-H-assert-expects) that `offset` does not exceed the `span`'s size,
and that `offset == `[`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent) or `offset + count` does not exceed the `span`'s size.
If `count` is `gsl::dynamic_extent`, the number of elements in the subspan is `size() - offset`.

```cpp
constexpr size_type size() const noexcept;

constexpr size_type size_bytes() const noexcept;
```

Returns the size respective the size in bytes of the `span`.

```cpp
constexpr bool empty() const noexcept;
```

Is the `span` empty?

```cpp
constexpr reference operator[](size_type idx) const noexcept;
```

Returns a reference to the element at the given index. [`Expects`](#user-content-H-assert-expects) that `idx` is less than the `span`'s size.

```cpp
constexpr reference front() const noexcept;
constexpr reference back() const noexcept;
```

Returns a reference to the first/last element in the `span`. [`Expects`](#user-content-H-assert-expects) that the `span` is not empty.

```cpp
constexpr pointer data() const noexcept;
```

Returns a pointer to the beginning of the contained data.

```cpp
constexpr iterator begin() const noexcept;
constexpr iterator end() const noexcept;
constexpr reverse_iterator rbegin() const noexcept;
constexpr reverse_iterator rend() const noexcept;
```

Returns an iterator to the first/last normal/reverse iterator.

```cpp
template <class Type, std::size_t Extent>
span(Type (&)[Extent]) -> span<Type, Extent>;

template <class Type, std::size_t Size>
span(std::array<Type, Size>&) -> span<Type, Size>;

template <class Type, std::size_t Size>
span(const std::array<Type, Size>&) -> span<const Type, Size>;

template <class Container,
          class Element = std::remove_pointer_t<decltype(std::declval<Container&>().data())>>
span(Container&) -> span<Element>;

template <class Container,
          class Element = std::remove_pointer_t<decltype(std::declval<const Container&>().data())>>
span(const Container&) -> span<Element>;
```

Deduction guides.

```cpp
template <class ElementType, std::size_t Extent>
span<const byte, details::calculate_byte_size<ElementType, Extent>::value>
as_bytes(span<ElementType, Extent> s) noexcept;

template <class ElementType, std::size_t Extent>
span<byte, details::calculate_byte_size<ElementType, Extent>::value>
as_writable_bytes(span<ElementType, Extent> s) noexcept;
```

Converts a `span` into a `span` of `byte`s.

`as_writable_bytes` will only be available for non-const `ElementType`s.

## <a name="H-span_ext" />`<span_ext>`

This file is a companion for and included by [`<gsl/span>`](#user-content-H-span), and should not be used on its own. It contains useful features that aren't part of the `std::span` API as found inside the STL `<span>` header (with the exception of [`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent), which is included here due to implementation constraints).

- [`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent)
- [`gsl::span`](#user-content-H-span_ext-span)
- [`gsl::span` comparison operators](#user-content-H-span_ext-span_comparison_operators)
- [`gsl::make_span`](#user-content-H-span_ext-make_span)
- [`gsl::at`](#user-content-H-span_ext-at)
- [`gsl::ssize`](#user-content-H-span_ext-ssize)
- [`gsl::span` iterator functions](#user-content-H-span_ext-span_iterator_functions)

### <a name="H-span_ext-dynamic_extent" />`gsl::dynamic_extent`

Defines the extent value to be used by all `gsl::span` with dynamic extent. 

Note: `std::dynamic_extent` is exposed by the STL `<span>` header and so ideally `gsl::dynamic_extent` would be under [`<gsl/span>`](#user-content-H-span), but to avoid cyclic dependency issues it is under `<span_ext>` instead.

### <a name="H-span_ext-span" />`gsl::span`

```cpp
template <class ElementType, std::size_t Extent = dynamic_extent>
class span;
```

Forward declaration of `gsl::span`.

### <a name="H-span_ext-span_comparison_operators" />`gsl::span` comparison operators

```cpp
template <class ElementType, std::size_t FirstExtent, std::size_t SecondExtent>
constexpr bool operator==(span<ElementType, FirstExtent> l, span<ElementType, SecondExtent> r);
template <class ElementType, std::size_t FirstExtent, std::size_t SecondExtent>
constexpr bool operator!=(span<ElementType, FirstExtent> l, span<ElementType, SecondExtent> r);
template <class ElementType, std::size_t Extent>
constexpr bool operator<(span<ElementType, Extent> l, span<ElementType, Extent> r);
template <class ElementType, std::size_t Extent>
constexpr bool operator<=(span<ElementType, Extent> l, span<ElementType, Extent> r);
template <class ElementType, std::size_t Extent>
constexpr bool operator>(span<ElementType, Extent> l, span<ElementType, Extent> r);
template <class ElementType, std::size_t Extent>
constexpr bool operator>=(span<ElementType, Extent> l, span<ElementType, Extent> r);
```

The comparison operators for two `span`s lexicographically compare the elements in the `span`s.

### <a name="H-span_ext-make_span" />`gsl::make_span`

```cpp
template <class ElementType>
constexpr span<ElementType> make_span(ElementType* ptr, typename span<ElementType>::size_type count);
template <class ElementType>
constexpr span<ElementType> make_span(ElementType* firstElem, ElementType* lastElem);
template <class ElementType, std::size_t N>
constexpr span<ElementType, N> make_span(ElementType (&arr)[N]) noexcept;
template <class Container>
constexpr span<typename Container::value_type> make_span(Container& cont);
template <class Container>
constexpr span<const typename Container::value_type> make_span(const Container& cont);
```

Utility function for creating a `span` with [`gsl::dynamic_extent`](#user-content-H-span_ext-dynamic_extent) from
- pointer and length,
- pointer to start and pointer to end,
- a C style array, or
- a container.

### <a name="H-span_ext-at" />`gsl::at`

```cpp
template <class ElementType, std::size_t Extent>
constexpr ElementType& at(span<ElementType, Extent> s, index i);
```

The function `gsl::at` offers a safe way to access data with index bounds checking.

This is the specialization of [`gsl::at`](#user-content-H-util-at) for [`span`](#user-content-H-span-span). It returns a reference to the `i`th element and
[`Expects`](#user-content-H-assert-expects) that the provided index is within the bounds of the `span`.

Note: `gsl::at` supports indexes up to `PTRDIFF_MAX`.

### <a name="H-span_ext-ssize" />`gsl::ssize`

```cpp
template <class ElementType, std::size_t Extent>
constexpr std::ptrdiff_t ssize(const span<ElementType, Extent>& s) noexcept;
```

Return the size of a [`span`](#user-content-H-span-span) as a `ptrdiff_t`.

### <a name="H-span_ext-span_iterator_functions" />`gsl::span` iterator functions

```cpp
template <class ElementType, std::size_t Extent>
constexpr typename span<ElementType, Extent>::iterator
begin(const span<ElementType, Extent>& s) noexcept;

template <class ElementType, std::size_t Extent = dynamic_extent>
constexpr typename span<ElementType, Extent>::iterator
end(const span<ElementType, Extent>& s) noexcept;

template <class ElementType, std::size_t Extent>
constexpr typename span<ElementType, Extent>::reverse_iterator
rbegin(const span<ElementType, Extent>& s) noexcept;

template <class ElementType, std::size_t Extent>
constexpr typename span<ElementType, Extent>::reverse_iterator
rend(const span<ElementType, Extent>& s) noexcept;

template <class ElementType, std::size_t Extent>
constexpr typename span<ElementType, Extent>::iterator
cbegin(const span<ElementType, Extent>& s) noexcept;

template <class ElementType, std::size_t Extent = dynamic_extent>
constexpr typename span<ElementType, Extent>::iterator
cend(const span<ElementType, Extent>& s) noexcept;

template <class ElementType, std::size_t Extent>
constexpr typename span<ElementType, Extent>::reverse_iterator
crbegin(const span<ElementType, Extent>& s) noexcept;

template <class ElementType, std::size_t Extent>
constexpr typename span<ElementType, Extent>::reverse_iterator
crend(const span<ElementType, Extent>& s) noexcept;
```

Free functions for getting a non-const/const begin/end normal/reverse iterator for a [`span`](#user-content-H-span-span).

## <a name="H-zstring" />`<zstring>`

This header exports a family of `*zstring` types.

A `gsl::XXzstring<T>` is a typedef to `T`. It adds no checks whatsoever, it is just for having a syntax to describe
that a pointer points to a zero terminated C style string. This helps static code analysis, and it helps human readers.

`basic_zstring` is a pointer to a C-string (zero-terminated array) with a templated char type. Used to implement the rest of the `*zstring` family.  
`zstring` is a zero terminated `char` string.  
`czstring` is a const zero terminated `char` string.  
`wzstring` is a zero terminated `wchar_t` string.  
`cwzstring` is a const zero terminated `wchar_t` string.  
`u16zstring` is a zero terminated `char16_t` string.  
`cu16zstring` is a const zero terminated `char16_t` string.  
`u32zstring` is a zero terminated `char32_t` string.  
`cu32zstring` is a const zero terminated `char32_t` string.  

See [GSL.view](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-views) and [SL.str.3: Use zstring or czstring to refer to a C-style, zero-terminated, sequence of characters](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rstr-zstring).

## <a name="H-util" />`<util>`

This header contains utility functions and classes. This header works without exceptions being available. The parts that require
exceptions being available are in their own header file [narrow](#user-content-H-narrow).

See [GSL.util: Utilities](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-utilities)

- [`gsl::narrow_cast`](#user-content-H-util-narrow_cast)
- [`gsl::final_action`](#user-content-H-util-final_action)
- [`gsl::at`](#user-content-H-util-at)

### <a name="H-util-index" />`gsl::index`

An alias to `std::ptrdiff_t`. It serves as the index type for all container indexes/subscripts/sizes.

### <a name="H-util-narrow_cast" />`gsl::narrow_cast`

`gsl::narrow_cast<T>(x)` is a named cast that is identical to a `static_cast<T>(x)`. It exists to make clear to static code analysis tools and to human readers that a lossy conversion is acceptable.

Note: compare the throwing version [`gsl::narrow`](#user-content-H-narrow-narrow) in header [narrow](#user-content-H-narrow).

See [ES.46: Avoid lossy (narrowing, truncating) arithmetic conversions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-narrowing) and [ES.49: If you must use a cast, use a named cast](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-casts-named)

### <a name="H-util-final_action" />`gsl::final_action`

```cpp
template <class F>
class final_action { ... };
```

`final_action` allows you to ensure something gets run at the end of a scope.

See [E.19: Use a final_action object to express cleanup if no suitable resource handle is available](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Re-finally)

#### Member functions

```cpp
explicit final_action(const F& ff) noexcept;
explicit final_action(F&& ff) noexcept;
```

Construct an object with the action to invoke in the destructor.

```cpp
~final_action() noexcept;
```

The destructor will call the action that was passed in the constructor.

```cpp
final_action(final_action&& other) noexcept;
final_action(const final_action&)   = delete;
void operator=(const final_action&) = delete;
void operator=(final_action&&)      = delete;
```

Move construction is allowed. Copy construction is deleted. Copy and move assignment are also explicitly deleted.

#### <a name="H-util-finally" />Non-member functions
```cpp
template <class F>
auto finally(F&& f) noexcept;
```

Creates a `gsl::final_action` object, deducing the template argument type from the type of the argument.

### <a name="H-util-at" />`gsl::at`

The function `gsl::at` offers a safe way to access data with index bounds checking.

Note: `gsl::at` supports indexes up to `PTRDIFF_MAX`.

See [ES.42: Keep use of pointers simple and straightforward](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-ptr)

```cpp
template <class T, std::size_t N>
constexpr T& at(T (&arr)[N], const index i);
```

This overload returns a reference to the `i`s element of a C style array `arr`. It [`Expects`](#user-content-H-assert-expects) that the provided index is within the bounds of the array.

```cpp
template <class Cont>
constexpr auto at(Cont& cont, const index i) -> decltype(cont[cont.size()]);
```

This overload returns a reference to the `i`s element of the container `cont`. It [`Expects`](#user-content-H-assert-expects) that the provided index is within the bounds of the array.

```cpp
template <class T>
constexpr T at(const std::initializer_list<T> cont, const index i);
```

This overload returns a reference to the `i`s element of the initializer list `cont`. It [`Expects`](#user-content-H-assert-expects) that the provided index is within the bounds of the array.

```cpp
template <class T, std::size_t extent = std::dynamic_extent>
constexpr auto at(std::span<T, extent> sp, const index i) -> decltype(sp[sp.size()]);
```

This overload returns a reference to the `i`s element of the `std::span` `sp`. It [`Expects`](#user-content-H-assert-expects) that the provided index is within the bounds of the array.

For [`gsl::at`](#user-content-H-span_ext-at) for [`gsl::span`](#user-content-H-span-span) see header [`span_ext`](#user-content-H-span_ext).

```cpp
template <class T, std::enable_if_t<std::is_move_assignable<T>::value && std::is_move_constructible<T>::value>>
void swap(T& a, T& b);
```

Swaps the contents of two objects. Exists only to specialize `gsl::swap<T>(gsl::not_null<T>&, gsl::not_null<T>&)`.
