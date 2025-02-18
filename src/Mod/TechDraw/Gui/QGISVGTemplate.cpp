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

#include "PreCompiled.h"   //NOLINT
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

QGISVGTemplate::QGISVGTemplate(QGSPage* scene) : QGITemplate(scene),
    m_svgItem(new QGraphicsSvgItem(this)),
    m_svgRender(new QSvgRenderer())
{
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

    createClickHandles();

    //convert from pixels or mm or inches in svg file to mm page size
    TechDraw::DrawSVGTemplate* tmplte = getSVGTemplate();
    double xaspect = tmplte->getWidth() / static_cast<double>(size.width());
    double yaspect = tmplte->getHeight() / static_cast<double>(size.height());

    QTransform qtrans;
    qtrans.translate(0.0, Rez::guiX(-tmplte->getHeight()));
    qtrans.scale(Rez::guiX(xaspect), Rez::guiX(yaspect));
    m_svgItem->setTransform(qtrans);

    if (Preferences::lightOnDark()) {
        auto color = PreferencesGui::getAccessibleQColor(QColor(Qt::black));
        auto* colorizeEffect = new QGraphicsColorizeEffect();
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

    return nullptr;
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

        constexpr int MaxLevels{4};
        SvgTextAttributes attributes;
        findTextAttributesForElement(attributes, tspan, MaxLevels);

        auto clickRectSize = calculateClickboxSize(QString::fromStdString(editableValue),
                                                   attributes);

        auto clickWidth  = clickRectSize.width();
        auto clickHeight = clickRectSize.height();

        const QString middleAnchorToken{QString::fromUtf8("middle")};
        const QString endAnchorToken{QString::fromUtf8("end")};

        constexpr double hPad{2.0};
        if (attributes.anchor() == middleAnchorToken) {
            x = x - (clickWidth / static_cast<double>(2)) ;
            x -= (hPad + hPad + hPad);
        } else if (attributes.anchor() == endAnchorToken) {
            x = x - clickWidth;
        } else {
            x -= hPad;
        }

        auto item(new TemplateTextField(this, svgTemplate, name.toStdString()));
        auto autoValue = svgTemplate->getAutofillByEditableName(name);
        item->setAutofill(autoValue);

        // svg positions (0, 0) at upper-left of the page with +Y down (and +X right).
        // in svg the position point of text is on the baseline of the text (the
        // y coord is the baseline).
        // in qt, the position point is upper-left of the text bounding rect (the baseline
        // position is not available).
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


//! estimate the size of the required clickbox from the font family and font size
QSizeF QGISVGTemplate::calculateClickboxSize(const QString& editableValue,
                                             const TechDraw::SvgTextAttributes& attributes) const
{
    constexpr double PixelsPerMM{3.78};     // based on CSS 96px / inch
    constexpr double StdCSSDpi{96};

    auto family = attributes.family().isEmpty() ? QString::fromUtf8("Sans") : attributes.family();

    double textHeight{0};
    if (attributes.size() == 0) {
        textHeight = Preferences::labelFontSizeMM() * PixelsPerMM;  // pixels
    } else {
        textHeight = QGIView::exactFontSize(family.toStdString(), attributes.size());  // pixels
    }

    QFont fontForLength(family);
    fontForLength.setPixelSize(static_cast<int>(textHeight));
    QFontMetricsF qfm{fontForLength};
    auto trect = qfm.tightBoundingRect(editableValue);  // pixels

    auto dpiFont = qfm.fontDpi();
    auto clickWidth  = trect.width() * StdCSSDpi / dpiFont;    // pixels
    auto clickHeight = trect.height() * StdCSSDpi / dpiFont;

    return { clickWidth, clickHeight };
}


//NOLINTNEXTLINE
#include <Mod/TechDraw/Gui/moc_QGISVGTemplate.cpp>
