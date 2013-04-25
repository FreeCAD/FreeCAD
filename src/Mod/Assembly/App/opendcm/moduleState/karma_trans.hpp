/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2011 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BOOST_SPIRIT_REPOSITORY_KARMA_TRANS
#define BOOST_SPIRIT_REPOSITORY_KARMA_TRANS


#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_same.hpp>


namespace boost { namespace spirit { namespace repository
{
    namespace tag
    {
        struct trans {};
    }

    namespace karma
    {
        // enables trans<T>(f)[...]
        template <typename T, typename F>
        inline
        spirit::stateful_tag_type<F, tag::trans, T> trans(F f)
        {
            return spirit::stateful_tag_type<F, tag::trans, T>(f);
        }

        // enables trans(f)[...]
        template <typename F>
        inline
        spirit::stateful_tag_type<F, tag::trans> trans(F f)
        {
            return spirit::stateful_tag_type<F, tag::trans>(f);
        }
    }
}}}


namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    // enables trans<T>(f)[...]
    template <typename F, typename T>
    struct use_directive<karma::domain, tag::stateful_tag<F, repository::tag::trans, T> >
      : mpl::true_ {};
}} // namespace boost::spirit


namespace boost { namespace spirit { namespace repository {namespace karma
{
    template <typename Subject, typename T, typename F>
    struct trans_directive
      : spirit::karma::unary_generator<trans_directive<Subject, T, F> >
    {
        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
          : mpl::eval_if
            <
                is_same<T, unused_type>
              , traits::attribute_of<subject_type, Context, Iterator>
              , mpl::identity<T>
            >
        {};

        trans_directive(Subject const& subject, F f)
          : subject(subject), f(f)
        {}

        template
        <
            typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute
        >
        bool generate
        (
            OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            return subject.generate(sink, ctx, d, f(attr));
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("trans", subject.what(context));
        }

        Subject subject;
        F f;
    };
}}}} // namespace boost::spirit::repository::karma


namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename F, typename T, typename Subject, typename Modifiers>
    struct make_directive<tag::stateful_tag<F, repository::tag::trans, T>, Subject, Modifiers>
    {
        typedef repository::karma::trans_directive<Subject, T, F> result_type;

        template <typename StatefulTag>
        result_type operator()(
            StatefulTag const& tag, Subject const& subject, unused_type) const
        {
            return result_type(subject, tag.data_);
        }
    };
}}} // namespace boost::spirit::karma


namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename T, typename F>
    struct has_semantic_action<repository::karma::trans_directive<Subject, T, F> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename T, typename F
        , typename Attribute, typename Context, typename Iterator>
    struct handles_container
    <
        repository::karma::trans_directive<Subject, T, F>
      , Attribute, Context, Iterator
    >
      : mpl::false_ {}; // FIXME
}}} // namespace boost::spirit::traits


#endif
