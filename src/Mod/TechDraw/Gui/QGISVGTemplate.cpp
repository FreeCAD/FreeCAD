/***************************************************************************
 *   Copyright (c) 2012-2014 Luke Parry <l.parry@warwick.ac.uk>            *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QDomDocument>
# include <QFile>
# include <QFontMetrics>
# include <QGraphicsColorizeEffect>
# include <QGraphicsEffect>
# include <QGraphicsSvgItem>
# include <QPen>
# include <QSvgRenderer>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
#endif// #ifndef _PreComp_

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/XMLQuery.h>

#include "QGISVGTemplate.h"
#include "QGIView.h"
#include "PreferencesGui.h"
#include "QGSPage.h"
#include "Rez.h"
#include "TemplateTextField.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGISVGTemplate::QGISVGTemplate(QGSPage* scene) : QGITemplate(scene), firstTime(true)
{

    m_svgItem = new QGraphicsSvgItem(this);
    m_svgRender = new QSvgRenderer();

    m_svgItem->setSharedRenderer(m_svgRender);

    m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_svgItem->setCacheMode(QGraphicsItem::NoCache);

    addToGroup(m_svgItem);

    m_svgItem->setZValue(ZVALUE::SVGTEMPLATE);
    setZValue(ZVALUE::SVGTEMPLATE);
}

QGISVGTemplate::~QGISVGTemplate() { delete m_svgRender; }

void QGISVGTemplate::openFile(const QFile& file) { Q_UNUSED(file); }

void QGISVGTemplate::load(const QByteArray& svgCode)
{
    m_svgRender->load(svgCode);

    QSize size = m_svgRender->defaultSize();
    m_svgItem->setSharedRenderer(m_svgRender);

    if (firstTime) {
        createClickHandles();
        firstTime = false;
    }

    //convert from pixels or mm or inches in svg file to mm page size
    TechDraw::DrawSVGTemplate* tmplte = getSVGTemplate();
    double xaspect, yaspect;
    xaspect = tmplte->getWidth() / static_cast<double>(size.width());
    yaspect = tmplte->getHeight() / static_cast<double>(size.height());

    QTransform qtrans;
    qtrans.translate(0.0, Rez::guiX(-tmplte->getHeight()));
    qtrans.scale(Rez::guiX(xaspect), Rez::guiX(yaspect));
    m_svgItem->setTransform(qtrans);

    if (Preferences::lightOnDark()) {
        QColor color = PreferencesGui::getAccessibleQColor(QColor(Qt::black));
        QGraphicsColorizeEffect* colorizeEffect = new QGraphicsColorizeEffect();
        colorizeEffect->setColor(color);
        m_svgItem->setGraphicsEffect(colorizeEffect);
    }
    else {
        //remove and delete any existing graphics effect
        if (m_svgItem->graphicsEffect()) {
            m_svgItem->setGraphicsEffect(nullptr);
        }
    }
}

TechDraw::DrawSVGTemplate* QGISVGTemplate::getSVGTemplate()
{
    if (pageTemplate && pageTemplate->isDerivedFrom<TechDraw::DrawSVGTemplate>()) {
        return static_cast<TechDraw::DrawSVGTemplate*>(pageTemplate);
    }
    else {
        return nullptr;
    }
}

void QGISVGTemplate::draw()
{
    TechDraw::DrawSVGTemplate* tmplte = getSVGTemplate();
    if (!tmplte) {
        throw Base::RuntimeError("Template Feature not set for QGISVGTemplate");
    }
    QString templateSvg = tmplte->processTemplate();
    load(templateSvg.toUtf8());
}

void QGISVGTemplate::updateView(bool update)
{
    if (update) {
        clearClickHandles();
        createClickHandles();
    }
    draw();
}

void QGISVGTemplate::clearClickHandles()
{
    constexpr int TemplateTextFieldType{QGraphicsItem::UserType + 160};
    auto templateChildren = childItems();
    for (auto& child : templateChildren) {
        if (child->type() == TemplateTextFieldType) {
            child->hide();
            scene()->removeItem(child);
            delete child;
        }
     }
}

void QGISVGTemplate::createClickHandles()
{
    TechDraw::DrawSVGTemplate* svgTemplate = getSVGTemplate();
    if (svgTemplate->isRestoring()) {
        //the embedded file is not available yet, so just return
        return;
    }

    QString templateFilename(QString::fromUtf8(svgTemplate->PageResult.getValue()));

    if (templateFilename.isEmpty()) {
        return;
    }

    QFile file(templateFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        Base::Console().Error(
            "QGISVGTemplate::createClickHandles - error opening template file %s\n",
            svgTemplate->PageResult.getValue());
        return;
    }

    QDomDocument templateDocument;
    if (!templateDocument.setContent(&file)) {
        Base::Console().Message("QGISVGTemplate::createClickHandles - xml loading error\n");
        return;
    }
    file.close();

    //TODO: Find location of special fields (first/third angle) and make graphics items for them

    QColor editClickBoxColor = PreferencesGui::templateClickBoxColor();

    auto textMap = svgTemplate->EditableTexts.getValues();

    TechDraw::XMLQuery query(templateDocument);

    // XPath query to select all <text> nodes with "freecad:editable" attribute
    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    // this is effectively a loop with each pass using the next <text>/<tspan>
    query.processItems(QString::fromUtf8("declare default element namespace \"" SVG_NS_URI "\"; "
                                         "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
                                         "//text[@" FREECAD_ATTR_EDITABLE "]/tspan"),
                       [&](QDomElement& tspan) -> bool {

        QDomElement textElement = tspan.parentNode().toElement();

        double x = Rez::guiX(
            textElement.attribute(QString::fromUtf8("x"), QString::fromUtf8("0.0")).toDouble());
        double y = Rez::guiX(
            textElement.attribute(QString::fromUtf8("y"), QString::fromUtf8("0.0")).toDouble());

        QString name = textElement.attribute(QString::fromUtf8(FREECAD_ATTR_EDITABLE));
        if (name.isEmpty()) {
            Base::Console().Warning(
                "QGISVGTemplate::createClickHandles - no name for editable text at %f, %f\n", x, y);
            return true;
        }

        std::string editableValue = textMap[name.toStdString()];
        if (editableValue.empty()) {
            editableValue = " ";
        }


        double textHeight{0};
        constexpr int MaxLevels{4};
        TextAttributes attributes;
        findTextAttributesForElement(attributes, tspan, MaxLevels);

        auto family = attributes.family().isEmpty() ? QString::fromUtf8("Sans") : attributes.family();
        auto align = attributes.align().isEmpty() ? QString::fromUtf8("start") : attributes.align();
        constexpr double PixelsPerMM{3.78};     // based on 96px / inch
        if (attributes.size() == 0) {
            textHeight = Preferences::labelFontSizeMM() * PixelsPerMM;  // pixels
        } else {
            textHeight = QGIView::exactFontSize(family.toStdString(), attributes.size());  // pixels
        }

        QGraphicsTextItem textItemForLength;
        QFont fontForLength(family);
        fontForLength.setPixelSize(static_cast<int>(textHeight));      // px is really mm if viewbox = page size
        textItemForLength.setFont(fontForLength);
        textItemForLength.setPlainText(QString::fromStdString(editableValue));
        QFontMetricsF qfm{fontForLength};
        auto trect = qfm.tightBoundingRect(QString::fromStdString(editableValue));  // pixels

        constexpr double StdDpi{96};
        auto dpiFont = qfm.fontDpi();

        auto clickWidth  = trect.width() * StdDpi / dpiFont;    // pixels but we want mm
        auto clickHeight = trect.height() * StdDpi / dpiFont;

        const QString middleAnchorToken{QString::fromUtf8("middle")};
        const QString endAnchorToken{QString::fromUtf8("end")};

        constexpr double hPad{2.0};
        if (align == middleAnchorToken) {
            x = x - (clickWidth / static_cast<double>(2)) ;
            x -= (hPad + hPad + hPad);
        } else if (align == endAnchorToken) {
            x = x - clickWidth;
        } else {
            x -= hPad;
        }

        auto item(new TemplateTextField(this, svgTemplate, name.toStdString()));
        auto autoValue = svgTemplate->getAutofillByEditableName(name);
        item->setAutofill(autoValue);

        // in svg the position point of text is on the baseline of the text.  in qt,
        // the position point is upper-left of the text bounding rect.
        // svg positions (0, 0) at upper-left of the page with +Y down (and +X right).
        // our scene coordinates have (0, 0) at lower-left with +Y down.

        auto bottomOfText = Rez::guiX(-svgTemplate->getHeight()) + y;
        constexpr double vPad{2.0};
        auto topOfText = bottomOfText - clickHeight + vPad;
        auto underlineY = bottomOfText;
        underlineY  += vPad;   // move down a bit


        QRectF clickrect{ x,  topOfText, clickWidth, clickHeight};
        item->setRectangle(clickrect);
        item->setLine(QPointF(x, underlineY),
                      QPointF(x + clickWidth, underlineY));
        item->setLineColor(editClickBoxColor);

        item->setZValue(ZVALUE::SVGTEMPLATE + 1);
        addToGroup(item);

        textFields.push_back(item);
        return true;
    });
}

// class TextInterpreter begins
void QGISVGTemplate::findTextAttributesForElement(TextAttributes& attributes, QDomElement element, int maxlevels, int thislevel)
{
    if (thislevel > maxlevels) {
        // we give up here
        return;
    }

    const QString StyleAttrName{QString::fromUtf8("style")};
    const QString SizeAttrName{QString::fromUtf8("font-size")};
    const QString FamilyAttrName{QString::fromUtf8("font-family")};
    const QString AnchorAttrName{QString::fromUtf8("text-anchor")};

    QString styleValue = element.attribute(StyleAttrName);
    if (!styleValue.isEmpty()) {
        QString styleFamily = findFamilyInStyle(styleValue);
        if (!styleFamily.isEmpty() &&
            attributes.family().isEmpty()) {
            attributes.setFamily(styleFamily);
        }

        double styleSize = findFontSizeInStyle(styleValue);
        if (styleSize != 0  &&
            attributes.size() == 0) {
            attributes.setSize(styleSize);
        }

        QString styleAlign = findAlignInStyle(styleValue);
        if (!styleAlign.isEmpty() &&
            attributes.align().isEmpty()) {
            attributes.setAlign(styleAlign);
        }
    }

    if (attributes.finished()) {
        return;
    }

    // check for a font-size attribute for element
    if (attributes.size() == 0) {
        QString sizeValue = element.attribute(SizeAttrName);
        if (!sizeValue.isEmpty()) {
            auto newSize = findFontSizeInAttribute(sizeValue);
            if (newSize != 0 &&
                attributes.size() == 0) {
                attributes.setSize(newSize);
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
    if (attributes.align().isEmpty()) {
        QString anchorValue = element.attribute(AnchorAttrName);
        if (!anchorValue.isEmpty()) {
            attributes.setAlign(anchorValue);
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

QString QGISVGTemplate::findRegexInString(QRegularExpression rx, QString searchThis)
{
    if (searchThis.isEmpty()) {
        return {};
    }

    QRegularExpressionMatch match;

    int pos{0};
    pos = searchThis.indexOf(rx, 0, &match);
    if (pos != -1) {
        return match.captured(match.lastCapturedIndex());
    }

    return {};
}


//! find the font-family hidden in a style string
QString QGISVGTemplate::findFamilyInStyle(QString style)
{
    // /font-family:([^;]+);/gm
    //                                          style="font-family:Arial;...
    QRegularExpression rxFontSize(QString::fromUtf8(R"(font-family:([^;]+)[;"]*)"));
    return findRegexInString(rxFontSize, style);
}


//! find the font-family hidden in a style string
QString QGISVGTemplate::findAlignInStyle(QString style)
{
    //                                          style="text-anchor:middle;">
    QRegularExpression rxTextAnchor(QString::fromUtf8(R"(text-anchor:([^;]+)[;"]*)"));
    return findRegexInString(rxTextAnchor, style);
}


//! find the font size hidden in a style string
double QGISVGTemplate::findFontSizeInStyle(QString style)
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
double QGISVGTemplate::findFontSizeInAttribute(QString attrText)
{
    //                                                 "3.95px"
    QRegularExpression rxFontSize(QString::fromUtf8(R"(([0-9]*\.?[0-9]*)\D)"));
    auto numberString = findRegexInString(rxFontSize, attrText);
    if (numberString.isEmpty()) {
        return 0.0;
    }
    return numberString.toDouble();
}
// end TextInterpreter

bool TextAttributes::finished() const
{
    return !(family().isNull() || align().isNull() || size() == 0);
}



#include <Mod/TechDraw/Gui/moc_QGISVGTemplate.cpp>
