/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <qmath.h>
#include "Rez.h"
#include "QGIView.h"
#include "QGISectionLine.h"

using namespace TechDrawGui;

QGISectionLine::QGISectionLine()
{
    m_symbol = "";
    m_symSize = 0.0;

    m_extLen = Rez::guiX(8.0);
    m_arrowSize = 0.0;

    m_line = new QGraphicsPathItem();
    addToGroup(m_line);
    m_arrow1 = new QGIArrow();
    addToGroup(m_arrow1);
    m_arrow2 = new QGIArrow();
    addToGroup(m_arrow2);
    m_symbol1 = new QGCustomText();
    addToGroup(m_symbol1);
    m_symbol2 = new QGCustomText();
    addToGroup(m_symbol2);

    setWidth(Rez::guiX(0.75));
    setStyle(getSectionStyle());
    setColor(getSectionColor());

}

void QGISectionLine::draw()
{
    prepareGeometryChange();
    makeLine();
    makeArrows();
    makeSymbols();
    update();
}

void QGISectionLine::makeLine()
{
    QPainterPath pp;
    QPointF extLineStart,extLineEnd;
    QPointF offset(m_arrowDir.x,-m_arrowDir.y);
    offset = 0.75 * m_extLen * offset;                  //0.75 is hack to hide line end behind arrowhead
    extLineStart = m_start + offset;
    extLineEnd = m_end + offset;
    pp.moveTo(extLineStart);
    pp.lineTo(m_start);
    pp.lineTo(m_end);
    pp.lineTo(extLineEnd);
    m_line->setPath(pp);
}

void QGISectionLine::makeArrows()
{
    double arrowRotation = 0.0;
    m_arrowDir.Normalize();
    double angle = atan2f(m_arrowDir.y,m_arrowDir.x);
    if (angle < 0.0) {
        angle = 2 * M_PI + angle;
    }
    arrowRotation = 360.0 - angle * (180.0/M_PI);   //convert to Qt rotation (clockwise degrees)

    QPointF extLineStart,extLineEnd;
    QPointF offset(m_arrowDir.x,-m_arrowDir.y);              //remember Y dir is flipped
    offset = m_extLen * offset;
    extLineStart = m_start + offset;
    extLineEnd = m_end + offset;

    m_arrow1->setStyle(0);
    m_arrow2->setStyle(0);
    m_arrow1->setPos(extLineStart);
    //m_arrow1->flip(true);
    m_arrow1->draw();
    m_arrow1->setRotation(arrowRotation);                   //rotation = 0  ==>  ->  horizontal, pointing right
    m_arrow2->setPos(extLineEnd);
    m_arrow2->draw();
    m_arrow2->setRotation(arrowRotation);
}

void QGISectionLine::makeSymbols()
{
    QPointF extLineStart,extLineEnd;
    QPointF offset(m_arrowDir.x,-m_arrowDir.y);
    offset = 1.5 * m_extLen * offset;
    extLineStart = m_start + offset;
    extLineEnd = m_end + offset;

    prepareGeometryChange();
    m_symFont.setPointSize(m_symSize);
    m_symbol1->setFont(m_symFont);
    m_symbol1->setPlainText(QString::fromUtf8(m_symbol));
    if (m_arrowDir.y < 0.0) {         //pointing down
        extLineStart  -= QPointF (0.0,m_symSize);  //move text up a bit
    }
    m_symbol1->centerAt(extLineStart);
    m_symbol2->setFont(m_symFont);
    m_symbol2->setPlainText(QString::fromUtf8(m_symbol));
    if (m_arrowDir.y < 0.0) {         //pointing down
        extLineEnd  -= QPointF (0.0,m_symSize);
    }
    m_symbol2->centerAt(extLineEnd);
}


void QGISectionLine::setBounds(double x1,double y1,double x2,double y2)
{
    m_start = QPointF(x1,y1);
    m_end = QPointF(x2,y2);
}

void QGISectionLine::setSymbol(char* sym)
{
    m_symbol = sym;
}

void QGISectionLine::setDirection(double xDir,double yDir)
{
    Base::Vector3d newDir(xDir,yDir,0.0);
    setDirection(newDir);
}

void QGISectionLine::setDirection(Base::Vector3d dir)
{
    m_arrowDir = dir;
}

void QGISectionLine::setFont(QFont f, double fsize)
{
    m_symFont = f;
    m_symSize = fsize;
}


QColor QGISectionLine::getSectionColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("SectionColor", 0x00000000));
    return fcColor.asValue<QColor>();
}

Qt::PenStyle QGISectionLine::getSectionStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw");
    Qt::PenStyle sectStyle = static_cast<Qt::PenStyle> (hGrp->GetInt("SectionLine", 2));
    return sectStyle;
}

void QGISectionLine::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    setTools();
    QGIDecoration::paint (painter, &myOption, widget);
}

void QGISectionLine::setTools()
{
    // Use our own style
    if (m_styleCurrent == Qt::DashDotLine) {
        QVector<qreal> dashes;
        // the stroke width is double the one of center lines, but we like to
        // have the same spacing. thus these values must be half as large
        qreal space = 2;  // in unit r_width
        qreal dash = 8;
        // dot must be really small when using CapStyle RoundCap but > 0
        // for CapStyle FlatCap you would need to set it to 1
        qreal dot = 0.000001;

        dashes << dot << space << dash << space;

        // TODO for fancyness: calculate the offset so both arrows start with a
        // dash!

        m_pen.setDashPattern(dashes);

        m_pen.setDashOffset(2);
    }
    else {
        m_pen.setStyle(m_styleCurrent);
    }
    m_pen.setWidthF(m_width);
    m_pen.setColor(m_colCurrent);
    m_pen.setCapStyle(Qt::RoundCap);
    m_brush.setStyle(m_brushCurrent);
    m_brush.setColor(m_colCurrent);

    m_line->setPen(m_pen);

    m_arrow1->setPen(m_pen);
    m_arrow2->setPen(m_pen);
    m_arrow1->setBrush(m_brush);
    m_arrow2->setBrush(m_brush);

    m_symbol1->setDefaultTextColor(m_colCurrent);
    m_symbol2->setDefaultTextColor(m_colCurrent);
}
