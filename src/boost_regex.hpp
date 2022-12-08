#ifndef FREECAD_REGEX_HPP_WORKAROUND
#define FREECAD_REGEX_HPP_WORKAROUND

#include <boost/version.hpp>

#if BOOST_VERSION >= 107800

// Workaround for boost >= 1.78
#ifdef WIN32
#include <Windows.h>
#endif

#endif
#include <boost/regex.hpp>

#endif // #ifndef FREECAD_REGEX_HPP_WORKAROUND
