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

#include <cmath>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawUtil.h>

#include "Rez.h"
#include "QGCustomSvg.h"
#include "QGCustomRect.h"
#include "QGIFace.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIFace::QGIFace(int index) :
    projIndex(index),
    m_colDefFill(Qt::white),          //Qt::transparent?  paper colour?
    m_styleDef(Qt::SolidPattern),
    m_styleSelect(Qt::SolidPattern)
{
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
}

QGIFace::~QGIFace()
{
    //nothing to do. every item is a child of QGIFace & will get removed/deleted when QGIF is deleted
}

void QGIFace::draw() 
{
    setPath(m_outline);                         //Face boundary

    if (isHatched()) {   
        if (m_mode == GeomHatchFill) {                             //crosshatch
            if (!m_geomHatchPaths.empty()) {                                  //surrogate for LineSets.empty
                m_brush.setTexture(QPixmap());
                m_fillStyle = m_styleDef;
                m_styleNormal = m_fillStyle;
                int pathNo = 0;
                for (auto& pp: m_geomHatchPaths) {
                    QGraphicsPathItem* fillItem = m_fillItems.at(pathNo);  
                    fillItem->setPath(pp);                  
                    QPen geomPen = setGeomPen(pathNo);
                    fillItem->setPen(geomPen);
                    pathNo++;
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
    m_geomHatchPaths.clear();
    m_dashSpecs.clear();
    clearFillItems();
}

//each line set needs a painterpath, a dashspec and a QGPItem to show them
void QGIFace::addLineSet(QPainterPath pp, std::vector<double> dp)
{
    m_geomHatchPaths.push_back(pp);
    m_dashSpecs.push_back(DashSpec(dp));
    addFillItem();
}   

QGraphicsPathItem*  QGIFace::addFillItem()
{
    QGraphicsPathItem* fillItem = new QGraphicsPathItem();
    fillItem->setParentItem(this);
    m_fillItems.push_back(fillItem);
    return fillItem;
}
  
void QGIFace::clearFillItems(void)
{
    for (auto& f: m_fillItems) {
        f->setParentItem(nullptr);
        this->scene()->removeItem(f);
        delete f;
    }
}

//convert from PAT style "-1,0,-1,+1" to Qt style "mark,space,mark,space"
QVector<qreal> QGIFace::decodeDashSpec(DashSpec patDash)
{
    //Rez::guiX(something)?
    double dotLength = 3.0;
    double unitLength = 6.0;
//    double penWidth = m_geomWeight;    //mark, space and dot lengths are to be in terms of penWidth(Qt) or mm(PAT)??
//                                        //if we want it in terms of mm, we need to divide by penWidth?
//    double minPen = 0.01;               //avoid trouble with cosmetic pen (zero width)
    std::vector<double> result;
    std::string prim;
    for (auto& d: patDash.get()) {
        double strokeLength;
        if (DrawUtil::fpCompare(d,0.0)) {                       //pat dot
             strokeLength = dotLength;
        } else if (Rez::guiX(d) < 0) {                          //pat space
             strokeLength = fabs(Rez::guiX(d)) * unitLength;
        } else {                                                //pat dash
             strokeLength = Rez::guiX(d) * unitLength;
        }
        result.push_back(strokeLength);
    }
    return QVector<qreal>::fromStdVector( result ); 
}


QPen QGIFace::setGeomPen(int i)
{
    //m_dashSpecs[i].dump("spec test");
    DashSpec ourSpec = m_dashSpecs.at(i);
    //ourSpec.dump("our spec");
    
    QPen result;
    result.setWidthF(Rez::guiX(m_geomWeight));    //Rez::guiX() ?? line weights are in mm?
    result.setColor(m_geomColor);
    if (ourSpec.empty()) {
       result.setStyle(Qt::SolidLine);
    } else {
       result.setStyle(Qt::CustomDashLine);
       result.setDashPattern(decodeDashSpec(ourSpec));
    }
    return result;
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
    for (int iw = 0; iw < int(nw); iw++) {
        for (int ih = 0; ih < int(nh); ih++) {
            QGCustomSvg* tile = new QGCustomSvg();
            tile->setScale(m_fillScale);
            if (tile->load(&colorXML)) {
                tile->setParentItem(m_rect);
                tile->setPos(iw*wTile,-h + ih*hTile);
            }
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

