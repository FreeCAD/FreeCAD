#ifndef FREECAD_REGEX_HPP_WORKAROUND
#define FREECAD_REGEX_HPP_WORKAROUND

#include <boost/version.hpp>

// Workaround for boost >= 1.78
#if BOOST_VERSION >= 107800
# ifdef WIN32
#  ifndef NOMINMAX
#   define NOMINMAX
#  endif
#  include <Windows.h>
#  undef BASETYPES
# endif
#endif

#include <boost/regex.hpp>

#endif // #ifndef FREECAD_REGEX_HPP_WORKAROUND
