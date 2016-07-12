/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainterPathStroker>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#endif

#include <QFile>
#include <QTextStream>
#include <QRectF>
#include <QPointF>

#include <cmath>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "QGIView.h"
#include "QGIFace.h"

using namespace TechDrawGui;

QGIFace::QGIFace(int index) :
    projIndex(index),
    m_colDefFill(Qt::white),          //Qt::transparent?  paper colour?
    m_styleDef(Qt::SolidPattern),
    m_styleSelect(Qt::SolidPattern)
{
    setFlag(QGraphicsItem::ItemClipsChildrenToShape,true);
    //setFiltersChildEvents(true);

    m_pen.setCosmetic(true);

    m_styleNormal = m_styleDef;
    m_colNormalFill = m_colDefFill;
    setPrettyNormal();

    m_svg = new QGCustomSvg();

    m_rect = new QGCustomRect();
    m_rect->setParentItem(this);

    m_svgCol = SVGCOLDEFAULT;
}

QGIFace::~QGIFace()
{
    //nothing to do. every item is a child of QGIFace & will get removed/deleted when QGIF is deleted
}

void QGIFace::setPrettyNormal() {
    m_fillStyle = m_styleNormal;
    m_fillColor = m_colNormalFill;
    QGIPrimPath::setPrettyNormal();
}

void QGIFace::setPrettyPre() {
    m_fillStyle = m_styleSelect;
    m_fillColor = m_colPre;
    QGIPrimPath::setPrettyPre();
}

void QGIFace::setPrettySel() {
    m_fillStyle = m_styleSelect;
    m_fillColor = m_colSel;
    QGIPrimPath::setPrettySel();
}

void QGIFace::setFill(QColor c, Qt::BrushStyle s) {
    m_colNormalFill = c;
    m_styleNormal = s;
}

void QGIFace::setFill(QBrush b) {
    m_colNormalFill = b.color();
    m_styleNormal = b.style();
}

void QGIFace::resetFill() {
    m_colNormalFill = m_colDefFill;
    m_styleNormal = m_styleDef;
}

void QGIFace::setHatch(std::string fileSpec)
{
    QString qfs(QString::fromStdString(fileSpec));
    QFile f(qfs);
    if (!f.open(QFile::ReadOnly | QFile::Text))  {
        Base::Console().Error("QGIFace could not read %s\n",fileSpec.c_str());
        return;
    }
    m_svgXML = f.readAll();
    if (!m_svg->load(&m_svgXML)) {
        Base::Console().Error("Error - Could not load hatch into SVG renderer for %s\n", fileSpec.c_str());
        return;
    }

    buildHatch();
}

void QGIFace::setPath(const QPainterPath & path)
{
    QGraphicsPathItem::setPath(path);
    if (!m_svgXML.isEmpty()) {
        buildHatch();
    }
}

void QGIFace::buildHatch()
{
    m_styleNormal = Qt::NoBrush;
    double w = boundingRect().width();
    double h = boundingRect().height();
    QRectF r = boundingRect();
    QPointF fCenter = r.center();
    double nw = ceil(w / SVGSIZEW);
    double nh = ceil(h / SVGSIZEH);
    w = nw * SVGSIZEW;
    h = nh * SVGSIZEW;
    m_rect->setRect(0.,0.,w,-h);
    m_rect->centerAt(fCenter);
    r = m_rect->rect();
    QByteArray before,after;
    before.append(QString::fromStdString(SVGCOLPREFIX + SVGCOLDEFAULT));
    after.append(QString::fromStdString(SVGCOLPREFIX + m_svgCol));
    QByteArray colorXML = m_svgXML.replace(before,after);
    for (int iw = 0; iw < int(nw); iw++) {
        for (int ih = 0; ih < int(nh); ih++) {
            QGCustomSvg* tile = new QGCustomSvg();
            if (tile->load(&colorXML)) {
                tile->setParentItem(m_rect);
                tile->setPos(iw*SVGSIZEW,-h + ih*SVGSIZEH);
            }
        }
    }

}

//c is a CSS color ie "#000000"
//set hatch color before building hatch
void QGIFace::setHatchColor(std::string c)
{
    m_svgCol = c;
}

QRectF QGIFace::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGIFace::shape() const
{
    return path();
}

void QGIFace::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    m_brush.setStyle(m_fillStyle);
    m_brush.setColor(m_fillColor);
    setBrush(m_brush);
    QGIPrimPath::paint (painter, &myOption, widget);
}
