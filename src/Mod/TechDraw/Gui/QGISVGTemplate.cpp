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
# include <QGraphicsColorizeEffect>
# include <QGraphicsEffect>
# include <QGraphicsSvgItem>
# include <QPen>
# include <QSvgRenderer>
#endif// #ifndef _PreComp_

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/XMLQuery.h>

#include "QGISVGTemplate.h"
#include "PreferencesGui.h"
#include "QGSPage.h"
#include "Rez.h"
#include "TemplateTextField.h"
#include "ZVALUE.h"


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
    if (pageTemplate && pageTemplate->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId())) {
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
    Q_UNUSED(update);
    draw();
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

    double editClickBoxSize = Rez::guiX(Preferences::getPreferenceGroup("General")->GetFloat("TemplateDotSize", 3.0));

    QColor editClickBoxColor = Qt::green;
    editClickBoxColor.setAlpha(128);//semi-transparent

    double width = editClickBoxSize;
    double height = editClickBoxSize;

    TechDraw::XMLQuery query(templateDocument);

    // XPath query to select all <text> nodes with "freecad:editable" attribute
    query.processItems(QString::fromUtf8("declare default element namespace \"" SVG_NS_URI "\"; "
                                         "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
                                         "//text[@freecad:editable]"),
                       [&](QDomElement& textElement) -> bool {
        QString name = textElement.attribute(QString::fromUtf8("freecad:editable"));
        double x = Rez::guiX(
            textElement.attribute(QString::fromUtf8("x"), QString::fromUtf8("0.0")).toDouble());
        double y = Rez::guiX(
            textElement.attribute(QString::fromUtf8("y"), QString::fromUtf8("0.0")).toDouble());

        if (name.isEmpty()) {
            Base::Console().Warning(
                "QGISVGTemplate::createClickHandles - no name for editable text at %f, %f\n", x, y);
            return true;
        }

        auto item(new TemplateTextField(this, svgTemplate, name.toStdString()));

        double pad = 1.0;
        item->setRect(x - pad, Rez::guiX(-svgTemplate->getHeight()) + y - height - pad,
                      width + 2.0 * pad, height + 2.0 * pad);

        QPen myPen;
        myPen.setStyle(Qt::SolidLine);
        myPen.setColor(editClickBoxColor);
        myPen.setWidth(0);// 0 means "cosmetic pen" - always 1px
        item->setPen(myPen);

        QBrush myBrush(editClickBoxColor, Qt::SolidPattern);
        item->setBrush(myBrush);

        item->setZValue(ZVALUE::SVGTEMPLATE + 1);
        addToGroup(item);

        textFields.push_back(item);
        return true;
    });
}

#include <Mod/TechDraw/Gui/moc_QGISVGTemplate.cpp>
