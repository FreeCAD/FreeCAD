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
# include <QGraphicsItem>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/Stream.h>
#include <Base/Tools.h>

#include <Mod/TechDraw/App/DrawTileWeld.h>

#include "QGITile.h"
#include "PreferencesGui.h"
#include "QGCustomSvg.h"
#include "QGCustomText.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGITile::QGITile(TechDraw::DrawTileWeld* dtw) :
    m_textL(QString::fromUtf8(" ")),
    m_textR(QString::fromUtf8(" ")),
    m_textC(QString::fromUtf8(" ")),
    m_scale(1.0),
    m_row(0),
    m_col(0),
    m_tailRight(true),
    m_altWeld(false),
    m_tileFeat(dtw)
{
    m_qgSvg = new QGCustomSvg();
    addToGroup(m_qgSvg);

//    m_effect = new QGraphicsColorizeEffect();

    m_qgTextL = new QGCustomText();
    addToGroup(m_qgTextL);

    m_qgTextR = new QGCustomText();
    addToGroup(m_qgTextR);

    m_qgTextC = new QGCustomText();
    addToGroup(m_qgTextC);

    m_wide = getSymbolWidth();
    m_high = getSymbolHeight();
//    m_high = prefFontSize();
    m_textL  = QString();
    m_textR  = QString();
    m_textC  = QString();
    m_fontName = prefTextFont();
    m_font = QFont(m_fontName);

    setFiltersChildEvents(true);    //qt5
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemStacksBehindParent, true);

    m_colNormal = prefNormalColor();
    m_colCurrent = m_colNormal;
}

QGITile::~QGITile()
{

}

void QGITile::draw()
{
//    Base::Console().Message("QGIT::draw()\n");

    prepareGeometryChange();
    m_wide = getSymbolWidth();
    m_high = getSymbolHeight();

    makeText();
    makeSymbol();

    double textWidthL = m_qgTextL->boundingRect().width();
    double textWidthR = m_qgTextR->boundingRect().width();
    double totalWidth = m_wide + textWidthL + textWidthR;
    if (m_row == 0) {     //arrowSide
        double x = m_origin.x();
        double y = m_origin.y() - (m_high * 0.5);    //inverted y!!
        setPos(x, y);
    } else if (m_row == -1) {    //otherSide
        if (getAltWeld()) {
            if (isTailRight()) {
                double x = m_origin.x() + (0.5 * totalWidth); //move to right 1/2 tile width
                double y = m_origin.y() +  (m_high * 0.5);    //inverted y!!
                setPos(x, y);
            } else {
                double x = m_origin.x() - (0.5 * totalWidth); //move to left 1/2 tile width
                double y = m_origin.y() +  (m_high * 0.5);    //inverted y!!
                setPos(x, y);
            }
        } else {
            double x = m_origin.x();
            double y = m_origin.y() + (m_high * 0.5);    //inverted y!!
            setPos(x, y);
        }
    } else {
        double x = m_origin.x() + m_col * totalWidth;
        double y = m_origin.y() - (m_row * m_high) - (m_high * 0.5);    //inverted y!!
        setPos(x, y);
    }
}

void QGITile::makeSymbol()
{
//    Base::Console().Message("QGIT::makeSymbol()\n");
//    m_effect->setColor(m_colCurrent);
//    m_qgSvg->setGraphicsEffect(m_effect);

    std::string symbolString = getStringFromFile(m_tileFeat->SymbolFile.getValue());
    QByteArray qba(symbolString.c_str(), symbolString.length());
    if (qba.isEmpty()) {
        return;
    }
    if (!m_qgSvg->load(&qba)) {
        Base::Console().Error("Error - Could not load SVG renderer with **%s**\n", qPrintable(m_svgPath));
        return;
   }
   m_qgSvg->setScale(getSymbolFactor());
   m_qgSvg->centerAt(0.0, 0.0);   //(0, 0) is based on symbol size
}

void QGITile::makeText()
{
//    Base::Console().Message("QGIT::makeText()\n");
    prepareGeometryChange();
//    m_font.setPixelSize(prefFontSize());
    double verticalFudge = 0.10;

    //(0, 0) is 1/2 up symbol (above line symbol)!
    m_qgTextL->setFont(m_font);
    m_qgTextL->setPlainText(m_textL);
    m_qgTextL->setColor(m_colCurrent);
    double textWidth = m_qgTextL->boundingRect().width();
    double charWidth = textWidth / m_textL.size();   //not good for non-ASCII chars
    double hMargin = 1;
    if (!m_textL.isEmpty()) {
        hMargin = (m_wide / 2.0) + (charWidth * 1.5);
    }

    double vertAdjust = 0.0;
    double minVertAdjust = PreferencesGui::labelFontSizePX() * 0.1;
    if (m_font.pixelSize() > m_high) {
        vertAdjust = ((m_font.pixelSize() - m_high) / 2.0) + minVertAdjust;
    }

    double textHeightL = m_qgTextL->boundingRect().height();
    double vOffset = 0.0;
    if (m_row < 0) {                      // below line
        vOffset = textHeightL * verticalFudge;
    } else {                              // above line
        vOffset = -vertAdjust;
    }
    m_qgTextL->justifyRightAt(-hMargin, vOffset, true);   //sb vCentered at 0.5 m_high

    m_qgTextR->setFont(m_font);
    m_qgTextR->setPlainText(m_textR);
    m_qgTextR->setColor(m_colCurrent);
    textWidth = m_qgTextR->boundingRect().width();
    charWidth = textWidth / m_textR.size();
    hMargin = 1;
    if (!m_textR.isEmpty()) {
        hMargin = (m_wide / 2.0) + (charWidth);
    }
    double textHeightR = m_qgTextR->boundingRect().height();
    if (m_row < 0) {                      // below line
        vOffset =  textHeightR * verticalFudge;
    } else {
        vOffset = -vertAdjust;
    }
    m_qgTextR->justifyLeftAt(hMargin, vOffset, true);

    m_qgTextC->setFont(m_font);
    m_qgTextC->setPlainText(m_textC);
    m_qgTextC->setColor(m_colCurrent);
    double textHeightC = m_qgTextC->boundingRect().height();
    if (m_row < 0) {                      // below line
        vOffset = m_high  * (1 + verticalFudge);
    } else {
        vOffset = -0.5 * (m_high + textHeightC);
    }
    m_qgTextC->centerAt(0.0, vOffset);
}

//read whole text file into std::string
std::string QGITile::getStringFromFile(std::string inSpec)
{
    Base::FileInfo fi(inSpec);
    Base::ifstream f(fi);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
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

void QGITile::setFont(QFont f, double fSizePx)
{
//    Base::Console().Message("QGIT::setFont(%s, %.3f)\n", qPrintable(f.family()), fSizePx);
    m_font = f;
    m_font.setPixelSize(fSizePx);
}

void QGITile::setFont(std::string fName, double fSizePx)
{
    QString qFName = Base::Tools::fromStdString(fName);
    QFont f(qFName);
    setFont(f, fSizePx);
}


void QGITile::setSymbolFile(std::string s)
{
//    Base::Console().Message("QGIT::setSymbolFile(%s)\n", s.c_str());
    if (!s.empty()) {
        m_svgPath = QString::fromUtf8(s.c_str());
    }
}

void QGITile::setPrettyNormal() {
    m_colCurrent = m_colNormal;

//    m_effect->setColor(m_colNormal);
    m_qgTextL->setColor(m_colNormal);
    m_qgTextR->setColor(m_colNormal);
    m_qgTextC->setColor(m_colNormal);

    draw();
}

void QGITile::setPrettyPre() {
    m_colCurrent = prefPreColor();

//    m_effect->setColor(m_colCurrent);
    m_qgTextL->setColor(m_colCurrent);
    m_qgTextR->setColor(m_colCurrent);
    m_qgTextC->setColor(m_colCurrent);

    draw();
}

void QGITile::setPrettySel() {
    m_colCurrent = prefSelectColor();

//    m_effect->setColor(m_colCurrent);
    m_qgTextL->setColor(m_colCurrent);
    m_qgTextR->setColor(m_colCurrent);
    m_qgTextC->setColor(m_colCurrent);

    draw();
}

bool QGITile::isTailRight()
{
    return m_tailRight;
}

bool QGITile::getAltWeld()
{
    return m_altWeld;
}

//TODO: this is Pen, not Brush. sb Brush to colour background
QColor QGITile::getTileColor() const
{
    App::Color fcColor = App::Color((uint32_t) Preferences::getPreferenceGroup("Colors")->GetUnsigned("TileColor", 0x00000000));
    return PreferencesGui::getAccessibleQColor( fcColor.asValue<QColor>());
}

double QGITile::getSymbolWidth() const
{
    double w = Preferences::getPreferenceGroup("Dimensions")->GetFloat("SymbolSize", 64);
//     symbols are only nominally 64x64. they actually have a "border" of 4 - 0.5*stroke(0.5)
//     so we'll say effectively 62x62? 60 x 60
//    double w = 64.0;
    double fudge = 4.0;              //allowance for tile border
    w = w - fudge;
    w = w * getSymbolFactor();
    return w;
}

double QGITile::getSymbolHeight() const
{
    double h = Preferences::getPreferenceGroup("Dimensions")->GetFloat("SymbolSize", 64);
    double fudge = 4.0;
    h = h - fudge;
//    double h = 60.0;
    h = h * getSymbolFactor();
    return h;
}

//make symbols larger or smaller than standard
double QGITile::getSymbolFactor() const
{
    return Preferences::getPreferenceGroup("Decorations")->GetFloat("SymbolFactor", 1.25);
}

double QGITile::prefFontSize() const
{
//    Base::Reference<ParameterGrp> hGrp = Preferences::getPreferenceGroup("Dimensions");
    return Preferences::dimFontSizeMM();
}

QString QGITile::prefTextFont() const
{
    return Preferences::labelFontQString();
}

QRectF QGITile::boundingRect() const
{
    return childrenBoundingRect();
}

