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

#include "QGIWeldSymbol.h"
#include "PreferencesGui.h"
#include "QGCustomText.h"
#include "QGILeaderLine.h"
#include "QGIPrimPath.h"
#include "QGITile.h"
#include "QGIVertex.h"

#include "ViewProviderWeld.h"
#include "ZVALUE.h"


using namespace TechDraw;
using namespace TechDrawGui;


//**************************************************************
QGIWeldSymbol::QGIWeldSymbol(QGILeaderLine* myParent) :
    m_weldFeat(nullptr),
    m_leadFeat(nullptr),
    m_arrowFeat(nullptr),
    m_otherFeat(nullptr),
    m_qgLead(myParent),
    m_tailText(nullptr),
    m_fieldFlag(nullptr),
    m_allAround(nullptr),
    m_blockDraw(false)
{
    setFiltersChildEvents(true);    //qt5
    setFlag(QGraphicsItem::ItemIsMovable, false);

    setCacheMode(QGraphicsItem::NoCache);

    setParentItem(m_qgLead);
    m_leadFeat = m_qgLead->getFeature();
    setZValue(ZVALUE::DIMENSION);

    m_tailText = new QGCustomText();
    m_tailText->setPlainText(
                QString::fromUtf8(" "));
    addToGroup(m_tailText);
    m_tailText->hide();
    m_tailText->setPos(0.0, 0.0);         //avoid bRect issues

    m_allAround = new QGIVertex(-1);
    addToGroup(m_allAround);
    m_allAround->setPos(0.0, 0.0);
    m_allAround->setAcceptHoverEvents(false);
    m_allAround->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_allAround->setFlag(QGraphicsItem::ItemIsMovable, false);
    m_allAround->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    m_allAround->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    m_allAround->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

    m_fieldFlag = new QGIPrimPath();
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
//    Base::Console().Message("QGIWS::itemChange(%d)\n", change);
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
//    Base::Console().Message("QGIWS::updateView()\n");
    Q_UNUSED(update);
    auto viewWeld( dynamic_cast<TechDraw::DrawWeldSymbol*>(getViewObject()) );
    if (!viewWeld)
        return;

    if (!getFeature()) {
        Base::Console().Warning("QGIWS::updateView - no feature!\n");
        return;
    }

    draw();
}

void QGIWeldSymbol::draw()
{
//    Base::Console().Message("QGIWS::draw()- %s\n", getFeature()->getNameInDocument());
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
//    Base::Console().Message("QGIWS::drawTile() - tileFeat: %X\n", tileFeat);
    if (!tileFeat) {
        Base::Console().Message("QGIWS::drawTile - tile is null\n");
        return;
    }

    const auto sym( dynamic_cast<TechDraw::DrawWeldSymbol *>(getViewObject()) );
    if (!sym)
        return;
    auto vp = static_cast<ViewProviderWeld*>(getViewProvider(getViewObject()));
    if (!vp)
        return;
    std::string fontName = vp->Font.getValue();
    int         fontSize = QGIView::exactFontSize(vp->Font.getValue(),
                                                  vp->TileFontSize.getValue());
    double featScale = m_leadFeat->getScale();

    std::string tileTextL = tileFeat->LeftText.getValue();
    std::string tileTextR = tileFeat->RightText.getValue();
    std::string tileTextC = tileFeat->CenterText.getValue();
//    std::string symbolFile = tileFeat->SymbolFile.getValue();
    int row = tileFeat->TileRow.getValue();
    int col = tileFeat->TileColumn.getValue();

    QGITile* tile = new QGITile(tileFeat);
    addToGroup(tile);

    QPointF org = getTileOrigin();
    tile->setTilePosition(org, row, col);
    tile->setFont(fontName, fontSize);
    tile->setColor(getCurrentColor());
    tile->setTileTextLeft(tileTextL);
    tile->setTileTextRight(tileTextR);
    tile->setTileTextCenter(tileTextC);
//    tile->setSymbolFile(symbolFile);
    tile->setZValue(ZVALUE::DIMENSION);
    tile->setTileScale(featScale);
    tile->setTailRight(m_weldFeat->isTailRightSide());
    tile->setAltWeld(m_weldFeat->AlternatingWeld.getValue());

    tile->draw();
}

void QGIWeldSymbol::drawAllAround()
{
//    Base::Console().Message("QGIWS::drawAllAround()\n");
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
//    m_allAround->setRadius(calculateFontPixelSize(getDimFontSize()));
    m_allAround->setRadius(PreferencesGui::dimFontSizePX());
    double width = m_qgLead->getLineWidth();
    m_allAround->setWidth(width);
    m_allAround->setZValue(ZVALUE::DIMENSION);
}

void QGIWeldSymbol::drawTailText()
{
//    Base::Console().Message("QGIWS::drawTailText()\n");
    QPointF textPos = getTailPoint();
    m_tailText->setPos(textPos);  //avoid messing up brect with empty item at 0, 0 !!!
    std::string tText = getFeature()->TailText.getValue();
    if (tText.empty()) {
        m_tailText->hide();
        return;
    } else {
        m_tailText->show();
    }
    const auto sym( dynamic_cast<TechDraw::DrawWeldSymbol *>(getViewObject()) );
    if (!sym)
        return;
    auto vp = static_cast<ViewProviderWeld*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    QString qFontName = Base::Tools::fromStdString(vp->Font.getValue());
    int fontSize = QGIView::exactFontSize(vp->Font.getValue(),
                                          vp->FontSize.getValue());

    m_font.setFamily(qFontName);
    m_font.setPixelSize(fontSize);

    m_tailText->setFont(m_font);
    m_tailText->setPlainText(
                QString::fromUtf8(tText.c_str()));
    m_tailText->setColor(getCurrentColor());
    m_tailText->setZValue(ZVALUE::DIMENSION);

    double textWidth = m_tailText->boundingRect().width();
    double charWidth = textWidth / tText.size();
    double hMargin = charWidth + prefArrowSize();

    double textHeight = m_tailText->boundingRect().width();
    double vFudge = textHeight * 0.1;

    if (getFeature()->isTailRightSide()) {
        m_tailText->justifyLeftAt(textPos.x() + hMargin, textPos.y() - vFudge, true);
    } else {
        m_tailText->justifyRightAt(textPos.x() - hMargin, textPos.y() - vFudge, true);
    }
}

void QGIWeldSymbol::drawFieldFlag()
{
//    Base::Console().Message("QGIWS::drawFieldFlag()\n");
    QPointF fieldFlagPos = getKinkPoint();
    m_fieldFlag->setPos(fieldFlagPos);

    if (getFeature()->FieldWeld.getValue()) {
        m_fieldFlag->show();
    } else {
        m_fieldFlag->hide();
        return;
    }
    std::vector<QPointF> flagPoints = { QPointF(0.0, 0.0),
                                        QPointF(0.0, -3.0),
                                        QPointF(-2.0, -2.5),
                                        QPointF(0.0, -2.0) };
    //flag sb about 2x text?
//    double scale = calculateFontPixelSize(getDimFontSize()) / 2.0;
    double scale = PreferencesGui::dimFontSizePX() / 2.0;
    QPainterPath path;
    path.moveTo(flagPoints.at(0) * scale);
    int i = 1;
    int stop = flagPoints.size();
    for ( ; i < stop; i++) {
        path.lineTo(flagPoints.at(i) * scale);
    }

    double width = m_qgLead->getLineWidth();
    m_fieldFlag->setWidth(width);
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
    std::vector<QGITile*> tiles = getQGITiles();
    for (auto t: tiles) {
            QList<QGraphicsItem*> tChildren = t->childItems();
            for (auto tc: tChildren) {
                t->removeFromGroup(tc);
                scene()->removeItem(tc);
                //tc gets deleted when QGIWS gets deleted
            }
        removeFromGroup(t);
        scene()->removeItem(t);
        delete t;
    }
}

std::vector<QGITile*> QGIWeldSymbol::getQGITiles() const
{
    std::vector<QGITile*> result;
    QList<QGraphicsItem*> children = childItems();
    for (auto& c:children) {
         QGITile* tile = dynamic_cast<QGITile*>(c);
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
    std::vector<QGITile*> tiles = getQGITiles();
    for (auto t: tiles) {
        t->setColor(getNormalColor());
        t->draw();
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
    std::vector<QGITile*> tiles = getQGITiles();
    for (auto t: tiles) {
        t->setColor(getPreColor());
        t->draw();
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
    std::vector<QGITile*> tiles = getQGITiles();
    for (auto t: tiles) {
        t->setColor(getSelectColor());
        t->draw();
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
    Base::Vector3d org = m_leadFeat->getTileOrigin();
    QPointF result(org.x, org.y);
    return result;
}

QPointF QGIWeldSymbol::getKinkPoint()
{
    Base::Vector3d org = m_leadFeat->getKinkPoint();
    QPointF result(org.x, org.y);
    return result;
}

QPointF QGIWeldSymbol::getTailPoint()
{
    Base::Vector3d org = m_leadFeat->getTailPoint();
    QPointF result(org.x, org.y);
    return result;
}

void QGIWeldSymbol::setFeature(TechDraw::DrawWeldSymbol* feat)
{
//    Base::Console().Message("QGIWS::setFeature(%s)\n", feat->getNameInDocument());
    m_weldFeat = feat;
    m_weldFeatName = feat->getNameInDocument();
}

TechDraw::DrawWeldSymbol* QGIWeldSymbol::getFeature()
{
    return m_weldFeat;
}

//preference
QColor QGIWeldSymbol::prefNormalColor()
{
    setNormalColor(PreferencesGui::leaderQColor());
    return getNormalColor();
}

double QGIWeldSymbol::prefArrowSize()
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
    for (auto& t: qgTiles) {
        QRectF childRect = mapFromItem(t, t->boundingRect()).boundingRect();
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

//    painter->setPen(Qt::red);
//    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}

#include <Mod/TechDraw/Gui/moc_QGIWeldSymbol.cpp>
