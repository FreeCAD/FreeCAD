// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#pragma once

#include <string>
#include <string_view>

#include <FCGlobal.h>

namespace App
{

class AppExport ExpressionTokenizer
{
public:
    // Tokenize UTF-8 input and return the completion prefix (UTF-8).
    // `posBytes` is a byte offset into `text`.
    std::string perform(std::string_view text, std::size_t posBytes);

    void getPrefixRange(int& start, int& end) const
    {
        start = prefixStartBytes;
        end = prefixEndBytes;
    }

    void updatePrefixEnd(int endBytes)
    {
        prefixEndBytes = endBytes;
    }

private:
    int prefixStartBytes = 0;
    int prefixEndBytes = 0;
};

}  // namespace App
