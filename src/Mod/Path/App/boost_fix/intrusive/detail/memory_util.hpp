//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Pablo Halpern 2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_ALLOCATOR_MEMORY_UTIL_HPP
#define BOOST_INTRUSIVE_ALLOCATOR_MEMORY_UTIL_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/detail/preprocessor.hpp>

namespace boost {
namespace intrusive {
namespace detail {

template <typename T>
inline T* addressof(T& obj)
{
   return static_cast<T*>
      (static_cast<void*>
         (const_cast<char*>
            (&reinterpret_cast<const char&>(obj))
         )
      );
}

template <typename T> struct unvoid { typedef T type; };
template <> struct unvoid<void> { struct type { }; };
template <> struct unvoid<const void> { struct type { }; };

template <typename T> struct unvoid_ref { typedef T &type; };
template <> struct unvoid_ref<void> { struct type_impl { }; typedef type_impl & type; };
template <> struct unvoid_ref<const void> { struct type_impl { }; typedef type_impl & type; };

template <typename T>
struct LowPriorityConversion
{
    // Convertible from T with user-defined-conversion rank.
    LowPriorityConversion(const T&) { }
};

// Infrastructure for providing a default type for T::TNAME if absent.
#define BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(TNAME)              \
   template <typename T, typename DefaultType>                             \
   struct boost_intrusive_default_type_ ## TNAME                           \
   {                                                                       \
      template <typename X>                                                \
      static char test(int, typename X::TNAME*);                           \
                                                                           \
      template <typename X>                                                \
      static int test(boost::intrusive::detail::                           \
         LowPriorityConversion<int>, void*);                               \
                                                                           \
      struct DefaultWrap { typedef DefaultType TNAME; };                   \
                                                                           \
      static const bool value = (1 == sizeof(test<T>(0, 0)));              \
                                                                           \
      typedef typename                                                     \
         ::boost::intrusive::detail::if_c                                  \
            <value, T, DefaultWrap>::type::TNAME type;                     \
   };                                                                      \
                                                                           \
   template <typename T, typename DefaultType>                             \
   struct boost_intrusive_eval_default_type_ ## TNAME                      \
   {                                                                       \
      template <typename X>                                                \
      static char test(int, typename X::TNAME*);                           \
                                                                           \
      template <typename X>                                                \
      static int test(boost::intrusive::detail::                           \
         LowPriorityConversion<int>, void*);                               \
                                                                           \
      struct DefaultWrap                                                   \
      { typedef typename DefaultType::type TNAME; };                       \
                                                                           \
      static const bool value = (1 == sizeof(test<T>(0, 0)));              \
                                                                           \
      typedef typename                                                     \
         ::boost::intrusive::detail::eval_if_c                             \
            < value                                                        \
            , ::boost::intrusive::detail::identity<T>                      \
            , ::boost::intrusive::detail::identity<DefaultWrap>            \
            >::type::TNAME type;                                           \
   };                                                                      \
//

#define BOOST_INTRUSIVE_OBTAIN_TYPE_WITH_DEFAULT(INSTANTIATION_NS_PREFIX, T, TNAME, TIMPL)   \
      typename INSTANTIATION_NS_PREFIX                                                       \
         boost_intrusive_default_type_ ## TNAME< T, TIMPL >::type                            \
//

#define BOOST_INTRUSIVE_OBTAIN_TYPE_WITH_EVAL_DEFAULT(INSTANTIATION_NS_PREFIX, T, TNAME, TIMPL) \
      typename INSTANTIATION_NS_PREFIX                                                          \
         boost_intrusive_eval_default_type_ ## TNAME< T, TIMPL >::type                          \
//

}}}   //namespace boost::intrusive::detail

#include "has_member_function_callable_with.hpp"

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME pointer_to
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace intrusive { namespace detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 1, "has_member_function_callable_with.hpp"))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME static_cast_from
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace intrusive { namespace detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 1, "has_member_function_callable_with.hpp"))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME const_cast_from
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace intrusive { namespace detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 1, "has_member_function_callable_with.hpp"))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME dynamic_cast_from
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace intrusive { namespace detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 1, "has_member_function_callable_with.hpp"))
#include BOOST_PP_ITERATE()

namespace boost {
namespace intrusive {
namespace detail {

BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(element_type)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(difference_type)

//////////////////////
//struct first_param
//////////////////////

template <typename T> struct first_param
{  typedef void type;   };

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

   template <template <typename, typename...> class TemplateClass, typename T, typename... Args>
   struct first_param< TemplateClass<T, Args...> >
   {
      typedef T type;
   };

#else //C++03 compilers

   #define BOOST_PP_LOCAL_MACRO(n)                                                  \
   template < template <typename                                                    \
               BOOST_PP_ENUM_TRAILING(n, BOOST_INTRUSIVE_PP_IDENTITY, typename) >   \
            class TemplateClass                                                     \
            , typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, class P)>                 \
   struct first_param                                                               \
      < TemplateClass<T BOOST_PP_ENUM_TRAILING_PARAMS(n, P)> >                      \
   {                                                                                \
      typedef T type;                                                               \
   };                                                                               \
   //
   #define BOOST_PP_LOCAL_LIMITS (0, BOOST_INTRUSIVE_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

#endif   //!defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

///////////////////////////
//struct type_rebind_mode
///////////////////////////
template <typename Ptr, typename T>
struct type_has_rebind
{
   template <typename X>
   #if !defined (__SUNPRO_CC)
   static char test(int, typename X::template rebind<T>*);
   #else
   static char test(int, typename X::rebind<T>*);
   #endif

   template <typename X>
   static int test(boost::intrusive::detail::LowPriorityConversion<int>, void*);

   static const bool value = (1 == sizeof(test<Ptr>(0, 0)));
};

template <typename Ptr, typename T>
struct type_has_rebind_other
{
   template <typename X>
   #if !defined (__SUNPRO_CC)
   static char test(int, typename X::template rebind<T>::other*);
   #else
   static char test(int, typename X::rebind<T>::other*);
   #endif

   template <typename X>
   static int test(boost::intrusive::detail::LowPriorityConversion<int>, void*);

   static const bool value = (1 == sizeof(test<Ptr>(0, 0)));
};

template <typename Ptr, typename T>
struct type_rebind_mode
{
   static const unsigned int rebind =       (unsigned int)type_has_rebind<Ptr, T>::value;
   static const unsigned int rebind_other = (unsigned int)type_has_rebind_other<Ptr, T>::value;
   static const unsigned int mode =         rebind + rebind*rebind_other;
};

////////////////////////
//struct type_rebinder
////////////////////////
template <typename Ptr, typename U, unsigned int RebindMode = type_rebind_mode<Ptr, U>::mode>
struct type_rebinder;

// Implementation of pointer_traits<Ptr>::rebind if Ptr has
// its own rebind::other type (C++03)
template <typename Ptr, typename U>
struct type_rebinder< Ptr, U, 2u >
{
   typedef typename Ptr::template rebind<U>::other type;
};

// Implementation of pointer_traits<Ptr>::rebind if Ptr has
// its own rebind template.
template <typename Ptr, typename U>
struct type_rebinder< Ptr, U, 1u >
{
   typedef typename Ptr::template rebind<U> type;
};

// Specialization of pointer_traits<Ptr>::rebind if Ptr does not
// have its own rebind template but has a the form Ptr<class T,
// OtherArgs>, where OtherArgs comprises zero or more type parameters.
// Many pointers fit this form, hence many pointers will get a
// reasonable default for rebind.
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template <template <class, class...> class Ptr, typename T, class... Tn, class U>
struct type_rebinder<Ptr<T, Tn...>, U, 0u >
{
   typedef Ptr<U, Tn...> type;
};

//Needed for non-conforming compilers like GCC 4.3
template <template <class> class Ptr, typename T, class U>
struct type_rebinder<Ptr<T>, U, 0u >
{
   typedef Ptr<U> type;
};

#else //C++03 compilers

#define BOOST_PP_LOCAL_MACRO(n)                                                  \
template < template <typename                                                    \
            BOOST_PP_ENUM_TRAILING(n, BOOST_INTRUSIVE_PP_IDENTITY, typename) >   \
           class Ptr                                                             \
         , typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, class P)                  \
         , class U>                                                              \
struct type_rebinder                                                             \
   < Ptr<T BOOST_PP_ENUM_TRAILING_PARAMS(n, P)>, U, 0u >                         \
{                                                                                \
   typedef Ptr<U BOOST_PP_ENUM_TRAILING_PARAMS(n, P)> type;                      \
};                                                                               \
//
#define BOOST_PP_LOCAL_LIMITS (0, BOOST_INTRUSIVE_MAX_CONSTRUCTOR_PARAMETERS)
#include BOOST_PP_LOCAL_ITERATE()

#endif   //!defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

}  //namespace detail {
}  //namespace intrusive {
}  //namespace boost {

#include <boost/intrusive/detail/config_end.hpp>

#endif // ! defined(BOOST_INTRUSIVE_ALLOCATOR_MEMORY_UTIL_HPP)
