///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef GSL_SPAN_H
#define GSL_SPAN_H

#include "./assert"   // for Expects
#include "./byte"     // for gsl::impl::byte
#include "./span_ext" // for span specialization of gsl::at and other span-related extensions
#include "./util"     // for narrow_cast

#include <array>       // for array
#include <cstddef>     // for ptrdiff_t, size_t, nullptr_t
#include <iterator>    // for reverse_iterator, distance, random_access_...
#include <memory>      // for pointer_traits
#include <type_traits> // for enable_if_t, declval, is_convertible, inte...

#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)

// turn off some warnings that are noisy about our Expects statements
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(                                                                                   \
    disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable : 4702) // unreachable code

// Turn MSVC /analyze rules that generate too much noise. TODO: fix in the tool.
#pragma warning(disable : 26495) // uninitialized member when constructor calls constructor
#pragma warning(disable : 26446) // parser bug does not allow attributes on some templates

#endif // _MSC_VER

// See if we have enough C++17 power to use a static constexpr data member
// without needing an out-of-line definition
#if !(defined(__cplusplus) && (__cplusplus >= 201703L))
#define GSL_USE_STATIC_CONSTEXPR_WORKAROUND
#endif // !(defined(__cplusplus) && (__cplusplus >= 201703L))

// GCC 7 does not like the signed unsigned mismatch (size_t ptrdiff_t)
// While there is a conversion from signed to unsigned, it happens at
// compiletime, so the compiler wouldn't have to warn indiscriminately, but
// could check if the source value actually doesn't fit into the target type
// and only warn in those cases.
#if defined(__GNUC__) && __GNUC__ > 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

// Turn off clang unsafe buffer warnings as all accessed are guarded by runtime checks
#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif // __has_warning("-Wunsafe-buffer-usage")
#endif // defined(__clang__)

namespace gsl
{

// implementation details
namespace details
{
    template <class T>
    struct is_span_oracle : std::false_type
    {
    };

    template <class ElementType, std::size_t Extent>
    struct is_span_oracle<gsl::span<ElementType, Extent>> : std::true_type
    {
    };

    template <class T>
    struct is_span : public is_span_oracle<std::remove_cv_t<T>>
    {
    };

    template <class T>
    struct is_std_array_oracle : std::false_type
    {
    };

    template <class ElementType, std::size_t Extent>
    struct is_std_array_oracle<std::array<ElementType, Extent>> : std::true_type
    {
    };

    template <class T>
    struct is_std_array : is_std_array_oracle<std::remove_cv_t<T>>
    {
    };

    template <std::size_t From, std::size_t To>
    struct is_allowed_extent_conversion
        : std::integral_constant<bool, From == To || To == dynamic_extent>
    {
    };

    template <class From, class To>
    struct is_allowed_element_type_conversion
        : std::integral_constant<bool, std::is_convertible<From (*)[], To (*)[]>::value>
    {
    };

    template <class Type>
    class span_iterator
    {
    public:
#if defined(__cpp_lib_ranges) || (defined(_MSVC_STL_VERSION) && defined(__cpp_lib_concepts))
        using iterator_concept = std::contiguous_iterator_tag;
#endif // __cpp_lib_ranges
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::remove_cv_t<Type>;
        using difference_type = std::ptrdiff_t;
        using pointer = Type*;
        using reference = Type&;

#ifdef _MSC_VER
        using _Unchecked_type = pointer;
        using _Prevent_inheriting_unwrap = span_iterator;
#endif // _MSC_VER
        constexpr span_iterator() = default;

        constexpr span_iterator(pointer begin, pointer end, pointer current)
            : begin_(begin), end_(end), current_(current)
        {
            Expects(begin_ <= current_ && current <= end_);
        }

        constexpr operator span_iterator<const Type>() const noexcept
        {
            return {begin_, end_, current_};
        }

        constexpr reference operator*() const noexcept
        {
            Expects(current_ != end_);
            return *current_;
        }

        constexpr pointer operator->() const noexcept
        {
            Expects(current_ != end_);
            return current_;
        }
        constexpr span_iterator& operator++() noexcept
        {
            Expects(current_ != end_);
            // clang-format off
            GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
            // clang-format on
            ++current_;
            return *this;
        }

        constexpr span_iterator operator++(int) noexcept
        {
            span_iterator ret = *this;
            ++*this;
            return ret;
        }

        constexpr span_iterator& operator--() noexcept
        {
            Expects(begin_ != current_);
            --current_;
            return *this;
        }

        constexpr span_iterator operator--(int) noexcept
        {
            span_iterator ret = *this;
            --*this;
            return ret;
        }

        constexpr span_iterator& operator+=(const difference_type n) noexcept
        {
            if (n != 0) Expects(begin_ && current_ && end_);
            if (n > 0) Expects(end_ - current_ >= n);
            if (n < 0) Expects(current_ - begin_ >= -n);
            // clang-format off
            GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
            // clang-format on
            current_ += n;
            return *this;
        }

        constexpr span_iterator operator+(const difference_type n) const noexcept
        {
            span_iterator ret = *this;
            ret += n;
            return ret;
        }

        friend constexpr span_iterator operator+(const difference_type n,
                                                 const span_iterator& rhs) noexcept
        {
            return rhs + n;
        }

        constexpr span_iterator& operator-=(const difference_type n) noexcept
        {
            if (n != 0) Expects(begin_ && current_ && end_);
            if (n > 0) Expects(current_ - begin_ >= n);
            if (n < 0) Expects(end_ - current_ >= -n);
            GSL_SUPPRESS(bounds .1)
            current_ -= n;
            return *this;
        }

        constexpr span_iterator operator-(const difference_type n) const noexcept
        {
            span_iterator ret = *this;
            ret -= n;
            return ret;
        }

        template <
            class Type2,
            std::enable_if_t<std::is_same<std::remove_cv_t<Type2>, value_type>::value, int> = 0>
        constexpr difference_type operator-(const span_iterator<Type2>& rhs) const noexcept
        {
            Expects(begin_ == rhs.begin_ && end_ == rhs.end_);
            return current_ - rhs.current_;
        }

        constexpr reference operator[](const difference_type n) const noexcept
        {
            return *(*this + n);
        }

        template <
            class Type2,
            std::enable_if_t<std::is_same<std::remove_cv_t<Type2>, value_type>::value, int> = 0>
        constexpr bool operator==(const span_iterator<Type2>& rhs) const noexcept
        {
            Expects(begin_ == rhs.begin_ && end_ == rhs.end_);
            return current_ == rhs.current_;
        }

        template <
            class Type2,
            std::enable_if_t<std::is_same<std::remove_cv_t<Type2>, value_type>::value, int> = 0>
        constexpr bool operator!=(const span_iterator<Type2>& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        template <
            class Type2,
            std::enable_if_t<std::is_same<std::remove_cv_t<Type2>, value_type>::value, int> = 0>
        constexpr bool operator<(const span_iterator<Type2>& rhs) const noexcept
        {
            Expects(begin_ == rhs.begin_ && end_ == rhs.end_);
            return current_ < rhs.current_;
        }

        template <
            class Type2,
            std::enable_if_t<std::is_same<std::remove_cv_t<Type2>, value_type>::value, int> = 0>
        constexpr bool operator>(const span_iterator<Type2>& rhs) const noexcept
        {
            return rhs < *this;
        }

        template <
            class Type2,
            std::enable_if_t<std::is_same<std::remove_cv_t<Type2>, value_type>::value, int> = 0>
        constexpr bool operator<=(const span_iterator<Type2>& rhs) const noexcept
        {
            return !(rhs < *this);
        }

        template <
            class Type2,
            std::enable_if_t<std::is_same<std::remove_cv_t<Type2>, value_type>::value, int> = 0>
        constexpr bool operator>=(const span_iterator<Type2>& rhs) const noexcept
        {
            return !(*this < rhs);
        }

#ifdef _MSC_VER
        // MSVC++ iterator debugging support; allows STL algorithms in 15.8+
        // to unwrap span_iterator to a pointer type after a range check in STL
        // algorithm calls
        friend constexpr void _Verify_range(span_iterator lhs, span_iterator rhs) noexcept
        { // test that [lhs, rhs) forms a valid range inside an STL algorithm
            Expects(lhs.begin_ == rhs.begin_ // range spans have to match
                    && lhs.end_ == rhs.end_ &&
                    lhs.current_ <= rhs.current_); // range must not be transposed
        }

        constexpr void _Verify_offset(const difference_type n) const noexcept
        { // test that *this + n is within the range of this call
            if (n != 0) Expects(begin_ && current_ && end_);
            if (n > 0) Expects(end_ - current_ >= n);
            if (n < 0) Expects(current_ - begin_ >= -n);
        }

        // clang-format off
        GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
        // clang-format on
        constexpr pointer _Unwrapped() const noexcept
        { // after seeking *this to a high water mark, or using one of the
            // _Verify_xxx functions above, unwrap this span_iterator to a raw
            // pointer
            return current_;
        }

        // Tell the STL that span_iterator should not be unwrapped if it can't
        // validate in advance, even in release / optimized builds:
#if defined(GSL_USE_STATIC_CONSTEXPR_WORKAROUND)
        static constexpr const bool _Unwrap_when_unverified = false;
#else
        static constexpr bool _Unwrap_when_unverified = false;
#endif
        // clang-format off
        GSL_SUPPRESS(con.3) // NO-FORMAT: attribute // TODO: false positive
        // clang-format on
        constexpr void _Seek_to(const pointer p) noexcept
        { // adjust the position of *this to previously verified location p
            // after _Unwrapped
            current_ = p;
        }
#endif

        pointer begin_ = nullptr;
        pointer end_ = nullptr;
        pointer current_ = nullptr;

        template <typename Ptr>
        friend struct std::pointer_traits;
    };
}} // namespace gsl::details

namespace std
{
template <class Type>
struct pointer_traits<::gsl::details::span_iterator<Type>>
{
    using pointer = ::gsl::details::span_iterator<Type>;
    using element_type = Type;
    using difference_type = ptrdiff_t;

    static constexpr element_type* to_address(const pointer i) noexcept { return i.current_; }
};
} // namespace std

namespace gsl { namespace details {
    template <std::size_t Ext>
    class extent_type
    {
    public:
        using size_type = std::size_t;

        constexpr extent_type() noexcept = default;

        constexpr explicit extent_type(extent_type<dynamic_extent>);

        constexpr explicit extent_type(size_type size) { Expects(size == Ext); }

        constexpr size_type size() const noexcept { return Ext; }

    private:
#if defined(GSL_USE_STATIC_CONSTEXPR_WORKAROUND)
        static constexpr const size_type size_ = Ext; // static size equal to Ext
#else
        static constexpr size_type size_ = Ext; // static size equal to Ext
#endif
    };

    template <>
    class extent_type<dynamic_extent>
    {
    public:
        using size_type = std::size_t;

        template <size_type Other>
        constexpr explicit extent_type(extent_type<Other> ext) : size_(ext.size())
        {}

        constexpr explicit extent_type(size_type size) : size_(size)
        {
            Expects(size != dynamic_extent);
        }

        constexpr size_type size() const noexcept { return size_; }

    private:
        size_type size_;
    };

    template <std::size_t Ext>
    constexpr extent_type<Ext>::extent_type(extent_type<dynamic_extent> ext)
    {
        Expects(ext.size() == Ext);
    }

    template <class ElementType, std::size_t Extent, std::size_t Offset, std::size_t Count>
    struct calculate_subspan_type
    {
        using type = span<ElementType, Count != dynamic_extent
                                           ? Count
                                           : (Extent != dynamic_extent ? Extent - Offset : Extent)>;
    };
} // namespace details

// [span], class template span
template <class ElementType, std::size_t Extent>
class span
{
public:
    // constants and types
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

#if defined(GSL_USE_STATIC_CONSTEXPR_WORKAROUND)
    static constexpr const size_type extent{Extent};
#else
    static constexpr size_type extent{Extent};
#endif

    // [span.cons], span constructors, copy, assignment, and destructor
    template <bool Dependent = false,
              // "Dependent" is needed to make "std::enable_if_t<Dependent || Extent == 0 || Extent
              // == dynamic_extent>" SFINAE, since "std::enable_if_t<Extent == 0 || Extent ==
              // dynamic_extent>" is ill-formed when Extent is greater than 0.
              class = std::enable_if_t<(Dependent ||
                                        details::is_allowed_extent_conversion<0, Extent>::value)>>
    constexpr span() noexcept : storage_(nullptr, details::extent_type<0>())
    {}

    template <std::size_t MyExtent = Extent, std::enable_if_t<MyExtent != dynamic_extent, int> = 0>
    constexpr explicit span(pointer ptr, size_type count) noexcept : storage_(ptr, count)
    {
        Expects(count == Extent);
    }

    template <std::size_t MyExtent = Extent, std::enable_if_t<MyExtent == dynamic_extent, int> = 0>
    constexpr span(pointer ptr, size_type count) noexcept : storage_(ptr, count)
    {}

    template <std::size_t MyExtent = Extent, std::enable_if_t<MyExtent != dynamic_extent, int> = 0>
    constexpr explicit span(pointer firstElem, pointer lastElem) noexcept
        : storage_(firstElem, narrow_cast<std::size_t>(lastElem - firstElem))
    {
        Expects(lastElem - firstElem == static_cast<difference_type>(Extent));
    }

    template <std::size_t MyExtent = Extent, std::enable_if_t<MyExtent == dynamic_extent, int> = 0>
    constexpr span(pointer firstElem, pointer lastElem) noexcept
        : storage_(firstElem, narrow_cast<std::size_t>(lastElem - firstElem))
    {}

    template <std::size_t N,
              std::enable_if_t<details::is_allowed_extent_conversion<N, Extent>::value, int> = 0>
    constexpr span(element_type (&arr)[N]) noexcept
        : storage_(KnownNotNull{arr}, details::extent_type<N>())
    {}

    template <
        class T, std::size_t N,
        std::enable_if_t<(details::is_allowed_extent_conversion<N, Extent>::value &&
                          details::is_allowed_element_type_conversion<T, element_type>::value),
                         int> = 0>
    constexpr span(std::array<T, N>& arr) noexcept
        : storage_(KnownNotNull{arr.data()}, details::extent_type<N>())
    {}

    template <class T, std::size_t N,
              std::enable_if_t<
                  (details::is_allowed_extent_conversion<N, Extent>::value &&
                   details::is_allowed_element_type_conversion<const T, element_type>::value),
                  int> = 0>
    constexpr span(const std::array<T, N>& arr) noexcept
        : storage_(KnownNotNull{arr.data()}, details::extent_type<N>())
    {}

    // NB: the SFINAE on these constructors uses .data() as an incomplete/imperfect proxy for the
    // requirement on Container to be a contiguous sequence container.
    template <std::size_t MyExtent = Extent, class Container,
              std::enable_if_t<
                  MyExtent != dynamic_extent && !details::is_span<Container>::value &&
                      !details::is_std_array<Container>::value &&
                      std::is_pointer<decltype(std::declval<Container&>().data())>::value &&
                      std::is_convertible<
                          std::remove_pointer_t<decltype(std::declval<Container&>().data())> (*)[],
                          element_type (*)[]>::value,
                  int> = 0>
    constexpr explicit span(Container& cont) noexcept : span(cont.data(), cont.size())
    {}

    template <std::size_t MyExtent = Extent, class Container,
              std::enable_if_t<
                  MyExtent == dynamic_extent && !details::is_span<Container>::value &&
                      !details::is_std_array<Container>::value &&
                      std::is_pointer<decltype(std::declval<Container&>().data())>::value &&
                      std::is_convertible<
                          std::remove_pointer_t<decltype(std::declval<Container&>().data())> (*)[],
                          element_type (*)[]>::value,
                  int> = 0>
    constexpr span(Container& cont) noexcept : span(cont.data(), cont.size())
    {}

    template <
        std::size_t MyExtent = Extent, class Container,
        std::enable_if_t<
            MyExtent != dynamic_extent && std::is_const<element_type>::value &&
                !details::is_span<Container>::value && !details::is_std_array<Container>::value &&
                std::is_pointer<decltype(std::declval<const Container&>().data())>::value &&
                std::is_convertible<
                    std::remove_pointer_t<decltype(std::declval<const Container&>().data())> (*)[],
                    element_type (*)[]>::value,
            int> = 0>
    constexpr explicit span(const Container& cont) noexcept : span(cont.data(), cont.size())
    {}

    template <
        std::size_t MyExtent = Extent, class Container,
        std::enable_if_t<
            MyExtent == dynamic_extent && std::is_const<element_type>::value &&
                !details::is_span<Container>::value && !details::is_std_array<Container>::value &&
                std::is_pointer<decltype(std::declval<const Container&>().data())>::value &&
                std::is_convertible<
                    std::remove_pointer_t<decltype(std::declval<const Container&>().data())> (*)[],
                    element_type (*)[]>::value,
            int> = 0>
    constexpr span(const Container& cont) noexcept : span(cont.data(), cont.size())
    {}

    constexpr span(const span& other) noexcept = default;

    template <class OtherElementType, std::size_t OtherExtent, std::size_t MyExtent = Extent,
              std::enable_if_t<(MyExtent == dynamic_extent || MyExtent == OtherExtent) &&
                                   details::is_allowed_element_type_conversion<OtherElementType,
                                                                               element_type>::value,
                               int> = 0>
    constexpr span(const span<OtherElementType, OtherExtent>& other) noexcept
        : storage_(other.data(), details::extent_type<OtherExtent>(other.size()))
    {}

    template <class OtherElementType, std::size_t OtherExtent, std::size_t MyExtent = Extent,
              std::enable_if_t<MyExtent != dynamic_extent && OtherExtent == dynamic_extent &&
                                   details::is_allowed_element_type_conversion<OtherElementType,
                                                                               element_type>::value,
                               int> = 0>
    constexpr explicit span(const span<OtherElementType, OtherExtent>& other) noexcept
        : storage_(other.data(), details::extent_type<OtherExtent>(other.size()))
    {}

    ~span() noexcept = default;
    constexpr span& operator=(const span& other) noexcept = default;

    // [span.sub], span subviews
    template <std::size_t Count>
    constexpr span<element_type, Count> first() const noexcept
    {
        static_assert(Extent == dynamic_extent || Count <= Extent,
                      "first() cannot extract more elements from a span than it contains.");
        Expects(Count <= size());
        return span<element_type, Count>{data(), Count};
    }

    template <std::size_t Count>
    // clang-format off
    GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
        // clang-format on
        constexpr span<element_type, Count> last() const noexcept
    {
        static_assert(Extent == dynamic_extent || Count <= Extent,
                      "last() cannot extract more elements from a span than it contains.");
        Expects(Count <= size());
        return span<element_type, Count>{data() + (size() - Count), Count};
    }

    template <std::size_t Offset, std::size_t Count = dynamic_extent>
    // clang-format off
    GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
        // clang-format on
        constexpr auto subspan() const noexcept ->
        typename details::calculate_subspan_type<ElementType, Extent, Offset, Count>::type
    {
        static_assert(Extent == dynamic_extent || (Extent >= Offset && (Count == dynamic_extent ||
                                                                        Count <= Extent - Offset)),
                      "subspan() cannot extract more elements from a span than it contains.");
        Expects((size() >= Offset) && (Count == dynamic_extent || (Count <= size() - Offset)));
        using type =
            typename details::calculate_subspan_type<ElementType, Extent, Offset, Count>::type;
        return type{data() + Offset, Count == dynamic_extent ? size() - Offset : Count};
    }

    constexpr span<element_type, dynamic_extent> first(size_type count) const noexcept
    {
        Expects(count <= size());
        return {data(), count};
    }

    constexpr span<element_type, dynamic_extent> last(size_type count) const noexcept
    {
        Expects(count <= size());
        return make_subspan(size() - count, dynamic_extent, subspan_selector<Extent>{});
    }

    constexpr span<element_type, dynamic_extent>
    subspan(size_type offset, size_type count = dynamic_extent) const noexcept
    {
        return make_subspan(offset, count, subspan_selector<Extent>{});
    }

    // [span.obs], span observers
    constexpr size_type size() const noexcept { return storage_.size(); }

    constexpr size_type size_bytes() const noexcept { return size() * sizeof(element_type); }

    constexpr bool empty() const noexcept { return size() == 0; }

    // [span.elem], span element access
    // clang-format off
    GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
    // clang-format on
    constexpr reference operator[](size_type idx) const noexcept
    {
        Expects(idx < size());
        return data()[idx];
    }

    constexpr reference front() const noexcept
    {
        Expects(size() > 0);
        return data()[0];
    }

    constexpr reference back() const noexcept
    {
        Expects(size() > 0);
        return data()[size() - 1];
    }

    constexpr pointer data() const noexcept { return storage_.data(); }

    // [span.iter], span iterator support
    constexpr iterator begin() const noexcept
    {
        const auto data = storage_.data();
        // clang-format off
        GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
        // clang-format on
        return {data, data + size(), data};
    }

    constexpr iterator end() const noexcept
    {
        const auto data = storage_.data();
        // clang-format off
        GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
        // clang-format on
        const auto endData = data + storage_.size();
        return {data, endData, endData};
    }

    constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
    constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }

#ifdef _MSC_VER
    // Tell MSVC how to unwrap spans in range-based-for
    constexpr pointer _Unchecked_begin() const noexcept { return data(); }
    constexpr pointer _Unchecked_end() const noexcept
    {
        // clang-format off
        GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
        // clang-format on
        return data() + size();
    }
#endif // _MSC_VER

private:
    // Needed to remove unnecessary null check in subspans
    struct KnownNotNull
    {
        pointer p;
    };

    // this implementation detail class lets us take advantage of the
    // empty base class optimization to pay for only storage of a single
    // pointer in the case of fixed-size spans
    template <class ExtentType>
    class storage_type : public ExtentType
    {
    public:
        // KnownNotNull parameter is needed to remove unnecessary null check
        // in subspans and constructors from arrays
        template <class OtherExtentType>
        constexpr storage_type(KnownNotNull data, OtherExtentType ext)
            : ExtentType(ext), data_(data.p)
        {}

        template <class OtherExtentType>
        constexpr storage_type(pointer data, OtherExtentType ext) : ExtentType(ext), data_(data)
        {
            Expects(data || ExtentType::size() == 0);
        }

        constexpr pointer data() const noexcept { return data_; }

    private:
        pointer data_;
    };

    storage_type<details::extent_type<Extent>> storage_;

    // The rest is needed to remove unnecessary null check
    // in subspans and constructors from arrays
    constexpr span(KnownNotNull ptr, size_type count) noexcept : storage_(ptr, count) {}

    template <std::size_t CallerExtent>
    class subspan_selector
    {
    };

    template <std::size_t CallerExtent>
    constexpr span<element_type, dynamic_extent>
    make_subspan(size_type offset, size_type count, subspan_selector<CallerExtent>) const noexcept
    {
        const span<element_type, dynamic_extent> tmp(*this);
        return tmp.subspan(offset, count);
    }

    // clang-format off
    GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
    // clang-format on
    constexpr span<element_type, dynamic_extent>
    make_subspan(size_type offset, size_type count, subspan_selector<dynamic_extent>) const noexcept
    {
        Expects(size() >= offset);

        if (count == dynamic_extent) { return {KnownNotNull{data() + offset}, size() - offset}; }

        Expects(size() - offset >= count);
        return {KnownNotNull{data() + offset}, count};
    }
};

#if (defined(__cpp_deduction_guides) && (__cpp_deduction_guides >= 201611L))

// Deduction Guides
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

#endif // ( defined(__cpp_deduction_guides) && (__cpp_deduction_guides >= 201611L) )

#if defined(GSL_USE_STATIC_CONSTEXPR_WORKAROUND)
#if defined(__clang__) && defined(_MSC_VER) && defined(__cplusplus) && (__cplusplus < 201703L)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated" // Bug in clang-cl.exe which raises a C++17 -Wdeprecated warning about this static constexpr workaround in C++14 mode.
#endif // defined(__clang__) && defined(_MSC_VER) && defined(__cplusplus) && (__cplusplus < 201703L)
template <class ElementType, std::size_t Extent>
constexpr const typename span<ElementType, Extent>::size_type span<ElementType, Extent>::extent;
#if defined(__clang__) && defined(_MSC_VER) && defined(__cplusplus) && (__cplusplus < 201703L)
#pragma clang diagnostic pop
#endif // defined(__clang__) && defined(_MSC_VER) && defined(__cplusplus) && (__cplusplus < 201703L)
#endif

namespace details
{
    // if we only supported compilers with good constexpr support then
    // this pair of classes could collapse down to a constexpr function

    // we should use a narrow_cast<> to go to std::size_t, but older compilers may not see it as
    // constexpr
    // and so will fail compilation of the template
    template <class ElementType, std::size_t Extent>
    struct calculate_byte_size : std::integral_constant<std::size_t, sizeof(ElementType) * Extent>
    {
        static_assert(Extent < dynamic_extent / sizeof(ElementType), "Size is too big.");
    };

    template <class ElementType>
    struct calculate_byte_size<ElementType, dynamic_extent>
        : std::integral_constant<std::size_t, dynamic_extent>
    {
    };
} // namespace details

// [span.objectrep], views of object representation
template <class ElementType, std::size_t Extent>
span<const gsl::impl::byte, details::calculate_byte_size<ElementType, Extent>::value>
as_bytes(span<ElementType, Extent> s) noexcept
{
    using type = span<const gsl::impl::byte, details::calculate_byte_size<ElementType, Extent>::value>;

    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return type{reinterpret_cast<const gsl::impl::byte*>(s.data()), s.size_bytes()};
}

template <class ElementType, std::size_t Extent,
          std::enable_if_t<!std::is_const<ElementType>::value, int> = 0>
span<gsl::impl::byte, details::calculate_byte_size<ElementType, Extent>::value>
as_writable_bytes(span<ElementType, Extent> s) noexcept
{
    using type = span<gsl::impl::byte, details::calculate_byte_size<ElementType, Extent>::value>;

    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return type{reinterpret_cast<gsl::impl::byte*>(s.data()), s.size_bytes()};
}

} // namespace gsl

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)
#endif // _MSC_VER

#if defined(__GNUC__) && __GNUC__ > 6
#pragma GCC diagnostic pop
#endif // __GNUC__ > 6

#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic pop
#endif // __has_warning("-Wunsafe-buffer-usage")
#endif // defined(__clang__)

#endif // GSL_SPAN_H
