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

#include <Mod/TechDraw/App/DrawTileWeld.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGITile.h"
#include "PreferencesGui.h"
#include "QGCustomSvg.h"
#include "QGCustomText.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;

QGITile::QGITile(TechDraw::DrawTileWeld* dtw) :
    m_textL(QStringLiteral(" ")),
    m_textR(QStringLiteral(" ")),
    m_textC(QStringLiteral(" ")),
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
}


void QGITile::draw()
{
    prepareGeometryChange();
    m_wide = getSymbolWidth();
    m_high = getSymbolHeight();

    makeText();
    makeSymbol();
}

void QGITile::makeSymbol()
{
//    m_qgSvg->setGraphicsEffect(m_effect);

    std::string symbolString = getStringFromFile(m_tileFeat->SymbolFile.getValue());
    QByteArray qba(symbolString.c_str(), symbolString.length());
    if (qba.isEmpty()) {
        return;
    }
    if (!m_qgSvg->load(&qba)) {
        Base::Console().error("Error - Could not load SVG renderer with **%s**\n", qPrintable(m_svgPath));
        return;
   }
   m_qgSvg->setScale(getSymbolFactor());
   m_qgSvg->centerAt(0.0, 0.0);   //(0, 0) is based on symbol size
}

void QGITile::makeText()
{
    prepareGeometryChange();
    double verticalFudge = 0.10;

    m_qgTextL->setFont(m_font);
    m_qgTextL->setPlainText(m_textL);
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
    textWidth = m_qgTextR->boundingRect().width();
    charWidth = textWidth / m_textR.size();
    hMargin = 1;
    if (!m_textR.isEmpty()) {
        hMargin = (m_wide / 2) + (charWidth);
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
    double textHeightC = m_qgTextC->boundingRect().height();
    if (m_row < 0) {                      // below line
        vOffset = m_high  * (1 + verticalFudge);
    } else {
        vOffset = -0.5 * (m_high + textHeightC);
    }
    m_qgTextC->centerAt(0.0, vOffset);
}

//read whole text file into std::string
std::string QGITile::getStringFromFile(const std::string& inSpec)
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

void QGITile::setTileTextLeft(const std::string& text)
{
    m_textL = QString::fromUtf8(text.c_str());
}

void QGITile::setTileTextRight(const std::string& text)
{
    m_textR = QString::fromUtf8(text.c_str());
}

void QGITile::setTileTextCenter(const std::string& text)
{
    m_textC = QString::fromUtf8(text.c_str());
}

void QGITile::setFont(const QFont& font, double fSizePx)
{
    m_font = font;
    m_font.setPixelSize(fSizePx);
}


void QGITile::setFont(const std::string &fName, double fSizePx)
{
    QString qFName = QString::fromStdString(fName);
    QFont font(qFName);
    setFont(font, fSizePx);
}


void QGITile::setSymbolFile(const std::string &fileSpec)
{
    if (!fileSpec.empty()) {
        m_svgPath = QString::fromUtf8(fileSpec.c_str());
    }
}

void QGITile::setPrettyNormal() {
//    m_effect->setColor(m_colNormal);
    m_qgTextL->setColor(m_colNormal);
    m_qgTextR->setColor(m_colNormal);
    m_qgTextC->setColor(m_colNormal);

    draw();
}

void QGITile::setPrettyPre() {
    QColor color = prefPreColor();

//    m_effect->setColor(color);
    m_qgTextL->setColor(color);
    m_qgTextR->setColor(color);
    m_qgTextC->setColor(color);

    draw();
}

void QGITile::setPrettySel() {
    QColor color = prefSelectColor();

//    m_effect->setColor(color);
    m_qgTextL->setColor(color);
    m_qgTextR->setColor(color);
    m_qgTextC->setColor(color);

    draw();
}

bool QGITile::isTailRight() const
{
    return m_tailRight;
}

bool QGITile::getAltWeld() const
{
    return m_altWeld;
}

//TODO: this is Pen, not Brush. sb Brush to colour background
QColor QGITile::getTileColor() const
{
    Base::Color fcColor = Base::Color((uint32_t) Preferences::getPreferenceGroup("Colors")->GetUnsigned("TileColor", 0x00000000));
    return PreferencesGui::getAccessibleQColor( fcColor.asValue<QColor>());
}

double QGITile::getSymbolWidth() const
{
    double symbolWidth = Preferences::getPreferenceGroup("Dimensions")->GetFloat("SymbolSize", 64);
//     symbols are only nominally 64x64. they actually have a "border" of 4 - 0.5*stroke(0.5)
//     so we'll say effectively 62x62? 60 x 60
    constexpr double borderAllow = 4.0;              //allowance for tile border
    symbolWidth = symbolWidth - borderAllow;
    symbolWidth = symbolWidth * getSymbolFactor();
    return symbolWidth;
}

double QGITile::getSymbolHeight() const
{
    double height = Preferences::getPreferenceGroup("Dimensions")->GetFloat("SymbolSize", 64);
    double borderAllow = 4.0;
    height = height - borderAllow;
    height = height * getSymbolFactor();
    return height;
}

//make symbols larger or smaller than standard
double QGITile::getSymbolFactor() const
{
    return Preferences::getPreferenceGroup("Decorations")->GetFloat("SymbolFactor", 1.25);
}

double QGITile::prefFontSize() const
{
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


//! determine where to position the tile based on which fields are used.
QPointF QGITile::calcTilePosition()
{
    constexpr double OneHalf{0.5};
    auto xDir = m_leaderXDirection;     // "right"
    auto yDir = m_leaderYDirection;     // "up"
    Base::Vector3d stdX{1, 0, 0};
    auto dot = stdX.Dot(xDir);
    if (dot < 0) {
        // our leader points left, so yDir should be +Y, but we need -Y up
        yDir = -yDir;
    }

    double textWidthL = m_qgTextL->boundingRect().width();
    double textWidthR = m_qgTextR->boundingRect().width();
    double totalWidth = m_wide + textWidthL + textWidthR;

    if (m_row == 0) {     //arrowSide
        double offsetDistanceY = getSymbolHeight() * OneHalf;
        auto vOffset = yDir * offsetDistanceY;
        auto netPosition = m_origin + DU::toQPointF(vOffset);
        return netPosition;
    }

    if (m_row == -1) {    //otherSide
        if (getAltWeld()) {
            if (isTailRight()) {
                auto hOffset = DU::toQPointF(xDir * (OneHalf * totalWidth));          //move to right 1/2 tile width
                auto vOffset = DU::toQPointF(yDir * (getSymbolHeight() * OneHalf));   // move down 1/2 tile height
                auto netPosition = m_origin + hOffset - vOffset;
                return netPosition;
            }
            auto hOffset = DU::toQPointF(xDir * (OneHalf * totalWidth));
            auto vOffset = DU::toQPointF(yDir * (OneHalf * getSymbolHeight()));
            auto netPosition = m_origin - hOffset - vOffset;    // left and down
            return netPosition;
        }

        auto vOffset = DU::toQPointF(yDir * (OneHalf * getSymbolHeight()));     // down 1/2 tile height
        auto netPosition = m_origin - vOffset;
        return netPosition;
    }

    auto hOffset = DU::toQPointF(xDir * (m_col * totalWidth));
    auto vOffset = DU::toQPointF(yDir * ((m_row * m_high) + (m_high * OneHalf)));
    auto netPosition = m_origin + hOffset - vOffset;

    return netPosition;
}

void QGITile::setLocalAxes(Base::Vector3d xdir, Base::Vector3d ydir)
{
    m_leaderXDirection = xdir;
    m_leaderYDirection = ydir;
}


