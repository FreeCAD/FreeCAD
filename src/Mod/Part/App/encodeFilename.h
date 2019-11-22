/***************************************************************************
 *   Copyright (c) 2014 Sebastian Hoogen <github[at]sebastianhoogen.de>    *
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


#ifndef PART_ENCODEFILENAME_H
#define PART_ENCODEFILENAME_H

#if (OCC_VERSION_HEX < 0x060800 && defined(_WIN32))
#include <QString>
#endif

namespace Part
{
inline std::string encodeFilename(std::string fn)
{
#if (OCC_VERSION_HEX < 0x060800 && defined(_WIN32))
    // Workaround to support latin1 characters in path until OCCT supports
    // conversion from UTF8 to wchar_t on windows
    // http://tracker.dev.opencascade.org/view.php?id=22484
    QByteArray str8bit = QString::fromUtf8(fn.c_str()).toLocal8Bit();
    return std::string(str8bit.constData());
#else
    return fn;
#endif
}

} //namespace Part

#endif // PART_ENCODEFILENAME_H
