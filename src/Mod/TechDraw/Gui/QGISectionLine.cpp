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
# include <QGraphicsScene>
# include <QPainter>
# include <QPainterPath>
# include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include "QGISectionLine.h"
#include "PreferencesGui.h"
#include "QGIArrow.h"
#include "QGIView.h"
#include "Rez.h"
#include "ZVALUE.h"


#define ANSISTANDARD 0
#define ISOSTANDARD 1
#define SINGLEDIRECTIONMODE 0       //both arrows point along the section normal
#define MULTIDIRECTIONMODE 1        //the arrows point along the normal of their section line segment

using namespace TechDrawGui;
using namespace TechDraw;

QGISectionLine::QGISectionLine() :
    m_pathMode(false),
    m_arrowMode()
{
    m_symbol = "";
    m_symSize = 0.0;

    m_extLen = 1.5 * Rez::guiX(QGIArrow::getPrefArrowSize());   //is there a standard for this??
    m_arrowSize = QGIArrow::getPrefArrowSize();

    m_line = new QGraphicsPathItem();
    addToGroup(m_line);
    m_extend = new QGraphicsPathItem();
    addToGroup(m_extend);
    m_arrow1 = new QGIArrow();
    addToGroup(m_arrow1);
    m_arrow2 = new QGIArrow();
    addToGroup(m_arrow2);
    m_symbol1 = new QGCustomText();
    addToGroup(m_symbol1);
    m_symbol2 = new QGCustomText();
    addToGroup(m_symbol2);

    setWidth(Rez::guiX(0.75));          //a default?
    setStyle(getSectionStyle());
    setColor(getSectionColor());

}

void QGISectionLine::draw()
{
    prepareGeometryChange();
    int format = getPrefSectionStandard();
    if (format == ANSISTANDARD) {                           //"ASME"/"ANSI"
        extensionEndsTrad();
    } else {
        extensionEndsISO();
    }

    if (!pathMode()) {
        makeSectionLine();
    }
    makeExtensionLine();
    makeArrows();
    makeSymbols();
    makeChangePointMarks();
    update();
}

void QGISectionLine::makeExtensionLine()
{
    QPen extendPen;
    extendPen.setWidthF(getWidth());
    extendPen.setColor(getSectionColor());
    extendPen.setStyle(Qt::SolidLine);
    extendPen.setCapStyle(Qt::FlatCap);
    m_extend->setPen(extendPen);

    QPainterPath pp;

    pp.moveTo(m_beginExt1);
    pp.lineTo(m_endExt1);

    pp.moveTo(m_beginExt2);
    pp.lineTo(m_endExt2);
    m_extend->setPath(pp);
}

//make the traditional straight section line
void QGISectionLine::makeSectionLine()
{
    QPainterPath pp;
    pp.moveTo(m_start);
    pp.lineTo(m_end);
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
    m_arrow1->setStyle(0);
    m_arrow1->setSize(QGIArrow::getPrefArrowSize());
    m_arrow1->setPos(m_start);
    m_arrow2->setStyle(0);
    m_arrow2->setSize(QGIArrow::getPrefArrowSize());
    m_arrow2->setPos(m_end);

    if (m_arrowMode == SINGLEDIRECTIONMODE) {
        double arrowRotation = getArrowRotation(m_arrowDir);
        m_arrow1->setRotation(arrowRotation);                   //rotation = 0  ==>  ->  horizontal, pointing right
        m_arrow2->setRotation(arrowRotation);
    } else {
        double arrowRotation1 = getArrowRotation(m_arrowDir1);
        m_arrow1->setRotation(arrowRotation1);
        double arrowRotation2 = getArrowRotation(m_arrowDir2);
        m_arrow2->setRotation(arrowRotation2);
    }
    m_arrow1->draw();
    m_arrow2->draw();
}

//make traditional (ASME) section arrows
void QGISectionLine::makeArrowsTrad()
{
    m_arrow1->setStyle(0);
    m_arrow1->setSize(QGIArrow::getPrefArrowSize());
    m_arrow2->setStyle(0);
    m_arrow2->setSize(QGIArrow::getPrefArrowSize());

    if (m_arrowMode == SINGLEDIRECTIONMODE) {
        double arrowRotation = getArrowRotation(m_arrowDir);
        m_arrow1->setRotation(arrowRotation);                   //rotation = 0  ==>  ->  horizontal, pointing right
        m_arrow2->setRotation(arrowRotation);
        m_arrowPos1 = getArrowPosition(m_arrowDir, m_start);
        m_arrow1->setPos(m_arrowPos1);
        m_arrowPos2 = getArrowPosition(m_arrowDir, m_end);
        m_arrow2->setPos(m_arrowPos2);
    } else {
        double arrowRotation1 = getArrowRotation(m_arrowDir1);
        m_arrow1->setRotation(arrowRotation1);
        m_arrowPos1 = getArrowPosition(m_arrowDir1, m_start);
        m_arrow1->setPos(m_arrowPos1);
        double arrowRotation2 = getArrowRotation(m_arrowDir2);
        m_arrow2->setRotation(arrowRotation2);
        m_arrowPos2 = getArrowPosition(m_arrowDir2, m_end);
        m_arrow2->setPos(m_arrowPos2);
    }

    m_arrow1->draw();
    m_arrow2->draw();
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

//arrows go at arrowhead position.
void QGISectionLine::makeSymbolsTrad()
{
    prepareGeometryChange();
    int fontSize = QGIView::exactFontSize(Base::Tools::toStdString(m_symFont.family()), m_symSize);
    m_symFont.setPixelSize(fontSize);
    m_symbol1->setFont(m_symFont);
    m_symbol1->setPlainText(QString::fromUtf8(m_symbol));
    m_symbol2->setFont(m_symFont);
    m_symbol2->setPlainText(QString::fromUtf8(m_symbol));

    QRectF symRect = m_symbol1->boundingRect();
    double symHeight = symRect.height();
    double gap = 0.5 * symHeight;  //symHeight as surrogate for char box

    QPointF motion1(m_arrowDir1.x, -m_arrowDir1.y);    //move in same direction as arrow
    QPointF motion2(m_arrowDir2.x, -m_arrowDir2.y);     //Qt y coords!

    QPointF symPos1 = m_arrowPos1 + motion1 * gap;
    QPointF symPos2 = m_arrowPos2 + motion2 * gap;

    m_symbol1->centerAt(symPos1);
    m_symbol2->centerAt(symPos2);

    m_symbol1->setTransformOriginPoint(m_symbol1->mapFromParent(symPos1));
    m_symbol1->setRotation(360.0 - rotation());         //to Qt angle
    m_symbol2->setTransformOriginPoint(m_symbol2->mapFromParent(symPos2));
    m_symbol2->setRotation(360.0 - rotation());
}

//symbols go at ends of extensions
void QGISectionLine::makeSymbolsISO()
{
    prepareGeometryChange();
    int fontSize = QGIView::exactFontSize(Base::Tools::toStdString(m_symFont.family()), m_symSize);
    m_symFont.setPixelSize(fontSize);
    m_symbol1->setFont(m_symFont);
    m_symbol1->setPlainText(QString::fromUtf8(m_symbol));
    m_symbol2->setFont(m_symFont);
    m_symbol2->setPlainText(QString::fromUtf8(m_symbol));

    QRectF symRect = m_symbol1->boundingRect();
    double symHeight = symRect.height();
    double gap = 0.5 * symHeight;  //symHeight as surrogate for char box

    QPointF motion1(-m_arrowDir1.x, m_arrowDir1.y);     //move away from extension end
    QPointF motion2(-m_arrowDir2.x, m_arrowDir2.y);     //Qt y coords!

    QPointF symPos1 = m_endExt1 + motion1 * gap;
    QPointF symPos2 = m_endExt2 + motion2 * gap;

    m_symbol1->centerAt(symPos1);
    m_symbol2->centerAt(symPos2);

    m_symbol1->setTransformOriginPoint(m_symbol1->mapFromParent(symPos1));
    m_symbol1->setRotation(360.0 - rotation());
    m_symbol2->setTransformOriginPoint(m_symbol2->mapFromParent(symPos2));
    m_symbol2->setRotation(360.0 - rotation());
}

//extension lines are on the stock side of the section line
void QGISectionLine::extensionEndsTrad()
{
    if (m_arrowMode == SINGLEDIRECTIONMODE) {
        QPointF offsetDir(m_arrowDir.x, -m_arrowDir.y);   //inverted Y
        offsetDir = normalizeQPointF(offsetDir);

        //draw from section line endpoint
        QPointF offsetEnd = m_extLen * offsetDir;
        m_beginExt1 = m_start;
        m_endExt1   = m_start + offsetEnd;
        m_beginExt2 = m_end;
        m_endExt2   = m_end + offsetEnd;
   } else {
        //extension lines run from point on section line to arrowhead
        m_beginExt1 = m_start;
        m_endExt1   = getArrowPosition(m_arrowDir1, m_start);
        m_beginExt2 = m_end;
        m_endExt2   = getArrowPosition(m_arrowDir2, m_end);
    }
}

//the extension lines are on the waste side of the section line!
void QGISectionLine::extensionEndsISO()
{
    if (m_arrowMode == SINGLEDIRECTIONMODE) {
        QPointF offsetDir(-m_arrowDir.x, m_arrowDir.y);     //reversed and inverted y
        offsetDir = normalizeQPointF(offsetDir);

        //draw from section line endpoint less arrow length
        QPointF offsetStart = offsetDir * Rez::guiX(QGIArrow::getPrefArrowSize());
        QPointF offsetEnd = m_extLen * offsetDir;

        m_beginExt1 = m_start + offsetStart;
        m_endExt1   = m_start + offsetStart + offsetEnd;
        m_beginExt2 = m_end + offsetStart;
        m_endExt2   = m_end + offsetStart + offsetEnd;
    } else {
        //extension lines run in reverse of arrow direction from base of arrowhead for distance m_extLen
        QPointF offsetDir1(-m_arrowDir1.x, m_arrowDir1.y);      //reversed and inverted y
        offsetDir1 = normalizeQPointF(offsetDir1);
        QPointF offsetStart1 =  offsetDir1 * Rez::guiX(QGIArrow::getPrefArrowSize());
        QPointF offsetEnd1 = m_extLen * offsetDir1;
        m_beginExt1 = m_start + offsetStart1;
        m_endExt1   = m_start + offsetStart1 + offsetEnd1;

        QPointF offsetDir2(-m_arrowDir2.x, m_arrowDir2.y);      //reversed and inverted y
        offsetDir2 = normalizeQPointF(offsetDir2);
        QPointF offsetStart2 =  offsetDir2 * Rez::guiX(QGIArrow::getPrefArrowSize());
        QPointF offsetEnd2 = m_extLen * offsetDir2;
        m_beginExt2 = m_end + offsetStart2;
        m_endExt2   = m_end + offsetStart2 + offsetEnd2;
    }
}

void QGISectionLine::makeChangePointMarks()
{
//    Base::Console().Message("QGISL::makeChangePointMarks()\n");
    double segmentLength = 0.50 * QGIArrow::getPrefArrowSize();
    QPen cPointPen;
    //TODO: this should really be 2.0 * thickLineWidth, but we only have one
    //width available (which should be 'thin', for the section line)
    cPointPen.setWidthF(2.0 * getWidth());
    cPointPen.setColor(getSectionColor());
    cPointPen.setStyle(Qt::SolidLine);
    for (auto& cPoint : m_changePointData) {
        QGraphicsPathItem* cPointItem = new QGraphicsPathItem();
        addToGroup(cPointItem);

        QPainterPath pPath;
        QPointF location = cPoint.getLocation();
        QPointF start = location + cPoint.getPreDirection() * segmentLength;
        QPointF end   = location + cPoint.getPostDirection() * segmentLength;
        pPath.moveTo(Rez::guiPt(start));
        pPath.lineTo(Rez::guiPt(location));
        pPath.lineTo(Rez::guiPt(end));

        cPointItem->setPath(pPath);
        cPointItem->setPen(cPointPen);
        cPointItem->setZValue(ZVALUE::SECTIONLINE + 1);
        cPointItem->setPos(0.0, 0.0);

        cPointItem->setRotation(rotation());

        m_changePointMarks.push_back(cPointItem);
    }
}

void QGISectionLine::setEnds(Base::Vector3d l1, Base::Vector3d l2)
{
    m_l1 = l1;
    m_start = QPointF(l1.x, l1.y);
    m_l2 = l2;
    m_end = QPointF(l2.x, l2.y);
}

void QGISectionLine::setBounds(double x1, double y1, double x2, double y2)
{
    m_start = QPointF(x1, y1);
    m_end = QPointF(x2, y2);
}

void QGISectionLine::setSymbol(char* sym)
{
    m_symbol = sym;
}

void QGISectionLine::setDirection(double xDir, double yDir)
{
    Base::Vector3d newDir(xDir, yDir, 0.0);
    setDirection(newDir);
}

void QGISectionLine::setDirection(Base::Vector3d dir)
{
    m_arrowMode = SINGLEDIRECTIONMODE;
    m_arrowDir = dir;
    m_arrowDir.Normalize();
    m_arrowDir1 = dir;
    m_arrowDir1.Normalize();
    m_arrowDir2 = dir;
    m_arrowDir2.Normalize();
}

void QGISectionLine::setArrowDirections(Base::Vector3d dir1, Base::Vector3d dir2)
{
    m_arrowMode = MULTIDIRECTIONMODE;
    m_arrowDir1 = dir1;
    m_arrowDir1.Normalize();
    m_arrowDir2 = dir2;
    m_arrowDir2.Normalize();
}

//convert an arrow direction vector into a Qt rotation angle degrees
double QGISectionLine::getArrowRotation(Base::Vector3d arrowDir)
{
    arrowDir.Normalize();
    double angle = atan2f(arrowDir.y, arrowDir.x);
    if (angle < 0.0) {
        angle = 2 * M_PI + angle;
    }
    double arrowRotation = 360.0 - angle * (180.0/M_PI);   //convert to Qt rotation (clockwise degrees)
    return arrowRotation;
}

QPointF QGISectionLine::getArrowPosition(Base::Vector3d arrowDir, QPointF refPoint)
{
    QPointF qArrowDir(arrowDir.x, -arrowDir.y);              //remember Y dir is flipped
    qArrowDir = normalizeQPointF(qArrowDir);

    double offsetLength = m_extLen + Rez::guiX(QGIArrow::getPrefArrowSize());
    QPointF offsetVec = offsetLength * qArrowDir;

    return QPointF(refPoint + offsetVec);
}

void QGISectionLine::setFont(QFont f, double fsize)
{
    m_symFont = f;
    m_symSize = fsize;
}

void QGISectionLine::setPath(QPainterPath& path)
{
    m_line->setPath(path);
}

void QGISectionLine::setChangePoints(TechDraw::ChangePointVector changePointData)
{
    m_changePointData = changePointData;
    clearChangePointMarks();
}

void QGISectionLine::clearChangePoints()
{
    clearChangePointMarks();
    m_changePointData.clear();
}

void QGISectionLine::clearChangePointMarks()
{
    if (!m_changePointMarks.empty()) {
        for (auto& cPoint : m_changePointMarks) {
          cPoint->hide();
          scene()->removeItem(cPoint);
          delete cPoint;
        }
        m_changePointMarks.clear();
    }
}

//QPointF does not have length or normalize methods
QPointF QGISectionLine::normalizeQPointF(QPointF inPoint)
{
    double x2 = inPoint.x() * inPoint.x();
    double y2 = inPoint.y() * inPoint.y();
    double root = sqrt(x2 + y2);
    return inPoint / root;
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
    return Preferences::getPreferenceGroup("Standards")->GetInt("SectionLineStandard", ISOSTANDARD);
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

    m_arrow1->setNormalColor(m_colCurrent);
    m_arrow1->setFillColor(m_colCurrent);
    m_arrow1->setPrettyNormal();
    m_arrow2->setNormalColor(m_colCurrent);
    m_arrow2->setFillColor(m_colCurrent);
    m_arrow2->setPrettyNormal();

    m_symbol1->setDefaultTextColor(m_colCurrent);
    m_symbol2->setDefaultTextColor(m_colCurrent);
}
