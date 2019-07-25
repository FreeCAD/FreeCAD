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
#include <QBitmap>
#include <QFile>
#include <QFileInfo>
#endif

#include <QFile>
#include <QTextStream>
#include <QRectF>
#include <QPointF>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTransform>

#include <cmath>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawUtil.h>

//debug
#include "QGICMark.h"
#include "ZVALUE.h"
//
#include "Rez.h"
#include "QGCustomSvg.h"
#include "QGCustomRect.h"
#include "QGIViewPart.h"
#include "QGIFace.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIFace::QGIFace(int index) :
    projIndex(index),
    m_colDefFill(Qt::white),
    m_styleDef(Qt::SolidPattern),
    m_styleSelect(Qt::SolidPattern)
{
    m_segCount = 0;
    setFillMode(NoFill);
    isHatched(false);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape,true);

    //setStyle(Qt::NoPen);    //don't draw face lines, just fill for debugging
    setStyle(Qt::DashLine);

    m_styleNormal = m_styleDef;
    m_fillStyle = m_styleDef;
    m_colNormalFill = m_colDefFill;
    m_geomColor = QColor(Qt::black);
    setLineWeight(0.5);                   //0 = cosmetic
    
    setPrettyNormal();
    m_texture = QPixmap();                      //empty texture

    m_svg = new QGCustomSvg();

    m_rect = new QGCustomRect();
    m_rect->setParentItem(this);

    m_svgCol = SVGCOLDEFAULT;
    m_fillScale = 1.0;
    
    getParameters();
}

QGIFace::~QGIFace()
{
    //nothing to do. every item is a child of QGIFace & will get removed/deleted when QGIF is deleted
}

void QGIFace::draw() 
{
    setPath(m_outline);                         //Face boundary

    if (isHatched()) {   
        if (m_mode == GeomHatchFill) {
            if (!m_lineSets.empty()) {
                m_brush.setTexture(QPixmap());
                m_fillStyle = m_styleDef;
                m_styleNormal = m_fillStyle;
                for (auto& ls: m_lineSets) {
                    lineSetToFillItems(ls);
                }
            }
        } else if ((m_mode == FromFile) ||
                   (m_mode == SvgFill)  ||
                   (m_mode == BitmapFill)) {  
            QFileInfo hfi(QString::fromUtf8(m_fileSpec.data(),m_fileSpec.size()));
            if (hfi.isReadable()) {
                QString ext = hfi.suffix();
                if (ext.toUpper() == QString::fromUtf8("SVG")) {
                    setFillMode(SvgFill);
                    m_brush.setTexture(QPixmap());
                    m_fillStyle = m_styleDef;
                    m_styleNormal = m_fillStyle;
                    loadSvgHatch(m_fileSpec);
                    buildSvgHatch();
                    toggleSvg(true);
                } else if ((ext.toUpper() == QString::fromUtf8("JPG"))   ||
                         (ext.toUpper() == QString::fromUtf8("PNG"))   ||
                         (ext.toUpper() == QString::fromUtf8("JPEG"))  ||
                         (ext.toUpper() == QString::fromUtf8("BMP")) ) {
                    setFillMode(BitmapFill);
                    toggleSvg(false);
                    m_fillStyle   = Qt::TexturePattern;
                    m_texture = textureFromBitmap(m_fileSpec);
                    m_brush.setTexture(m_texture);
                }
            }
        }
    }
    show();
}

void QGIFace::setPrettyNormal() {
    if (isHatched()  &&
        (m_mode == BitmapFill) ) {                               //hatch with bitmap fill
        m_fillStyle = Qt::TexturePattern;
        m_brush.setTexture(m_texture);
    } else {
        m_fillStyle = m_styleNormal;
        m_brush.setTexture(QPixmap());
        m_brush.setStyle(m_fillStyle);
        m_fillColor = m_colNormalFill;
    }
    QGIPrimPath::setPrettyNormal();
}

void QGIFace::setPrettyPre() {
    m_brush.setTexture(QPixmap());
    m_fillStyle = m_styleSelect;
    m_fillColor = getPreColor();
    QGIPrimPath::setPrettyPre();
}

void QGIFace::setPrettySel() {
    m_brush.setTexture(QPixmap());
    m_fillStyle = m_styleSelect;
    m_fillColor = getSelectColor();
    QGIPrimPath::setPrettySel();
}

void QGIFace::setDrawEdges(bool b) {
    if (b) {
        setStyle(Qt::DashLine);
    } else {
        setStyle(Qt::NoPen);    //don't draw face lines, just fill
    }
}

void QGIFace::setHatchFile(std::string fileSpec)
{
    m_fileSpec = fileSpec;
}   
 
void QGIFace::loadSvgHatch(std::string fileSpec)
{
    QString qfs(QString::fromUtf8(fileSpec.data(),fileSpec.size()));
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
}

void QGIFace::setFillMode(QGIFace::fillMode m)
{
    m_mode = m;
    if ((m_mode == NoFill) ||
        (m_mode == PlainFill)) {
        isHatched(false);
    } else {
        isHatched(true);
    }
}

void QGIFace::setOutline(const QPainterPath & path)
{
    m_outline = path;
}

void QGIFace::clearLineSets(void) 
{
    m_dashSpecs.clear();
    clearFillItems();
}

void QGIFace::addLineSet(LineSet& ls)
{
    m_lineSets.push_back(ls);
}

void QGIFace::lineSetToFillItems(LineSet& ls)
{
    m_segCount = 0;
    QPen pen = setGeomPen();
    for (auto& g: ls.getGeoms()) {
        if (ls.isDashed()) {
            double offset = 0.0;
            Base::Vector3d pStart = ls.getPatternStartPoint(g, offset,m_fillScale);
            offset = Rez::guiX(offset);
            Base::Vector3d gStart(g->getStartPoint().x,
                                  g->getStartPoint().y,
                                  0.0);
            Base::Vector3d gEnd(g->getEndPoint().x,
                                g->getEndPoint().y,
                                0.0);
            if (DrawUtil::fpCompare(offset,0.0, 0.00001)) {                              //no offset
                QGraphicsPathItem* item1 = lineFromPoints(pStart, gEnd, ls.getDashSpec());
                item1->setPen(pen);
                m_fillItems.push_back(item1);
                if (!pStart.IsEqual(gStart,0.00001)) {
                    QGraphicsPathItem* item2 = lineFromPoints(pStart, gStart, ls.getDashSpec().reversed());
                    item2->setPen(pen);
                    m_fillItems.push_back(item2);
                }
            } else {                                                                  //offset - pattern start not in g
                double remain = dashRemain(decodeDashSpec(ls.getDashSpec()),offset);
                QGraphicsPathItem* shortItem = geomToStubbyLine(g, remain, ls);
                shortItem->setPen(pen);
                m_fillItems.push_back(shortItem);
            }
        } else {                                                //not dashed
            QGraphicsPathItem* fillItem = geomToLine(g, ls);
            fillItem->setPen(pen);
            m_fillItems.push_back(fillItem);
        }

        if (m_segCount > m_maxSeg) {
            Base::Console().Warning("PAT segment count exceeded: %ld\n",m_segCount);
            break;
        }
    }
}

QGraphicsPathItem*  QGIFace::lineFromPoints(Base::Vector3d start, Base::Vector3d end, DashSpec ds)
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem(this);
    fillItem->setPath(dashedPPath(decodeDashSpec(ds),
                                  Rez::guiX(start), 
                                  Rez::guiX(end)));
    return fillItem;
}

QGraphicsPathItem*  QGIFace::geomToLine(TechDraw::BaseGeom* base, LineSet& ls)
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem(this);
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
QGraphicsPathItem*  QGIFace::geomToStubbyLine(TechDraw::BaseGeom* base, double remain, LineSet& ls)
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem(this);
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

QPen QGIFace::setGeomPen(void)
{
    QPen result;
    result.setWidthF(Rez::guiX(m_geomWeight));
    result.setColor(m_geomColor);
    result.setStyle(Qt::SolidLine);
    return result;
}

//!convert from mm to scene units
std::vector<double> QGIFace::decodeDashSpec(DashSpec patDash)
{
    double penWidth = Rez::guiX(m_geomWeight);
    double scale = m_fillScale;
    double minPen = 0.01;                         //avoid trouble with cosmetic pen (zero width)?
    if (penWidth <= minPen) {
        penWidth = minPen;
    }
    std::vector<double> result;
    for (auto& d: patDash.get()) {
        double strokeLength;
        if (DrawUtil::fpCompare(d,0.0)) {                       //pat dot
             strokeLength = penWidth;
        } else {                                                //pat mark/space
             strokeLength = Rez::guiX(d);
        }
        result.push_back(scale * strokeLength);
    }
    return result;
}

//! make a dashed QPainterPath from start to end in scene coords
QPainterPath QGIFace::dashedPPath(const std::vector<double> dv, const Base::Vector3d start, const Base::Vector3d end)
{
      QPainterPath result;
      Base::Vector3d dir = (end - start);
      dir.Normalize();
      result.moveTo(start.x,-start.y);
      Base::Vector3d currentPos = start;
      if (dv.empty()) {
          result.lineTo(end.x,-end.y);
          m_segCount++;
      } else {
         double lineLength = (end - start).Length();
         double travel = 0.0;
         Base::Vector3d lineProgress;
         while (travel < lineLength) {
             bool stop = false;
            if (m_segCount > 10000) {
                Base::Console().Warning("PAT segment count exceeded: %ld\n",m_segCount);
                break;
            }

             for (auto& d: dv) {
                  travel += fabs(d);
                  Base::Vector3d segmentEnd = (currentPos + dir * fabs(d));
                  if ((start - segmentEnd).Length() > lineLength)  {            //don't draw past end of line
                      segmentEnd = end;
                      stop = true;
                  }
                  if (d < 0.0) {
                      result.moveTo(segmentEnd.x,-segmentEnd.y);                //space
                  } else {
                      result.lineTo(segmentEnd.x,-segmentEnd.y);                //mark
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
// dv & offset are already scaled.
std::vector<double> QGIFace::offsetDash(const std::vector<double> dv, const double offset)
{
    std::vector<double> result;
    double length = 0.0;
    for (auto& d: dv) {
        length += fabs(d);
    }
    if (offset > length) {
        result = dv;
        return result;
    }
    //find the dash cell that includes offset
    double accum = 0;
    int i = 0;
    for (auto& d:dv) {
        accum += fabs(d);
        if (accum > offset) {
           break;
        }
        i++;
    }
    
    double firstCell = accum - offset;
    if (dv.at(i) < 0.0) {                    //offset found in a space cell
        result.push_back(-1.0* firstCell);
    } else {
        result.push_back(firstCell);
    }
    unsigned int iCell = i + 1;
    for ( ; iCell < dv.size() ; iCell++) {
        result.push_back(dv.at(iCell));
    }
    
    return result;
}

//! find remaining length of a dash pattern after offset
double QGIFace::dashRemain(const std::vector<double> dv, const double offset)
{
    double result;
    double length = 0.0;
    for (auto& d: dv) {
        length += fabs(d);
    }
    if (offset > length) {
        result = 0.0;
    } else {
        result = length - offset;
    }
    return result;
}

//! get zoom level (scale) from QGraphicsView
// not used currently
double QGIFace::getXForm(void)
{
    //try to keep the pattern the same when View scales
    double result = 1.0;
    auto s = scene();
    if (s) {
        auto vs = s->views();     //ptrs to views
        if (!vs.empty()) {
            auto v = vs.at(0);
            auto i = v->transform().inverted();
            result = i.m11();
        }
    }
    return result;
}

void QGIFace::clearFillItems(void)
{
    for (auto& f: m_fillItems) {
        f->setParentItem(nullptr);
        this->scene()->removeItem(f);
        delete f;
    }
}

void QGIFace::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x,y);
    cmItem->setThick(1.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

void QGIFace::buildSvgHatch()
{
    double wTile = SVGSIZEW * m_fillScale;
    double hTile = SVGSIZEH * m_fillScale;
    double w = m_outline.boundingRect().width();
    double h = m_outline.boundingRect().height();
    QRectF r = m_outline.boundingRect();
    QPointF fCenter = r.center();
    double nw = ceil(w / wTile);
    double nh = ceil(h / hTile);
    w = nw * wTile;
    h = nh * hTile;
    m_rect->setRect(0.,0.,w,-h);
    m_rect->centerAt(fCenter);
    r = m_rect->rect();
    QByteArray before,after;
    before.append(QString::fromStdString(SVGCOLPREFIX + SVGCOLDEFAULT));
    after.append(QString::fromStdString(SVGCOLPREFIX + m_svgCol));
    QByteArray colorXML = m_svgXML.replace(before,after);
    long int tileCount = 0;
    for (int iw = 0; iw < int(nw); iw++) {
        for (int ih = 0; ih < int(nh); ih++) {
            QGCustomSvg* tile = new QGCustomSvg();
            tile->setScale(m_fillScale);
            if (tile->load(&colorXML)) {
                tile->setParentItem(m_rect);
                tile->setPos(iw*wTile,-h + ih*hTile);
            }
            tileCount++;
            if (tileCount > m_maxTile) {
                Base::Console().Warning("SVG tile count exceeded: %ld\n",tileCount);
                break;
            }
        }
        if (tileCount > m_maxTile) {
            break;
        }
    }
}

void QGIFace::clearSvg()
{
    toggleSvg(false);
}

//this isn't used currently
QPixmap QGIFace::textureFromSvg(std::string fileSpec)
{
    QPixmap result;
    QString qs(QString::fromStdString(fileSpec));
    QFileInfo ffi(qs);
    if (ffi.isReadable()) {
        QSvgRenderer renderer(qs);
        QPixmap pixMap(renderer.defaultSize());
        pixMap.fill(Qt::white);                                            //try  Qt::transparent?
        QPainter painter(&pixMap);
        renderer.render(&painter);                                         //svg texture -> bitmap
        result = pixMap.scaled(m_fillScale,m_fillScale);
    }  //else return empty pixmap
    return result;
}

void QGIFace::setHatchColor(App::Color c)
{
    m_svgCol = c.asCSSString();
    m_geomColor = c.asValue<QColor>();
}

void QGIFace::setHatchScale(double s)
{
    m_fillScale = s;
}

//QtSvg does not handle clipping, so we must be able to turn the hatching on/off
void QGIFace::toggleSvg(bool b)
{
    if (b) {
        m_rect->show();
    } else {
        m_rect->hide();
    }
    update();
}

QPixmap QGIFace::textureFromBitmap(std::string fileSpec)
{
    QPixmap pix;
    QString qs = QString::fromUtf8(fileSpec.data(),fileSpec.size());
    QFileInfo ffi(qs);
    if (ffi.isReadable()) {
        QImage img = QImage(qs);
        img = img.scaled(Rez::guiX(m_fillScale),Rez::guiX(m_fillScale));
        pix = QPixmap::fromImage(img);
    }
    return pix;
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

void QGIFace::setLineWeight(double w) {
    m_geomWeight = w;
}

void QGIFace::getParameters(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/PAT");

    m_maxSeg = hGrp->GetInt("MaxSeg",10000l);

    hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    m_maxTile = hGrp->GetInt("MaxSVGTile",10000l);
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
//    painter->drawRect(boundingRect());          //good for debugging

    m_brush.setStyle(m_fillStyle);
    m_brush.setColor(m_fillColor);
    setBrush(m_brush);
    QGIPrimPath::paint (painter, &myOption, widget);
}

