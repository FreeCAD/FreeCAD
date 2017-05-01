//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
//////////////////////////////////////////////////////////////////////////////

// sample.h

#if !defined(BOOST_PP_IS_ITERATING)

   #ifndef BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_DETAILS_INCLUDED
   #define BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_DETAILS_INCLUDED

      #include <boost/intrusive/detail/config_begin.hpp>
      #include <boost/intrusive/detail/workaround.hpp>
      #include <boost/intrusive/detail/preprocessor.hpp>
      #include <boost/intrusive/detail/mpl.hpp>
      #include <boost/static_assert.hpp>
      #include <boost/move/move.hpp>

      //Mark that we don't support 0 arg calls due to compiler ICE in GCC 3.4/4.0/4.1 and
      //wrong SFINAE for GCC 4.2/4.3
      #if defined(__GNUC__) && !defined(__clang__) && ((__GNUC__*100 + __GNUC_MINOR__*10) >= 340) && ((__GNUC__*100 + __GNUC_MINOR__*10) <= 430)
      #define BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED
      #elif defined(BOOST_INTEL) && (BOOST_INTEL < 1200 )
      #define BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED
      #endif

      namespace boost_intrusive_has_member_function_callable_with {

      struct dont_care
      {
         dont_care(...);
      };

      struct private_type
      {
         static private_type p;
         private_type const &operator,(int) const;
      };

      typedef char yes_type;            // sizeof(yes_type) == 1
      struct no_type{ char dummy[2]; }; // sizeof(no_type)  == 2

      template<typename T>
      no_type is_private_type(T const &);
      yes_type is_private_type(private_type const &);

      }  //boost_intrusive_has_member_function_callable_with

      #include <boost/intrusive/detail/config_end.hpp>

   #endif   //BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_DETAILS_INCLUDED

#else //!BOOST_PP_IS_ITERATING

   #ifndef  BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME
   #error "BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME not defined!"
   #endif

   #ifndef  BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN
   #error "BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN not defined!"
   #endif

   #ifndef  BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END
   #error "BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END not defined!"
   #endif

   #if BOOST_PP_ITERATION_START() != 0
   #error "BOOST_PP_ITERATION_START() must be zero (0)"
   #endif

   #if BOOST_PP_ITERATION() == 0

      BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN

      template <typename Type>
      class BOOST_PP_CAT(has_member_function_named_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)
      {
         struct BaseMixin
         {
            void BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME();
         };

         struct Base : public ::boost::intrusive::detail::remove_cv<Type>::type, public BaseMixin { Base(); };
         template <typename T, T t> class Helper{};

         template <typename U>
         static boost_intrusive_has_member_function_callable_with::no_type  deduce
            (U*, Helper<void (BaseMixin::*)(), &U::BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME>* = 0);
         static boost_intrusive_has_member_function_callable_with::yes_type deduce(...);

         public:
         static const bool value =
            sizeof(boost_intrusive_has_member_function_callable_with::yes_type) == sizeof(deduce((Base*)(0)));
      };

      #if !defined(BOOST_INTRUSIVE_PERFECT_FORWARDING)

         template<typename Fun, bool HasFunc
                  BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION_FINISH(), BOOST_INTRUSIVE_PP_TEMPLATE_PARAM_VOID_DEFAULT, _)>
         struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME), _impl);
         //!

         template<typename Fun BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION_FINISH(), class P)>
         struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME), _impl)
            <Fun, false BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION_FINISH(), P)>
         {
            static const bool value = false;
         };
         //!

         #if !defined(_MSC_VER) || (_MSC_VER < 1600)

            #if defined(BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED)

            template<typename Fun>
            struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl)
               <Fun, true BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(BOOST_PP_ITERATION_FINISH(), BOOST_PP_ITERATION()), BOOST_INTRUSIVE_PP_IDENTITY, void)>
            {
               //Mark that we don't support 0 arg calls due to compiler ICE in GCC 3.4/4.0/4.1 and
               //wrong SFINAE for GCC 4.2/4.3
               static const bool value = true;
            };

            #else //defined(BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED)

            //Special case for 0 args
            template< class F
                  , std::size_t N =
                        sizeof((boost::move_detail::declval<F>().
                           BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME (), 0))>
            struct BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)
            {
               boost_intrusive_has_member_function_callable_with::yes_type dummy;
               BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)(int);
            };

            //For buggy compilers like MSVC 7.1+ ((F*)0)->func() does not
            //SFINAE-out the zeroarg_checker_ instantiation but sizeof yields to 0.
            template<class F>
            struct BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)<F, 0>
            {
               boost_intrusive_has_member_function_callable_with::no_type dummy;
               BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)(int);
            };

            template<typename Fun>
            struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl)
               <Fun, true BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(BOOST_PP_ITERATION_FINISH(), BOOST_PP_ITERATION()), BOOST_INTRUSIVE_PP_IDENTITY, void)>
            {
               template<class U>
               static BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)<U>
                  Test(BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)<U>*);

               template <class U>
               static boost_intrusive_has_member_function_callable_with::no_type Test(...);

               static const bool value = sizeof(Test< Fun >(0))
                                    == sizeof(boost_intrusive_has_member_function_callable_with::yes_type);
            };
            #endif   //defined(BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED)

         #else //#if !defined(_MSC_VER) || (_MSC_VER < 1600)
            template<typename Fun>
            struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl)
               <Fun, true BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(BOOST_PP_ITERATION_FINISH(), BOOST_PP_ITERATION()), BOOST_INTRUSIVE_PP_IDENTITY, void)>
            {
               template<class U>
               static decltype( boost::move_detail::declval<Fun>().BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME()
                              , boost_intrusive_has_member_function_callable_with::yes_type())
                  Test(Fun*);

               template<class U>
               static boost_intrusive_has_member_function_callable_with::no_type Test(...);

               static const bool value = sizeof(Test<Fun>(0))
                                    == sizeof(boost_intrusive_has_member_function_callable_with::yes_type);
            };
         #endif   //#if !defined(_MSC_VER) || (_MSC_VER < 1600)

      #else   //#if !defined(BOOST_INTRUSIVE_PERFECT_FORWARDING)

         template<typename Fun, bool HasFunc, class ...Args>
         struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl);

         template<typename Fun, class ...Args>
         struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl)
            <Fun, false, Args...>
         {
            static const bool value = false;
         };

         //Special case for 0 args
         template< class F
               , std::size_t N =
                     sizeof((boost::move_detail::declval<F>().
                        BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME (), 0))>
         struct BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)
         {
            boost_intrusive_has_member_function_callable_with::yes_type dummy;
            BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)(int);
         };

         //For buggy compilers like MSVC 7.1+ ((F*)0)->func() does not
         //SFINAE-out the zeroarg_checker_ instantiation but sizeof yields to 0.
         template<class F>
         struct BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)<F, 0>
         {
            boost_intrusive_has_member_function_callable_with::no_type dummy;
            BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)(int);
         };

         template<typename Fun>
         struct BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl)
            <Fun, true>
         {
            #ifdef BOOST_MSVC
               template<class U>
               static decltype( boost::move_detail::declval<Fun>().BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME()
                              , boost_intrusive_has_member_function_callable_with::yes_type())
                  Test(Fun*);
            #else
               template<class U>
               static BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)
                  <U> Test(BOOST_PP_CAT(zeroarg_checker_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)<U>*);
            #endif
			

            template <class U>
            static boost_intrusive_has_member_function_callable_with::no_type Test(...);

            static const bool value = sizeof(Test< Fun >(0))
                                 == sizeof(boost_intrusive_has_member_function_callable_with::yes_type);
         };

         template<typename Fun, class ...DontCares>
         struct BOOST_PP_CAT( funwrap_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME )
            : Fun
         {
            BOOST_PP_CAT( funwrap_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME )();
            using Fun::BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME;

            boost_intrusive_has_member_function_callable_with::private_type
               BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME
                  ( DontCares...)  const;
         };

         template<typename Fun, class ...Args>
         struct BOOST_PP_CAT( BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME), _impl)
            <Fun, true , Args...>
         {
            template<class T>
            struct make_dontcare
            {
               typedef boost_intrusive_has_member_function_callable_with::dont_care type;
            };

            typedef BOOST_PP_CAT( funwrap_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME )
               <Fun, typename make_dontcare<Args>::type...> FunWrap;

            static bool const value = (sizeof(boost_intrusive_has_member_function_callable_with::no_type) ==
                                       sizeof(boost_intrusive_has_member_function_callable_with::is_private_type
                                                ( (::boost::move_detail::declval< FunWrap >().
                                          BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME
                                             ( ::boost::move_detail::declval<Args>()... ), 0) )
                                             )
                                       );
         };

         template<typename Fun, class ...Args>
         struct BOOST_PP_CAT( has_member_function_callable_with_
                            , BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)
            : public BOOST_PP_CAT( BOOST_PP_CAT(has_member_function_callable_with_
                                 , BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl)
               < Fun
               , BOOST_PP_CAT( has_member_function_named_
                             , BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME )<Fun>::value
               , Args... >
         {};

      #endif   //#if !defined(BOOST_INTRUSIVE_PERFECT_FORWARDING)

      BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END

   #else   //BOOST_PP_ITERATION() == 0

      #if !defined(BOOST_INTRUSIVE_PERFECT_FORWARDING)

         BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN

         template<typename Fun>
         struct BOOST_PP_CAT( BOOST_PP_CAT(funwrap, BOOST_PP_ITERATION())
                           , BOOST_PP_CAT(_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME))
            : Fun
         {
            BOOST_PP_CAT( BOOST_PP_CAT(funwrap, BOOST_PP_ITERATION())
                        , BOOST_PP_CAT(_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME))();

            using Fun::BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME;
            boost_intrusive_has_member_function_callable_with::private_type
               BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME
                  ( BOOST_PP_ENUM(BOOST_PP_ITERATION()
                  , BOOST_INTRUSIVE_PP_IDENTITY
                  , boost_intrusive_has_member_function_callable_with::dont_care))  const;
         };

         template<typename Fun BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), class P)>
         struct BOOST_PP_CAT( BOOST_PP_CAT(has_member_function_callable_with_
                            , BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME),_impl)
            <Fun, true
            BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), P)
            BOOST_PP_ENUM_TRAILING( BOOST_PP_SUB(BOOST_PP_ITERATION_FINISH(), BOOST_PP_ITERATION())
                                  , BOOST_INTRUSIVE_PP_IDENTITY
                                  , void)>
         {
            typedef BOOST_PP_CAT( BOOST_PP_CAT(funwrap, BOOST_PP_ITERATION())
                              , BOOST_PP_CAT(_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME))<Fun>
                     FunWrap;
            static bool const value =
            (sizeof(boost_intrusive_has_member_function_callable_with::no_type) ==
               sizeof(boost_intrusive_has_member_function_callable_with::is_private_type
                        (  (boost::move_detail::declval<FunWrap>().
                              BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME
                                 ( BOOST_PP_ENUM( BOOST_PP_ITERATION(), BOOST_INTRUSIVE_PP_DECLVAL, _) ), 0
                           )
                        )
                     )
            );
         };

         BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END
      #endif   //#if !defined(BOOST_INTRUSIVE_PERFECT_FORWARDING)

   #endif   //BOOST_PP_ITERATION() == 0

   #if BOOST_PP_ITERATION() == BOOST_PP_ITERATION_FINISH()

      #if !defined(BOOST_INTRUSIVE_PERFECT_FORWARDING)

         BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN

         template<typename Fun
                  BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION_FINISH(), BOOST_INTRUSIVE_PP_TEMPLATE_PARAM_VOID_DEFAULT, _)>
         struct BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)
            : public BOOST_PP_CAT(BOOST_PP_CAT(has_member_function_callable_with_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME), _impl)
               <Fun, BOOST_PP_CAT(has_member_function_named_, BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME)<Fun>::value
               BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION_FINISH(), P) >
         {};

         BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END

      #endif //#if !defined(BOOST_INTRUSIVE_PERFECT_FORWARDING)

      #undef BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME
      #undef BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN
      #undef BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END

   #endif   //#if BOOST_PP_ITERATION() == BOOST_PP_ITERATION_FINISH()

#endif   //!BOOST_PP_IS_ITERATING
