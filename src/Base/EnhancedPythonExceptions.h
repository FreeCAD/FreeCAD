/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_ENHANCED_PYTHON_EXCEPTIONS_H
#define BASE_ENHANCED_PYTHON_EXCEPTIONS_H

#include <fmt/core.h>
#include <CXX/Objects.hxx>

namespace Base
{

/** Triggers an enhanced Python Exception
 *
 * An enhanced Python Exception stores separately a formatter string and arguments. This
 * allows it to produce untranslated and translated dynamic strings (provided that the
 * strings provided are marked for translation using the QT_TRANSLATE_NOOP with the "Notifications"
 * context).
 *
 * @param exceptiontype, one of the types of Python exception (e.g. PyExc_ValueError)
 * @param format string following the syntax of fmt::format
 * @param args one or more args to be substituted in the format string (as in fmt::format)
 *
 * This method expects that any translatable string in format and args is marked with
 * QT_TRANSLATE_NOOP with the context "Notifications". It marks that the string CAN be
 * translated. Failure to provide it with marked strings is a bug by client code and will
 * result in untranslated strings appearing to the user.
 *
 * When a Base::Exception is constructed from an enhanced Python Exception, the what() method
 * provides the formatted untranslated string. The formatted translated string can be obtained
 * using the translateEnhancedMessage method, requiring a translating function. See Base::Exception
 * for more details.
 *
 * Examples:
 * PyError_SetEnhancedString(PyExc_ValueError,
 *               QT_TRANSLATE_NOOP("Notifications", "Invalid constraint index:  {}"), Index);
 *
 *
 */
template<typename... Args>
inline void PyError_SetEnhancedString(PyObject * exceptiontype, const char* format, Args&&... args)
{
    std::string swhat = fmt::format(format, args...);

    Py::Dict d;

    d.setItem("swhat", Py::String(swhat));

    // if it has arguments, then we need to store formater and arguments separately
    if constexpr(sizeof...(args) > 0) {
        d.setItem("sformatter", Py::String(format));

        Py::Tuple t(static_cast<size_t>(sizeof...(args)));

        int i = 0;

        (
            (
                t.setItem(i, Py::String(fmt::format("{}", (std::forward<decltype(args)>(args))))),
                i++
            ),
        ...
        );

        d.setItem("sformatterArguments",t);
    }

    d.setItem("btranslatable", Py::Boolean(true));

    PyErr_SetObject(exceptiontype, Py::new_reference_to(d));
}

} //namespace Base

#endif // BASE_ENHANCED_PYTHON_EXCEPTIONS_H
