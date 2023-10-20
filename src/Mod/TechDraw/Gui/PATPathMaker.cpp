/***************************************************************************
 *   Copyright (c) 2023 WandererFan  <wandererfan@gmail.com>               *
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
# include <cmath>
# include <QPainter>
# include <QPainterPath>
# include <QPointF>
# include <QRectF>
# include <QTransform>
#endif
#include <QByteArrayMatcher>

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "Rez.h"
#include "PATPathMaker.h"

using namespace TechDrawGui;
using namespace TechDraw;

PATPathMaker::PATPathMaker(QGraphicsItem* parent, double lineWidth, double fillScale) :
    m_parent(parent),
    m_fillScale(fillScale),
    m_lineWidth(lineWidth)
{
    m_maxSeg = Preferences::getPreferenceGroup("PAT")->GetInt("MaxSeg", 10000l);
}


/// convert the PAT line set to QGraphicsPathItems
void PATPathMaker::lineSetToFillItems(LineSet& ls)
{
    m_segCount = 0;
    QPen pen = getPen();
    for (auto& geom : ls.getGeoms()) {
        //geom is a tdGeometry representation of 1 line in the pattern
        if (ls.isDashed()) {
            double offset = 0.0;
            Base::Vector3d pStart = ls.getPatternStartPoint(geom, offset, m_fillScale);
            offset = Rez::guiX(offset);
            Base::Vector3d gStart(geom->getStartPoint().x,
                                  geom->getStartPoint().y,
                                  0.0);
            Base::Vector3d gEnd(geom->getEndPoint().x,
                                geom->getEndPoint().y,
                                0.0);
            if (DrawUtil::fpCompare(offset, 0.0, 0.00001)) {                              //no offset
                QGraphicsPathItem* item1 = lineFromPoints(pStart, gEnd, ls.getDashSpec());
                item1->setPen(pen);
                m_fillItems.push_back(item1);
                if (!pStart.IsEqual(gStart, 0.00001)) {
                    QGraphicsPathItem* item2 = lineFromPoints(pStart, gStart, ls.getDashSpec().reversed());
                    item2->setPen(pen);
                    m_fillItems.push_back(item2);
                }
            } else {                                                                  //offset - pattern start not in g
                double remain = dashRemain(decodeDashSpec(ls.getDashSpec()), offset);
                QGraphicsPathItem* shortItem = geomToStubbyLine(geom, remain, ls);
                shortItem->setPen(pen);
                m_fillItems.push_back(shortItem);
            }
        } else {                                                //not dashed
            QGraphicsPathItem* fillItem = geomToLine(geom, ls);
            fillItem->setPen(pen);
            m_fillItems.push_back(fillItem);
        }

        if (m_segCount > m_maxSeg) {
            Base::Console().Warning("PAT segment count exceeded: %ld\n", m_segCount);
            break;
        }
    }
}


/// create a PAT fill line from 2 points and a dash configuration
QGraphicsPathItem*  PATPathMaker::lineFromPoints(Base::Vector3d start, Base::Vector3d end, DashSpec ds)
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem(m_parent);
    fillItem->setPath(dashedPPath(decodeDashSpec(ds),
                                  Rez::guiX(start),
                                  Rez::guiX(end)));
    return fillItem;
}


/// create a PAT fill line from geometry
QGraphicsPathItem*  PATPathMaker::geomToLine(TechDraw::BaseGeomPtr base, LineSet& ls)
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem(m_parent);
    Base::Vector3d start(base->getStartPoint().x,
                            base->getStartPoint().y,
                            0.0);
    Base::Vector3d end(base->getEndPoint().x,
                            base->getEndPoint().y,
                            0.0);
    fillItem->setPath(dashedPPath(decodeDashSpec(ls.getDashSpec()),
                                  Rez::guiX(start),
                                  Rez::guiX(end)));
    return fillItem;
}


//! make a fragment (length = remain) of a dashed line, with pattern starting at +offset
QGraphicsPathItem*  PATPathMaker::geomToStubbyLine(TechDraw::BaseGeomPtr base, double remain, LineSet& ls)
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem(m_parent);
    Base::Vector3d start(base->getStartPoint().x,
                         base->getStartPoint().y,
                         0.0);
    Base::Vector3d end(base->getEndPoint().x,
                       base->getEndPoint().y,
                       0.0);
    double origLen = (end - start).Length();

    double appRemain = Rez::appX(remain);
    Base::Vector3d newEnd = start + (ls.getUnitDir() * appRemain);

    double newLen = (newEnd - start).Length();

    if (newLen > origLen) {
        newEnd = end;
    }

    double offset = Rez::guiX(m_fillScale * ls.getDashSpec().length()) - remain;

    fillItem->setPath(dashedPPath(offsetDash(decodeDashSpec(ls.getDashSpec()), offset),
                                  Rez::guiX(start),
                                  Rez::guiX(newEnd)));
    m_fillItems.push_back(fillItem);
    return fillItem;
}


//! convert from mm to scene units
std::vector<double> PATPathMaker::decodeDashSpec(DashSpec patDash)
{
    double penWidth = Rez::guiX(m_lineWidth);
    double scale = m_fillScale;
    double minPen = 0.01;                         //avoid trouble with cosmetic pen (zero width)?
    if (penWidth <= minPen) {
        penWidth = minPen;
    }
    std::vector<double> result;
    for (auto& d: patDash.get()) {
        double strokeLength;
        if (DrawUtil::fpCompare(d, 0.0)) {                       //pat dot
             strokeLength = penWidth;
        } else {                                                //pat mark/space
             strokeLength = Rez::guiX(d);
        }
        result.push_back(scale * strokeLength);
    }
    return result;
}

//! make a dashed QPainterPath from start to end in scene coords
QPainterPath PATPathMaker::dashedPPath(const std::vector<double> dashPattern, const Base::Vector3d start, const Base::Vector3d end)
{
      QPainterPath result;
      Base::Vector3d dir = (end - start);
      dir.Normalize();
      result.moveTo(start.x, -start.y);
      Base::Vector3d currentPos = start;
      if (dashPattern.empty()) {
          result.lineTo(end.x, -end.y);
          m_segCount++;
      } else {
         double lineLength = (end - start).Length();
         double travel = 0.0;
         Base::Vector3d lineProgress;
         while (travel < lineLength) {
             bool stop = false;
            if (m_segCount > 10000) {
                Base::Console().Warning("PAT segment count exceeded: %ld\n", m_segCount);
                break;
            }

             for (auto& d: dashPattern) {
                  travel += fabs(d);
                  Base::Vector3d segmentEnd = (currentPos + dir * fabs(d));
                  if ((start - segmentEnd).Length() > lineLength)  {            //don't draw past end of line
                      segmentEnd = end;
                      stop = true;
                  }
                  if (d < 0.0) {
                      result.moveTo(segmentEnd.x, -segmentEnd.y);                //space
                  } else {
                      result.lineTo(segmentEnd.x, -segmentEnd.y);                //mark
                  }
                  if (stop) {
                      break;
                  }
                  m_segCount++;
                  currentPos = segmentEnd;
              }
          }
      }
      return result;
}


//! convert a dash pattern to an offset dash pattern  (ie offset -> end)
// dashPattern & offset are already scaled.
std::vector<double> PATPathMaker::offsetDash(const std::vector<double> dashPattern, const double offset)
{
    std::vector<double> result;
    double length = 0.0;
    for (auto& d: dashPattern) {
        length += fabs(d);
    }
    if (offset > length) {
        result = dashPattern;
        return result;
    }
    //find the dash cell that includes offset
    double accum = 0;
    int i = 0;
    for (auto& d:dashPattern) {
        accum += fabs(d);
        if (accum > offset) {
           break;
        }
        i++;
    }

    double firstCell = accum - offset;
    if (dashPattern.at(i) < 0.0) {                    //offset found in a space cell
        result.push_back(-1.0* firstCell);
    } else {
        result.push_back(firstCell);
    }
    unsigned int iCell = i + 1;
    for ( ; iCell < dashPattern.size() ; iCell++) {
        result.push_back(dashPattern.at(iCell));
    }

    return result;
}


//! find remaining length of a dash pattern after offset
double PATPathMaker::dashRemain(const std::vector<double> dashPattern, const double offset)
{
    double length = 0.0;
    for (auto& d: dashPattern) {
        length += fabs(d);
    }
    if (offset > length) {
        return 0.0;
    }
    return length - offset;
}


