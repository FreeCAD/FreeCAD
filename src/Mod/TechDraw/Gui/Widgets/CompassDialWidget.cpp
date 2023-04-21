/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

//largely based on a python widget from:
//https://github.com/tcalmant/demo-ipopo-qt/blob/master/pc/details/compass.py

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QtGui>
#endif

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Base/Console.h>
#include <Base/Tools.h>

#include "CompassDialWidget.h"

using namespace TechDrawGui;
using CardinalMap = std::map<int, std::string>;

CompassDialWidget::CompassDialWidget(QWidget* parent) : QWidget(parent),
    m_markInterval(15),
    m_defaultSize(75),
    m_defaultMargin(10),
    m_designRadius(64)
{
    setObjectName(QString::fromUtf8("Compass"));
    m_rect = QRect(0, 0, m_defaultSize, m_defaultSize);
    m_angle = 0.0;
    m_margin = m_defaultMargin;
    m_designDiameter = 2 * m_designRadius;

    QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    sizePolicy.setHorizontalStretch(1);
    sizePolicy.setVerticalStretch(1);
    setSizePolicy(sizePolicy);

    repaint();
}

QSize CompassDialWidget::sizeHint() const
{
    return m_rect.size();
}

QSize CompassDialWidget::minimumSizeHint() const
{
    return QRect(0, 0, m_defaultSize, m_defaultSize).size();
}

void CompassDialWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    drawWidget(painter);
    QWidget::paintEvent(event);
}

void CompassDialWidget::drawWidget(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw the background
    drawBackground(painter);

    //Draw the cardinal points
    drawMarkings(painter);

    //Draw the needle
    drawNeedle(painter);
}

void CompassDialWidget::drawBackground(QPainter& painter)
{
    painter.save();

    //Move to the center of the compass
    painter.translate(width() / 2, height() / 2);
    double scale = std::min((float) width() / (float) (m_designDiameter + 2.0 * m_margin),
                            (float) height() / (float) (m_designDiameter + 2.0 * m_margin));
    painter.scale(scale, scale);

    painter.setPen(QPen(Qt::NoPen));
    // Clear the background
    int circleWidth = 2.0 * (m_designRadius + m_margin);
    int circleHeight = 2.0 * (m_designRadius + m_margin);
    QRect circleRect(-circleWidth / 2, -circleHeight / 2, circleWidth, circleHeight);
    painter.drawEllipse(circleRect);
//    QRect backRect(-m_rect.width() / 2, -m_rect.height() / 2, m_rect.width(), m_rect.height());
    QPainterPath backPath;
    backPath.addEllipse(circleRect);
    painter.fillPath(backPath, palette().brush((QPalette::Window)));
//    painter.fillRect(backRect, palette().brush((QPalette::Window)));
    painter.restore();
}

//Draws the cardinal points on painter. This widget was designed such that
//the dial gradations extend to 50 units from center and the compass point text
//begins at 52 units from the center.  With the font size set to 12 pixels, this
//gives a design area of a circle with a radius of approximately 64 units.  All of
//the constants reflect this design size.
void CompassDialWidget::drawMarkings(QPainter& painter)
{
    CardinalMap CompassPointText( { {0, "Y"}, {45, "XY"}, {90, "X"}, {135, "X-Y"}, {180, "-Y"},
                                    {225, "-X-Y"}, {270, "-X"}, {315, "-XY"} } );
    painter.save();
    int markInterval(15);

    //Move to the center of the compass
    painter.translate(width() / 2, height() / 2);
    double scale = std::min((float) width() / (float) (m_designDiameter + 2.0 * m_margin),
                            (float) height() / (float) (m_designDiameter + 2.0 * m_margin));
    painter.scale(scale, scale);

    // Setup the fonts and the painter
    QFont widgetFont = font();
    widgetFont.setPixelSize(12);
    QFontMetrics metrics(widgetFont);

    //outer circle
    int circleWidth = 2.0 * (m_designRadius + m_margin);
    int circleHeight = 2.0 * (m_designRadius + m_margin);
    QRect circleRect(-circleWidth / 2, -circleHeight / 2, circleWidth, circleHeight);
    painter.drawEllipse(circleRect);

    painter.setFont(widgetFont);
    painter.setPen(QPen(palette().color(QPalette::WindowText)));

    int iDegree = 0;
    while ( iDegree < 360 ) {
        if (iDegree % 45 == 0) {
            //Named direction (every 45°)
            painter.drawLine(0, -40, 0, -50);    //this has to depend on m_rect or size?
            QString qPointText = Base::Tools::fromStdString(CompassPointText.at(iDegree));
            painter.drawText(-metrics.boundingRect(qPointText).width() / 2.0, -52, qPointText);
            // what is -52? line end point y = -50 + 2 for margin?
        } else {
            //Small line
            painter.drawLine(0, -45, 0, -50);
        }

        //Next line (+15°)
        painter.rotate(markInterval);
        iDegree += markInterval;
    }

    painter.restore();
}

//Draws a needle on painter
void CompassDialWidget::drawNeedle(QPainter& painter)
{
    painter.save();

    //Move to the center of the compass
    painter.translate(width() / 2, height() / 2);

    //Rotate to the correct angle
    painter.rotate(m_angle);
    double scale = std::min((float) width() / (float) (m_designDiameter + 2.0 * m_margin),
                            (float) height() / (float) (m_designDiameter + 2.0 * m_margin));
    painter.scale(scale, scale);

    //Setup the painter
    QPen needlePen(palette().color(QPalette::WindowText));
    needlePen.setWidth(2);
    needlePen.setStyle(Qt::DashDotLine);
    painter.setPen(needlePen);
    painter.setBrush(palette().color(QPalette::WindowText));

    //Draw the section line
    int sectionLineExtent(25);
    painter.drawLine(0, sectionLineExtent, 0, -sectionLineExtent);
    needlePen.setStyle(Qt::SolidLine);
    painter.setPen(needlePen);
    int viewDirectionExtent(15);
    painter.drawLine(-viewDirectionExtent, sectionLineExtent, 0, sectionLineExtent);
    painter.drawLine(-viewDirectionExtent, -sectionLineExtent, 0, -sectionLineExtent);

    //Draw the arrowheads of the needle section line
    needlePen.setWidth(1);
    needlePen.setStyle(Qt::SolidLine);
    painter.setPen(needlePen);
    int arrowLength(5);
    int arrowWidth(3);  //actual 1/2 width
    painter.drawPolygon(
        QPolygon( { QPoint(0, sectionLineExtent),
                    QPoint(-arrowLength, sectionLineExtent + arrowWidth),
                    QPoint(-arrowLength, sectionLineExtent - arrowWidth),
                    QPoint(0, sectionLineExtent) } ) );
    painter.drawPolygon(
        QPolygon( { QPoint(0, -sectionLineExtent),
                    QPoint(-arrowLength, -(sectionLineExtent + arrowWidth)),
                    QPoint(-arrowLength, -(sectionLineExtent - arrowWidth)),
                    QPoint(0, -sectionLineExtent) } ) );

    //draw the actual needle
    needlePen.setStyle(Qt::SolidLine);
    painter.setPen(needlePen);
    painter.setBrush(palette().color(QPalette::BrightText));
    int needleExtent(40);
    painter.drawPolygon(
        QPolygon( { QPoint(needleExtent, 0),
                    QPoint(0, 5),
                    QPoint(-15, 2),
                    QPoint(-15, -2),
                    QPoint(0, -5),
                    QPoint(needleExtent, 0) } ) );

    //draw needle pivot
    painter.setBrush(palette().color(QPalette::WindowText));
    int pivotSize(4);
    QRect pivotRect(-pivotSize / 2, -pivotSize / 2, pivotSize, pivotSize);
    painter.drawEllipse(pivotRect);

    //draw needle point
    painter.setBrush(QBrush(Qt::red));
    int pointLength(5);
    int pointWidth(3);
    painter.drawPolygon(
        QPolygon( { QPoint(needleExtent, 0),
                    QPoint(needleExtent - pointLength, pointWidth),
                    QPoint(needleExtent - pointLength, -pointWidth),
                    QPoint(needleExtent, 0) } ) );

    painter.restore();
}

//convert a conventional angle to a Qt angle and set the dial accordingly
void CompassDialWidget::setAngle(double newAngle)
{
    m_angle = fmod(360.0 - newAngle, 360.0);
    repaint();
}

void CompassDialWidget::setSize(int newSize)
{
    m_rect = QRect(0, 0, newSize, newSize);
    repaint();
}
