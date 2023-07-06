/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATERIAL_MATERIALVALUE_H
#define MATERIAL_MATERIALVALUE_H

#include <string>
// #include <Base/BaseClass.h>

// namespace fs = boost::filesystem;

namespace Materials {

enum MaterialValueType {
    String = 0,
    Int = 1,
    Float = 2,
    Quantity = 3,
    Distribution = 4,
    List = 5,
    Array = 6,
    Table = 7,
    Color = 8,
    Image = 9,
    ImageRef = 10
};

template<class T, MaterialValueType valueType>
class MaterialsExport MaterialValue
{
public:

    MaterialValueType getType() { return valueType; }

    const T &getValue(void) const { return _value; }
    void setValue (const T &value) { _value = value; }

protected:
    T _value;
};

class MaterialsExport MaterialValueString : public MaterialValue<std::string, MaterialValueType::String>
{
public:
    const std::string evaluate(void) const { return _value; }
};

class MaterialsExport MaterialValueInt : public MaterialValue<int, MaterialValueType::Int>
{
public:
    int evaluate(void) const { return _value; }
};

class MaterialsExport MaterialValueFloat : public MaterialValue<double, MaterialValueType::Float>
{
public:
    double evaluate(void) const { return _value; }
};

} // namespace Materials

#endif // MATERIAL_MATERIALVALUE_H
