// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

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
