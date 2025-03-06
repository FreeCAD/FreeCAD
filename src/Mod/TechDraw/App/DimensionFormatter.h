/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DIMENSIONFORMATTER_H
#define DIMENSIONFORMATTER_H

#include <Mod/TechDraw/TechDrawGlobal.h>

//#include <DrawViewDimension.h> Cyclic dependency issue!


namespace TechDraw {
class DrawViewDimension;

//TODO: Why is this a class if it has no state???
class TechDrawExport DimensionFormatter {
public:
    enum class Format : int {
        UNALTERED, // return the unaltered user string from the Units subsystem
        FORMATTED, // return value formatted according to the format spec and
                   // preferences for useAltDecimals and showUnits
        UNIT       // return only the unit of measure
    };

    DimensionFormatter() {}
    DimensionFormatter(DrawViewDimension* dim) { m_dimension = dim; }
    ~DimensionFormatter() = default;

    //void setDimension(DrawViewDimension* dim) { m_dimension = dim; }
    bool isMultiValueSchema() const;
    std::string formatValue(const qreal value,
                            const QString& qFormatSpec,
                            const Format partial,
                            const bool isDim) const;
    std::string getFormattedToleranceValue(const Format partial) const;
    std::pair<std::string, std::string> getFormattedToleranceValues(const Format partial) const;
    std::string getFormattedDimensionValue(const Format partial) const;
    QStringList getPrefixSuffixSpec(const QString& fSpec) const;
    std::string getDefaultFormatSpec(bool isToleranceFormat) const;
    bool isTooSmall(const double value, const QString& formatSpec) const;
    QString formatValueToSpec(const double value, QString formatSpecifier) const;
    bool isNumericFormat(const QString& formatSpecifier) const;

private:
    DrawViewDimension* m_dimension;
};

} //end namespace TechDraw
#endif
