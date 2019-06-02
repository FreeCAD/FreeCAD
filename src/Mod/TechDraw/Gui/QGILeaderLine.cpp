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
  #include <BRep_Builder.hxx>
  #include <TopoDS_Compound.hxx>
  # include <TopoDS_Shape.hxx>
  # include <TopoDS_Edge.hxx>
  # include <TopoDS.hxx>
  # include <BRepAdaptor_Curve.hxx>
  # include <Precision.hxx>

  # include <QGraphicsScene>
  # include <QGraphicsSceneMouseEvent>
  # include <QPainter>
  # include <QPaintDevice>
  # include <QSvgGenerator>

  # include <math.h>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>
#include <Gui/Command.h>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "QGIArrow.h"
#include "ViewProviderLeader.h"
#include "MDIViewPage.h"
#include "DrawGuiUtil.h"
#include "QGVPage.h"
#include "QGIPrimPath.h"
#include "QGEPath.h"

#include "QGILeaderLine.h"

using namespace TechDraw;
using namespace TechDrawGui;


//**************************************************************
QGILeaderLine::QGILeaderLine(QGraphicsItem* myParent,
                         TechDraw::DrawLeaderLine* leader) :
    m_parentItem(myParent),
    m_lineWidth(0.0),
    m_lineColor(Qt::black),
    m_hasHover(false),
    m_blockDraw(false)

{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    
    setCacheMode(QGraphicsItem::NoCache);

    m_lineColor = Qt::blue;

    m_line = new QGEPath();
    m_line->setNormalColor(getNormalColor());
//    m_line->setPrettyNormal();

    addToGroup(m_line);
    m_line->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_line->setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    m_line->setZValue(ZVALUE::DIMENSION);

    m_arrow1 = new QGIArrow();
    addToGroup(m_arrow1);
    m_arrow1->hide();
    m_arrow2 = new QGIArrow();
    addToGroup(m_arrow2);
    m_arrow2->hide();

    setParentItem(m_parentItem);

    setViewFeature(leader);

    setZValue(ZVALUE::DIMENSION);

    QObject::connect(
        m_line, SIGNAL(pointsUpdated(QPointF, std::vector<QPointF>)),
        this  , SLOT  (onLineEditFinished(QPointF, std::vector<QPointF>))
            );
    QObject::connect(
        m_line, SIGNAL(attachMoved(QPointF)),
        this  , SLOT  (onAttachMoved(QPointF))
            );
    QObject::connect(
        m_line, SIGNAL(selected(bool)),
        this  , SLOT  (select(bool)));

    QObject::connect(
        m_line, SIGNAL(hover(bool)),
        this  , SLOT  (hover(bool)));
            
}

QVariant QGILeaderLine::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGILL::itemChange(%d)\n", change);
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            m_line->setSelected(true);
            m_arrow1->setSelected(true);
            m_arrow2->setSelected(true);
        } else {
            m_line->setSelected(false);
            m_arrow1->setSelected(false);
            m_arrow2->setSelected(false);
        }
        draw();
    } else if(change == ItemSceneChange && scene()) {
        // nothing special!
    }
    return QGIView::itemChange(change, value);
}

void QGILeaderLine::onSourceChange(TechDraw::DrawView* newParent)
{
//    Base::Console().Message("QGILL::onSoureChange(%s)\n",newParent->getNameInDocument());
    std::string parentName = newParent->getNameInDocument();
    QGIView* qgiParent = getQGIVByName(parentName);
    if (qgiParent != nullptr) {
        m_parentItem = qgiParent;
        setParentItem(m_parentItem);
        draw();
    } else {
        Base::Console().Warning("QGILL::onSourceChange - new parent %s has no QGIView\n",parentName.c_str());
    }
}

void QGILeaderLine::onAttachMoved(QPointF attach)
{
    auto leadFeat( dynamic_cast<TechDraw::DrawLeaderLine*>(getViewObject()) );
    TechDraw::DrawView* parent = leadFeat->getBaseView();
    double pScale = 1.0;
    if (parent != nullptr) {
        pScale = parent->getScale();
    }
    QPointF mapped = m_parentItem->mapFromItem(this,attach);  //map point to baseView
    Base::Vector3d attachPoint = Base::Vector3d(mapped.x(),mapped.y(),0.0);
    getFeature()->setPosition(Rez::appX(attachPoint.x / pScale),  //attach point must scale with parent.
                              Rez::appX(- attachPoint.y / pScale),
                              true);
}


void QGILeaderLine::select(bool state)
{
    setSelected(state);
    draw();
}

void QGILeaderLine::hover(bool state)
{
    m_hasHover = state;
    draw();
}

void QGILeaderLine::closeEdit(void)
{
//    Base::Console().Message("QGIL::closeEdit()\n");
    if (m_line != nullptr) {
        m_line->onEndEdit();   //to QGEPath
    }
}

//from QGEPath
void QGILeaderLine::onLineEditFinished(QPointF attach, std::vector<QPointF> deltas)
{
//    Base::Console().Message("QGILL::onLineEditFinished(%s, %d)\n", 
//                            TechDraw::DrawUtil::formatVector(attach).c_str(),
//                            deltas.size());
    m_blockDraw = true;
    double pScale = 1.0;
    auto leadFeat( dynamic_cast<TechDraw::DrawLeaderLine*>(getViewObject()) );
    TechDraw::DrawView* parent = leadFeat->getBaseView();
    if (parent != nullptr) {
        pScale = parent->getScale();
    }

    QPointF p0 = deltas.front();
    if ( !(TechDraw::DrawUtil::fpCompare(attach.x(),0.0) &&
           TechDraw::DrawUtil::fpCompare(attach.y(),0.0))  )  {
        //p0 was moved. need to change AttachPoint and intervals from p0
        QPointF mapped = m_parentItem->mapFromItem(this,attach);  //map point to baseView
        Base::Vector3d attachPoint = Base::Vector3d(mapped.x(),mapped.y(),0.0);
        for (auto& p : deltas) {
            p -= p0;
        }
        deltas.at(0) = QPointF(0.0,0.0);
        getFeature()->setPosition(Rez::appX(attachPoint.x / pScale),  //attach point must scale with parent.
                                  Rez::appX(- attachPoint.y / pScale),
                                  true);
    }

    std::vector<Base::Vector3d> waypoints;
    for (auto& p: deltas) {
        Base::Vector3d v(p.x(),p.y(),0.0);
        waypoints.push_back(v);
    }

    getFeature()->WayPoints.setValues(waypoints);
    if (getFeature()->AutoHorizontal.getValue()) {
        getFeature()->adjustLastSegment();
    }

    if (m_parentItem == nullptr) {
        Base::Console().Warning("QGILL::onLineEditFinished - m_parentItem is NULL\n");
    } else {
        QGIView* qgiv = dynamic_cast<QGIView*>(m_parentItem);
        if (qgiv != nullptr) {
            Q_EMIT editComplete();           //to task
        }
    }
    m_blockDraw = false;
    draw();
}

void QGILeaderLine::startPathEdit(void)
{
    saveState();

    double scale = getScale();
    m_line->setScale(scale);
    m_line->inEdit(true);
    m_line->startPathEdit();
}

void QGILeaderLine::saveState(void)
{
//    Base::Console().Message("QGILL::saveState()\n");
    auto leadFeat = getFeature();
    if (leadFeat != nullptr) {
        m_savePoints = leadFeat->WayPoints.getValues();
        m_saveX = leadFeat->X.getValue();
        m_saveY = leadFeat->Y.getValue();
    }
}

void QGILeaderLine::restoreState(void)
{
//    Base::Console().Message("QGILL::restoreState()\n");
    auto leadFeat = getFeature();
    if (leadFeat != nullptr) {
        leadFeat->WayPoints.setValues(m_savePoints);
        leadFeat->X.setValue(m_saveX);
        leadFeat->Y.setValue(m_saveY);
        leadFeat->recomputeFeature();
    }
}

void QGILeaderLine::updateView(bool update)
{
//    Base::Console().Message("QGIL::updateView() %s\n",getViewObject()->getNameInDocument());
    Q_UNUSED(update);
    auto leadFeat( dynamic_cast<TechDraw::DrawLeaderLine*>(getViewObject()) );
    if ( leadFeat == nullptr ) {
        Base::Console().Warning("QGILL::updateView - no feature!\n");
        return;
    }

    auto vp = static_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }
    draw();
}

void QGILeaderLine::draw()
{
//    Base::Console().Message("QGILL::draw()- %s\n", getViewObject()->getNameInDocument());
    if (m_blockDraw) {
//        Base::Console().Message("QGIL::draw - block draw\n");
        return;
    }

    if (!isVisible()) {
//        Base::Console().Message("QGIL::draw - not visible\n");
        return;
    }

    TechDraw::DrawLeaderLine* leadFeat = getFeature();
    if((!leadFeat) ) {
//        Base::Console().Message("QGIL::draw - no feature\n");
        return;
    }

    auto vp = static_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
//        Base::Console().Message("QGIL::draw - no viewprovider\n");
        return;
    }

    TechDraw::DrawView* parent = leadFeat->getBaseView();
    QGVPage* view = QGIView::getGraphicsView(parent);
    if (view == nullptr) {
//        Base::Console().Message("QGIL::draw - no graphcisView for parent!! - setup?\n");
        return;
    }

    if (m_line->inEdit()) {
//        Base::Console().Message("QGIL::draw - m_line in edit\n");
        return;
    }

    if (leadFeat->isLocked()) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
    } else {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());
    m_lineColor = vp->Color.getValue().asValue<QColor>();
    m_lineStyle = (Qt::PenStyle) vp->LineStyle.getValue();

    double scale = parent->getScale();               //only attach point scales with parent.
    double x = Rez::guiX(leadFeat->X.getValue());
    double y = - Rez::guiX(leadFeat->Y.getValue());
    QPointF aPoint(x,y);                          //1:1 parent coords
    aPoint *= scale;                              //scaled parent coords
    setPos(aPoint);

// this is for Page as parent
//    double ptScale = 1.0;
//    if (leadFeat->Scalable.getValue()) {
//        ptScale = scale;
//   }
    std::vector<QPointF> qPoints = waypointsToQPoints();
    if (qPoints.empty()) {
//        Base::Console().Warning("QGIL::draw - no points\n");
        return;
    }

    m_line->setStyle(m_lineStyle);
    double scaler = 1.0;
    m_line->setWidth(scaler * m_lineWidth);

    m_line->setNormalColor(m_lineColor);
    scale = getScale();
    m_line->setScale(scale);
    if (leadFeat->StartSymbol.getValue() > -1) {
        m_line->setStartAdjust(QGIArrow::getOverlapAdjust(leadFeat->StartSymbol.getValue(),
                                                          QGIArrow::getPrefArrowSize()));
    }
    if (leadFeat->EndSymbol.getValue() > -1) {
        m_line->setEndAdjust(QGIArrow::getOverlapAdjust(leadFeat->EndSymbol.getValue(),
                                                        QGIArrow::getPrefArrowSize()));
    }

    m_line->makeDeltasFromPoints(qPoints);
    m_line->setPos(0,0); 
    m_line->updatePath();
    m_line->show();

    setArrows(qPoints);

    if (m_hasHover && !isSelected()) {
        m_arrow1->setPrettyPre();
        m_arrow2->setPrettyPre();
        m_line->setPrettyPre();
    } else if (isSelected()) {
        m_arrow1->setPrettySel();
        m_arrow2->setPrettySel();
        m_line->setPrettySel();
    } else {
        m_arrow1->setPrettyNormal();
        m_arrow2->setPrettyNormal();
        m_line->setPrettyNormal();
    }
//    Base::Console().Message("QGIL::draw - exits\n");
}

void QGILeaderLine::drawBorder()
{
////Leaders have no border!
//    QGIView::drawBorder();   //good for debugging
}

//waypoints converted to QPointF
std::vector<QPointF> QGILeaderLine::waypointsToQPoints() 
{
//    Base::Console().Message("QGIL::waypointsToQPoints - #1 ()\n");
    TechDraw::DrawLeaderLine* featLine = getFeature();
    std::vector<QPointF> result;
    std::vector<Base::Vector3d> vPts = featLine->WayPoints.getValues();
    if (vPts.empty()) {
        Base::Console().Warning("QGIL::waypointsToQPoints - no points from feature!\n");
        return result;
    }

    for (auto& p: vPts) {
        QPointF pConvert(p.x, p.y);
        result.push_back(pConvert);
    }
    return result;
}

std::vector<QPointF> QGILeaderLine::waypointsToQPoints(std::vector<Base::Vector3d> pts)
{ 
//    Base::Console().Message("QGIL::waypointsToQPoints(%d) - #2\n", pts.size());
    std::vector<QPointF> result;
    for (auto& p: pts) {
        QPointF pConvert(p.x, p.y);
        result.push_back(pConvert);
    }
    return result;
}

void QGILeaderLine::setArrows(std::vector<QPointF> pathPoints)
{
    Base::Vector3d stdX(1.0,0.0,0.0);
    TechDraw::DrawLeaderLine* line = getFeature();
    
    double scale = getScale();
    QPointF lastOffset = (pathPoints.back() - pathPoints.front()) * scale;

    if (line->StartSymbol.getValue() > -1) {
        m_arrow1->setStyle(line->StartSymbol.getValue());
        m_arrow1->setWidth(m_lineWidth);
//        TODO: variable size arrow heads
        m_arrow1->setSize(QGIArrow::getPrefArrowSize());
        m_arrow1->setDirMode(true);
        m_arrow1->setDirection(stdX);
        m_arrow1->setNormalColor(m_lineColor);

        if (pathPoints.size() > 1) {
            auto it = pathPoints.begin();
            QPointF s = (*it);
            QPointF e = (*(it + 1));
            QPointF qsVec = s - e;
            Base::Vector3d sVec(qsVec.x(),qsVec.y(),0.0);
            m_arrow1->setDirection(sVec);
            m_arrow1->setPos(0.0,0.0);
        }
        m_arrow1->draw();
        m_arrow1->show();
    } else {
        m_arrow1->hide();
    }
    
    if (line->EndSymbol.getValue() > -1) {
        m_arrow2->setStyle(line->EndSymbol.getValue());
        m_arrow2->setWidth(m_lineWidth);
        m_arrow2->setDirMode(true);
        m_arrow2->setDirection(-stdX);
        m_arrow2->setNormalColor(m_lineColor);
        if (pathPoints.size() > 1) {
            auto itr = pathPoints.rbegin();
            QPointF s = (*itr);
            QPointF e = (*(itr + 1));
            QPointF qeVec = s - e;
            Base::Vector3d eVec(qeVec.x(),qeVec.y(),0.0);
            m_arrow2->setDirection(eVec);
            m_arrow2->setPos(lastOffset);
        }
        m_arrow2->draw();
        m_arrow2->show();
    } else {
        m_arrow2->hide();
    }
}

void QGILeaderLine::abandonEdit(void)
{
//    Base::Console().Message("QGIL::abandonEdit()\n");
    m_line->clearMarkers();
    m_line->restoreState();
    restoreState();
}

double QGILeaderLine::getScale(void)
{
    bool isScalable = getFeature()->Scalable.getValue();
    double scale = getFeature()->getScale();
    if (!isScalable) {
        scale = 1.0;
    }
    return scale;
}

TechDraw::DrawLeaderLine* QGILeaderLine::getFeature(void)
{
    TechDraw::DrawLeaderLine* result = 
         static_cast<TechDraw::DrawLeaderLine*>(getViewObject());
    return result;
}

double QGILeaderLine::getEdgeFuzz(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double result = hGrp->GetFloat("EdgeFuzz",10.0);
    return result;
}

QColor QGILeaderLine::getNormalColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/LeaderLinens");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();

    auto lead( dynamic_cast<TechDraw::DrawLeaderLine*>(getViewObject()) );
    if( lead == nullptr )
        return m_colNormal;

    auto vp = static_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return m_colNormal;
    }

    m_colNormal = vp->Color.getValue().asValue<QColor>();
    return m_colNormal;
}

QRectF QGILeaderLine::boundingRect() const
{
    return childrenBoundingRect();
}

QPainterPath QGILeaderLine::shape() const
{
    return QGraphicsItemGroup::shape();
}

void QGILeaderLine::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}

#include <Mod/TechDraw/Gui/moc_QGILeaderLine.cpp>
