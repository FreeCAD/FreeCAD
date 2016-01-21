/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
  # include <QPainterPathStroker>
  # include <QPainter>
  # include <strstream>
  # include <math.h>
  # include <QGraphicsPathItem>
  # include <QGraphicsTextItem>
#endif

#include <QBitmap>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QSvgRenderer>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "QGIView.h"
#include "QGIHatch.h"

using namespace TechDrawGui;

QGIHatch::QGIHatch(std::string parentHatch) :
    m_hatch(parentHatch),
    m_fill(Qt::NoBrush),
    m_lastFill("")
    //m_fill(Qt::Dense3Pattern)
    //m_fill(Qt::CrossPattern)
    //m_fill(Qt::Dense6Pattern)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptHoverEvents(true);
    setCacheMode(QGraphicsItem::NoCache);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("NormalColor", 0x00000000));
    m_colNormal = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x0000FF00));
    m_colSel = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0x00080800));
    m_colPre = fcColor.asQColor();

    m_pen.setCosmetic(true);
    m_pen.setWidthF(1.);
    //m_pen.setStyle(Qt::NoPen);
    m_pen.setColor(m_colNormal);
    m_brush.setStyle(m_fill);
    setPrettyNormal();
}

QGIHatch::~QGIHatch()
{
}

#if 0
void QGIHatch::draw()
{
    const std::vector<App::DocumentObject*> &objects = hatch->Edges.getValues();
    //const std::vector<std::string> &SubNames         = hatch->Edges.getSubValues();
    //const TechDraw::DrawViewPart *refObj = static_cast<const TechDraw::DrawViewPart*>(objects[0]);

    //for edgeName in SubNames
    //    iEdge = _getIndexFromName(edgeName)    //from CommandCreateDims.cpp
    //    geom = refObj->getEdgeGeomByRef(iEdge)
    //    subPath = drawPainterPath(geom)        //from qgiViewPart
    //    m_path.addPath(subPath)
    //m_path = m_path.simplified()               //????
    //m_face->setPath(m_path);
}
#endif

QVariant QGIHatch::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            setPrettySel();
        } else {
            setPrettyNormal();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void QGIHatch::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    setPrettyPre();
}

void QGIHatch::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());

    if(!isSelected() && !view->isSelected()) {
        setPrettyNormal();
    }
}

void QGIHatch::setPrettyNormal()
{
    m_pen.setColor(m_colNormal);
    m_brush.setColor(m_colNormal);
    setPen(m_pen);
    setBrush(m_brush);
}

void QGIHatch::setPrettyPre()
{
    m_pen.setColor(m_colPre);
    m_brush.setColor(m_colPre);
    setPen(m_pen);
    setBrush(m_brush);
}

void QGIHatch::setPrettySel()
{
    m_pen.setColor(m_colSel);
    m_brush.setColor(m_colSel);
    setPen(m_pen);
    setBrush(m_brush);
}

void QGIHatch::setFill(std::string fillSpec)
{
    if (fillSpec.empty()) {
        return;
    }

    if (fillSpec == m_lastFill) {
        return;
    }

    QString qs(QString::fromStdString(fillSpec));
    m_lastFill = fillSpec;
    //QString qs(QString::fromUtf8("../src/Mod/Drawing/patterns/simple.svg"));
    //QString qs(QString::fromUtf8("../src/Mod/Drawing/patterns/square.svg"));
    QSvgRenderer renderer(qs);
    //QBitmap pixMap(64,64);                         //this size is scene units (mm) instead of pixels?
    QBitmap pixMap(renderer.defaultSize());
    pixMap.fill(Qt::white);   //try  Qt::transparent?
    QPainter painter(&pixMap);
    renderer.render(&painter);                                         //svg texture -> bitmap

    m_texture = pixMap;
    m_brush = QBrush(m_texture);
    m_brush.setStyle(Qt::TexturePattern);
}

void QGIHatch::setColor(App::Color c)
{
    m_colNormal = c.asQColor();
}

void QGIHatch::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;
    painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );   //doesn't seem to change much
    setPen(m_pen);
    setBrush(m_brush);
    QGraphicsPathItem::paint (painter, &myOption, widget);
}
