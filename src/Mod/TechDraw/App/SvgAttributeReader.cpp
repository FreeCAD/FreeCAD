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
# include <QStringLiteral>
# include <QDomElement>
# include <QTransform>
#endif// #ifndef _PreComp_

#include <Mod/TechDraw/App/DrawUtil.h>

#include "SvgAttributeReader.h"

using namespace TechDraw;


//! look for the font-family, font-size and text-anchor that applies to element.  This may require
//! travelling back up the svg tree.
void SvgAttributeReader::findTextAttributesForElement(SvgTextAttributes& attributes,
                                                      const QDomElement &element,
                                                      int maxlevels,
                                                      int thislevel)
{
    if (thislevel > maxlevels) {
        // we give up here
        return;
    }

    const QString StyleAttrName{QStringLiteral("style")};
    const QString FamilyAttrName{QStringLiteral("font-family")};
    const QString AnchorAttrName{QStringLiteral("text-anchor")};

    // get values affecting the text from element's style attribute
    QString styleValue = element.attribute(StyleAttrName);
    if (!styleValue.isEmpty()) {
        findAttributesInStyle(styleValue, attributes);
    }

    //! ok to bail w/o looking for transform???
    if (attributes.finished()) {
        return;
    }

    // get values affecting the text from element's other attributes
    if (attributes.fontSize() == 0) {
        lookForFontSizeAttribute(element, attributes);
    }

    if (attributes.family().isEmpty()) {
        QString familyValue = element.attribute(FamilyAttrName);
        if (!familyValue.isEmpty()) {
            attributes.setFamily(familyValue);
        }
    }

    if (attributes.anchor().isEmpty()) {
        QString anchorValue = element.attribute(AnchorAttrName);
        if (!anchorValue.isEmpty()) {
            attributes.setAnchor(anchorValue);
        }
    }

    lookForTransformAttribute(element, attributes);

    if (!attributes.finished()) {
        // try next level
        auto parent = element.parentNode().toElement();
        if (!parent.isNull()) {
            // look harder
            findTextAttributesForElement(attributes, parent, maxlevels, ++thislevel);
        }
    }
}

//! get the font-family, font-size and text-anchor from a style attribute's text
void SvgAttributeReader::findAttributesInStyle(const QString& styleValue,
                                               SvgTextAttributes& attributes)
{
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

//! check element for a transform attribute and update attributes if found
void SvgAttributeReader::lookForTransformAttribute(const QDomElement& element,
                                                   SvgTextAttributes& attributes)
{
    if (attributes.rotateParameters().isSet&&
        attributes.translateParameters().isSet) {
        return;
    }

    const QString TransformAttrName{QStringLiteral("transform")};
    QString transformValue = element.attribute(TransformAttrName);

    if (!transformValue.isEmpty()) {
        if (!attributes.translateParameters().isSet) {
            auto newTranslateParms = findTranslateInTransform(transformValue);
            if (newTranslateParms.isSet) {
                attributes.setTranslateParameters(newTranslateParms);
            }
        }

        if (!attributes.rotateParameters().isSet) {
            auto newRotateParms = findRotateInTransform(transformValue);
            if (newRotateParms.isSet) {
                attributes.setRotateParameters(newRotateParms);
            }
        }
    }
}


//! check element for a font-size attribute and update attributes if found
void SvgAttributeReader::lookForFontSizeAttribute(const QDomElement& element,
                                                  SvgTextAttributes& attributes)
{
    if (attributes.fontSize() != 0) {
        return;
    }

    const QString SizeAttrName{QStringLiteral("font-size")};

    QString sizeValue = element.attribute(SizeAttrName);
    if (!sizeValue.isEmpty()) {
        auto newSize = findFontSizeInAttribute(sizeValue);
        if (newSize != 0) {
            attributes.setFontSize(newSize);
        }
    }
}

//! return the inner-most match from applying rx to searchThis.
QString SvgAttributeReader::findRegexInString(const QRegularExpression& rx,
                                              const QString& searchThis)
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

//! return up to maxResults matches from within the string - ex finds the numbers in
//! "(1, 2, 3)" as "1", "2", "3"
QStringList SvgAttributeReader::findMultiRegexInString(const QRegularExpression& rx,
                                                       const QString& searchThis,
                                                       const int maxResults)
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

    auto strings = match.capturedTexts();

    int availableResults = std::min(strings.size(), maxResults);
    auto itrString = strings.end() - availableResults;
    QStringList results;
    for (; itrString != strings.end(); itrString++) {
        results.append(*itrString);
    }

    return results;
}


//! find the font-family hidden in a style string
QString SvgAttributeReader::findFamilyInStyle(const QString& style)
{
    // /font-family:([^;]+);/gm
    //                                          style="font-family:Arial;...
    QRegularExpression rxFamily(QString::fromUtf8(R"(font-family:([^;]+)[;"]*)"));
    return findRegexInString(rxFamily, style);
}


//! find the text-anchor hidden in a style string
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

TranslateParameters SvgAttributeReader::findTranslateInTransform(const QString& transformValue)
{
    //                                       transform="...translate(-36 45.5)..."
    QRegularExpression rxTranslate(
            QStringLiteral(R"(translate\((-?[0-9]*\.?[0-9]*)[ ,]+(-?[0-9]*\.?[0-9]*)\))"));
    constexpr int NumbersInTranslate{2};
    auto translateStrings = findMultiRegexInString(rxTranslate, transformValue, NumbersInTranslate);
    if (translateStrings.empty()) {
        return {};
    }

    std::vector<double> translateNumbers;
    for (auto& string : translateStrings) {
        bool ok{false};
        auto number = string.toDouble(&ok);
        if (ok) {
            translateNumbers.push_back(number);
        }
    }
    if (translateNumbers.empty()) {
        return {};
    }

    TranslateParameters result;
    result.dx = translateNumbers.front();
    if (translateNumbers.size() == NumbersInTranslate) {
        // the y value is optional
        result.dy = translateNumbers.back();
    }
    result.isSet = true;
    return result;

}


RotateParameters SvgAttributeReader::findRotateInTransform(const QString& transformValue)
{
    QRegularExpression rxRotate(
        QStringLiteral(R"((rotate\((-?[0-9]*\.?[0-9]*)[ ,]+(-?[0-9]*\.?[0-9]*)[ ,]+(-?[0-9]*\.?[0-9]*)\)))"));
//                            9.9          ,         9.9            ,        9.9
    constexpr int NumbersInRotate{3};

    auto rotateStrings = findMultiRegexInString(rxRotate, transformValue, NumbersInRotate);
    if (rotateStrings.empty()) {
        return {};
    }

    std::vector<double> rotateNumbers;
    for (auto& string : rotateStrings) {
        bool ok{false};
        auto number = string.toDouble(&ok);
        if (ok) {
            rotateNumbers.push_back(number);
        }
    }
    if (rotateNumbers.empty()) {
        return {};
    }

    RotateParameters result;
    result.degrees = rotateNumbers.front();
    if (rotateNumbers.size() == NumbersInRotate) {
        // we need 0 or 2 more numbers for the rotation center
        result.xCenter = *(rotateNumbers.end() - 2);
        result.yCenter = rotateNumbers.back();
    }
    result.isSet = true;
    return result;
}


void SvgTextAttributes::setTranslateParameters(double newDx, double newDy)
{
    m_translate.dx = newDx;
    m_translate.dy = newDy;
    m_translate.isSet = true;
}

void SvgTextAttributes::setTranslateParameters(const TranslateParameters &newParms)
{
    setTranslateParameters(newParms.dx, newParms.dy);
}


void SvgTextAttributes::setRotateParameters(double newAngle,
                                            double newXCenter,
                                            double newYCenter)
{
    m_rotate.degrees = newAngle;
    m_rotate.xCenter = newXCenter;
    m_rotate.yCenter = newYCenter;
    m_rotate.isSet = true;

}


void SvgTextAttributes::setRotateParameters(const RotateParameters &newParms)
{
    setRotateParameters(newParms.degrees, newParms.xCenter, newParms.yCenter);
}


bool SvgTextAttributes::finished() const
{
    return (!family().isNull() &&
            !anchor().isNull() &&
            fontSize() != 0  &&
            translateParameters().isSet &&
            rotateParameters().isSet);
}

