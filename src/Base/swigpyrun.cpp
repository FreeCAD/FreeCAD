/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#include <sstream>
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-register"
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined (__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# if __GNUC__ >= 8
# pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
#endif
#include "PyExport.h"
#include "Exception.h"
namespace Swig_python {
#define SWIG_PYTHON_NO_BUILD_NONE
#include <swigpyrun.h>
#include "swigpyrun.inl"
}
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined (__GNUC__)
# pragma GCC diagnostic pop
#endif
#endif // HAVE_SWIG

