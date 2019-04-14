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
  # include <QGraphicsItem>
  # include <QPainter>
  # include <QPaintDevice>
  # include <QSvgGenerator>
  #include <QRegExp>

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

#include <Mod/TechDraw/App/DrawTextLeader.h>
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
#include "QGMText.h"
#include "QGILeaderLine.h"

#include "QGITextLeader.h"

using namespace TechDraw;
using namespace TechDrawGui;


//**************************************************************
QGITextLeader::QGITextLeader(QGraphicsItem* myParent,
                         TechDraw::DrawTextLeader* leader) : 
    QGILeaderLine(myParent,leader)
{
//    Base::Console().Message("QGITL::QGITL() - %s\n", leader->getNameInDocument());
    setHandlesChildEvents(false);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);

    m_text = new QGMText();
    m_text->setTextInteractionFlags(Qt::NoTextInteraction);
    addToGroup(m_text);
//    m_text->setZValue(ZVALUE::DIMENSION);

    QObject::connect(
        m_text, SIGNAL(dragging()),
        this  , SLOT  (textDragging())
            );
    QObject::connect(
        m_text, SIGNAL(dragFinished()),
        this  , SLOT  (textDragFinished())
            );
}

QVariant QGITextLeader::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        //There's nothing special for QGIVL to do when selection changes!
    } else if(change == ItemSceneChange && scene()) {
        // nothing special!
    }
    return QGILeaderLine::itemChange(change, value);
}

void QGITextLeader::textDragging(void)
{
//    Base::Console().Message("QGIL::textDragging()\n");
    //this is the long way around.  can we do it without crossing the App/Gui boundary?
    //just update graphics until drag finished.
//    auto lead( dynamic_cast<TechDraw::DrawTextLeader*>(getFeature()) );

//    if( lead == nullptr ) {
//        return;
//    }

//    double x = Rez::appX(m_text->x()),
//           y = Rez::appX(m_text->y());
//    Base::Vector3d tPos(x,-y,0.0);
//    Gui::Command::openCommand("Drag Text");
//    lead->TextPosition.setValue(tPos);
//    Gui::Command::commitCommand();
//      draw();
}

void QGITextLeader::textDragFinished(void)
{
//    Base::Console().Message("QGIL::textDragFinished()\n");
    auto lead( dynamic_cast<TechDraw::DrawTextLeader*>(getFeature()) );

    if( lead == nullptr ) {
        return;
    }

    double x = Rez::appX(m_text->x()),
           y = Rez::appX(m_text->y());
    Base::Vector3d tPos(x,-y,0.0);
    lead->TextPosition.setValue(tPos);
}

void QGITextLeader::onTextSelect(bool state)
{
    Q_UNUSED(state);
    draw();
}

void QGITextLeader::draw()
{
//    Base::Console().Message("QGITL::draw()- %s\n", getViewObject()->getNameInDocument());
    if (!isVisible()) {
        Base::Console().Log("QGITL::draw - not visible\n");
        return;
    }

    TechDraw::DrawTextLeader* leadFeat = getFeature();
    if((!leadFeat) ) {
        Base::Console().Log("QGITL::draw - no feature\n");
        return;
    }

    auto vp = static_cast<ViewProviderTextLeader*>(getViewProvider(getFeature()));
    if ( vp == nullptr ) {
        Base::Console().Log("QGITL::draw - no viewprovider\n");
        return;
    }

    TechDraw::DrawView* parent = leadFeat->getBaseView();
    QGVPage* view = QGILeaderLine::getGraphicsView(parent);
    if (view == nullptr) {
        Base::Console().Log("QGITL::draw - no graphcisView for parent!! - setup?\n");
        return;
    }

//    double scale = leadFeat->getScale();
    QGILeaderLine::draw();

    setTextItem();

}

void QGITextLeader::setTextItem()
{
//    Base::Console().Message("QGIVL::setTextItem()\n");
    TechDraw::DrawTextLeader* leadFeat = getFeature();
    auto vp = static_cast<ViewProviderTextLeader*>(getViewProvider(getFeature()));
    if ( vp == nullptr ) {
        Base::Console().Log("QGIVL::setTextItem - no ViewProvider\n");
        return;
    }
    QFont font = m_text->font();
    font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
    font.setFamily(QString::fromLatin1(vp->Font.getValue()));
    m_text->setFont(font);

    //convert point font sizes to (Rez,mm) font sizes
    QRegExp rxFontSize(QString::fromUtf8("font-size:([0-9]*)pt;"));
    QString inHtml = QString::fromUtf8(leadFeat->LeaderText.getValue());
    QString match;
    double mmPerPoint = 0.353;
    double sizeConvert = Rez::getRezFactor() * mmPerPoint;
    int pos = 0;
    QStringList findList;
    QStringList replList;
    while ((pos = rxFontSize.indexIn(inHtml, pos)) != -1) {
        QString found = rxFontSize.cap(0);
        findList << found;
        QString qsOldSize = rxFontSize.cap(1); 

        QString repl = found;
        double newSize = qsOldSize.toDouble();
        newSize = newSize * sizeConvert;
        QString qsNewSize = QString::number(newSize, 'f', 2);
        repl.replace(qsOldSize,qsNewSize);
        replList << repl;
        pos += rxFontSize.matchedLength();
    }
    QString outHtml = inHtml;
    int iRepl = 0;
    //TODO: check list for duplicates?
    for ( ; iRepl < findList.size(); iRepl++) {
        outHtml = outHtml.replace(findList[iRepl], replList[iRepl]);
    }

    m_text->setHtml(outHtml);
    m_text->setPrettyNormal();

    m_text->setTextWidth(Rez::guiX(vp->MaxWidth.getValue()));
    m_text->showBox(vp->ShowFrame.getValue());

    double scale = getScale();
    Base::Vector3d textPos = Rez::guiX(leadFeat->TextPosition.getValue());
    QPointF tPos(textPos.x * scale,- textPos.y * scale);
    m_text->setPos(tPos);
}

//void QGITextLeader::drawBorder()
//{
//////Leaders have no border!
////    QGIView::drawBorder();   //good for debugging
//}


TechDraw::DrawTextLeader* QGITextLeader::getFeature(void)
{
    TechDraw::DrawTextLeader* result = 
         static_cast<TechDraw::DrawTextLeader*>(getViewObject());
    return result;
}

QRectF QGITextLeader::boundingRect() const
{
    return childrenBoundingRect();
}

QPainterPath QGITextLeader::shape() const
{
    return QGraphicsItemGroup::shape();
}

void QGITextLeader::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGILeaderLine::paint (painter, &myOption, widget);
}

#include <Mod/TechDraw/Gui/moc_QGITextLeader.cpp>
