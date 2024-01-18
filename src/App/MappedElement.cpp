// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************************************
 *                                                                                                 *
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>                       *
 *   Copyright (c) 2023 FreeCAD Project Association                                                *
 *                                                                                                 *
 *   This file is part of FreeCAD.                                                                 *
 *                                                                                                 *
 *   FreeCAD is free software: you can redistribute it and/or modify it under the terms of the     *
 *   GNU Lesser General Public License as published by the Free Software Foundation, either        *
 *   version 2.1 of the License, or (at your option) any later version.                            *
 *                                                                                                 *
 *   FreeCAD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;          *
 *   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     *
 *   See the GNU Lesser General Public License for more details.                                   *
 *                                                                                                 *
 *   You should have received a copy of the GNU Lesser General Public License along with           *
 *   FreeCAD. If not, see <https://www.gnu.org/licenses/>.                                         *
 *                                                                                                 *
 **************************************************************************************************/

// NOLINTNEXTLINE
#include "PreCompiled.h"

#ifndef _PreComp_
# include <cstdlib>
# include <unordered_set>
#endif

#include "MappedElement.h"

using namespace Data;

bool ElementNameComp::operator()(const MappedName &a, const MappedName &b) const {
    size_t size = std::min(a.size(),b.size());
    if(!size)
        return a.size()<b.size();
    size_t i=0;
    if(b[0] == '#') {
        if(a[0]!='#')
            return true;
        // If both string starts with '#', compare the following hex digits by
        // its integer value.
        int res = 0;
        for(i=1;i<size;++i) {
            unsigned char ac = (unsigned char)a[i];
            unsigned char bc = (unsigned char)b[i];
            if(std::isxdigit(bc)) {
                if(!std::isxdigit(ac))
                    return true;
                if(res==0) {
                    if(ac<bc)
                        res = -1;
                    else if(ac>bc)
                        res = 1;
                }
            }else if(std::isxdigit(ac))
                return false;
            else
                break;
        }
        if(res < 0)
            return true;
        else if(res > 0)
            return false;

        for (; i<size; ++i) {
            char ac = a[i];
            char bc = b[i];
            if (ac < bc)
                return true;
            if (ac > bc)
                return false;
        }
        return a.size()<b.size();
    }
    else if (a[0] == '#')
        return false;

    // If the string does not start with '#', compare the non-digits prefix
    // using lexical order.
    for(i=0;i<size;++i) {
        unsigned char ac = (unsigned char)a[i];
        unsigned char bc = (unsigned char)b[i];
        if(!std::isdigit(bc)) {
            if(std::isdigit(ac))
                return true;
            if(ac<bc)
                return true;
            if(ac>bc)
                return false;
        } else if(!std::isdigit(ac)) {
            return false;
        } else
            break;
    }

    // Then compare the following digits part by integer value
    int res = 0;
    for(;i<size;++i) {
        unsigned char ac = (unsigned char)a[i];
        unsigned char bc = (unsigned char)b[i];
        if(std::isdigit(bc)) {
            if(!std::isdigit(ac))
                return true;
            if(res==0) {
                if(ac<bc)
                    res = -1;
                else if(ac>bc)
                    res = 1;
            }
        }else if(std::isdigit(ac))
            return false;
        else
            break;
    }
    if(res < 0)
        return true;
    else if(res > 0)
        return false;

    // Finally, compare the remaining tail using lexical order
    for (; i<size; ++i) {
        char ac = a[i];
        char bc = b[i];
        if (ac < bc)
            return true;
        if (ac > bc)
            return false;
    }
    return a.size()<b.size();
}
