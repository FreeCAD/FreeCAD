/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PARAMS_HELPER_H
#define PARAMS_HELPER_H

/** \page ParamPage Parameter helper macros
 * \ingroup PATH
 * Collections of macros for managing groups of parameters.
 *
 * \section Motivation
 *
 * For an application like FreeCAD, there are often cases where the same set of
 * parameters are referred in dozons of different places. The macros here is
 * designed to help managing those parameters, so that you can define groups of
 * parameters once, and refer them everywhere in groups with simple macro calls for
 * all kinds of purposes. Any changing, adding and removing of parameters in the
 * group become much easier. And by everywhere, I mean \ref ParamCommon
 * "class definition, implementation", \ref ParamProperty "document object properties",
 * \ref ParamPy "python c++ classes", and even \ref ParamDoc "doc string",
 * pretty much everything except the python code, which although not implemented
 * yet, is in fact also possible to be done using C preprocessor (No one says C
 * preprocessor must produce C code :). It is also possible (not implemented
 * yet) to use macros to generate python wrapper class instead of using
 * FreeCAD's current xml python export.
 *
 * \section Debugging
 *
 * Extensive use of macros has one noticeable disadvantage, though. If some thing
 * goes wrong, the compiler error message is kind of cryptic. If so, first
 * double check your macro definition of the parameter is correctly, not missing
 * or having extra parenthesis or comma.  Then, you can use the CMake
 * intermediate file target to get the preprocessor output for checking. For
 * example, for a file located at \c src/Mod/CAM/App/Area.cpp,
 * \code{.sh}
 *     cd <your_build_dir>/src/Mod/CAM/App
 *     make Area.cpp.i
 * \endcode
 *
 * The preprocessed intermediate output will be at,
 * \code{.sh}
 *     <your_build_dir>/src/Mod/CAM/App.CMakeFiles/Path.dir/Area.cpp.i
 * \endcode
 *
 * \section Introduction of Boost.Preprocessor
 *
 * The macros here make heavy use of the awesome
 * [Boost.Preprocessor](http://www.boost.org/libs/preprocessor/) (short for
 * Boost.PP).  Here are is a brief introduction on Boost.PP concept in order to
 * explain why this marco library is designed the way it is.
 *
 * In Boost.PP, a sequence is defined as,
 * \code{.sh}
 *     (a)(b)(c)...
 * \endcode
 *
 * A sequence cannot be empty. Thus, \c () is not a sequence. And also those
 * <tt>a, b, c</tt> here cannot directly contain <tt>,</tt>. These restriction
 * is due to the fact that <tt>( ) ,</tt> are among those very few special
 * characters recognized by the preprocssor. \c a can itself be a sequence or
 * other Boost.PP types, so by right, our parameter can be defined as something
 * like
 * \code{.sh}
 *     ((type)(name)(default)...)
 * \endcode
 *
 * A bit awkward to write. So another Boost.PP type is chosen, tuple, to define
 * each individual parameter. A tuple is defined as
 * \code{.sh}
 *     (a,b,c ...)
 * \endcode
 *
 * This is why the parameter definition requires a double parenthesis, as shown
 * in the following section.
 *
 * \section Library Overview
 *
 * In this macro library, a parameter is defined using a tuple inside a sequence,
 * \code{.sh}
 *     ((<type>, <arg>, <name>, <default>, <doc>, <seq>, <info>))
 * \endcode
 *
 * - \c type is the type of the parameter. Currently only five types of
 *   parameters are defined, <tt>short, long, double, bool, enum, enum2</tt>.
 *   \enum2 type is the same as \enum with additional information to be able to
 *   map to a user defined C enum type. To add more types, search this file for
 *   keyword \a _short, and supply all relevant macros.  It's quite trivial
 *   actually.
 *
 * - \c arg is the argument name. It is intended to be used as function argument.
 *   By convention, the name shall be all small cases, but that's not required.
 *   This \c arg can be repurposed, if the parameter is not going to be used as
 *   function argument.  The #AREA_PARAMS_CAREA parameters repurposed this field
 *   to CArea internal setting variables to implement save, apply and restore
 *   function using CAreaConfig class.
 *
 * - \c name is normally a %CamelCase name which are used as member variable and
 *   property name. Because of this, make sure the names are unique to avoid
 *   conflicts.
 *
 * - \c default is the default value of this parameter. Right now, you must
 *   supply a default value. Boost.PP has trouble dealing with empty values.
 *   Remember that a sequence cannot be empty. Neither can tuple. Only array,
 *   something like <tt>(0,())</tt> for an empty array. It is awkward to write,
 *   and didn't add much functionality I want, hence the restriction of
 *   non-empty defaults here.
 *
 * - \c doc is a string to describe the variable.
 *
 * - \c seq \anchor ParamSeq. Right now this field is used by \c enum and
 *   \c enum2 type parameter to define its enumerations. As the name suggests,
 *   it must be a sequence.  It is not a tuple because looping through tuple is
 *   not as easy as sequence. Other type of parameter do not need to have this
 *   field
 *
 * - \c info is used to provide the supplimentery information for \c enum2 type
 *   of parameter, which can be converted to a user defined enum type by
 *   #PARAM_ENUM_CONVERT. \c info must be a tuple, with the user defined enum
 *   type as the first element, and a prefix as the second element. For \c enum2
 *   type of parameter, this field is mandatory.
 *
 * The common usage is that you define a macro of a group of parameters. And use
 * the macro helper here to do operation on each parameter in the group. See
 * AreaParams.h file for an example of parameter definitions.
 *
 * Area.h, Area.cpp, FeatureArea.h, FeatureArea.cpp for usage of variouse macros.
 *
 * See struct AreaDoc for an example of doc string generation.
 *
 * Each field of the parameter can be referred to with various
 * \ref ParamAccessor "various accessor macros", and can be easily
 * \ref ParamStringizer "stringified".
 *
 * \anchor ParamField You can also use #PARAM_FIELD(_field,_param) to refer to
 * each field, where \a _field is one of <tt>TYPE, ARG, NAME, DEF, DOC, or SEQ</tt>.
 * And #PARAM_FIELD_STR to stringify.
 *
 * Here \a _param is the parameter definition described above in the form of a
 * Boost.PP tuple, and is usually supplied by various \ref ParamLooper "looper macros"
 *
 * You can of course directly use various Boost.PP sequence looper to pass
 * additional arguments to the operation macro. See #PARAM_PY_DICT_SET_VALUE for
 * an example of using tuple, and the more complex example #PARAM_ENUM_CONVERT
 *
 * Note that when generating comma separated list, the first and last comma are
 * conveniently omitted, so that the macros can be mixed with others intuitively
 */

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/pop_front.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/comparison/greater.hpp>

/** \defgroup ParamHelper Parameters helper macros
 * \ingroup PATH
 * Collections of macros for managing groups of parameters */

/**
 * \defgroup ParamAccessor Field accessors
 * To abstract parameter field details
 * \ingroup ParamHelper
 * @{
 */
#define PARAM_ITYPE 0
#define PARAM_IARG  1
#define PARAM_INAME 2
#define PARAM_IDEF  3
#define PARAM_IDOC  4
#define PARAM_ISEQ  5
#define PARAM_IPROP  5
#define PARAM_IINFO 6

#define PARAM_FIELD(_idx,_param) BOOST_PP_TUPLE_ELEM(PARAM_I##_idx,_param)

#define PARAM_FTYPE(_param) PARAM_FIELD(TYPE,_param)
#define PARAM_FARG(_param) PARAM_FIELD(ARG,_param)
#define PARAM_FNAME(_param) PARAM_FIELD(NAME,_param)
#define PARAM_FDEF(_param) PARAM_FIELD(DEF,_param)
#define PARAM_FDOC(_param) PARAM_FIELD(DOC,_param)
#define PARAM_FSEQ(_param) PARAM_FIELD(SEQ,_param)
#define PARAM_FPROP(_param) PARAM_FIELD(PROP,_param)
#define PARAM_FINFO(_param) PARAM_FIELD(INFO,_param)
#define PARAM_FENUM_TYPE(_param) BOOST_PP_TUPLE_ELEM(0,PARAM_FINFO(_param))
#define PARAM_FENUM_PREFIX(_param) BOOST_PP_TUPLE_ELEM(1,PARAM_FINFO(_param))
/** @} */


/**
 * \defgroup ParamStringizer Field stringizers
 * \ingroup ParamHelper
 * @{ */
#define PARAM_FIELD_STR(_idx,_param) \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(PARAM_I##_idx,_param))

#define PARAM_FTYPE_STR(_param) PARAM_FIELD_STR(TYPE,_param)
#define PARAM_FARG_STR(_param) PARAM_FIELD_STR(ARG,_param)
#define PARAM_FNAME_STR(_param) PARAM_FIELD_STR(NAME,_param)
#define PARAM_FDEF_STR(_param) PARAM_FIELD_STR(DEF,_param)
/** @} */

/** Helper for #PARAM_FSEQ_STR */
#define PARAM_FSEQ_STR_(_i,_elem) \
    BOOST_PP_COMMA_IF(_i) BOOST_PP_STRINGIZE(_elem)

/** \c SEQ stringizer will stringify each element separately
 *
 * Expands to:
 *      #seq[0], #seq[1] ...
 * \ingroup ParamHelper
 */
#define PARAM_FSEQ_STR(_param) \
    PARAM_FOREACH_I(PARAM_FSEQ_STR_,PARAM_FSEQ(_param))


/** \defgroup ParamLooper Looper macros
 * Macros for looping through sequence to parameters
 * \ingroup ParamHelper
 */

/** Helper for #PARAM_FOREACH */
#define PARAM_FOREACH_(_,_op,_param) _op(_param)

/** Apply macro \a _op to each parameter in sequence \a _seq
 *
 * Operation macro \a _op should be defined as,
 * \code
 *      _op(_param)
 * \endcode
 * \ingroup ParamLooper
 */
#define PARAM_FOREACH(_op,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_FOREACH_,_op,_seq)

/** Helper for #PARAM_FOREACH_I */
#define PARAM_FOREACH_I_(_,_op,_i,_param) _op(_i,_param)

/** Apply macro \a _op to each parameter in sequence \a _seq with additional index
 *
 * Operation macro \a _op should be defined as,
 * \code
 *      _op(_i,_param)
 * \endcode
 * \ingroup ParamLooper
 * */
#define PARAM_FOREACH_I(_op,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_FOREACH_I_,_op,_seq)


/** Helper for #PARAM_TYPED_FOREACH */
#define PARAM_TYPED_FOREACH_(_1,_op,_param) \
    PARAM_TYPED(_op,_param)(_param)

/** Type depended macro construction
 *
 * Convert macro \a _op to \a _op##\<type\>. Note that it only converts the
 * macro name, not contsucts a macro call. To expand to a macro call, simply
 * \code
 *      PARAM_TYPED(_op,_param)(_param)
 * \endcode
 * \ingroup ParamLooper
 */
#define PARAM_TYPED(_op,_param) \
    BOOST_PP_CAT(_op,PARAM_FTYPE(_param))

/** Apply type dependent macro call to a sequence of parameters
 *
 * \a _op will be converted to \a _op##\<type\> for each parameter
 * \ingroup ParamLooper
 */
#define PARAM_TYPED_FOREACH(_op,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_TYPED_FOREACH_,_op,_seq)


/** \defgroup ParamCommon Common helpers
 * \ingroup ParamHelper
 */

#define PARAM_TYPE_short     short
#define PARAM_TYPE_long      long
#define PARAM_TYPE_double    double
#define PARAM_TYPE_bool      bool
#define PARAM_TYPE_enum      short
#define PARAM_TYPE_enum2     short

/** Obtain parameter type
 *
 * The main purpose is to alias enum type to short
 * \ingroup ParamCommon
 */
#define PARAM_TYPE(_param) \
    PARAM_TYPED(PARAM_TYPE_,_param)


/** Helper for #PARAM_DECLARE */
#define PARAM_DECLARE_(_1,_src,_param) \
    PARAM_TYPE(_param) _src(_param);

/**
 * Declares parameters using the given field as name
 *
 * \arg \c _src: \anchor ParamSrc Macro to generate source variable. The
 * signature must be <tt>_src(_param)<\tt>, where \c _param is the tuple
 * defining the parameter.  You pass any of the \ref ParamAccessor "parameter
 * accessors" to directly access the field. Or, supply your own macro to append
 * any prefix as you like. For example:
 * \code{.unparsed}
 *      #define MY_SRC(_param) BOOST_PP_CAT(my,PARAM_FNAME(_param))
 *      ->
 *          my##<name>
 * \endcode
 *
 * Expands to:
 * \code{.unparsed}
 *         type1 _src(_param1);type2 _src(_param2); ...
 * \endcode
 * \ingroup ParamCommon
 */
#define PARAM_DECLARE(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_DECLARE_,_src,_seq)


/** Helper for #PARAM_DECLARE_INIT */
#define PARAM_DECLARE_INIT_(_1,_src,_param) \
    PARAM_TYPE(_param) _src(_param) = PARAM_FDEF(_param);

/**
 * Declares parameters with initialization to default using the given field as
 * name
 *
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 * more details
 *
 * Expands to:
 * \code{.unparsed}
 *         type1 _src(_param1)=_def1;type2 _src(_param2)=_def2; ...
 * \endcode
 * \ingroup ParamCommon
 */
#define PARAM_DECLARE_INIT(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_DECLARE_INIT_,_src,_seq)


#define PARAM_ENUM_DECLARE_enum_(_1,_name,_i,_elem) \
     BOOST_PP_COMMA_IF(_i) BOOST_PP_CAT(_name,_elem)

#define PARAM_ENUM_DECLARE_enum(_param) \
    enum {BOOST_PP_SEQ_FOR_EACH_I(PARAM_ENUM_DECLARE_enum_,PARAM_FNAME(_param),PARAM_FSEQ(_param))};

#define PARAM_ENUM_DECLARE_short(_param)
#define PARAM_ENUM_DECLARE_long(_param)
#define PARAM_ENUM_DECLARE_double(_param)
#define PARAM_ENUM_DECLARE_bool(_param)
#define PARAM_ENUM_DECLARE_enum2 PARAM_ENUM_DECLARE_enum

/** \defgroup ParamEnumHelper Enum convert helpers
 * \ingroup ParamCommon
 */

/** Make anonymous \c enum type
 *
 * Make anonymous \c enum type for \c enum type parameters in \a _seq. All other
 * types are ignored. The enum member is prefixed with \a _name. Expand to:
 * \code{.unparsed}
 *      enum {_name1##_seq1[0], _name1##_seq1[1] ...};
 *      enum {_name2##_seq2[0], _name2##_seq2[1] ...};
 *      ...
 * \endcode
 * \ingroup ParamEnumHelper*/
#define PARAM_ENUM_DECLARE(_seq) \
    PARAM_TYPED_FOREACH(PARAM_ENUM_DECLARE_,_seq)


/** \addgroup ParamEnumHelper Enum convert helpers
 * @{ */
#define PARAM_ENUM_CONVERT_short(...)
#define PARAM_ENUM_CONVERT_long(...)
#define PARAM_ENUM_CONVERT_double(...)
#define PARAM_ENUM_CONVERT_bool(...)
#define PARAM_ENUM_CONVERT_enum(...)
#define PARAM_ENUM_CONVERT_enum2 PARAM_ENUM_CONVERT_SINGLE

#define PARAM_ENUM_CONVERT_enum_(_dst,_name,_prefix,_elem) \
        case BOOST_PP_CAT(_name,_elem):\
            _dst = BOOST_PP_CAT(_prefix,_elem);\
            break;

#define PARAM_ENUM_CONVERT__(_1,_args,_i,_elem) \
        PARAM_ENUM_CONVERT_enum_(BOOST_PP_TUPLE_ELEM(0,_args),\
                                 BOOST_PP_TUPLE_ELEM(1,_args),\
                                 BOOST_PP_TUPLE_ELEM(2,_args),\
                                 _elem);

#define PARAM_ENUM_CONVERT_(_1,_args,_param) \
        PARAM_TYPED(PARAM_ENUM_CONVERT_,_param)(BOOST_PP_TUPLE_ELEM(0,_args),\
                                            BOOST_PP_TUPLE_ELEM(1,_args),\
                                            BOOST_PP_TUPLE_ELEM(2,_args),\
                                            _param)

/** Convert single enum parameter value into user defined enum type
 *
 * This macro is used by #PARAM_ENUM_CONVERT to convert each parameter, but
 * you can use it directly for a single parameter. Check #PARAM_NUM_CONVERT
 * for more detail. Make sure the outer parenthesis of \c _param is stripped,
 * i.e. not double but single parenthesis
 */
#define PARAM_ENUM_CONVERT_SINGLE(_src,_dst,_default,_param) \
        PARAM_FENUM_TYPE(_param) _dst(_param);\
        switch(_src(_param)) {\
        BOOST_PP_SEQ_FOR_EACH_I(PARAM_ENUM_CONVERT__,\
                (_dst(_param),PARAM_FNAME(_param),PARAM_FENUM_PREFIX(_param)),PARAM_FSEQ(_param))\
        default: \
            _default(_param);\
        }

/** Default handling in #PARAM_ENUM_CONVERT and #PARAM_ENUM_CHECK*/
#define PARAM_ENUM_EXCEPT(_param) \
    throw Base::ValueError("invalid value for enum " PARAM_FNAME_STR(_param))

/** @} */

/* Convert ParamHelper defined enum type to user defined ones
 *
 * This assumes the user defined enum type is given in \ref ParamSeq "seq_type"
 * of the parameter definition, and it has the same postfix as the ones
 * specified in \ref ParamSeq "seq" member of the parameter definition. See
 * \ref ParamEnumHelper "here" for implementations
 *
 * \ingroup ParamEnumHelper
 *
 * \arg \c _src: Macro to generate source variable. The signature must be
 * <tt>_src(_param)<\tt>, where \c _param is the tuple defining the parameter.
 * You pass any of the \ref ParamAccessor "parameter accessors" to directly
 * access the field. Or, supply your own macro to append any prefix as you
 * like.
 * \arg \c _dst: Same as above.
 * \arg \c _default: A macro to call for invalid value. Signature should be
 * <tt>_default(_param)<\tt>, where \c _param is the parameter definition. You
 * can use #PARAM_ENUM_EXCEPT to throw Base::ValueError exception in FreeCAD
 * \arg \c _seq: Parameter sequence
 *
 * For example, with the following parameter definition
 * \code{.unparsed}
 * #define MY_PARAM_TEST \
 *      ((enum,test1,Test1,0,"it's a test",(Foo)(Bar),(MyEnum1,myEnum1)) \
 *      ((enum,test2,Test2,0,"it's a test",(Foo)(Bar),(MyEnum2,myEnum2)))
 *
 *  #define MY_DST(_param) BOOST_PP_CAT(my,PARAM_FNAME(_param))
 * \code{.unparsed}
 *
 * calling
 * \code{.unparsed}
 *      PARAM_ENUM_CONVERT(PARAM_FNAME,MY_DST,My,PARAM_ENUM_EXCEP,MY_PARAM_TEST)
 * \code{.unparsed}
 *
 * expands to
 * \code{.unparsed}
 *      MyEnum1 myTest1;
 *      switch(Test1) {
 *      case Test1Foo:
 *          myTest1 = myEnum1Foo;
 *          break;
 *      case Test1Bar:
 *          myTest1 = myEnum1Bar;
 *          break;
 *      default:
 *          throw Base::ValueError("invalid value for enum Test1");
 *      }
 *      MyEnum2 myTest2;
 *      switch(Test2) {
 *      case Test1Foo:
 *          myTest2 = myEnum2Foo;
 *          break;
 *      case Test2Bar:
 *          myTest2 = myEnum2Bar;
 *          break;
 *      default:
 *          throw Base::ValueError("invalid value for enum Test2");
 *      }
 * \endcode
 *
 * The above code assumes you've already defined \a Test1 and \a Test2 some
 * where as the source variable.
 */
#define PARAM_ENUM_CONVERT(_src,_dst,_default,_seq) \
        BOOST_PP_SEQ_FOR_EACH(PARAM_ENUM_CONVERT_,(_src,_dst,_default),_seq)


#define PARAM_ENUM_CHECK_short(...)
#define PARAM_ENUM_CHECK_long(...)
#define PARAM_ENUM_CHECK_double(...)
#define PARAM_ENUM_CHECK_bool(...)
#define PARAM_ENUM_CHECK_enum PARAM_ENUM_CHECK_SINGLE
#define PARAM_ENUM_CHECK_enum2 PARAM_ENUM_CHECK_SINGLE

#define PARAM_ENUM_CHECK_enum_(_1,_name,_i,_elem) \
        case BOOST_PP_CAT(_name,_elem): break;

#define PARAM_ENUM_CHECK_(_1,_args,_param) \
        PARAM_TYPED(PARAM_ENUM_CHECK_,_param)(BOOST_PP_TUPLE_ELEM(0,_args),\
                                              BOOST_PP_TUPLE_ELEM(1,_args),\
                                              _param)

#define PARAM_ENUM_CHECK_SINGLE(_src,_default,_param) \
        switch(_src(_param)) {\
        BOOST_PP_SEQ_FOR_EACH_I(PARAM_ENUM_CHECK_enum_,\
                                PARAM_FNAME(_param),PARAM_FSEQ(_param))\
        default: \
            _default(_param);\
        }

/* Validate enum type parameters
 *
 * This macro validates the value a variable of enum type parameters. See
 * similar macro #PARAM_ENUM_CONVERT for detail usage.
 *
 * \ingroup ParamEnumHelper
 *
 * \arg \c _src: Macro to generate source variable. The signature must be
 * <tt>_src(_param)<\tt>, where \c _param is the tuple defining the parameter.
 * You pass any of the \ref ParamAccessor "parameter accessors" to directly
 * access the field. Or, supply your own macro to append any prefix as you
 * like.
 *
 * \arg \c _default: A macro to call for invalid value. Signature should be
 * <tt>_default(_param)<\tt>, where \c _param is the parameter definition. You
 * can use #PARAM_ENUM_EXCEPT to throw Base::ValueError exception in FreeCAD
 *
 * \arg \c _seq: Parameter sequence
 */
#define PARAM_ENUM_CHECK(_src,_default,_seq) \
        BOOST_PP_SEQ_FOR_EACH(PARAM_ENUM_CHECK_,(_src,_default),_seq)


#define PARAM_ENUM_STRING_DECLARE_short(...)
#define PARAM_ENUM_STRING_DECLARE_long(...)
#define PARAM_ENUM_STRING_DECLARE_double(...)
#define PARAM_ENUM_STRING_DECLARE_bool(...)
#define PARAM_ENUM_STRING_DECLARE_enum2 PARAM_ENUM_STRING_DECLARE_enum

/** Helper for #PARAM_ENUM_STRING_DECLARE */
#define PARAM_ENUM_STRING_DECLARE_enum(_prefix,_param) \
    BOOST_PP_CAT(_prefix,PARAM_FNAME(_param))[] = {PARAM_FSEQ_STR(_param),NULL};

/** Helper for #PARAM_ENUM_STRING_DECLARE */
#define PARAM_ENUM_STRING_DECLARE_(_1,_prefix,_param) \
    PARAM_TYPED(PARAM_ENUM_STRING_DECLARE_,_param)(_prefix,_param)

/** Make \c enum string list
 *
 * Roughly translated:
 * \code{.unparsed}
 *      _prefix##_name1[] = {#seq1[0], #seq1[1], ...,NULL};
 *      _prefix##_name2[] = {#seq2[0], #seq2[1], ...,NULL};
 *      ...
 * \endcode
 * Example usage:
 *      PARAM_ENUM_STRING_DECLARE(static const char *Enum, MyParamsSeq)
 * \ingroup ParamEnumHelper
 */
#define PARAM_ENUM_STRING_DECLARE(_prefix,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_ENUM_STRING_DECLARE_,_prefix,_seq)


/** Helper for #PARAM_INIT */
#define PARAM_INIT_(_,_src,_i,_param) \
    BOOST_PP_COMMA_IF(_i) _src(_param)(PARAM_FDEF(_param))

/** Constructor initialization
 *
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 * more details
 *
 * Expand to,
 * \code{.unparsed}
 *       _src(_param1)(def1), _src(_param1)(def2)...
 * \endcode
 * \ingroup ParamCommon
 */
#define PARAM_INIT(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_INIT_,_src,_seq)


/** Helper for #PARAM_OP */
#define PARAM_OP_(_,_args,_param) \
     BOOST_PP_TUPLE_ELEM(0,_args)(_param) BOOST_PP_TUPLE_ELEM(1,_args) \
            BOOST_PP_TUPLE_ELEM(2,_args)(_param);

/** Perform operation on two instance of each parameter in a sequence
 *
 * \arg \c _src: Macro to generate source variable. The signature must be
 * <tt>_src(_param)<\tt>, where \c _param is the tuple defining the parameter.
 * You pass any of the \ref ParamAccessor "parameter accessors" to directly
 * access the field. Or, supply your own macro to append any prefix as you
 * like.
 * \arg \c _op: a boolean operator
 * \arg \c _dst: Same as \c _src above.
 *
 * Expands to:
 * \code{.unparsed}
 *      _src(_param1) _op _src(_param2);
 * \endcode
 *
 * \ingroup ParamCommon
 */
#define PARAM_OP(_src,_op,_dst,_seq) \
     BOOST_PP_SEQ_FOR_EACH(PARAM_COPY_,(_src,_op,_dst),_seq)


/** Helper for #PARAM_ARGS_DEF */
#define PARAM_ARGS_DEF_(_,_src,_i,_param) \
    BOOST_PP_COMMA_IF(_i) PARAM_TYPE(_param) _src(_param)=PARAM_FDEF(_param)

/** Declare the parameters as function argument list with defaults.
 *
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 * more details
 *
 * Expand to:
 * \code{.unparsed}
 *      type1 _src(_param1)=def1, type2 _src(_param1)=def2 ...
 * \endcode
 * \ingroup ParamCommon
 */
#define PARAM_ARGS_DEF(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_ARGS_DEF_,_src,_seq)


/** Helper for #PARAM_ARGS */
#define PARAM_ARGS_(_,_src,_i,_param) \
    BOOST_PP_COMMA_IF(_i) PARAM_TYPE(_param) _src(_param)

/** Declare the parameters as function argument list without defaults.
 *
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 * more details
 *
 * Expand to:
 * \code{.unparsed}
 *      type1 _src(_param1), type2 _src(_param2) ...
 * \endcode
 * \ingroup ParamCommon
 */
#define PARAM_ARGS(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_ARGS_,_src,_seq)


/** \defgroup ParamPy Python helper
 * Helper macros for Python bindings
 * \ingroup ParamHelper
 */

/** \defgroup ParamDoc Python doc helper
 * Generate argument doc string for Python
 * \ingroup ParamPy
 */

/** Helper for #PARAM_PY_DOC_enum */
#define PARAM_PY_DOC_enum_(_i,_elem) \
    BOOST_PP_IF(_i,","," ") #_i "=" #_elem

/** Generate doc for an enum parameter */
#define PARAM_PY_DOC_enum(_field,_param) \
    "\n* " PARAM_FIELD_STR(_field,_param) "(" PARAM_FDEF_STR(_param)"):" \
        PARAM_FOREACH_I(PARAM_PY_DOC_enum_, PARAM_FSEQ(_param)) ". " \
        PARAM_FDOC(_param) "\n"

/* Generate doc for other type of parameter */
#define PARAM_PY_DOC_short(_field,_param) \
    "\n* " PARAM_FIELD_STR(_field,_param) "(" PARAM_FDEF_STR(_param)"): " \
            PARAM_FDOC(_param) "\n"
#define PARAM_PY_DOC_long PARAM_PY_DOC_short
#define PARAM_PY_DOC_double PARAM_PY_DOC_short
#define PARAM_PY_DOC_bool PARAM_PY_DOC_short
#define PARAM_PY_DOC_enum2 PARAM_PY_DOC_enum

#define PARAM_PY_DOC_(_,_field,_param) \
    PARAM_TYPED(PARAM_PY_DOC_,_param)(_field,_param)

/* Generate document of a sequence of parameters
 * \ingroup ParamDoc
 */
#define PARAM_PY_DOC(_field,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_PY_DOC_,_field,_seq)


/** Helper for #PARAM_PY_ARGS_DOC */
#define PARAM_PY_ARGS_DOC_(_,_field,_i,_param) \
    BOOST_PP_IF(_i,", "," ") PARAM_FIELD_STR(_field,_param) "=" PARAM_FDEF_STR(_param)

/** Generate argument list string
 * \arg \c _field: specifies the \ref ParamField "field" to use as name
 *
 * Expand to a single string:
 * \code{.unparsed}
 *      "_field1=_def1,_field2=_def2 ..."
 * \endcode
 *
 * \ingroup ParamDoc
 */
#define PARAM_PY_ARGS_DOC(_field,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_PY_ARGS_DOC_,_field,_seq)


/** Helper for #PARAM_FIELDS */
#define PARAM_FIELDS_(_1,_src,_i,_param) \
    BOOST_PP_COMMA_IF(_i) _src(_param)

/** Expand to a list of the given field in the parameter sequence
 *
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 * more details
 *
 * For example, PARAM_FIELDS(PARAM_FARG, _seq) expands to:
 * \code{.unparsed}
 *      arg1,arg2 ...
 * \endcode
 * \ingroup ParamCommon ParamPy
 */
#define PARAM_FIELDS(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_FIELDS_,_src,_seq)


#define PARAM_PY_CAST_short(_v)  (_v)
#define PARAM_PY_CAST_long(_v)   (_v)
#define PARAM_PY_CAST_double(_v) (_v)
#define PARAM_PY_CAST_bool(_v)   ((_v)?Py_True:Py_False)
#define PARAM_PY_CAST_enum(_v)   (_v)
#define PARAM_PY_CAST_enum2(_v)   (_v)

#define PARAM_CAST_PY_short(_v)  (_v)
#define PARAM_CAST_PY_long(_v)   (_v)
#define PARAM_CAST_PY_double(_v) (_v)
#define PARAM_CAST_PY_bool(_v)   (PyObject_IsTrue(_v)?true:false)
#define PARAM_CAST_PY_enum(_v)   (_v)
#define PARAM_CAST_PY_enum2(_v)   (_v)


/** Helper for #PARAM_PY_FIELDS */
#define PARAM_PY_FIELDS_(_1,_src,_i,_param) \
    BOOST_PP_COMMA_IF(_i) PARAM_TYPED(PARAM_CAST_PY_,_param)(_src(_param))

/** Expand to a comma separated list of the given field in the sequence
 *
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 * more details
 *
 * The field will be casted from python C to C type
 * \ingroup ParamCommon ParamPy
 */
#define PARAM_PY_FIELDS(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_PY_FIELDS_,_src,_seq)


/** Helper for #PARAM_FIELD_STRINGS */
#define PARAM_FIELD_STRINGS_(_1,_field,_i,_param) \
    BOOST_PP_COMMA_IF(_i) PARAM_FIELD_STR(_field,_param)

/** Expand to a list of stringified fields
 * \ingroup ParamStringizer ParamPy
 */
#define PARAM_FIELD_STRINGS(_field,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_FIELD_STRINGS_,_field,_seq)


#define PARAM_PYARG_short  "h"
#define PARAM_PYARG_long   "l"
#define PARAM_PYARG_double "d"
#define PARAM_PYARG_bool   "O"
#define PARAM_PYARG_enum   "h"
#define PARAM_PYARG_enum2  "h"

/** Helper for #PARAM_PY_KWDS */
#define PARAM_PY_KWDS_(_param) \
    PARAM_TYPED(PARAM_PYARG_,_param)

/** Generate a format string for keywords based argument
 * \ingroup ParamPy
 */
#define PARAM_PY_KWDS(_seq) \
    PARAM_FOREACH(PARAM_PY_KWDS_,_seq)

#define PARAM_PY_TYPE_short     short
#define PARAM_PY_TYPE_long      long
#define PARAM_PY_TYPE_double    double
#define PARAM_PY_TYPE_bool      PyObject*
#define PARAM_PY_TYPE_enum      short
#define PARAM_PY_TYPE_enum2     short

/** Helper for #PARAM_PY_DECLARE */
#define PARAM_PY_DECLARE_(_1,_src,_param) \
    PARAM_TYPED(PARAM_PY_TYPE_,_param) _src(_param);

/** Declare field variables for Python C type without initialization
 * \ingroup ParamPy
 */
#define PARAM_PY_DECLARE(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_PY_DECLARE_,_src,_seq)

#define PARAM_PY_INIT_short(_v)     _v
#define PARAM_PY_INIT_long(_v)      _v
#define PARAM_PY_INIT_double(_v)    _v
#define PARAM_PY_INIT_bool(_v)      ((_v)?Py_True:Py_False)
#define PARAM_PY_INIT_enum(_v)      _v
#define PARAM_PY_INIT_enum2(_v)      _v

/** Helper for #PARAM_PY_DECLARE_INIT */
#define PARAM_PY_DECLARE_INIT_(_1,_src,_param) \
    PARAM_TYPED(PARAM_PY_TYPE_,_param) _src(_param) = \
        PARAM_TYPED(PARAM_PY_INIT_,_param)(PARAM_FDEF(_param));

/** Declare field variables of Python c type with initialization to default
 * \ingroup ParamPy
 */
#define PARAM_PY_DECLARE_INIT(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_PY_DECLARE_INIT_,_src,_seq)


/** Helper for #PARAM_REF */
#define PARAM_REF_(_1,_src,_i,_param) \
    BOOST_PP_COMMA_IF(_i) &_src(_param)

/** Generate a list of field references
 *
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 *
 * more details
 * Expand to:
 * \code{.unparsed}
 *      &_src(_param1), &_src(_param1) ...
 * \endcode
 * \ingroup ParamPy
 */
#define PARAM_REF(_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_REF_,_src,_seq)

#define PARAM_CAST_PYOBJ_short(_v)   PyLong_FromLong(_v)
#define PARAM_CAST_PYOBJ_long(_v)    PyLong_FromLong(_v)

#define PARAM_CAST_PYOBJ_double(_v)  PyFloat_FromDouble(_v)
#define PARAM_CAST_PYOBJ_bool(_v)    ((_v)?Py_True:Py_False)
#define PARAM_CAST_PYOBJ_enum        PARAM_CAST_PYOBJ_short
#define PARAM_CAST_PYOBJ_enum2       PARAM_CAST_PYOBJ_short


/** Stringize field to a Python string
 * \ingroup ParamPy ParamStringizer
 */
#define PARAM_PY_STR(_field,_param)  \
    PyUnicode_FromString(PARAM_FIELD_STR(_field,_param))

/** Helper for #PARAM_PY_DICT_SET_VALUE */
#define PARAM_PY_DICT_SET_VALUE_(_1,_args,_param) \
    PyDict_SetItem(BOOST_PP_TUPLE_ELEM(0,_args), \
            PARAM_PY_STR(BOOST_PP_TUPLE_ELEM(1,_args),_param),\
            PARAM_TYPED(PARAM_CAST_PYOBJ_,_param)(\
                BOOST_PP_TUPLE_ELEM(2,_args)(_param)));

/** Populate a Python dict with a structure variable
 *
 * \arg \c _dict: the Python dictionary object
 * \arg \c _field: specifies the \ref ParamField "field" to use as key
 * \arg \c _src: macro to generate source field. See \ref ParamSrc "here" for
 * more details
 *
 * Roughly translated to:
 * \code{.unparsed}
 *      PyDict_SetItem(_dict,#_field1,_src(_param));
 *      PyDict_SetItem(_dict,#_field2,_src(_param));
 *      ...
 * \endcode
 * \ingroup ParamPy
 */
#define PARAM_PY_DICT_SET_VALUE(_dict,_field,_src,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_PY_DICT_SET_VALUE_,(_dict,_field,_src),_seq)


#define PARAM_PY_DICT_DOC_enum_(_i,_elem) \
    BOOST_PP_IF(_i,","," ") #_i "=" #_elem

/** Generate doc for an enum parameter */
#define PARAM_PY_DICT_DOC_enum(_param) \
    "(" PARAM_FDEF_STR(_param) ") - " \
        PARAM_FOREACH_I(PARAM_PY_DOC_enum_, PARAM_FSEQ(_param)) ".\n" \
        PARAM_FDOC(_param) "\n"

/* Generate doc for other type of parameter */
#define PARAM_PY_DICT_DOC_(_param) \
    "(" PARAM_FDEF_STR(_param) ") - " PARAM_FDOC(_param) "\n"


#define PARAM_PY_DICT_DOC_short PARAM_PY_DICT_DOC_
#define PARAM_PY_DICT_DOC_long PARAM_PY_DICT_DOC_
#define PARAM_PY_DICT_DOC_double PARAM_PY_DICT_DOC_
#define PARAM_PY_DICT_DOC_bool PARAM_PY_DICT_DOC_
#define PARAM_PY_DICT_DOC_enum2 PARAM_PY_DICT_DOC_enum

/** Helper for #PARAM_PY_DICT_SET_DOC */
#define PARAM_PY_DICT_SET_DOC_(_1,_args,_param) \
    PyDict_SetItem(BOOST_PP_TUPLE_ELEM(0,_args), \
            PARAM_PY_STR(BOOST_PP_TUPLE_ELEM(1,_args),_param),\
            PyUnicode_FromString(PARAM_TYPED(PARAM_PY_DICT_DOC_,_param)(_param)));

/** Populate a Python dict with the doc field of the parameter sequence
 *
 * \arg \c _dict: the Python dictionary object
 * \arg \c _field: specifies the \ref ParamField "field" to use as key
 *
 * Roughly translated to:
 * \code{.unparsed}
 *      PyDict_SetItem(_dict,#_field1,doc1);
 *      PyDict_SetItem(_dict,#_field1,doc2);
 *      ...
 * \endcode
 * \ingroup ParamDoc
 */
#define PARAM_PY_DICT_SET_DOC(_dict,_field,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_PY_DICT_SET_DOC_,(_dict,_field),_seq)


/** \defgroup ParamProperty Property Macros
 * Helper macros for FreeCAD properties
 * \ingroup ParamHelper
 * @{*/
#define PARAM_PROP_bool(_param) App::PropertyBool PARAM_FNAME(_param)
#define PARAM_PROP_double(_param) PARAM_FPROP(_param) PARAM_FNAME(_param)
#define PARAM_PROP_short(_param) App::PropertyInteger PARAM_FNAME(_param)
#define PARAM_PROP_long(_param) App::PropertyInteger PARAM_FNAME(_param)
#define PARAM_PROP_enum(_param) App::PropertyEnumeration PARAM_FNAME(_param)
#define PARAM_PROP_enum2(_param) App::PropertyEnumeration PARAM_FNAME(_param)
/** @} */

/** Helper for #PARAM_PROP_DECLARE */
#define PARAM_PROP_DECLARE_(_param) \
    PARAM_TYPED(PARAM_PROP_,_param)(_param);

/** Declare FreeCAD properties
 * \ingroup ParamProperty
 */
#define PARAM_PROP_DECLARE(_seq) \
    PARAM_FOREACH(PARAM_PROP_DECLARE_,_seq)

/** Replace FreeCAD #ADD_PROPERTY_TYPE to fix singifying macro */
#define PARAM_ADD_PROPERTY_TYPE(_prop_, _defaultval_, _group_,_type_,_Docu_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    this->_prop_.setContainer(this); \
    propertyData.addProperty(static_cast<App::PropertyContainer*>(this), BOOST_PP_STRINGIZE(_prop_), &this->_prop_, (_group_),(_type_),(_Docu_)); \
  } while (0)

/** Generic property adding */
#define PARAM_PROP_ADD_(_group,_param) \
    PARAM_ADD_PROPERTY_TYPE(PARAM_FNAME(_param), (PARAM_FDEF(_param)),\
            _group,App::Prop_None,PARAM_FDOC(_param));

#define PARAM_PROP_ADD_short PARAM_PROP_ADD_
#define PARAM_PROP_ADD_long PARAM_PROP_ADD_
#define PARAM_PROP_ADD_double PARAM_PROP_ADD_
#define PARAM_PROP_ADD_bool PARAM_PROP_ADD_
#define PARAM_PROP_ADD_enum2 PARAM_PROP_ADD_enum

/** Add \c enum type parameter as property */
#define PARAM_PROP_ADD_enum(_group,_param) \
    PARAM_ADD_PROPERTY_TYPE(PARAM_FNAME(_param), ((long)PARAM_FDEF(_param)),\
            _group,App::Prop_None,PARAM_FDOC(_param));

/** Helper for #PARAM_PROP_ADD */
#define PARAM_PROP_ADD_TYPED(_1,_group,_i,_param) \
    PARAM_TYPED(PARAM_PROP_ADD_,_param)(_group,_param)

/** Add FreeCAD properties
 * \ingroup ParamProperty
 */
#define PARAM_PROP_ADD(_group,_seq) \
    BOOST_PP_SEQ_FOR_EACH_I(PARAM_PROP_ADD_TYPED,_group,_seq)

#define PARAM_PROP_SET_ENUM_short(...)
#define PARAM_PROP_SET_ENUM_long(...)
#define PARAM_PROP_SET_ENUM_bool(...)
#define PARAM_PROP_SET_ENUM_double(...)
#define PARAM_PROP_SET_ENUM_enum2 PARAM_PROP_SET_ENUM_enum

/** Setup \c enum type parameter */
#define PARAM_PROP_SET_ENUM_enum(_prefix,_param) \
    PARAM_FNAME(_param).setEnums(BOOST_PP_CAT(_prefix,PARAM_FNAME(_param)));

/** Helper for #PARAM_PROP_SET_ENUM */
#define PARAM_PROP_SET_ENUM_TYPED(_1,_prefix,_param) \
    PARAM_TYPED(PARAM_PROP_SET_ENUM_,_param)(_prefix,_param)

/* Setup the \c enum string list for \c enum type properties
 * \ingroup ParamProperty
 */
#define PARAM_PROP_SET_ENUM(_prefix,_seq) \
    BOOST_PP_SEQ_FOR_EACH(PARAM_PROP_SET_ENUM_TYPED,_prefix,_seq)


/** Helper for #PARAM_PROP_ARGS */
#define PARAM_PROP_ARGS_(_i,_param) \
    BOOST_PP_COMMA_IF(_i) PARAM_FNAME(_param).getValue()

/** Expand the property list as function arguments
 *
 * Expand to:
 * \code{.unparsed}
 *      name1.getValue(), name2.getValue() ...
 * \endcode
 * \ingroup ParamProperty
 */
#define PARAM_PROP_ARGS(_seq) \
    PARAM_FOREACH_I(PARAM_PROP_ARGS_,_seq)


/** Helper for #PARAM_PROP_TOUCHED */
#define PARAM_PROP_TOUCHED_(_param) \
    if(PARAM_FNAME(_param).isTouched()) return 1;

/** Returns 1 if any properties is touched
 *
 * Expand to:
 * \code{.unparsed}
 *      if(name1.isTouched()) return 1;
 *      if(name2.isTouched()) return 1;
 *      ...
 * \ingroup ParamProperty
 */
#define PARAM_PROP_TOUCHED(_seq) \
    PARAM_FOREACH(PARAM_PROP_TOUCHED_,_seq)

#endif // PARAMS_HELPER_H
