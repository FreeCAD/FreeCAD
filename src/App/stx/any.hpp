//
// Implementation of N4562 std::experimental::any (merged into C++17) for C++11 compilers.
//
// See also:
//   + http://en.cppreference.com/w/cpp/any
//   + http://en.cppreference.com/w/cpp/experimental/any
//   + http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4562.html#any
//   + https://cplusplus.github.io/LWG/lwg-active.html#2509
//
//
// Copyright (c) 2016 Denilson das Mercês Amorim
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Original implementation at 
//    
//  https://github.com/thelink2012/any
//  
//  With modification from
//  
//  https://github.com/tcbrindle/cpp17_headers

#ifndef STX_ANY_HPP_INCLUDED
#define STX_ANY_HPP_INCLUDED

// For BOOST_NOEXCEPT as workaround of VS2013
#include <boost/config.hpp>

#ifndef STX_NAMESPACE_NAME
#define STX_NAMESPACE_NAME App
#endif

// libstdc++ std::experimental::any only works in C++14 mode
#if !defined(STX_NO_STD_ANY) && defined(__GNUC__) && (__cplusplus < 201402)
#define STX_NO_STD_ANY
#endif

#if defined(__has_include) && !defined(STX_NO_STD_ANY)
#    if __has_include(<any>) && (__cplusplus > 201402)
#       include <any>
        namespace STX_NAMESPACE_NAME {
            using std::any;
            using std::any_cast;
            using std::bad_any_cast;
        }
#       define STX_HAVE_STD_ANY 1
#   elif __has_include(<experimental/any>)
#       include <experimental/any>
        namespace STX_NAMESPACE_NAME {
            using std::experimental::any;
            using std::experimental::any_cast;
            using std::experimental::bad_any_cast;
        }
#       define STX_HAVE_STD_ANY 1
#    endif // __hasinclude(any)
#endif // defined(__hasinclude)


#ifndef STX_HAVE_STD_ANY

#include <new>
#include <stdexcept>
#include <typeinfo>
#include <type_traits>

namespace STX_NAMESPACE_NAME
{

class bad_any_cast : public std::bad_cast
{
public:
    const char* what() const BOOST_NOEXCEPT override
    {
        return "bad any cast";
    }
};

class any final
{
public:
    /// Constructs an object of type any with an empty state.
    any() :
        vtable(nullptr)
    {
    }

    /// Constructs an object of type any with an equivalent state as other.
    any(const any& rhs) :
        vtable(rhs.vtable)
    {
        if(!rhs.empty())
        {
            rhs.vtable->copy(rhs.storage, this->storage);
        }
    }

    /// Constructs an object of type any with a state equivalent to the original state of other.
    /// rhs is left in a valid but otherwise unspecified state.
    any(any&& rhs) BOOST_NOEXCEPT :
        vtable(rhs.vtable)
    {
        if(!rhs.empty())
        {
            rhs.vtable->move(rhs.storage, this->storage);
            rhs.vtable = nullptr;
        }
    }

    /// Same effect as this->clear().
    ~any()
    {
        this->clear();
    }

    /// Constructs an object of type any that contains an object of type T direct-initialized with std::forward<ValueType>(value).
    ///
    /// T shall satisfy the CopyConstructible requirements, otherwise the program is ill-formed.
    /// This is because an `any` may be copy constructed into another `any` at any time, so a copy should always be allowed.
    template<typename ValueType, typename = typename std::enable_if<!std::is_same<typename std::decay<ValueType>::type, any>::value>::type>
    any(ValueType&& value)
    {
        static_assert(std::is_copy_constructible<typename std::decay<ValueType>::type>::value,
            "T shall satisfy the CopyConstructible requirements.");
        this->construct(std::forward<ValueType>(value));
    }

    /// Has the same effect as any(rhs).swap(*this). No effects if an exception is thrown.
    any& operator=(const any& rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

    /// Has the same effect as any(std::move(rhs)).swap(*this).
    ///
    /// The state of *this is equivalent to the original state of rhs and rhs is left in a valid
    /// but otherwise unspecified state.
    any& operator=(any&& rhs) BOOST_NOEXCEPT
    {
        any(std::move(rhs)).swap(*this);
        return *this;
    }

    /// Has the same effect as any(std::forward<ValueType>(value)).swap(*this). No effect if a exception is thrown.
    ///
    /// T shall satisfy the CopyConstructible requirements, otherwise the program is ill-formed.
    /// This is because an `any` may be copy constructed into another `any` at any time, so a copy should always be allowed.
    template<typename ValueType, typename = typename std::enable_if<!std::is_same<typename std::decay<ValueType>::type, any>::value>::type>
    any& operator=(ValueType&& value)
    {
        static_assert(std::is_copy_constructible<typename std::decay<ValueType>::type>::value,
            "T shall satisfy the CopyConstructible requirements.");
        any(std::forward<ValueType>(value)).swap(*this);
        return *this;
    }

    /// If not empty, destroys the contained object.
    void clear() BOOST_NOEXCEPT
    {
        if(!empty())
        {
            this->vtable->destroy(storage);
            this->vtable = nullptr;
        }
    }

    /// Returns true if *this has no contained object, otherwise false.
    bool empty() const BOOST_NOEXCEPT
    {
        return this->vtable == nullptr;
    }

    /// If *this has a contained object of type T, typeid(T); otherwise typeid(void).
    const std::type_info& type() const BOOST_NOEXCEPT
    {
        return empty()? typeid(void) : this->vtable->type();
    }

    /// Exchange the states of *this and rhs.
    void swap(any& rhs) BOOST_NOEXCEPT
    {
        if(this->vtable != rhs.vtable)
        {
            any tmp(std::move(rhs));

            // move from *this to rhs.
            rhs.vtable = this->vtable;
            if(this->vtable != nullptr)
            {
                this->vtable->move(this->storage, rhs.storage);
                //this->vtable = nullptr; -- uneeded, see below
            }

            // move from tmp (previously rhs) to *this.
            this->vtable = tmp.vtable;
            if(tmp.vtable != nullptr)
            {
                tmp.vtable->move(tmp.storage, this->storage);
                tmp.vtable = nullptr;
            }
        }
        else // same types
        {
            if(this->vtable != nullptr)
                this->vtable->swap(this->storage, rhs.storage);
        }
    }

private: // Storage and Virtual Method Table

    typedef std::aligned_storage<2 * sizeof(void*), std::alignment_of<void*>::value>::type stack_storage_t;
    union storage_union
    {

        void*               dynamic;
        stack_storage_t     stack;      // 2 words for e.g. shared_ptr
    };

    /// Base VTable specification.
    struct vtable_type
    {
        // Note: The caller is responssible for doing .vtable = nullptr after destructful operations
        // such as destroy() and/or move().

        /// The type of the object this vtable is for.
        const std::type_info& (*type)() BOOST_NOEXCEPT;

        /// Destroys the object in the union.
        /// The state of the union after this call is unspecified, caller must ensure not to use src anymore.
        void(*destroy)(storage_union&) BOOST_NOEXCEPT;

        /// Copies the **inner** content of the src union into the yet unitialized dest union.
        /// As such, both inner objects will have the same state, but on separate memory locations.
        void(*copy)(const storage_union& src, storage_union& dest);

        /// Moves the storage from src to the yet unitialized dest union.
        /// The state of src after this call is unspecified, caller must ensure not to use src anymore.
        void(*move)(storage_union& src, storage_union& dest) BOOST_NOEXCEPT;

        /// Exchanges the storage between lhs and rhs.
        void(*swap)(storage_union& lhs, storage_union& rhs) BOOST_NOEXCEPT;
    };

    /// VTable for dynamically allocated storage.
    template<typename T>
    struct vtable_dynamic
    {
        static const std::type_info& type() BOOST_NOEXCEPT
        {
            return typeid(T);
        }

        static void destroy(storage_union& storage) BOOST_NOEXCEPT
        {
            //assert(reinterpret_cast<T*>(storage.dynamic));
            delete reinterpret_cast<T*>(storage.dynamic);
        }

        static void copy(const storage_union& src, storage_union& dest)
        {
            dest.dynamic = new T(*reinterpret_cast<const T*>(src.dynamic));
        }

        static void move(storage_union& src, storage_union& dest) BOOST_NOEXCEPT
        {
            dest.dynamic = src.dynamic;
            src.dynamic = nullptr;
        }

        static void swap(storage_union& lhs, storage_union& rhs) BOOST_NOEXCEPT
        {
            // just exchage the storage pointers.
            std::swap(lhs.dynamic, rhs.dynamic);
        }
    };

    /// VTable for stack allocated storage.
    template<typename T>
    struct vtable_stack
    {
        static const std::type_info& type() BOOST_NOEXCEPT
        {
            return typeid(T);
        }

        static void destroy(storage_union& storage) BOOST_NOEXCEPT
        {
            reinterpret_cast<T*>(&storage.stack)->~T();
        }

        static void copy(const storage_union& src, storage_union& dest)
        {
            new (&dest.stack) T(reinterpret_cast<const T&>(src.stack));
        }

        static void move(storage_union& src, storage_union& dest) BOOST_NOEXCEPT
        {
            // one of the conditions for using vtable_stack is a nothrow move constructor,
            // so this move constructor will never throw a exception.
            new (&dest.stack) T(std::move(reinterpret_cast<T&>(src.stack)));
            destroy(src);
        }

        static void swap(storage_union& lhs, storage_union& rhs) BOOST_NOEXCEPT
        {
            storage_union tmp_storage;
            move(rhs, tmp_storage);
            move(lhs, rhs);
            move(tmp_storage, lhs);
        }
    };

    /// Whether the type T must be dynamically allocated or can be stored on the stack.
    template<typename T>
    struct requires_allocation :
        std::integral_constant<bool,
                !(std::is_nothrow_move_constructible<T>::value      // N4562 §6.3/3 [any.class]
                  && sizeof(T) <= sizeof(stack_storage_t)
                  && std::alignment_of<T>::value <= std::alignment_of<stack_storage_t>::value)>
    {};

    /// Returns the pointer to the vtable of the type T.
    template<typename T>
    static vtable_type* vtable_for_type()
    {
        using VTableType = typename std::conditional<requires_allocation<T>::value, vtable_dynamic<T>, vtable_stack<T>>::type;
        static vtable_type table = {
            VTableType::type, VTableType::destroy,
            VTableType::copy, VTableType::move,
            VTableType::swap,
        };
        return &table;
    }

protected:
    template<typename T>
    friend const T* any_cast(const any* operand) BOOST_NOEXCEPT;
    template<typename T>
    friend T* any_cast(any* operand) BOOST_NOEXCEPT;

    /// Same effect as is_same(this->type(), t);
    bool is_typed(const std::type_info& t) const
    {
        return is_same(this->type(), t);
    }

    /// Checks if two type infos are the same.
    ///
    /// If ANY_IMPL_FAST_TYPE_INFO_COMPARE is defined, checks only the address of the
    /// type infos, otherwise does an actual comparision. Checking addresses is
    /// only a valid approach when there's no interaction with outside sources
    /// (other shared libraries and such).
    static bool is_same(const std::type_info& a, const std::type_info& b)
    {
#ifdef ANY_IMPL_FAST_TYPE_INFO_COMPARE
        return &a == &b;
#else
        return a == b;
#endif
    }

    /// Casts (with no type_info checks) the storage pointer as const T*.
    template<typename T>
    const T* cast() const BOOST_NOEXCEPT
    {
        return requires_allocation<typename std::decay<T>::type>::value?
            reinterpret_cast<const T*>(storage.dynamic) :
            reinterpret_cast<const T*>(&storage.stack);
    }

    /// Casts (with no type_info checks) the storage pointer as T*.
    template<typename T>
    T* cast() BOOST_NOEXCEPT
    {
        return requires_allocation<typename std::decay<T>::type>::value?
            reinterpret_cast<T*>(storage.dynamic) :
            reinterpret_cast<T*>(&storage.stack);
    }

private:
    storage_union storage; // on offset(0) so no padding for align
    vtable_type*  vtable;

    template<typename ValueType, typename T>
    typename std::enable_if<requires_allocation<T>::value>::type
    do_construct(ValueType&& value)
    {
        storage.dynamic = new T(std::forward<ValueType>(value));
    }

    template<typename ValueType, typename T>
    typename std::enable_if<!requires_allocation<T>::value>::type
    do_construct(ValueType&& value)
    {
        new (&storage.stack) T(std::forward<ValueType>(value));
    }

    /// Chooses between stack and dynamic allocation for the type decay_t<ValueType>,
    /// assigns the correct vtable, and constructs the object on our storage.
    template<typename ValueType>
    void construct(ValueType&& value)
    {
        using T = typename std::decay<ValueType>::type;

        this->vtable = vtable_for_type<T>();

        do_construct<ValueType,T>(std::forward<ValueType>(value));
    }
};



namespace detail
{
    template<typename ValueType>
    inline ValueType any_cast_move_if_true(typename std::remove_reference<ValueType>::type* p, std::true_type)
    {
        return std::move(*p);
    }

    template<typename ValueType>
    inline ValueType any_cast_move_if_true(typename std::remove_reference<ValueType>::type* p, std::false_type)
    {
        return *p;
    }
}

/// Performs *any_cast<add_const_t<remove_reference_t<ValueType>>>(&operand), or throws bad_any_cast on failure.
template<typename ValueType>
inline ValueType any_cast(const any& operand)
{
    auto p = any_cast<typename std::add_const<typename std::remove_reference<ValueType>::type>::type>(&operand);
    if(p == nullptr) throw bad_any_cast();
    return *p;
}

/// Performs *any_cast<remove_reference_t<ValueType>>(&operand), or throws bad_any_cast on failure.
template<typename ValueType>
inline ValueType any_cast(any& operand)
{
    auto p = any_cast<typename std::remove_reference<ValueType>::type>(&operand);
    if(p == nullptr) throw bad_any_cast();
    return *p;
}

///
/// If ANY_IMPL_ANYCAST_MOVEABLE is not defined, does as N4562 specifies:
///     Performs *any_cast<remove_reference_t<ValueType>>(&operand), or throws bad_any_cast on failure.
///
/// If ANY_IMPL_ANYCAST_MOVEABLE is defined, does as LWG Defect 2509 specifies:
///     If ValueType is MoveConstructible and isn't a lvalue reference, performs
///     std::move(*any_cast<remove_reference_t<ValueType>>(&operand)), otherwise
///     *any_cast<remove_reference_t<ValueType>>(&operand). Throws bad_any_cast on failure.
///
template<typename ValueType>
inline ValueType any_cast(any&& operand)
{
// #ifdef ANY_IMPL_ANY_CAST_MOVEABLE
#if 1
    // https://cplusplus.github.io/LWG/lwg-active.html#2509
    using can_move = std::integral_constant<bool,
        std::is_move_constructible<ValueType>::value
        && !std::is_lvalue_reference<ValueType>::value>;
#else
    using can_move = std::false_type;
#endif

    auto p = any_cast<typename std::remove_reference<ValueType>::type>(&operand);
    if(p == nullptr) throw bad_any_cast();
    return detail::any_cast_move_if_true<ValueType>(p, can_move());
}

/// If operand != nullptr && operand->type() == typeid(ValueType), a pointer to the object
/// contained by operand, otherwise nullptr.
template<typename T>
inline const T* any_cast(const any* operand) BOOST_NOEXCEPT
{
    if(operand == nullptr || !operand->is_typed(typeid(T)))
        return nullptr;
    else
        return operand->cast<T>();
}

/// If operand != nullptr && operand->type() == typeid(ValueType), a pointer to the object
/// contained by operand, otherwise nullptr.
template<typename T>
inline T* any_cast(any* operand) BOOST_NOEXCEPT
{
    if(operand == nullptr || !operand->is_typed(typeid(T)))
        return nullptr;
    else
        return operand->cast<T>();
}

} // end namespace

namespace std
{
    inline void swap(STX_NAMESPACE_NAME::any& lhs, STX_NAMESPACE_NAME::any& rhs) BOOST_NOEXCEPT
    {
        lhs.swap(rhs);
    }
}

#endif // STX_HAVE_STD_ANY

#endif //  STX_ANY_HPP_INCLUDED
