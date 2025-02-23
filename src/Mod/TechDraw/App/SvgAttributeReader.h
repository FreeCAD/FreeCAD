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

#include <QString>
#include <Mod/TechDraw/TechDrawGlobal.h>

class QDomElement;
class QTransform;

namespace TechDraw
{

struct RotateParameters {
    double degrees{0};
    double xCenter{0};
    double yCenter{0};
    bool isSet{false};
};

struct TranslateParameters {
    double dx{0};
    double dy{0};
    bool isSet{false};
};

class TechDrawExport SvgTextAttributes
{
public:
    SvgTextAttributes() : m_fontSize(0)  {}

// lint will complain about not using std::move, then complain that std::move has no effect
// since the parameters are const
//NOLINTBEGIN
    SvgTextAttributes(const QString&  family,
                      const double& fontSize,
                      const QString&  anchor) :
        m_family(family),
        m_fontSize(fontSize),
        m_anchor(anchor)  {}
//NOLINTEND

    QString family() const  { return m_family; }
    double fontSize() const { return m_fontSize; }
    QString anchor() const { return m_anchor; }

    TranslateParameters translateParameters() const { return m_translate; }
    RotateParameters rotateParameters() const { return m_rotate; }

    void setFamily(const QString& newFamily)  { m_family = newFamily; }
    void setFontSize(double newFontSize)  { m_fontSize = newFontSize; }
    void setAnchor(const QString& newAnchor)  { m_anchor = newAnchor;}

    void setTranslateParameters(double newDx, double newDy);
    void setTranslateParameters(const TranslateParameters& newParms);

    void setRotateParameters(double newAngle,
                             double newXCenter,
                             double newYCenter);
    void setRotateParameters(const RotateParameters& newParms);

    bool finished() const;

private:
    QString m_family;
    double m_fontSize;
    QString m_anchor;
    TranslateParameters m_translate;
    RotateParameters m_rotate;
};


class TechDrawExport SvgAttributeReader
{
public:
    static void findTextAttributesForElement(SvgTextAttributes& attributes,
                                             const QDomElement& element,
                                             int maxlevels,
                                             int thislevel = 0);

    static void findAttributesInStyle(const QString& style, SvgTextAttributes& attributes);
    static QString findFamilyInStyle(const QString& styleValue);
    static QString findAnchorInStyle(const QString& styleValue);
    static double findFontSizeInStyle(const QString& style);

    static void lookForFontSizeAttribute(const QDomElement& element, SvgTextAttributes& attributes);
    static double findFontSizeInAttribute(const QString& fontSizeAttributeText);

    static void lookForTransformAttribute(const QDomElement& element, SvgTextAttributes& attributes);
    static RotateParameters findRotateInTransform(const QString& transformValue);
    static TranslateParameters findTranslateInTransform(const QString& transformValue);

    static QString findRegexInString(const QRegularExpression& rx, const QString &searchThis);
    static QStringList findMultiRegexInString(const QRegularExpression& rx,
                                              const QString& searchThis,
                                              const int maxResults = 1);


};// class SvgAttributeReader

}// namespace TechDrawGui

#endif  // TECHDRAW_SVGATTRIBUTEREADER_H

