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

#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "QGIArrow.h"
#include "ViewProviderRichAnno.h"
#include "MDIViewPage.h"
#include "DrawGuiUtil.h"
#include "QGVPage.h"
#include "QGIPrimPath.h"
#include "QGEPath.h"
#include "QGMText.h"
#include "QGIView.h"

#include "QGIRichAnno.h"

using namespace TechDraw;
using namespace TechDrawGui;


//**************************************************************
QGIRichAnno::QGIRichAnno(QGraphicsItem* myParent,
                         TechDraw::DrawRichAnno* anno)
{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);         //we actually select & drag m_text
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);

    if (myParent != nullptr) {
        setParentItem(myParent);
    }
    setViewFeature(anno);

    m_text = new QGMText();
    m_text->setTextInteractionFlags(Qt::NoTextInteraction);
    addToGroup(m_text);
    m_text->setZValue(ZVALUE::DIMENSION);

    setZValue(ZVALUE::DIMENSION);

    QObject::connect(
        m_text, SIGNAL(dragging()),
        this  , SLOT  (textDragging())
            );
    QObject::connect(
        m_text, SIGNAL(dragFinished()),
        this  , SLOT  (textDragFinished())
            );
    QObject::connect(
        m_text, SIGNAL(selected(bool)),
        this  , SLOT  (select(bool)));

    QObject::connect(
        m_text, SIGNAL(hover(bool)),
        this  , SLOT  (hover(bool)));
}

QVariant QGIRichAnno::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        //There's nothing special for QGIRA to do when selection changes!
    } else if(change == ItemSceneChange && scene()) {
        // nothing special!
    }
    return QGIView::itemChange(change, value);
}

void QGIRichAnno::textDragging(void)
{
//    Base::Console().Message("QGIRA::textDragging()\n");
    //this is the long way around.  can we do it without crossing the App/Gui boundary?
    //just update graphics until drag finished.
//    auto lead( dynamic_cast<TechDraw::DrawRichAnno*>(getFeature()) );

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

void QGIRichAnno::textDragFinished(void)
{
//    Base::Console().Message("QGIRA::textDragFinished()\n");
    auto anno( dynamic_cast<TechDraw::DrawRichAnno*>(getFeature()) );

    if( anno == nullptr ) {
        return;
    }

    double x = Rez::appX(m_text->x()),
           y = - Rez::appX(m_text->y());
    anno->X.setValue(x);
    anno->Y.setValue(y);
}

void QGIRichAnno::select(bool state)
{
    setSelected(state);
    draw();
}

void QGIRichAnno::hover(bool state)
{
    m_hasHover = state;
    draw();
}

void QGIRichAnno::updateView(bool update)
{
//    Base::Console().Message("QGIRA::updateView() - %s\n", getViewName());
    Q_UNUSED(update);
    auto annoFeat( dynamic_cast<TechDraw::DrawRichAnno*>(getViewObject()) );
    if ( annoFeat == nullptr ) {
        Base::Console().Log("QGIRA::updateView - no feature!\n");
        return;
    }

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    draw();
}

void QGIRichAnno::drawBorder()
{
////Leaders have no border!
//    QGIView::drawBorder();   //good for debugging
}


void QGIRichAnno::draw()
{
//    Base::Console().Log("QGITL::draw() - %s\n",getFeature()->getNameInDocument());
    if (!isVisible()) {
        Base::Console().Log("QGITL::draw - not visible\n");
        return;
    }

    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if((!annoFeat) ) {
        Base::Console().Log("QGITL::draw - no feature\n");
        return;
    }

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getFeature()));
    if ( vp == nullptr ) {
        Base::Console().Log("QGITL::draw - no viewprovider\n");
        return;
    }

    QGIView::draw();

    setTextItem();
}

void QGIRichAnno::setTextItem()
{
//    Base::Console().Message("QGIRA::setTextItem() - %s\n",getViewName());
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(annoFeat));
    if ( vp == nullptr ) {
        Base::Console().Log("QGIRA::setTextItem - no ViewProvider\n");
        return;
    }
    QFont font = m_text->font();
    font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
    font.setFamily(QString::fromLatin1(vp->Font.getValue()));
    m_text->setFont(font);

    //convert point font sizes to (Rez,mm) font sizes
    QRegExp rxFontSize(QString::fromUtf8("font-size:([0-9]*)pt;"));
    QString inHtml = QString::fromUtf8(annoFeat->AnnoText.getValue());
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

    m_text->setTextWidth(Rez::guiX(annoFeat->MaxWidth.getValue()));
    m_text->showBox(annoFeat->ShowFrame.getValue());

    double scale = getScale();
    double x = Rez::guiX(annoFeat->X.getValue());
    double y = Rez::guiX(annoFeat->Y.getValue());
    Base::Vector3d textPos(x,y,0.0);
    QPointF tPos(textPos.x * scale,- textPos.y * scale);
    m_text->setPos(tPos);
}

//void QGIRichAnno::drawBorder()
//{
//////Leaders have no border!
////    QGIView::drawBorder();   //good for debugging
//}


TechDraw::DrawRichAnno* QGIRichAnno::getFeature(void)
{
    TechDraw::DrawRichAnno* result = 
         static_cast<TechDraw::DrawRichAnno*>(getViewObject());
    return result;
}

QRectF QGIRichAnno::boundingRect() const
{
    QRectF rect = mapFromItem(m_text,m_text->boundingRect()).boundingRect();
    return rect.adjusted(-10.,-10.,10.,10.);
}

QPainterPath QGIRichAnno::shape() const
{
    return QGraphicsItemGroup::shape();
}

void QGIRichAnno::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}

#include <Mod/TechDraw/Gui/moc_QGIRichAnno.cpp>
