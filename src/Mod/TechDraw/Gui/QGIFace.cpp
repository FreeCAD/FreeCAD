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
#include "QGCustomRect.h"
#include "QGCustomSvg.h"
#include "QGCustomImage.h"
#include "QGICMark.h"
#include "QGIPrimPath.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ZVALUE.h"

using namespace TechDrawGui;
using namespace TechDraw;

using DU = DrawUtil;

QGIFace::QGIFace(int index) :
    m_hideSvgTiles(false),
    projIndex(index),
    m_hatchRotation(0.0)
{
    isHatched(false);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    setStyle(Qt::NoPen);    //don't draw face lines, just fill for debugging
    //setStyle(Qt::DashLine);
    m_geomColor = PreferencesGui::getAccessibleQColor(QColor(Qt::black));
    m_styleCurrent = Qt::NoPen;
    m_pen.setStyle(m_styleCurrent);
    setLineWeight(0.0);                   //0 = cosmetic

    m_texture = QPixmap();                      //empty texture

    m_svgHatchArea = new QGCustomRect();
    m_svgHatchArea->setParentItem(this);
    m_imageSvgHatchArea = new QGCustomImage();
    m_imageSvgHatchArea->setParentItem(this);

    getParameters();

    // set up style & colour defaults
    m_colDefFill = App::Color(static_cast<uint32_t>(Preferences::getPreferenceGroup("Colors")->GetUnsigned("FaceColor", COLWHITE)))
                   .asValue<QColor>();
    m_colDefFill.setAlpha(Preferences::getPreferenceGroup("Colors")->GetBool("ClearFace", false) ? ALPHALOW : ALPHAHIGH);

    m_fillDef = Qt::SolidPattern;
    m_fillSelect = Qt::SolidPattern;

    setFillMode(NoFill);
    if (m_colDefFill.alpha() > 0) {
        setFillMode(PlainFill);
    }
    setFill(m_colDefFill, m_fillDef);

    m_sharedRender = new QSvgRenderer();
    m_patMaker = new PATPathMaker(this, 1.0, 1.0);
}

QGIFace::~QGIFace()
{
    delete m_sharedRender;
    delete m_patMaker;
}

/// redraw this face
void QGIFace::draw()
{
//    Base::Console().Message("QGIF::draw - pen style: %d\n", m_pen.style());
    setPath(m_outline);                         //Face boundary

    m_svgHatchArea->hide();
    m_imageSvgHatchArea->hide();

    if (isHatched()) {
        if (m_mode == GeomHatchFill) {
            //GeomHatch does not appear in pdf if clipping is set to true
            setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
            if (!m_lineSets.empty()) {
                m_brush.setTexture(QPixmap());
                m_fillStyleCurrent = m_fillDef;
                m_fillNormal = m_fillStyleCurrent;
                for (auto& ls: m_lineSets) {
                    lineSetToFillItems(ls);
                }
            }
        } else if (m_mode == SvgFill) {
            m_brush.setTexture(QPixmap());
            m_fillNormal = m_fillDef;
            m_fillStyleCurrent = m_fillNormal;
            setFlag(QGraphicsItem::ItemClipsChildrenToShape,true);
            loadSvgHatch(m_fileSpec);
            if (exporting()) {
                buildPixHatch();
                m_imageSvgHatchArea->show();
            } else {
                buildSvgHatch();
                m_svgHatchArea->show();
            }
        } else if (m_mode == BitmapFill) {
            m_fillStyleCurrent = Qt::TexturePattern;
            m_texture = textureFromBitmap(m_fileSpec);
            m_brush.setTexture(m_texture);
        } else if (m_mode == PlainFill) {
            setFill(m_colNormalFill, m_fillNormal);
        }
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
void QGIFace::setDrawEdges(bool state) {
//    Base::Console().Message("QGIF::setDrawEdges(%d)\n", b);
    if (state) {
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
    QFile file(qfs);
    if (!file.open(QFile::ReadOnly | QFile::Text))  {
        Base::Console().Error("QGIFace could not read %s\n", fileSpec.c_str());
        return;
    }
    m_svgXML = file.readAll();

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

void QGIFace::setFillMode(QGIFace::fillMode mode)
{
    m_mode = mode;
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
    m_patMaker->setLineWidth(Rez::guiX(m_geomWeight));
    m_patMaker->setScale(m_fillScale);
    m_patMaker->setPen(setGeomPen());
    m_patMaker->lineSetToFillItems(ls);
}

QPen QGIFace::setGeomPen()
{
    QPen result;
    result.setWidthF(Rez::guiX(m_geomWeight));
    result.setColor(m_geomColor);
    result.setStyle(Qt::SolidLine);
    return result;
}


//! get zoom level (scale) from QGraphicsView
// not used currently
double QGIFace::getXForm()
{
    //try to keep the pattern the same when View scales
    auto ourScene = scene();
    if (ourScene) {
        auto viewsAll = ourScene->views();     //ptrs to views
        if (!viewsAll.empty()) {
            auto view = viewsAll.at(0);
            auto iView = view->transform().inverted();
            return iView.m11();
        }
    }
    return 1.0;
}

/// remove the children that make up a PAT fill
void QGIFace::clearFillItems()
{
    for (auto& fill: m_fillItems) {
        fill->setParentItem(nullptr);
        this->scene()->removeItem(fill);
        delete fill;
    }
}

/// debugging tool draws a mark at a position on this face
void QGIFace::makeMark(double x, double y)  // NOLINT readability-identifier-length
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

    QByteArray before = QString::fromStdString(SVGCOLPREFIX + SVGCOLDEFAULT).toUtf8();
    QByteArray after = QString::fromStdString(SVGCOLPREFIX + m_svgCol).toUtf8();
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

//! similar to svg hatch, but using pixmaps. we do this because QGraphicsSvgItems are not clipped
//! when we export the scene to svg, but pixmaps are clipped.
void QGIFace::buildPixHatch()
{
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

    // handle color by brute force find & replace
    QByteArray before = QString::fromStdString(SVGCOLPREFIX + SVGCOLDEFAULT).toUtf8();
    QByteArray after = QString::fromStdString(SVGCOLPREFIX + m_svgCol).toUtf8();
    QByteArray colorXML = m_svgXML.replace(before,after);


    // TODO: there is a lot of switching back and forth between svg, QPixmap and QImage here that I
    //       don't really understand.
    // render svg tile onto a QImage
    if (!m_sharedRender->load(colorXML)) {
        Base::Console().Message("QGIF::buildSvgHatch - failed to load svg string\n");
        return;
    }

    QImage svgImage(SVGSIZEW, SVGSIZEH, QImage::Format_ARGB32);
    svgImage.fill(Qt::transparent);
    QPainter painter(&svgImage);
    if (svgImage.isNull()) {
        Base::Console().Error("QGIF::buildPixHatch - svgImage is null\n");
        return;
    }

    m_sharedRender->render(&painter);

    // convert the QImage into a QPixmap
    QPixmap tilePixmap(SVGSIZEW, SVGSIZEH);
    tilePixmap  = QPixmap::fromImage(svgImage);
    tilePixmap = tilePixmap.scaled(wTile, hTile);
    if (tilePixmap.isNull()) {
        Base::Console().Error("QGIF::buildPixHatch - tilePixmap is null\n");
        return;
    }

    // layout a field of bitmap tiles big enough to cover this face onto a Qimage
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
    for (int iw = 0; iw < int(tilesWide); iw++) {
        for (int ih = 0; ih < int(tilesHigh); ih++) {
            painter2.drawPixmap(QRectF(iw * wTile, ih * hTile, wTile, hTile),   //target rect
                               tilePixmap,
                               QRectF(0, 0, wTile, hTile));  //source rect
            tileCount++;
            if (tileCount > m_maxTile) {
                Base::Console().Warning("Pixmap tile count exceeded: %ld\n",tileCount);
                break;
            }
        }
        if (tileCount > m_maxTile) {
            break;
        }
    }

    // turn the QImage field into a pixmap
    QPixmap fieldPixmap(overlayWidth, overlayHeight);
    fieldPixmap = QPixmap::fromImage(tileField);

    // TODO: figure out how to rotate the pixmap without it looking terrible - far worse than the unrotated pixmap.  svg hatch exported to svg will not be shown rotated
    // QTransform xFormPixmap;
    // xFormPixmap.rotate(m_hatchRotation);
    // xFormPixmap.translate(getHatchOffset().x, getHatchOffset().y);
    // m_imageSvgHatchArea->load(fieldPixmap.transformed(xFormPixmap));

    QPixmap nothing;
    m_imageSvgHatchArea->setPixmap(nothing);
    m_imageSvgHatchArea->load(fieldPixmap);
    m_imageSvgHatchArea->centerAt(faceCenter);

}


void QGIFace::setHatchColor(App::Color color)
{
    m_svgCol = color.asHexString();
    m_geomColor = color.asValue<QColor>();
}

void QGIFace::setHatchScale(double scale)
{
    m_fillScale = scale;
}

/// turn svg tiles on or off. QtSvg does not handle clipping,
/// so we must be able to turn the hatching on/off when exporting a face with an
/// svg hatch.  Otherwise the full tile pattern is shown in the export.
/// NOTE: there appears to have been a change in Qt that it now clips svg items
void QGIFace::hideSvg(bool state)
{
    m_hideSvgTiles = state;
}


/// create a QPixmap from a bitmap file.  The QPixmap will be used as a QBrush
/// texture.
QPixmap QGIFace::textureFromBitmap(std::string fileSpec) const
{
    QPixmap pix;

    QString qfs(QString::fromUtf8(fileSpec.data(), fileSpec.size()));
    QFile file(qfs);
    if (!file.open(QFile::ReadOnly))  {
        Base::Console().Error("QGIFace could not read %s\n", fileSpec.c_str());
        return pix;
    }
    QByteArray bytes = file.readAll();
    pix.loadFromData(bytes);
    if (m_hatchRotation != 0.0) {
        QTransform rotator;
        rotator.rotate(m_hatchRotation);
        pix = pix.transformed(rotator);
    }
    return pix;
}

void QGIFace::setLineWeight(double weight) {
    m_geomWeight = weight;
}

void QGIFace::getParameters()
{
    m_maxSeg = Preferences::getPreferenceGroup("PAT")->GetInt("MaxSeg", MAXSEGMENT);
    m_maxTile = Preferences::getPreferenceGroup("Decorations")->GetInt("MaxSVGTile", MAXTILES);
}

QRectF QGIFace::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGIFace::shape() const
{
    return path();
}

bool QGIFace::exporting() const
{
    auto tdScene = dynamic_cast<QGSPage*>(scene());
    if (!tdScene) {
        return false;
    }
    return tdScene->getExportingSvg() ||
           tdScene->getExportingPdf();
}

