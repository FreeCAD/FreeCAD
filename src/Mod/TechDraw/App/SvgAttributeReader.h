// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 wandererfan <wandererfan[at]gmail[dot]com>         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

//! classes that assist in gathering the applicable text attributes for an svg element

#ifndef TECHDRAW_SVGATTRIBUTEREADER_H
#define TECHDRAW_SVGATTRIBUTEREADER_H

#include <utility>

#include <QString>
#include <Mod/TechDraw/TechDrawGlobal.h>

class QDomElement;

namespace TechDraw
{

class SvgTextAttributes
{
public:
    SvgTextAttributes() : m_size(0)  {}

    SvgTextAttributes(QString  family, const double& size, QString  anchor) :
        m_family(std::move(family)), m_size(size), m_anchor(std::move(anchor))  {}

    QString family() const  { return m_family; }
    double size() const { return m_size; }
    QString anchor() const { return m_anchor; }

    void setFamily(const QString& newFamily)  { m_family = newFamily; }
    void setSize(double newSize)  { m_size = newSize; }
    void setAnchor(const QString& newAnchor)  { m_anchor = newAnchor;}

    bool finished() const;

private:
    QString m_family;
    double m_size;
    QString m_anchor;
};


class TechDrawGuiExport SvgAttributeReader
{
public:
    static void findTextAttributesForElement(SvgTextAttributes& attributes, QDomElement element, int maxlevels, int thislevel = 0);
    static QString findRegexInString(QRegularExpression rx, QString searchThis);
    static QString findFamilyInStyle(QString styleValue);
    static QString findAnchorInStyle(QString styleValue);
    static double findFontSizeInStyle(QString style);
    static double findFontSizeInAttribute(QString attrText);

};// class SvgAttributeReader

}// namespace TechDrawGui

#endif  // TECHDRAW_SVGATTRIBUTEREADER_H

