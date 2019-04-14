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
    m_lineColor(Qt::black)
{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    
    setCacheMode(QGraphicsItem::NoCache);

    m_lineColor = Qt::blue;

    m_line = new QGEPath();
    addToGroup(m_line);
    m_line->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_line->setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    m_line->setAcceptHoverEvents(false);
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
        m_line, SIGNAL(pointsUpdated(std::vector<QPointF>)),
        this  , SLOT  (onLineEditFinished(std::vector<QPointF>))
            );
}

QVariant QGILeaderLine::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGIV::itemChange(%d)\n", change);
    if (change == ItemSelectedHasChanged && scene()) {
        //There's nothing special for QGIVL to do when selection changes!
    } else if(change == ItemSceneChange && scene()) {
        // nothing special!
    }
    return QGIView::itemChange(change, value);
}

void QGILeaderLine::onLineEditFinished(std::vector<QPointF> pts)
{
//    Base::Console().Message("QGIVL::onLineEditFinished(%d)\n",pts.size());
    std::vector<Base::Vector3d> waypoints;
    for (auto& p: pts) {
        Base::Vector3d v(p.x(),p.y(),0.0);
        waypoints.push_back(v);
    }

    if (m_parentItem == nullptr) {
        Base::Console().Log("QGIVL::onLineEditFinished - m_parentItem is NULL\n");
    } else {
        QGIView* qgiv = dynamic_cast<QGIView*>(m_parentItem);
        if (qgiv != nullptr) {
            Q_EMIT editComplete(pts, qgiv);  //leader's parent if QGIView
        }
    }
}

void QGILeaderLine::startPathEdit(void)
{
    double scale = getScale();
    m_line->setScale(scale);
    m_line->inEdit(true);
    m_line->startPathEdit();
}

void QGILeaderLine::updateView(bool update)
{
//    Base::Console().Message("QGIL::updateView() %s\n",getViewObject()->getNameInDocument());
    Q_UNUSED(update);
    auto leadFeat( dynamic_cast<TechDraw::DrawLeaderLine*>(getViewObject()) );
    if ( leadFeat == nullptr ) {
        Base::Console().Log("QGIL::updateView - no feature!\n");
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
//    Base::Console().("QGIL::draw()- %s\n", getViewObject()->getNameInDocument());
    if (!isVisible()) {
        Base::Console().Log("QGIL::draw - not visible\n");
        return;
    }

    TechDraw::DrawLeaderLine* leadFeat = getFeature();
    if((!leadFeat) ) {
        Base::Console().Log("QGIL::draw - no feature\n");
        return;
    }

    auto vp = static_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        Base::Console().Log("QGIL::draw - no viewprovider\n");
        return;
    }

    TechDraw::DrawView* parent = leadFeat->getBaseView();
    QGVPage* view = QGIView::getGraphicsView(parent);
    if (view == nullptr) {
        Base::Console().Log("QGIL::draw - no graphcisView for parent!! - setup?\n");
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
    std::vector<QPointF> qPoints = convertWaypoints();
    if (qPoints.empty()) {
        Base::Console().Log("QGIL::draw - no points\n");
        return;
    }
//    m_line->setAttach(aPoint);
    m_line->setStyle(m_lineStyle);
    double scaler = 1.0;
    m_line->setWidth(scaler * m_lineWidth);

    m_line->setNormalColor(m_lineColor);
    scale = getScale();
    m_line->setScale(scale);
    m_line->makeDeltasFromPoints(qPoints);
    m_line->setPos(0,0); 
    m_line->updatePath();
    m_line->show();

    setArrows(qPoints);

    QGIView::draw();
}

void QGILeaderLine::drawBorder()
{
////Leaders have no border!
//    QGIView::drawBorder();   //good for debugging
}

//waypoints converted to QPointF
std::vector<QPointF> QGILeaderLine::convertWaypoints() 
{
//    Base::Console().Message("QGIL::convertWaypoints()\n");
    TechDraw::DrawLeaderLine* featLine = getFeature();
    std::vector<QPointF> result;
    std::vector<Base::Vector3d> vPts = featLine->WayPoints.getValues();
    if (vPts.empty()) {
        Base::Console().Log("QGIL::convertWaypoints - no points from feature!\n");
        return result;
    }

    QGraphicsItem* myParent = parentItem();
    if (myParent != nullptr) {
        for (auto& p: vPts) {
            QPointF pConvert(p.x, p.y);
            result.push_back(pConvert);
        }
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
        if (pathPoints.size() > 1) {
            auto it = pathPoints.begin();
            QPointF s = (*it);
            QPointF e = (*(it + 1));
            QPointF qsVec = s - e;
            Base::Vector3d sVec(qsVec.x(),qsVec.y(),0.0);
            m_arrow1->setDirection(sVec);
            m_arrow1->setNormalColor(m_lineColor);
            m_arrow1->setPos(0.0,0.0);
            m_arrow1->setPrettyNormal();
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
        if (pathPoints.size() > 1) {
            auto itr = pathPoints.rbegin();
            QPointF s = (*itr);
            QPointF e = (*(itr + 1));
            QPointF qeVec = s - e;
            Base::Vector3d eVec(qeVec.x(),qeVec.y(),0.0);
            m_arrow2->setDirection(eVec);
            m_arrow2->setNormalColor(m_lineColor);
            m_arrow2->setPos(lastOffset);
            m_arrow2->setPrettyNormal();
        }
        m_arrow2->draw();
        m_arrow2->show();
    } else {
        m_arrow2->hide();
    }
}

void QGILeaderLine::abandonEdit(void)
{
    m_line->clearMarkers();
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
