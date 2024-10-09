/***************************************************************************
 *   Copyright (c) 2024 Eric Price (CorvusCorax) <eric.price@tuebingen.mpg.de>
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

#ifndef PART_FUZZYHELPER_H
#define PART_FUZZYHELPER_H

#include <Mod/Part/PartGlobal.h>

namespace Part
{

/**
 * @brief The FuzzyHelper class provides a static helper function to determine fuzzy value for a boolean operation
 */
class PartExport FuzzyHelper
{
public:
    static double getDefaultFuzzyValue(const double size);
};


}

#endif // PART_FUZZYHELPER_H
