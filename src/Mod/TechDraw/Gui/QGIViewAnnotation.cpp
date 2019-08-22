/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                 2014 wandererfan <WandererFan@gmail.com>                *
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
#include <cmath>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsTextItem>
#include <QPainterPathStroker>
#include <QPainter>
#include <QString>
#include <QTextOption>
#include <sstream>
#endif

#include <qmath.h>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextFrame>
#include <QSizeF>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include "Rez.h"
#include "QGIViewAnnotation.h"
#include "QGCustomText.h"

using namespace TechDrawGui;

QGIViewAnnotation::QGIViewAnnotation()
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setAcceptHoverEvents(true);

    m_textItem = new QGCustomText();
    m_textItem->setTextInteractionFlags(Qt::NoTextInteraction);
    //To allow on screen editing of text:
    //m_textItem->setTextInteractionFlags(Qt::TextEditorInteraction);   //this works
    //QObject::connect(QGraphicsTextItem::document(), SIGNAL(contentsChanged()),m_textItem, SLOT(updateText()));  //not tested
    addToGroup(m_textItem);
    m_textItem->setPos(0.,0.);

}


QVariant QGIViewAnnotation::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGIView::itemChange(change, value);
}

void QGIViewAnnotation::setViewAnnoFeature(TechDraw::DrawViewAnnotation *obj)
{
    // called from QGVPage. (once)
    setViewFeature(static_cast<TechDraw::DrawView *>(obj));
}

void QGIViewAnnotation::updateView(bool update)
{
    auto viewAnno( dynamic_cast<TechDraw::DrawViewAnnotation *>(getViewObject()) );
    if( viewAnno == nullptr)
        return;

    if (update ||
        viewAnno->isTouched() ||
        viewAnno->Text.isTouched() ||
        viewAnno->Font.isTouched() ||
        viewAnno->TextColor.isTouched() ||
        viewAnno->TextSize.isTouched() ) {

        draw();
    }

    QGIView::updateView(update);
}

void QGIViewAnnotation::draw()
{
    if (!isVisible()) {
        return;
    }

    drawAnnotation();
    QGIView::draw();
    rotateView();

}

//TODO: text is positioned slightly high (and left??) on page save to SVG file

void QGIViewAnnotation::drawAnnotation()
{
    auto viewAnno( dynamic_cast<TechDraw::DrawViewAnnotation *>(getViewObject()) );
    if( viewAnno == nullptr ) {
        return;
    }

    const std::vector<std::string>& annoText = viewAnno->Text.getValues();
    int fontSize = calculateFontPixelSize(viewAnno->TextSize.getValue());

    //build HTML/CSS formatting around Text lines
    std::stringstream ss;
    ss << "<html>\n<head>\n<style>\n";
    ss << "p {";
    ss << "font-family:" << viewAnno->Font.getValue() << "; ";
    ss << "font-size:" << fontSize << "px; ";
    if (viewAnno->TextStyle.isValue("Normal")) {
        ss << "font-weight:normal; font-style:normal; ";
    } else if (viewAnno->TextStyle.isValue("Bold")) {
        ss << "font-weight:bold; font-style:normal; ";
    } else if (viewAnno->TextStyle.isValue("Italic")) {
        ss << "font-weight:normal; font-style:italic; ";
    } else if (viewAnno->TextStyle.isValue("Bold-Italic")) {
        ss << "font-weight:bold; font-style:italic; ";
    } else {
        Base::Console().Warning("%s has invalid TextStyle\n",viewAnno->getNameInDocument());
        ss << "font-weight:normal; font-style:normal; ";
    }
    ss << "line-height:" << viewAnno->LineSpace.getValue() << "%; ";
    App::Color c = viewAnno->TextColor.getValue();
    ss << "color:" << c.asCSSString() << "; ";
    ss << "}\n</style>\n</head>\n<body>\n<p>";
    for(std::vector<std::string>::const_iterator it = annoText.begin(); it != annoText.end(); it++) {
        if (it == annoText.begin()) {
            ss << *it;
        } else {
            ss << "<br>" << *it ;
        }
    }
    ss << "</p>\n</body>\n</html> ";

    prepareGeometryChange();
    m_textItem->setTextWidth(Rez::guiX(viewAnno->MaxWidth.getValue()));
    QString qs = QString::fromUtf8(ss.str().c_str());
    m_textItem->setHtml(qs);
    m_textItem->centerAt(0.,0.);
}

void QGIViewAnnotation::rotateView(void)
{
    QRectF r = m_textItem->boundingRect();
    m_textItem->setTransformOriginPoint(r.center());
    double rot = getViewObject()->Rotation.getValue();
    m_textItem->setRotation(-rot);
}


