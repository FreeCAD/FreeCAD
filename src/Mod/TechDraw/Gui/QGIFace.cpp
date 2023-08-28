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
# include <cmath>

# include <QFileInfo>
# include <QGraphicsView>
# include <QPainter>
# include <QPainterPath>
# include <QPointF>
# include <QRectF>
# include <QTransform>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "PreferencesGui.h"
#include "QGIFace.h"
#include <QByteArrayMatcher>
#include "QGCustomImage.h"
#include "QGCustomRect.h"
#include "QGCustomSvg.h"
#include "QGICMark.h"
#include "QGIPrimPath.h"
#include "Rez.h"
#include "ZVALUE.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIFace::QGIFace(int index) :
    projIndex(index),
    m_hideSvgTiles(false),
    m_hatchRotation(0.0)
{
    m_segCount = 0;
//    setFillMode(NoFill);
    isHatched(false);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    //setStyle(Qt::NoPen);    //don't draw face lines, just fill for debugging
    setStyle(Qt::DashLine);
    m_geomColor = PreferencesGui::getAccessibleQColor(QColor(Qt::black));
    setLineWeight(0.5);                   //0 = cosmetic

    setPrettyNormal();
    m_texture = QPixmap();                      //empty texture

    m_imageHatchArea = new QGCustomImage();
    m_imageHatchArea->setParentItem(this);

    m_svgHatchArea = new QGCustomRect();
    m_svgHatchArea->setParentItem(this);

    m_svgCol = SVGCOLDEFAULT;
    m_fillScale = 1.0;

    getParameters();

    // set up style & colour defaults
    m_styleDef = Qt::SolidPattern;
    m_styleSelect = Qt::SolidPattern;
    App::Color temp {static_cast<uint32_t>(Preferences::getPreferenceGroup("Colors")->GetUnsigned("FaceColor",0xffffffff))};
    setFillColor(temp.asValue<QColor>());
    m_colDefFill = temp.asValue<QColor>();

    if (m_defClearFace) {
        setFillMode(NoFill);
        m_colDefFill = Qt::transparent;
        setFill(Qt::transparent, m_styleDef);
    } else {
        setFillMode(PlainFill);
        setFill(m_colDefFill, m_styleDef);
    }

    m_sharedRender = new QSvgRenderer();
}

QGIFace::~QGIFace()
{
    delete m_sharedRender;
}

/// redraw this face
void QGIFace::draw()
{
    setPath(m_outline);                         //Face boundary

    if (isHatched()) {
        if (m_mode == GeomHatchFill) {
            //GeomHatch does not appear in pdf if clipping is set to true
            setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
            if (!m_lineSets.empty()) {
                m_brush.setTexture(QPixmap());
                m_fillStyleCurrent = m_styleDef;
                m_styleNormal = m_fillStyleCurrent;
                for (auto& ls: m_lineSets) {
                    lineSetToFillItems(ls);
                }
            }
            m_imageHatchArea->hide();
            m_svgHatchArea->hide();
        } else if (m_mode == SvgFill) {
            m_brush.setTexture(QPixmap());
            m_styleNormal = m_styleDef;
            m_fillStyleCurrent = m_styleNormal;
            loadSvgHatch(m_fileSpec);
            if (m_hideSvgTiles) {
                //bitmap hatch doesn't need clipping
                setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
                buildPixHatch();
                m_svgHatchArea->hide();
                m_imageHatchArea->show();
            } else {
                //SVG tiles need to be clipped
                setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
                buildSvgHatch();
                m_imageHatchArea->hide();
                m_svgHatchArea->show();
            }
        } else if (m_mode == BitmapFill) {
            m_fillStyleCurrent = Qt::TexturePattern;
            m_texture = textureFromBitmap(m_fileSpec);
            m_brush.setTexture(m_texture);
        } else if (m_mode == PlainFill) {
            setFill(m_colNormalFill, m_styleNormal);
            m_imageHatchArea->hide();
            m_svgHatchArea->hide();
        }
    } else {
        m_imageHatchArea->hide();
        m_svgHatchArea->hide();
    }
    show();
}

/// show the face style & colour in normal configuration
void QGIFace::setPrettyNormal() {
//    Base::Console().Message("QGIF::setPrettyNormal() - hatched: %d\n", isHatched());
    if (isHatched()  &&
        (m_mode == BitmapFill) ) {                               //hatch with bitmap fill
        m_fillStyleCurrent = Qt::TexturePattern;
        m_brush.setTexture(m_texture);
    } else {
        m_brush.setTexture(QPixmap());
    }
    QGIPrimPath::setPrettyNormal();
}

/// show the face style & colour in pre-select configuration
void QGIFace::setPrettyPre() {
//    Base::Console().Message("QGIF::setPrettyPre()\n");
    m_fillStyleCurrent = Qt::SolidPattern;
    m_brush.setTexture(QPixmap());
    QGIPrimPath::setPrettyPre();
}

/// show the face style & colour in selected configuration
void QGIFace::setPrettySel() {
//    Base::Console().Message("QGIF::setPrettySel()\n");
    m_fillStyleCurrent = Qt::SolidPattern;
    m_brush.setTexture(QPixmap());
    QGIPrimPath::setPrettySel();
}

/// show or hide the edges of this face.  Usually just for debugging
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

/// get the .svg file to use for hatching this face
void QGIFace::loadSvgHatch(std::string fileSpec)
{
    QString qfs(QString::fromUtf8(fileSpec.data(), fileSpec.size()));
    QFile f(qfs);
    if (!f.open(QFile::ReadOnly | QFile::Text))  {
        Base::Console().Error("QGIFace could not read %s\n", fileSpec.c_str());
        return;
    }
    m_svgXML = f.readAll();

    // search in the file for the "stroke" specification in order to find out what declaration style is used
    // this is necessary to apply a color set by the user to the SVG
    QByteArray pattern("stroke:");
    QByteArrayMatcher matcher(pattern);
    int pos = 0;
    if (matcher.indexIn(m_svgXML, pos) != -1) {
        SVGCOLPREFIX = "stroke:"; // declaration part of a style="" statement
    } else {
        SVGCOLPREFIX = "stroke=\""; // declaration of its own
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

/// update the outline of this face
void QGIFace::setOutline(const QPainterPath & path)
{
    m_outline = path;
}

/// remove the PAT hatch lines
void QGIFace::clearLineSets()
{
    m_dashSpecs.clear();
    clearFillItems();
}

/// add PAT hatch line set
void QGIFace::addLineSet(LineSet& ls)
{
    m_lineSets.push_back(ls);
}

/// convert the PAT line set to QGraphicsPathItems
void QGIFace::lineSetToFillItems(LineSet& ls)
{
    m_segCount = 0;
    QPen pen = setGeomPen();
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
QGraphicsPathItem*  QGIFace::lineFromPoints(Base::Vector3d start, Base::Vector3d end, DashSpec ds)
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem(this);
    fillItem->setPath(dashedPPath(decodeDashSpec(ds),
                                  Rez::guiX(start),
                                  Rez::guiX(end)));
    return fillItem;
}

/// create a PAT fill line from geometry
QGraphicsPathItem*  QGIFace::geomToLine(TechDraw::BaseGeomPtr base, LineSet& ls)
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
QGraphicsPathItem*  QGIFace::geomToStubbyLine(TechDraw::BaseGeomPtr base, double remain, LineSet& ls)
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

QPen QGIFace::setGeomPen()
{
    QPen result;
    result.setWidthF(Rez::guiX(m_geomWeight));
    result.setColor(m_geomColor);
    result.setStyle(Qt::SolidLine);
    return result;
}

//! convert from mm to scene units
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
QPainterPath QGIFace::dashedPPath(const std::vector<double> dv, const Base::Vector3d start, const Base::Vector3d end)
{
      QPainterPath result;
      Base::Vector3d dir = (end - start);
      dir.Normalize();
      result.moveTo(start.x, -start.y);
      Base::Vector3d currentPos = start;
      if (dv.empty()) {
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

             for (auto& d: dv) {
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
    double length = 0.0;
    for (auto& d: dv) {
        length += fabs(d);
    }
    if (offset > length) {
        return 0.0;
    }
    return length - offset;
}

//! get zoom level (scale) from QGraphicsView
// not used currently
double QGIFace::getXForm()
{
    //try to keep the pattern the same when View scales
    auto s = scene();
    if (s) {
        auto vs = s->views();     //ptrs to views
        if (!vs.empty()) {
            auto v = vs.at(0);
            auto i = v->transform().inverted();
            return i.m11();
        }
    }
    return 1.0;
}

/// remove the children that make up a PAT fill
void QGIFace::clearFillItems()
{
    for (auto& f: m_fillItems) {
        f->setParentItem(nullptr);
        this->scene()->removeItem(f);
        delete f;
    }
}

/// debugging tool draws a mark at a position on this face
void QGIFace::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x, y);
    cmItem->setThick(1.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

/// make an array of svg tiles to cover this face
void QGIFace::buildSvgHatch()
{
//    Base::Console().Message("QGIF::buildSvgHatch() - offset: %s\n", DrawUtil::formatVector(getHatchOffset()).c_str());
    double wTile = SVGSIZEW * m_fillScale;
    double hTile = SVGSIZEH * m_fillScale;
    double faceWidth = m_outline.boundingRect().width();
    double faceHeight = m_outline.boundingRect().height();
    double faceOverlaySize = Preferences::svgHatchFactor() * std::max(faceWidth, faceHeight);
    QPointF faceCenter = m_outline.boundingRect().center();
    double tilesWide = ceil(faceOverlaySize / wTile);
    double tilesHigh = ceil(faceOverlaySize / hTile);

    double overlayWidth = tilesWide * wTile;
    double overlayHeight = tilesHigh * hTile;
    m_svgHatchArea->setRect(0., 0., overlayWidth,-overlayHeight);
    m_svgHatchArea->centerAt(faceCenter);
    QByteArray before, after;
    before = QString::fromStdString(SVGCOLPREFIX + SVGCOLDEFAULT).toUtf8();
    after = QString::fromStdString(SVGCOLPREFIX + m_svgCol).toUtf8();
    QByteArray colorXML = m_svgXML.replace(before, after);
    if (!m_sharedRender->load(colorXML)) {
        Base::Console().Message("QGIF::buildSvgHatch - failed to load svg string\n");
        return;
    }
    long int tileCount = 0;
    for (int iw = 0; iw < int(tilesWide); iw++) {
        for (int ih = 0; ih < int(tilesHigh); ih++) {
            QGCustomSvg* tile = new QGCustomSvg();
            tile->setScale(m_fillScale);
            tile->setSharedRenderer(m_sharedRender);
            tile->setParentItem(m_svgHatchArea);
            tile->setPos(iw*wTile + getHatchOffset().x,
                         -overlayWidth + ih*hTile + getHatchOffset().y);
            tileCount++;
            if (tileCount > m_maxTile) {
                Base::Console().Warning("SVG tile count exceeded: %ld. Change hatch scale or raise limit.\n", tileCount);
                break;
            }
        }
        if (tileCount > m_maxTile) {
            break;
        }
    }
    QPointF faceCenterToMRect = mapToItem(m_svgHatchArea, faceCenter);
    m_svgHatchArea->setTransformOriginPoint(faceCenterToMRect);
    m_svgHatchArea->setRotation(m_hatchRotation);
}

void QGIFace::clearSvg()
{
    hideSvg(true);
}

/// make an array of bitmap tiles to cover this face
void QGIFace::buildPixHatch()
{
//    Base::Console().Message("QGIF::buildPixHatch() - offset: %s\n", DrawUtil::formatVector(getHatchOffset()).c_str());
    double wTile = SVGSIZEW * m_fillScale;
    double hTile = SVGSIZEH * m_fillScale;
    double faceWidth = m_outline.boundingRect().width();
    double faceHeight = m_outline.boundingRect().height();
    QRectF faceRect = m_outline.boundingRect();
    QPointF faceCenter = faceRect.center();
    double hatchOverlaySize = Preferences::svgHatchFactor() * std::max(faceWidth, faceHeight);
    double numberWide = ceil(hatchOverlaySize / wTile);
    double numberHigh = ceil(hatchOverlaySize / hTile);
    double overlayWidth = numberWide * wTile;
    double overlayHeight= numberHigh * hTile;

    m_svgHatchArea->setRect(0., 0., overlayWidth, -overlayHeight);
    m_svgHatchArea->centerAt(faceCenter);

    QByteArray before, after;
    before = QString::fromStdString(SVGCOLPREFIX + SVGCOLDEFAULT).toUtf8();
    after = QString::fromStdString(SVGCOLPREFIX + m_svgCol).toUtf8();
    QByteArray colorXML = m_svgXML.replace(before, after);
    QSvgRenderer renderer;
    bool success = renderer.load(colorXML);
    if (!success) {
        Base::Console().Error("QGIF::buildPixHatch - renderer failed to load\n");
    }

    //get the svg tile graphics as a QImage
    QImage imageIn(64, 64, QImage::Format_ARGB32);
    imageIn.fill(Qt::transparent);
    QPainter painter(&imageIn);
    renderer.render(&painter);
    if (imageIn.isNull()) {
        Base::Console().Error("QGIF::buildPixHatch - imageIn is null\n");
        return;
    }
    //make a QPixmap tile of the QImage
    QPixmap pm(64, 64);
    pm  = QPixmap::fromImage(imageIn);
    pm = pm.scaled(wTile, hTile);
    if (pm.isNull()) {
        Base::Console().Error("QGIF::buildPixHatch - pixmap is null\n");
        return;
    }

    //layout a field of QPixmap tiles as a QImage
    QImage tileField(overlayWidth, overlayHeight, QImage::Format_ARGB32);
    QPointF fieldCenter(overlayWidth / 2.0, overlayHeight / 2.0);

    tileField.fill(Qt::transparent);
    QPainter painter2(&tileField);
    QPainter::RenderHints hints = painter2.renderHints();
    hints = hints & QPainter::Antialiasing;
    painter2.setRenderHints(hints);
    QPainterPath clipper = path();
    QPointF offset = (fieldCenter - faceCenter);
    clipper.translate(offset);
    painter2.setClipPath(clipper);

    long int tileCount = 0;
    for (int iw = 0; iw < int(numberWide); iw++) {
        for (int ih = 0; ih < int(numberHigh); ih++) {
            painter2.drawPixmap(QRectF(iw*wTile + getHatchOffset().x, ih*hTile + getHatchOffset().y,
                                       wTile, hTile),   //target rect
                               pm,                                           //map
                               QRectF(0, 0, wTile, hTile));  //source rect
            tileCount++;
            if (tileCount > m_maxTile) {
                Base::Console().Warning("Pixmap tile count exceeded: %ld. Change hatch scale or raise limit.\n", tileCount);
                break;
            }
        }
        if (tileCount > m_maxTile) {
            break;
        }
    }

    QPixmap bigMap(fabs(faceRect.width()), fabs(faceRect.height()));
    bigMap = QPixmap::fromImage(tileField);

    QPixmap nothing;
    m_imageHatchArea->setPixmap(nothing);
    m_imageHatchArea->load(bigMap);
    m_imageHatchArea->centerAt(faceCenter);
}

//this isn't used currently
QPixmap QGIFace::textureFromSvg(std::string fileSpec)
{
    QString qs(QString::fromStdString(fileSpec));
    QFileInfo ffi(qs);
    if (!ffi.isReadable()) {
        return QPixmap();
    }
    QSvgRenderer renderer(qs);
    QPixmap pixMap(renderer.defaultSize());
    pixMap.fill(Qt::white);                                            //try  Qt::transparent?
    QPainter painter(&pixMap);
    renderer.render(&painter);                                         //svg texture -> bitmap
    return pixMap.scaled(m_fillScale, m_fillScale);
}

void QGIFace::setHatchColor(App::Color c)
{
    m_svgCol = c.asHexString();
    m_geomColor = c.asValue<QColor>();
}

void QGIFace::setHatchScale(double s)
{
    m_fillScale = s;
}

/// turn svg tiles on or off. QtSvg does not handle clipping,
/// so we must be able to turn the hatching on/off when exporting a face with an
/// svg hatch.  Otherwise the full tile pattern is shown in the export.
void QGIFace::hideSvg(bool b)
{
    m_hideSvgTiles = b;
}


/// create a QPixmap from a bitmap file.  The QPixmap will be used as a QBrush
/// texture.
QPixmap QGIFace::textureFromBitmap(std::string fileSpec)
{
    QPixmap pix;

    QString qfs(QString::fromUtf8(fileSpec.data(), fileSpec.size()));
    QFile f(qfs);
    if (!f.open(QFile::ReadOnly))  {
        Base::Console().Error("QGIFace could not read %s\n", fileSpec.c_str());
        return pix;
    }
    QByteArray bytes = f.readAll();
    pix.loadFromData(bytes);
    if (m_hatchRotation != 0.0) {
        QTransform rotator;
        rotator.rotate(m_hatchRotation);
        pix = pix.transformed(rotator);
    }
    return pix;
}

void QGIFace::setLineWeight(double w) {
    m_geomWeight = w;
}

void QGIFace::getParameters()
{
    m_maxSeg = Preferences::getPreferenceGroup("PAT")->GetInt("MaxSeg", 10000l);
    m_maxTile = Preferences::getPreferenceGroup("Decorations")->GetInt("MaxSVGTile", 10000l);

    m_defClearFace = Preferences::getPreferenceGroup("Colors")->GetBool("ClearFace", false);
}

QRectF QGIFace::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGIFace::shape() const
{
    return path();
}
