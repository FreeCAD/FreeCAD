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
#include "QGIUserTypes.h"
#include "PreferencesGui.h"
#include "QGSPage.h"
#include "Rez.h"
#include "TemplateTextField.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"



namespace {
    QFont getFont(QDomElement& elem)
    {
        if(elem.hasAttribute(QStringLiteral("font-family"))) {
            return elem.attribute(QStringLiteral("font-family"));
        }
        QDomElement parent = elem.parentNode().toElement();
        // Check if has parent
        if(!parent.isNull()) {
            // Traverse up and check if parent node has attribute
            return getFont(parent);
        }
        // No attribute and no parent nodes left? Defaulting:
        return QFont(QStringLiteral("sans"));
    }
    
    std::vector<QDomElement> getFCElements(QDomDocument& doc) {
        QDomNodeList textElements = doc.elementsByTagName(QStringLiteral("text"));
        std::vector<QDomElement> filteredTextElements;
        filteredTextElements.reserve(textElements.size());
        for(int i = 0; i < textElements.size(); i++) {
            QDomElement textElement = textElements.at(i).toElement();
            if(textElement.hasAttribute(QStringLiteral(FREECAD_ATTR_EDITABLE))) {
                filteredTextElements.push_back(textElement);
            }
        }
        return filteredTextElements;
    }

    void applyWorkaround(QByteArray& svgCode)
    {
        QDomDocument doc;
        doc.setContent(svgCode);

        // Example <text font-size="12px"><tspan font-size"12px">sadasd</tspan></text>
        // QSvgRenderer::boundsOnElement calculates the bounds of the text element using text width of the `tspan`
        // but faultly the text height of the `text` element that might have another font size. The width
        // of a `tspan will also always be one character too wide.
        // Workaround: apply the font-size of the `tspan` to its parent `text` and remove `tspan` completely
        std::vector<QDomElement> textElements = getFCElements(doc);
        for(QDomElement& textElement : textElements) {
            QDomElement tspan = textElement.firstChildElement(QStringLiteral("tspan"));
            if(tspan.isNull()) {
                continue;
            }

            if(tspan.hasAttribute(QStringLiteral("font-size"))) {
                QString fontSize = tspan.attribute(QStringLiteral("font-size"));
                textElement.setAttribute(QStringLiteral("font-size"), fontSize);
            }
            if(tspan.hasAttribute(QStringLiteral("x"))) {
                QString x = tspan.attribute(QStringLiteral("x"));
                textElement.setAttribute(QStringLiteral("x"), x);
            }
            if(tspan.hasAttribute(QStringLiteral("y"))) {
                QString y = tspan.attribute(QStringLiteral("y"));
                textElement.setAttribute(QStringLiteral("y"), y);
            }

            // Delete tspan, but keep tspan content
            textElement.replaceChild(tspan.firstChild(), tspan);
        }

        // All `text` elements must have an id for using QSvgRenderer::transformForElement later on
        int counter = 0;
        for(QDomElement& textElement : textElements) {
            if (!textElement.hasAttribute(QStringLiteral("id")) ||
                textElement.attribute(QStringLiteral("id")).isEmpty()) {
                QString id = QStringLiteral("freecad_id_") + QString::number(counter);
                textElement.setAttribute(QStringLiteral("id"), id);
                counter++;
            }
        }

        // QSvgRenderer::transformForElement only returns transform for parents, not for the element itself
        // If the `text` element itself has transform, let's wrap it in a shallow group so it's taken into
        // account by QSvgRenderer::transformForElement
        for(QDomElement& textElement : textElements) {
            if (textElement.hasAttribute(QStringLiteral("transform"))) {
                QDomElement group = doc.createElement(QStringLiteral("g"));
                QString transform = textElement.attribute(QStringLiteral("transform"));
                textElement.removeAttribute(QStringLiteral("transform"));
                group.setAttribute(QStringLiteral("transform"), transform);
                textElement.parentNode().replaceChild(group, textElement);
                group.appendChild(textElement);
            }
        }

        svgCode = doc.toByteArray();
    }
}  // anonymous namespace


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

void QGISVGTemplate::load(QByteArray svgCode)
{
    applyWorkaround(svgCode);
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

std::vector<TemplateTextField*> QGISVGTemplate::getTextFields()
{
    std::vector<TemplateTextField*> result;
    result.reserve(childItems().size());

    QList<QGraphicsItem*> templateChildren = childItems();
    for (auto& child : templateChildren) {
        if (child->type() == UserType::TemplateTextField) {
            result.push_back(static_cast<TemplateTextField*>(child));
        }
    }

    return result;
}

void QGISVGTemplate::clearClickHandles()
{
    prepareGeometryChange();
    std::vector<TemplateTextField*> textFields = getTextFields(); 
    for (auto& textField : textFields) {
        textField->hide();
        scene()->removeItem(textField);
        delete textField;
     }
}

void QGISVGTemplate::createClickHandles()
{
    prepareGeometryChange();
    TechDraw::DrawSVGTemplate* svgTemplate = getSVGTemplate();
    if (svgTemplate->isRestoring()) {
        //the embedded file is not available yet, so just return
        return;
    }

    QByteArray svgCode = svgTemplate->processTemplate().toUtf8();
    applyWorkaround(svgCode);
    QDomDocument templateDocument;
    if (!templateDocument.setContent(svgCode)) {
        Base::Console().message("QGISVGTemplate::createClickHandles - xml loading error\n");
        return;
    }

    //TODO: Find location of special fields (first/third angle) and make graphics items for them

    auto textMap = svgTemplate->EditableTexts.getValues();

    TechDraw::XMLQuery query(templateDocument);


    std::vector<QDomElement> textElements = getFCElements(templateDocument);
    for(QDomElement& textElement : textElements) {
        // Get elements bounding box of text
        QString id = textElement.attribute(QStringLiteral("id"));
        QRectF textRect = m_svgRender->boundsOnElement(id);
        
        // Get tight bounding box of text
        QDomElement tspan = textElement.firstChildElement();
        QFont font = getFont(tspan);
        QFontMetricsF fm(font);
        double factor = textRect.height() / fm.height();  // Correcting font metrics and SVG text due to different font sizes
        QRectF tightTextRect = textRect.adjusted(0.0, 0.0, 0.0, -fm.descent() * factor);
        tightTextRect.setTop(tightTextRect.bottom() - fm.capHeight() * factor);

        // Ensure min size; if no text content, tightTextRect will have no size
        // and factor will also be incorrect

        // Default font size guess. Getting attribute seems complicated, as it can have different units
        // and both be in style attribute and native attribute
        font.setPointSizeF(1.5);
        fm = QFontMetricsF(font);
        
        if (tightTextRect.height() < fm.capHeight()) {
            tightTextRect.setTop(tightTextRect.bottom() - fm.capHeight());
        }
        double charWidth = fm.horizontalAdvance(QLatin1Char(' '));
        if(tightTextRect.width() < charWidth) {
            tightTextRect.setWidth(charWidth);
        }

        // Transform tight bounding box of text
        QPolygonF tightTextPoly(tightTextRect);  // Polygon because rect cannot be rotated
        QTransform SVGTransform = m_svgRender->transformForElement(id);
        tightTextPoly = SVGTransform.map(tightTextPoly);  // SVGTransform always before templateTransform
        QTransform templateTransform;
        templateTransform.translate(0.0, Rez::guiX(-getSVGTemplate()->getHeight()));
        templateTransform.scale(Rez::getRezFactor(), Rez::getRezFactor());
        tightTextPoly = templateTransform.map(tightTextPoly);

        QString name = textElement.attribute(QStringLiteral(FREECAD_ATTR_EDITABLE));
        auto item(new TemplateTextField(this, svgTemplate, name.toStdString()));
        auto autoValue = svgTemplate->getAutofillByEditableName(name);
        item->setAutofill(autoValue);

        QMarginsF padding(
            0.0,
            0.15 * tightTextRect.height(),
            0.0,
            0.2 * tightTextRect.height()
        );
        QRectF clickrect = tightTextRect.marginsAdded(padding);
        QPolygonF clickpoly = SVGTransform.map(clickrect);
        clickpoly = templateTransform.map(clickpoly);
        item->setRectangle(clickpoly.boundingRect());  // TODO: templateTextField doesn't support polygon yet

        QPointF bottomLeft = clickpoly.at(3);
        QPointF bottomRight = clickpoly.at(2);
        item->setLine(bottomLeft, bottomRight);
        item->setLineColor(PreferencesGui::templateClickBoxColor());
        item->setZValue(ZVALUE::SVGTEMPLATE + 1);

        addToGroup(item);
    }
}

#include <Mod/TechDraw/Gui/moc_QGISVGTemplate.cpp>
