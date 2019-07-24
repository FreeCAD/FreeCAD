/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
#include <QFile>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawTile.h>
#include <Mod/TechDraw/App/DrawTileWeld.h>

#include <qmath.h>
#include "Rez.h"
#include "DrawGuiUtil.h"
#include "QGIView.h"
#include "QGITile.h"

using namespace TechDrawGui;

QGITile::QGITile(TechDraw::DrawTile* feat) :
    m_tileFeat(feat),
    m_textL(QString()),
    m_textR(QString()),
    m_textC(QString()),
    m_textSize(0.0),
    m_row(0),
    m_col(0),
    m_scale(1.0)
{
    m_qgSvg = new QGCustomSvg();
    m_qgSvg->setParentItem(this);
    m_effect = new QGraphicsColorizeEffect();
    m_qgTextL = new QGCustomText();
    m_qgTextL->setParentItem(this);
    m_qgTextR = new QGCustomText();
    m_qgTextR->setParentItem(this);
    m_qgTextC = new QGCustomText();
    m_qgTextC->setParentItem(this);

    m_wide = getSymbolWidth();
    m_high = getFontSize();
    m_textSize = getFontSize();
    m_textL  = QString();
    m_textR  = QString();
    m_textC  = QString();
    m_fontName = getTextFont();
    m_font = QFont(m_fontName);

//    setHandlesChildEvents(true);    //qt4
    setFiltersChildEvents(true);    //qt5
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    
    m_colNormal = prefNormalColor();
    m_colCurrent = m_colNormal;
}

QVariant QGITile::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGIT::itemChange(%d)\n", change);
    return QGIDecoration::itemChange(change, value);
}

void QGITile::draw(void)
{
//    Base::Console().Message("QGIT::draw()\n");
    prepareGeometryChange();
    m_wide = getSymbolWidth();
    m_high = getSymbolHeight() * scaleToFont();

    makeText();
    makeSymbol();

    double textWidthL = m_qgTextL->boundingRect().width();
    double textWidthR = m_qgTextR->boundingRect().width();
    double totalWidth = m_wide + textWidthL + textWidthR;

    double x = m_origin.x() + m_col * totalWidth;   //bit of a hack. sb 0.5 of prev tile + 0.5 of this tile
    double y = m_origin.y() - (m_row * m_high) - (m_high * 0.5);    //inverted y!!
    setPos(x,y);
}

void QGITile::makeSymbol(void)
{
//    Base::Console().Message("QGIT::makeSymbol()\n");
    m_effect->setColor(m_colCurrent);
    
    m_qgSvg->setGraphicsEffect(m_effect);
    QFile svgFile(m_svgPath);
    if(svgFile.open(QIODevice::ReadOnly)) {
        QByteArray qba = svgFile.readAll();
        if (!m_qgSvg->load(&qba)) {
            Base::Console().Error("Error - Could not load SVG renderer with %s\n", qPrintable(m_svgPath));
        }
        svgFile.close();
    } else {
        Base::Console().Error("Error - Could not open file %s\n", qPrintable(m_svgPath));  
    } 
    
    m_qgSvg->setScale(scaleToFont());
    m_qgSvg->centerAt(0.0, 0.0);   //(0,0) is based on symbol size
}

void QGITile::makeText(void)
{
//    Base::Console().Message("QGIT::makeText()\n");
    prepareGeometryChange();
    m_font.setPixelSize(getFontSize());
    double verticalFudge = 0.10;             //% of textHeight

    m_qgTextL->setFont(m_font);
    m_qgTextL->setPlainText(m_textL);
    m_qgTextL->setColor(m_colCurrent);
    double textWidth = m_qgTextL->boundingRect().width();
    double charWidth = textWidth / m_textL.size();   //not good for non-ASCII chars
    double hMargin = (m_wide / 2.0) + (charWidth / 2.0);

    double textHeightL = m_qgTextL->boundingRect().height();
    double offsetAdjustL = 0.0;
    if (m_row < 0) {
        offsetAdjustL = -textHeightL * verticalFudge;
    } else {
        offsetAdjustL = textHeightL * verticalFudge;
    }
    double offset = (textHeightL * verticalFudge * m_row) + offsetAdjustL;
    m_qgTextL->justifyRightAt(-hMargin, -offset, true);

    m_qgTextR->setFont(m_font);
    m_qgTextR->setPlainText(m_textR);
    m_qgTextR->setColor(m_colCurrent);
    textWidth = m_qgTextR->boundingRect().width();
    charWidth = textWidth / m_textR.size();
    double textHeightR = m_qgTextR->boundingRect().height();
    double offsetAdjustR = 0.0;
    if (m_row < 0) {
        offsetAdjustR = -textHeightR * verticalFudge;
    } else {
        offsetAdjustR = textHeightR * verticalFudge;
    }
    offset = (textHeightR * verticalFudge * m_row) + offsetAdjustR;
    m_qgTextR->justifyLeftAt(hMargin, -offset, true);

    m_qgTextC->setFont(m_font);
    m_qgTextC->setPlainText(m_textC);
    m_qgTextC->setColor(m_colCurrent);
    double textHeightC = m_qgTextC->boundingRect().height();
    textHeightC = textHeightC;
    int rowAdjustC = m_row;
    if (m_row >= 0) {
        rowAdjustC++;
    }
    double offsetAdjustC = textHeightC * verticalFudge;
    if (m_row < 0) {
        offsetAdjustC = - offsetAdjustC;
    }
    offset = (textHeightC * rowAdjustC) - offsetAdjustC;
    m_qgTextC->centerAt(0.0, -offset);
}

void QGITile::setTilePosition(QPointF org, int r, int c)
{
    m_origin = org;
    m_row = r;
    m_col = c;
}

void QGITile::setTileScale(double s)
{
    m_scale = s;
}

void QGITile::setTileTextLeft(std::string s)
{
    m_textL = QString::fromUtf8(s.c_str());
}

void QGITile::setTileTextRight(std::string s)
{
    m_textR = QString::fromUtf8(s.c_str());
}

void QGITile::setTileTextCenter(std::string s)
{
    m_textC = QString::fromUtf8(s.c_str());
}

//using label font and dimension font size.  could change later
//void QGITile::setFont(QFont f, double fsize)
//{
//    m_font = f;
//    m_textSize = fsize;
//}

void QGITile::setSymbolFile(std::string s)
{
//    Base::Console().Message("QGIT::setSymbolFile(%s)\n",s.c_str());
    if (!s.empty()) {
        m_svgPath = QString::fromUtf8(s.c_str());
    }
}

void QGITile::setPrettyNormal() {
    m_colCurrent = m_colNormal;

    m_effect->setColor(m_colNormal);
    m_qgTextL->setColor(m_colNormal);
    m_qgTextR->setColor(m_colNormal);
    m_qgTextC->setColor(m_colNormal);

    draw();
}

void QGITile::setPrettyPre() {
    m_colCurrent = prefPreColor();

    m_effect->setColor(m_colCurrent);
    m_qgTextL->setColor(m_colCurrent);
    m_qgTextR->setColor(m_colCurrent);
    m_qgTextC->setColor(m_colCurrent);

    draw();
}

void QGITile::setPrettySel() {
    m_colCurrent = prefSelectColor();

    m_effect->setColor(m_colCurrent);
    m_qgTextL->setColor(m_colCurrent);
    m_qgTextR->setColor(m_colCurrent);
    m_qgTextC->setColor(m_colCurrent);

    draw();
}


//TODO: this is Pen, not Brush. sb Brush to colour background
QColor QGITile::getTileColor(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("TileColor", 0x00000000));
    return fcColor.asValue<QColor>();
}

double QGITile::getSymbolWidth(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    double w = hGrp->GetFloat("SymbolSize",64);
//     symbols are only nominally 64x64. they actually have a "border" of 4 - 0.5*stroke(0.5)
//     so we'll say effectively 62x62? 60 x 60
//    double w = 64.0;
    double fudge = 4.0;              //allowance for tile border
    w = w - fudge;
    return w;
}

double QGITile::getSymbolHeight(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    double h = hGrp->GetFloat("SymbolSize",64);
    double fudge = 4.0;
    h = h - fudge;
//    double h = 60.0;
    return h;
}

double QGITile::getSymbolFactor(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    double s = hGrp->GetFloat("SymbolFactor",1.25);
//    double s = 1.25;
    return s;
}

double QGITile::getFontSize(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                       GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    double sizeMM = hGrp->GetFloat("FontSize", QGIView::DefaultFontSizeInMM);
    double fontSize = QGIView::calculateFontPixelSize(sizeMM);
    return fontSize;
}

//factor to scale symbol to match font size
double QGITile::scaleToFont(void) const
{
    double fpx = getFontSize();
    double spx = getSymbolHeight();
    double factor = getSymbolFactor();
    double sf = (fpx / spx) * factor;
    return sf;
}

QString QGITile::getTextFont(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Labels");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");
    return QString::fromStdString(fontName);
}

void QGITile::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->setPen(Qt::magenta);
//    painter->drawRect(boundingRect());          //good for debugging
    
    QGIDecoration::paint (painter, &myOption, widget);
}

QRectF QGITile::boundingRect() const
{
    return childrenBoundingRect();
}

