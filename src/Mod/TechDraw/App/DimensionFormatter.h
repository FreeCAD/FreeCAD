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

#include <DrawViewDimension.h>


namespace TechDraw {

class TechDrawExport DimensionFormatter {
public:
    DimensionFormatter() {}
    DimensionFormatter(DrawViewDimension* dim) { m_dimension = dim; }
    ~DimensionFormatter() = default;

    void setDimension(DrawViewDimension* dim) { m_dimension = dim; }
    bool isMultiValueSchema() const;
    std::string formatValue(const qreal value,
                            const QString& qFormatSpec,
                            const int partial,
                            const bool isDim) const;
    std::string getFormattedToleranceValue(const int partial) const;
    std::pair<std::string, std::string> getFormattedToleranceValues(const int partial) const;
    std::string getFormattedDimensionValue(const int partial) const;
    QStringList getPrefixSuffixSpec(const QString& fSpec) const;
    std::string getDefaultFormatSpec(bool isToleranceFormat) const;
    bool isTooSmall(const double value, const QString& formatSpec) const;
    QString formatValueToSpec(const double value, const QString& formatSpecifier) const;
    bool isNumericFormat(const QString& formatSpecifier) const;

private:
    DrawViewDimension* m_dimension;
};

} //end namespace TechDraw
#endif
