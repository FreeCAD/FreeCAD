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

//! a class that assists in gathering the applicable text attributes for an svg element

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QFontMetrics>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QString>
# include <QDomElement>
#endif// #ifndef _PreComp_

#include <Mod/TechDraw/App/DrawUtil.h>

#include "SvgAttributeReader.h"

using namespace TechDraw;


//! look for the font-family, font-size and text-anchor that applies to element.  This may require
//! travelling back up the svg tree.
void SvgAttributeReader::findTextAttributesForElement(SvgTextAttributes& attributes, QDomElement element, int maxlevels, int thislevel)
{
    if (thislevel > maxlevels) {
        // we give up here
        return;
    }

    const QString StyleAttrName{QStringLiteral("style")};
    const QString SizeAttrName{QStringLiteral("font-size")};
    const QString FamilyAttrName{QStringLiteral("font-family")};
    const QString AnchorAttrName{QStringLiteral("text-anchor")};

    QString styleValue = element.attribute(StyleAttrName);
    if (!styleValue.isEmpty()) {
        QString styleFamily = findFamilyInStyle(styleValue);
        if (!styleFamily.isEmpty() &&
            attributes.family().isEmpty()) {
            attributes.setFamily(styleFamily);
        }

        double styleSize = findFontSizeInStyle(styleValue);
        if (styleSize != 0  &&
            attributes.fontSize() == 0) {
            attributes.setFontSize(styleSize);
        }

        QString styleAnchor = findAnchorInStyle(styleValue);
        if (!styleAnchor.isEmpty() &&
            attributes.anchor().isEmpty()) {
            attributes.setAnchor(styleAnchor);
        }
    }

    if (attributes.finished()) {
        return;
    }

    // check for a font-size attribute for element
    if (attributes.fontSize() == 0) {
        QString sizeValue = element.attribute(SizeAttrName);
        if (!sizeValue.isEmpty()) {
            auto newSize = findFontSizeInAttribute(sizeValue);
            if (newSize != 0 &&
                attributes.fontSize() == 0) {
                attributes.setFontSize(newSize);
            }
        }
    }

    // check for a font-family attribute for element
    if (attributes.family().isEmpty()) {
        QString familyValue = element.attribute(FamilyAttrName);
        if (!familyValue.isEmpty()) {
            attributes.setFamily(familyValue);
        }
    }

    // check for a text-anchor attribute for element
    if (attributes.anchor().isEmpty()) {
        QString anchorValue = element.attribute(AnchorAttrName);
        if (!anchorValue.isEmpty()) {
            attributes.setAnchor(anchorValue);
        }
    }
    if (!attributes.finished()) {
        // try next level
        auto parent = element.parentNode().toElement();
        if (!parent.isNull()) {
            // look harder
            findTextAttributesForElement(attributes, parent, maxlevels, ++thislevel);
        }
    }
}

QString SvgAttributeReader::findRegexInString(const QRegularExpression& rx, const QString& searchThis)
{
    if (searchThis.isEmpty()) {
        return {};
    }

    QRegularExpressionMatch match;

    int pos{0};
    pos = searchThis.indexOf(rx, 0, &match);
    if (pos == -1) {
        return {};
    }

    return match.captured(match.lastCapturedIndex());
}

//! find the font-family hidden in a style string
QString SvgAttributeReader::findFamilyInStyle(const QString& style)
{
    // /font-family:([^;]+);/gm
    //                                          style="font-family:Arial;...
    QRegularExpression rxFontSize(QString::fromUtf8(R"(font-family:([^;]+)[;"]*)"));
    return findRegexInString(rxFontSize, style);
}


//! find the font-family hidden in a style string
QString SvgAttributeReader::findAnchorInStyle(const QString& style)
{
    //                                          style="text-anchor:middle;">
    QRegularExpression rxTextAnchor(QString::fromUtf8(R"(text-anchor:([^;]+)[;"]*)"));
    return findRegexInString(rxTextAnchor, style);
}


//! find the font size hidden in a style string
double SvgAttributeReader::findFontSizeInStyle(const QString& style)
{
    //                                          style="font-size:2.82222px;">
    QRegularExpression rxFontSize(QString::fromUtf8(R"(font-size:([0-9]*\.?[0-9]*)\D)"));
    auto numberString = findRegexInString(rxFontSize, style);
    if (numberString.isEmpty()) {
        return 0.0;
    }
    return numberString.toDouble();
}


//! find the numbers in a font-size attribute text
double SvgAttributeReader::findFontSizeInAttribute(const QString &attrText)
{
    //                                                 "3.95px"
    QRegularExpression rxFontSize(QString::fromUtf8(R"(([0-9]*\.?[0-9]*)\D)"));
    auto numberString = findRegexInString(rxFontSize, attrText);
    if (numberString.isEmpty()) {
        return 0.0;
    }
    return numberString.toDouble();
}


bool SvgTextAttributes::finished() const
{
    return (!family().isNull() && !anchor().isNull() && fontSize() != 0);
}

