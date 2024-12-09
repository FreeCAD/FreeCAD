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
#include <qpainter.h>
#ifndef _PreComp_
# include <cmath>
# include <QGraphicsScene>
# include <QPainterPath>
#endif

#include <Base/Console.h>
#include <Base/Tools.h>

#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawTile.h>
#include <Mod/TechDraw/App/DrawTileWeld.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIWeldSymbol.h"
#include "PreferencesGui.h"
#include "QGCustomText.h"
#include "QGILeaderLine.h"
#include "QGIPrimPath.h"
#include "QGITile.h"
#include "QGIVertex.h"
#include "Rez.h"

#include "ViewProviderWeld.h"
#include "ZVALUE.h"


using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;


//**************************************************************
QGIWeldSymbol::QGIWeldSymbol() :
    m_arrowFeat(nullptr),
    m_otherFeat(nullptr),
    m_tailText(new QGCustomText()),
    m_fieldFlag(new QGIPrimPath()),
    m_allAround(new QGIVertex(-1)),
    m_blockDraw(false)
{
    setFiltersChildEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    setCacheMode(QGraphicsItem::NoCache);
    setZValue(ZVALUE::DIMENSION);

    m_tailText->setPlainText(
                QString::fromUtf8(" "));
    addToGroup(m_tailText);
    m_tailText->hide();
    m_tailText->setPos(0.0, 0.0);         //avoid bRect issues

    addToGroup(m_allAround);
    m_allAround->setPos(0.0, 0.0);
    m_allAround->setAcceptHoverEvents(false);
    m_allAround->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_allAround->setFlag(QGraphicsItem::ItemIsMovable, false);
    m_allAround->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    m_allAround->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    m_allAround->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

    addToGroup(m_fieldFlag);
    m_fieldFlag->setPos(0.0, 0.0);
    m_fieldFlag->setAcceptHoverEvents(false);
    m_fieldFlag->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_fieldFlag->setFlag(QGraphicsItem::ItemIsMovable, false);
    m_fieldFlag->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    m_fieldFlag->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    m_fieldFlag->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

    m_fieldFlag->setFill(prefNormalColor(), Qt::SolidPattern);
    setNormalColor(prefNormalColor());

    setCurrentColor(getNormalColor());
    setSettingColor(getNormalColor());

    setPrettyNormal();
}

QVariant QGIWeldSymbol::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            setPrettySel();
        } else {
            setPrettyNormal();
        }
    } else if(change == ItemSceneChange && scene()) {
        // nothing special!
    }
    return QGIView::itemChange(change, value);
}

void QGIWeldSymbol::updateView(bool update)
{
    Q_UNUSED(update);

    TechDraw::DrawWeldSymbol *feature = getFeature();
    if (!feature) {
        Base::Console().Warning("QGIWS::updateView - no feature!\n");
        return;
    }

    if (feature->isRestoring() || !getLeader()) {
        return;
    }

    draw();
}

void QGIWeldSymbol::draw()
{
    if (!isVisible()) {
        return;
    }
    getTileFeats();

    removeQGITiles();

    if (m_arrowFeat) {
        drawTile(m_arrowFeat);
    }

    if (m_otherFeat) {
        drawTile(m_otherFeat);
    }

    drawAllAround();

    drawFieldFlag();

    drawTailText();
}

void QGIWeldSymbol::drawTile(TechDraw::DrawTileWeld* tileFeat)
{
    if (!tileFeat) {
        Base::Console().Message("QGIWS::drawTile - tile is null\n");
        return;
    }

    const auto symbol = getFeature();
    if (!symbol) {
        return;
    }
    auto vp = dynamic_cast<ViewProviderWeld *>(getViewProvider(symbol));
    if (!vp) {
        return;
    }

    std::string fontName = vp->Font.getValue();
    int         fontSize = QGIView::exactFontSize(vp->Font.getValue(),
                                                  vp->TileFontSize.getValue());

    double featScale = getLeader()->getScale();

    std::string tileTextL = tileFeat->LeftText.getValue();
    std::string tileTextR = tileFeat->RightText.getValue();
    std::string tileTextC = tileFeat->CenterText.getValue();
    int row = (int)tileFeat->TileRow.getValue();
    int col = (int)tileFeat->TileColumn.getValue();

    auto tile = new QGITile(tileFeat);
    addToGroup(tile);

    QPointF org = getTileOrigin();
    tile->setTilePosition(org, row, col);
    tile->setFont(fontName, fontSize);
    tile->setColor(getCurrentColor());
    tile->setTileTextLeft(tileTextL);
    tile->setTileTextRight(tileTextR);
    tile->setTileTextCenter(tileTextC);
    tile->setZValue(ZVALUE::DIMENSION);
    tile->setTileScale(featScale);
    tile->setTailRight(getFeature()->isTailRightSide());
    tile->setAltWeld(getFeature()->AlternatingWeld.getValue());

    auto localAxes = getLocalAxes();
    tile->setLocalAxes(localAxes.first, localAxes.second);
    auto tilePos = tile->calcTilePosition();    // this is not the center! pos is left and up from center

    auto rotationDeg = getLastSegAngle();
    Base::Vector3d stdX{1, 0, 0};
    auto xdir = localAxes.first;
    auto dot = stdX.Dot(xdir);
    constexpr double DegreesHalfCircle{180.0};
    if (dot < 0) {
        rotationDeg -= DegreesHalfCircle;
    }
    tile->setRotation(rotationDeg);        // Qt angles are clockwise

    tile->draw();
    tile->setPos(tilePos);

}

void QGIWeldSymbol::drawAllAround()
{
    QPointF allAroundPos = getKinkPoint();
    m_allAround->setPos(allAroundPos);

    if (getFeature()->AllAround.getValue()) {
        m_allAround->show();
    } else {

        m_allAround->hide();
        return;
    }

    m_allAround->setNormalColor(getCurrentColor());

    m_allAround->setFill(Qt::NoBrush);
    m_allAround->setRadius(PreferencesGui::dimFontSizePX());

    auto qgiLead = dynamic_cast<QGILeaderLine *>(getQGIVByName(getLeader()->getNameInDocument()));
    if (qgiLead) {
         m_allAround->setWidth(qgiLead->getLineWidth());
    }
    m_allAround->setZValue(ZVALUE::DIMENSION);
}

void QGIWeldSymbol::drawTailText()
{
    constexpr double DegreesHalfCircle{180.0};
    constexpr double verticalBRectAdjustFactor{1.2};    // text bounding rect is bigger than font size and text is not
                                                        // vertically aligned with brect midline, so we artificially
                                                        // bump the text size so the middle aligns with the leader

    QPointF textPos = getTailPoint();
    m_tailText->setPos(textPos);  //avoid messing up brect with empty item at 0, 0 !!!
    std::string tText = getFeature()->TailText.getValue();
    if (tText.empty()) {
        m_tailText->hide();
        return;
    }

    m_tailText->show();

    const auto symbol = getFeature();
    if (!symbol) {
        return;
    }
    auto vp = dynamic_cast<ViewProviderWeld *>(getViewProvider(symbol));
    if (!vp) {
        return;
    }
    QString qFontName = QString::fromStdString(vp->Font.getValue());
    int fontSize = QGIView::exactFontSize(vp->Font.getValue(),
                                          vp->FontSize.getValue());     // this is different from the size used in the tiles

    m_font.setFamily(qFontName);
    m_font.setPixelSize(fontSize);

    m_tailText->setFont(m_font);
    m_tailText->setPlainText(
                QString::fromUtf8(tText.c_str()));
    m_tailText->setColor(getCurrentColor());
    m_tailText->setZValue(ZVALUE::DIMENSION);

    auto brWidth = m_tailText->boundingRect().width();
    auto brHeight = m_tailText->boundingRect().height();
    auto brCharWidth = brWidth / tText.length();

    double hMargin = brCharWidth + prefArrowSize();  // leave a gap from the graphic
    auto angleDeg = getLastSegAngle();

    auto localAxes = getLocalAxes();
    auto xdir = localAxes.first;
    auto ydir = localAxes.second;
    Base::Vector3d stdX{1, 0, 0};
    auto xdirStartOffset = DU::toQPointF(xdir * brWidth) * -1;  // start of text in xDir when leader points left
    auto dot = stdX.Dot(xdir);
    if (dot < 0) {
        angleDeg -= DegreesHalfCircle;
        xdirStartOffset = QPointF(0, 0);
        ydir = -ydir;
    }

    auto xAdjust = DU::toQPointF(xdir * hMargin);
    auto yAdjust = DU::toQPointF(ydir * (brHeight * verticalBRectAdjustFactor) / 2);

    // QGCustomText doesn't know about rotation so we can't use the justify methods
    QPointF justPoint = textPos - xAdjust + yAdjust;            // original
    justPoint += xdirStartOffset;

    m_tailText->setPos(justPoint);

    auto kink = getLeader()->getKinkPoint();
    m_tailText->setTransformOriginPoint(DU::toQPointF(kink));

    m_tailText->setRotation(angleDeg);
}

void QGIWeldSymbol::drawFieldFlag()
{
    QPointF fieldFlagPos = getKinkPoint();
    m_fieldFlag->setPos(fieldFlagPos);

    if (getFeature()->FieldWeld.getValue()) {
        m_fieldFlag->show();
    } else {
        m_fieldFlag->hide();
        return;
    }

    auto localAxes = getLocalAxes();
    auto xdir = localAxes.first;
    auto ydir = localAxes.second;

    // TODO: similar code used here and in QGITiled to handle leader points left.  different actions, though.
    // used here and QGITile.
    Base::Vector3d stdX{1, 0, 0};
    auto dot = stdX.Dot(xdir);
    if (dot < 0) {
        // our leader points left, so yDir should be +Y, but we need -Y up
        ydir = -ydir;
    }

// NOLINTBEGIN readability-magic-numbers
    std::vector<QPointF> flagPoints = { QPointF(0.0, 0.0),
                                        DU::toQPointF(ydir * 3.0),
                                        DU::toQPointF(xdir * -2.0 + ydir * 2.5),
                                        DU::toQPointF(ydir * 2.0) };
// NOLINTEND readability-magic-numbers

    double scale = (float)PreferencesGui::dimFontSizePX() / 2;
    QPainterPath path;
    path.moveTo(flagPoints.at(0) * scale);
    size_t iSeg = 1;
    for ( ; iSeg < flagPoints.size(); iSeg++) {
        path.lineTo(flagPoints.at(iSeg) * scale);
    }

    auto qgiLead = dynamic_cast<QGILeaderLine *>(getQGIVByName(getLeader()->getNameInDocument()));
    if (qgiLead) {
        m_fieldFlag->setWidth(qgiLead->getLineWidth());
    }
    m_fieldFlag->setZValue(ZVALUE::DIMENSION);

    m_fieldFlag->setPath(path);
}

void QGIWeldSymbol::getTileFeats()
{
    std::vector<TechDraw::DrawTileWeld*> tiles = getFeature()->getTiles();
    m_arrowFeat = nullptr;
    m_otherFeat = nullptr;

    if (!tiles.empty()) {
        TechDraw::DrawTileWeld* tempTile = tiles.at(0);
        if (tempTile->TileRow.getValue() == 0) {
            m_arrowFeat = tempTile;
        } else {
            m_otherFeat = tempTile;
        }
    }
    if (tiles.size() > 1) {
        TechDraw::DrawTileWeld* tempTile = tiles.at(1);
        if (tempTile->TileRow.getValue() == 0) {
            m_arrowFeat = tempTile;
        } else {
            m_otherFeat = tempTile;
        }
    }
}

void QGIWeldSymbol::removeQGITiles()
{
    std::vector<QGITile*> tilesAll = getQGITiles();
    for (auto tile: tilesAll) {
            QList<QGraphicsItem*> tChildren = tile->childItems();
            for (auto tc: tChildren) {
                tile->removeFromGroup(tc);
                scene()->removeItem(tc);
                //tc gets deleted when QGIWS gets deleted
            }
        removeFromGroup(tile);
        scene()->removeItem(tile);
        delete tile;
    }
}

std::vector<QGITile*> QGIWeldSymbol::getQGITiles() const
{
    std::vector<QGITile*> result;
    QList<QGraphicsItem*> children = childItems();
    for (auto& child:children) {
         auto tile = dynamic_cast<QGITile*>(child);
         if (tile) {
            result.push_back(tile);
         }
     }
     return result;
}

void QGIWeldSymbol::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    if (isSelected()) {
        setCurrentColor(getSelectColor());
        setPrettySel();
    } else {
        setCurrentColor(getPreColor());
        setPrettyPre();
    }
    QGIView::hoverEnterEvent(event);
}

void QGIWeldSymbol::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    if(isSelected()) {
        setCurrentColor(getSelectColor());
        setPrettySel();
    } else {
        setCurrentColor(getNormalColor());
        setPrettyNormal();
    }
    QGIView::hoverLeaveEvent(event);
}

void QGIWeldSymbol::drawBorder()
{
////Weld Symbols have no border!
//    QGIView::drawBorder();   //good for debugging
}

void QGIWeldSymbol::setPrettyNormal()
{
    std::vector<QGITile*> tilesAll = getQGITiles();
    for (auto tile: tilesAll) {
        tile->setColor(getNormalColor());
        tile->draw();
    }
    setCurrentColor(getNormalColor());
    m_fieldFlag->setNormalColor(getNormalColor());
    m_fieldFlag->setPrettyNormal();
    m_allAround->setNormalColor(getNormalColor());
    m_allAround->setPrettyNormal();
    m_tailText->setColor(getNormalColor());
    m_tailText->setPrettyNormal();
}

void QGIWeldSymbol::setPrettyPre()
{
    std::vector<QGITile*> tilesAll = getQGITiles();
    for (auto tile: tilesAll) {
        tile->setColor(getPreColor());
        tile->draw();
    }

    setCurrentColor(getPreColor());
    m_fieldFlag->setNormalColor(getPreColor());
    m_fieldFlag->setPrettyPre();
    m_allAround->setNormalColor(getPreColor());
    m_allAround->setPrettyPre();
    m_tailText->setColor(getPreColor());
    m_tailText->setPrettyPre();
}

void QGIWeldSymbol::setPrettySel()
{
    std::vector<QGITile*> tilesAll = getQGITiles();
    for (auto tile: tilesAll) {
        tile->setColor(getSelectColor());
        tile->draw();
    }

    setCurrentColor(getSelectColor());
    m_fieldFlag->setNormalColor(getSelectColor());
    m_fieldFlag->setPrettySel();
    m_allAround->setNormalColor(getSelectColor());
    m_allAround->setPrettySel();
    m_tailText->setColor(getSelectColor());
    m_tailText->setPrettySel();
}

QPointF QGIWeldSymbol::getTileOrigin()
{
    Base::Vector3d org = getLeader()->getTileOrigin();
    QPointF result = DU::toQPointF(Rez::guiX(org));
    return result;
}

QPointF QGIWeldSymbol::getKinkPoint()
{
    Base::Vector3d org = getLeader()->getKinkPoint();
    QPointF result = DU::toQPointF(Rez::guiX(org));
    return result;
}

QPointF QGIWeldSymbol::getTailPoint()
{
    Base::Vector3d org = getLeader()->getTailPoint();
    QPointF result = DU::toQPointF(Rez::guiX(org));
    return result;
}

TechDraw::DrawWeldSymbol* QGIWeldSymbol::getFeature()
{
    return dynamic_cast<TechDraw::DrawWeldSymbol *>(getViewObject());
}

TechDraw::DrawLeaderLine *QGIWeldSymbol::getLeader()
{
    DrawWeldSymbol *feature = getFeature();
    if (!feature) {
        return nullptr;
    }

    return dynamic_cast<TechDraw::DrawLeaderLine *>(feature->Leader.getValue());
}

//! get the angle for the transformed last segment
double QGIWeldSymbol::getLastSegAngle()
{
    auto lastSegDirection = getLeader()->lastSegmentDirection();
    auto lastSegAngleRad = DU::angleWithX(lastSegDirection);
    return Base::toDegrees(lastSegAngleRad);
}

std::pair<Base::Vector3d, Base::Vector3d> QGIWeldSymbol::getLocalAxes()
{
    auto localX = getLeader()->lastSegmentDirection();
    auto localY = DU::invertY(localX);
    localY.RotateZ(M_PI_2);
    localY.Normalize();
    localY = DU::invertY(localY);
    return {localX, localY};
}


//preference
QColor QGIWeldSymbol::prefNormalColor()
{
    setNormalColor(PreferencesGui::leaderQColor());
    return getNormalColor();
}

double QGIWeldSymbol::prefArrowSize() const
{
    return PreferencesGui::dimArrowSize();
}

double QGIWeldSymbol::prefFontSize() const
{
    return Preferences::labelFontSizeMM();
}

QRectF QGIWeldSymbol::boundingRect() const
{
    return customBoundingRect();
}

QRectF QGIWeldSymbol::customBoundingRect() const
{
    QRectF result;

    if (m_tailText) {
        QRectF childRect = mapFromItem(m_tailText, m_tailText->boundingRect()).boundingRect();
        result = result.united(childRect);
    }

    if (m_fieldFlag) {
        QRectF childRect = mapFromItem(m_fieldFlag, m_fieldFlag->boundingRect()).boundingRect();
        result = result.united(childRect);
    }

    if (m_allAround) {
        QRectF childRect = mapFromItem(m_allAround, m_allAround->boundingRect()).boundingRect();
        result = result.united(childRect);
    }

    std::vector<QGITile*> qgTiles = getQGITiles();
    for (auto& tile: qgTiles) {
        QRectF childRect = mapFromItem(tile, tile->boundingRect()).boundingRect();
        result = result.united(childRect);
    }
    return result;
}


QPainterPath QGIWeldSymbol::shape() const
{
    return QGraphicsItemGroup::shape();
}

void QGIWeldSymbol::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

   // painter->setPen(Qt::red);
   // painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}


#include <Mod/TechDraw/Gui/moc_QGIWeldSymbol.cpp>
