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

//#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include <qmath.h>
#include "Rez.h"
#include "PreferencesGui.h"
#include "QGIView.h"
#include "QGISectionLine.h"

#define ANSISTANDARD 0
#define ISOSTANDARD 1

using namespace TechDrawGui;
using namespace TechDraw;

QGISectionLine::QGISectionLine()
{
    m_symbol = "";
    m_symSize = 0.0;

    m_extLen = 1.5 * Rez::guiX(QGIArrow::getPrefArrowSize());
    m_arrowSize = QGIArrow::getPrefArrowSize();

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
    QPointF beginExtLine1,beginExtLine2;   //ext line start pts for measure Start side and measure End side
    QPointF endExtLine1, endExtLine2;
    QPointF offsetDir(m_arrowDir.x,-m_arrowDir.y);
    int format = getPrefSectionStandard();
    if (format == ANSISTANDARD) {                           //"ASME"/"ANSI"
        //draw from section line endpoint
        QPointF offsetBegin = m_extLen * offsetDir;
        beginExtLine1 = m_start;           //from
        beginExtLine2 = m_end;             //to
        endExtLine1 = m_start + offsetBegin;
        endExtLine2   = m_end + offsetBegin;
        pp.moveTo(beginExtLine1);
        pp.lineTo(endExtLine1);
        pp.moveTo(beginExtLine2);
        pp.lineTo(endExtLine2);
    } else {                                     //"ISO"
        //draw from just short of section line away from section line
        QPointF offsetBegin = Rez::guiX(QGIArrow::getOverlapAdjust(0,QGIArrow::getPrefArrowSize())) * offsetDir;
        QPointF offsetEnd   = offsetBegin + (m_extLen * offsetDir);
        beginExtLine1 = m_start - offsetBegin;
        beginExtLine2 = m_end - offsetBegin;
        endExtLine1 = m_start - offsetEnd;
        endExtLine2   = m_end - offsetEnd;
        pp.moveTo(beginExtLine1);
        pp.lineTo(endExtLine1);
        pp.moveTo(beginExtLine2);
        pp.lineTo(endExtLine2);
    }

    pp.moveTo(m_end);
    pp.lineTo(m_start);          //sectionLine
    m_line->setPath(pp);
}

void QGISectionLine::makeArrows()
{
    int format = getPrefSectionStandard();
    if (format == ANSISTANDARD) {
        makeArrowsTrad();
    } else {
        makeArrowsISO();
    }
}

//make Euro (ISO) Arrows
void QGISectionLine::makeArrowsISO()
{
    double arrowRotation = 0.0;
    m_arrowDir.Normalize();
    double angle = atan2f(m_arrowDir.y,m_arrowDir.x);
    if (angle < 0.0) {
        angle = 2 * M_PI + angle;
    }
    arrowRotation = 360.0 - angle * (180.0/M_PI);   //convert to Qt rotation (clockwise degrees)

    m_arrow1->setStyle(0);
    m_arrow1->setSize(QGIArrow::getPrefArrowSize());
    m_arrow1->setPos(m_start);
    m_arrow1->draw();
    m_arrow1->setRotation(arrowRotation);                   //rotation = 0  ==>  ->  horizontal, pointing right

    m_arrow2->setStyle(0);
    m_arrow2->setSize(QGIArrow::getPrefArrowSize());
    m_arrow2->setPos(m_end);
    m_arrow2->draw();
    m_arrow2->setRotation(arrowRotation);
}

//make traditional (ASME) section arrows
void QGISectionLine::makeArrowsTrad()
{
    double arrowRotation = 0.0;
    m_arrowDir.Normalize();
    double angle = atan2f(m_arrowDir.y,m_arrowDir.x);
    if (angle < 0.0) {
        angle = 2 * M_PI + angle;
    }
    arrowRotation = 360.0 - angle * (180.0/M_PI);   //convert to Qt rotation (clockwise degrees)

    QPointF posArrow1,posArrow2;
    QPointF offsetDir(m_arrowDir.x,-m_arrowDir.y);              //remember Y dir is flipped
    double offsetLength = m_extLen + Rez::guiX(QGIArrow::getOverlapAdjust(0,QGIArrow::getPrefArrowSize()));
    QPointF offsetVec = offsetLength * offsetDir;
    posArrow1 = m_start + offsetVec;
    posArrow2 = m_end + offsetVec;

    m_arrow1->setStyle(0);
    m_arrow1->setSize(QGIArrow::getPrefArrowSize());
    m_arrow1->setPos(posArrow1);
    m_arrow1->draw();
    m_arrow1->setRotation(arrowRotation);                   //rotation = 0  ==>  ->  horizontal, pointing right

    m_arrow2->setStyle(0);
    m_arrow2->setSize(QGIArrow::getPrefArrowSize());
    m_arrow2->setPos(posArrow2);
    m_arrow2->draw();
    m_arrow2->setRotation(arrowRotation);
}

void QGISectionLine::makeSymbols()
{
    int format = getPrefSectionStandard();
    if (format == ANSISTANDARD) {
        makeSymbolsTrad();
    } else {
        makeSymbolsISO();
    }
}

void QGISectionLine::makeSymbolsTrad()
{
    QPointF extLineStart,extLineEnd;
    QPointF offset(m_arrowDir.x,-m_arrowDir.y);
    offset = 1.5 * m_extLen * offset;
    extLineStart = m_start + offset;
    extLineEnd = m_end + offset;
    prepareGeometryChange();
    m_symFont.setPixelSize(QGIView::calculateFontPixelSize(m_symSize));
    m_symbol1->setFont(m_symFont);
    m_symbol1->setPlainText(QString::fromUtf8(m_symbol));

    QRectF symRect = m_symbol1->boundingRect();
    double symWidth = symRect.width();
    double symHeight = symRect.height();
    double symbolFudge = 1.0;
    double angle = atan2f(m_arrowDir.y,m_arrowDir.x);
    if (angle < 0.0) {
        angle = 2 * M_PI + angle;
    }
    Base::Vector3d adjustVector(cos(angle) * symWidth, sin(angle) * symHeight, 0.0);
    adjustVector = (DrawUtil::invertY(adjustVector) / 2.0) * symbolFudge;
    QPointF qAdjust(adjustVector.x, adjustVector.y);

    extLineStart += qAdjust;
    m_symbol1->centerAt(extLineStart);

    m_symbol2->setFont(m_symFont);
    m_symbol2->setPlainText(QString::fromUtf8(m_symbol));
    extLineEnd += qAdjust;
    m_symbol2->centerAt(extLineEnd);
}

void QGISectionLine::makeSymbolsISO()
{
    QPointF symPosStart, symPosEnd;
    QPointF dist = (m_start - m_end);
    double lenDist = sqrt(dist.x()*dist.x() + dist.y()*dist.y());
    QPointF distDir = dist / lenDist;

    QPointF offset = m_extLen * distDir;
    symPosStart = m_start + offset;
    symPosEnd = m_end - offset;

    prepareGeometryChange();
    m_symFont.setPixelSize(QGIView::calculateFontPixelSize(m_symSize));
    m_symbol1->setFont(m_symFont);
    m_symbol1->setPlainText(QString::fromUtf8(m_symbol));
    m_symbol1->centerAt(symPosStart);

    m_symbol2->setFont(m_symFont);
    m_symbol2->setPlainText(QString::fromUtf8(m_symbol));
    m_symbol2->centerAt(symPosEnd);

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

void QGISectionLine::setSectionColor(QColor c)
{
    setColor(c);
}

QColor QGISectionLine::getSectionColor()
{
    return PreferencesGui::sectionLineQColor();
}

//SectionLineStyle
void QGISectionLine::setSectionStyle(int style)
{
    Qt::PenStyle sectStyle = static_cast<Qt::PenStyle> (style);
    setStyle(sectStyle);
}

Qt::PenStyle QGISectionLine::getSectionStyle()
{
    return PreferencesGui::sectionLineStyle();
}

//ASME("traditional") vs ISO("reference arrow method") arrows
int QGISectionLine::getPrefSectionStandard()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Standards");
    int format = hGrp->GetInt("SectionLineStandard", ISOSTANDARD);
    return format;
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

        // TODO for fanciness: calculate the offset so both arrows start with a
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

//    m_arrow1->setPen(m_pen);
//    m_arrow2->setPen(m_pen);
//    m_arrow1->setBrush(m_brush);
//    m_arrow2->setBrush(m_brush);
    
    m_arrow1->setNormalColor(m_colCurrent);
    m_arrow1->setFillColor(m_colCurrent);
    m_arrow1->setPrettyNormal();
    m_arrow2->setNormalColor(m_colCurrent);
    m_arrow2->setFillColor(m_colCurrent);
    m_arrow2->setPrettyNormal();

    m_symbol1->setDefaultTextColor(m_colCurrent);
    m_symbol2->setDefaultTextColor(m_colCurrent);
}
