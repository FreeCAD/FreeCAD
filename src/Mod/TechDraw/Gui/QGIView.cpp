/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
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
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#include <QTextOption>
#include <QTransform>
#include <strstream>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include "QGCustomBorder.h"
#include "QGCustomLabel.h"

#include "QGIView.h"
#include "QGCustomClip.h"
#include "QGIViewClip.h"

#include "../App/DrawViewClip.h"

using namespace TechDrawGui;

void _debugRect(char* text, QRectF r);

QGIView::QGIView(const QPoint &pos, QGraphicsScene *scene)
    :QGraphicsItemGroup(),
     locked(false),
     borderVisible(true),
     m_innerView(false)
{
    setFlag(QGraphicsItem::ItemIsSelectable,true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    setAcceptHoverEvents(true);
    setPos(pos);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("NormalColor", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x0000FF00));
    m_colSel = fcColor.asValue<QColor>();
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0x00080800));
    m_colPre = fcColor.asValue<QColor>();

    m_colCurrent = m_colNormal;
    m_pen.setColor(m_colCurrent);

    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");
    m_font.setFamily(QString::fromStdString(fontName));
    m_font.setPointSize(5.0);     //scene units (mm), not points

    //Add object to scene
    if(scene) // TODO: Get rid of the ctor args as in the refactor attempt
        scene->addItem(this);

    m_decorPen.setStyle(Qt::DashLine);
    m_decorPen.setWidth(0); // 0 => 1px "cosmetic pen"

    m_label = new QGCustomLabel();
    addToGroup(m_label);
    m_label->setFont(m_font);

    m_border = new QGCustomBorder();
    addToGroup(m_border);

    isVisible(true);
}


void QGIView::alignTo(QGraphicsItem*item, const QString &alignment)
{
    alignHash.clear();
    alignHash.insert(alignment, item);
}

QVariant QGIView::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();

        if(locked){
            newPos.setX(pos().x());
            newPos.setY(pos().y());
        }

        // TODO  find a better data structure for this
        if(alignHash.size() == 1) {
            QGraphicsItem*item = alignHash.begin().value();
            QString alignMode   = alignHash.begin().key();

            if(alignMode == QString::fromAscii("Vertical")) {
                newPos.setX(item->pos().x());
            } else if(alignMode == QString::fromAscii("Horizontal")) {
                newPos.setY(item->pos().y());
            } else if(alignMode == QString::fromAscii("45slash")) {
                double dist = ( (newPos.x() - item->pos().x()) +
                                (item->pos().y() - newPos.y()) ) / 2.0;

                newPos.setX( item->pos().x() + dist);
                newPos.setY( item->pos().y() - dist );
            } else if(alignMode == QString::fromAscii("45backslash")) {
                double dist = ( (newPos.x() - item->pos().x()) +
                                (newPos.y() - item->pos().y()) ) / 2.0;

                newPos.setX( item->pos().x() + dist);
                newPos.setY( item->pos().y() + dist );
            }
        }
        return newPos;
    }

    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            m_colCurrent = m_colSel;
        } else {
            m_colCurrent = m_colNormal;
        }
        drawBorder();
    }

    return QGraphicsItemGroup::itemChange(change, value);
}

void QGIView::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if(locked) {
        event->ignore();
    } else {
      QGraphicsItem::mousePressEvent(event);
    }
}

void QGIView::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIView::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if(!locked && isSelected()) {
        if (!isInnerView()) {
            double tempX = x(),
                   tempY = getY();
            getViewObject()->X.setValue(tempX);
            getViewObject()->Y.setValue(tempY);
        } else {
            getViewObject()->X.setValue(x());
            getViewObject()->Y.setValue(getYInClip(y()));
        }
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIView::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // TODO don't like this but only solution at the minute
    if (isSelected()) {
        m_colCurrent = m_colSel;
    } else {
        m_colCurrent = m_colPre;
        //if(shape().contains(event->pos())) {                     // TODO don't like this for determining preselect
        //    m_colCurrent = m_colPre;
        //}
    }
    drawBorder();
    //update();
}

void QGIView::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if(isSelected()) {
        m_colCurrent = m_colSel;
    } else {
        m_colCurrent = m_colNormal;
    }
    drawBorder();
    //update();
}

void QGIView::setPosition(qreal x, qreal y)
{
    if (!isInnerView()) {
        setPos(x,-y);                                                 //position on page
    } else {
        setPos(x,getYInClip(y));                                      //position in Clip
    }
}

double QGIView::getYInClip(double y)
{
    QGCustomClip* parentClip = dynamic_cast<QGCustomClip*>(parentItem());
    if (parentClip) {
        QGIViewClip* parentView = dynamic_cast<QGIViewClip*>(parentClip->parentItem());
        TechDraw::DrawViewClip* parentFeat = dynamic_cast<TechDraw::DrawViewClip*>(parentView->getViewObject());
        double newY = parentFeat->Height.getValue() - y;
        return newY;
    } else {
        Base::Console().Log("Logic Error - getYInClip called for child (%s) not in Clip\n",getViewName());
    }
    return 0;
}

void QGIView::updateView(bool update)
{
    if (update ||
        getViewObject()->X.isTouched() ||
        getViewObject()->Y.isTouched()) {
        double featX = getViewObject()->X.getValue();
        double featY = getViewObject()->Y.getValue();
        setPosition(featX,featY);
    }

    if (update ||
        getViewObject()->Rotation.isTouched()) {
        //NOTE: QPainterPaths have to be rotated individually. This transform handles everything else.
        double rot = getViewObject()->Rotation.getValue();
        QPointF centre = boundingRect().center();
        setTransform(QTransform().translate(centre.x(), centre.y()).rotate(-rot).translate(-centre.x(), -centre.y()));
    }

    if (update)
        QGraphicsItem::update();
}

const char * QGIView::getViewName() const
{
    return viewName.c_str();
}

TechDraw::DrawView * QGIView::getViewObject() const
{
     return viewObj;
}

void QGIView::setViewFeature(TechDraw::DrawView *obj)
{
    if(obj == 0)
        return;

    viewObj = obj;
    viewName = obj->getNameInDocument();

    // Set the QGIGroup initial position based on the DrawView
    float x = obj->X.getValue();
    float y = obj->Y.getValue();
    setPosition(x, y);

    Q_EMIT dirty();
}

void QGIView::toggleCache(bool state)
{
    // TODO: huh?  IR  //temp for devl. chaching was hiding problems WF
    setCacheMode((state)? NoCache : NoCache);
}


void QGIView::toggleBorder(bool state)
{
    borderVisible = state;
    drawBorder();
}
void QGIView::draw()
{
    if (isVisible()) {
        show();
    } else {
        hide();
    }
}

void QGIView::drawBorder()
{
    if (!borderVisible) {
        return;
    }

    //double margin = 2.0;
    prepareGeometryChange();
    m_label->hide();
    m_border->hide();

    m_label->setDefaultTextColor(m_colCurrent);
    m_label->setFont(m_font);
    QString labelStr = QString::fromUtf8(getViewObject()->Label.getValue());
    m_label->setPlainText(labelStr);
    QRectF labelArea = m_label->boundingRect();
    double labelWidth = m_label->boundingRect().width();
    double labelHeight = m_label->boundingRect().height();

    m_border->hide();
    m_decorPen.setColor(m_colCurrent);
    m_border->setPen(m_decorPen);

    QRectF displayArea = customChildrenBoundingRect();
    double displayWidth = displayArea.width();
    double displayHeight = displayArea.height();

    double frameWidth = displayWidth;
    if (labelWidth > displayWidth) {
        frameWidth = labelWidth;
    }
    double frameHeight = labelHeight + displayHeight;
    QPointF displayCenter = displayArea.center();

    m_label->setX(displayCenter.x() - labelArea.width()/2.);
    m_label->setY(displayArea.bottom());

    QRectF frameArea = QRectF(displayCenter.x() - frameWidth/2.,
                              displayArea.top(),
                              frameWidth,
                              frameHeight);
    m_border->setRect(frameArea);
    m_border->setPos(0.,0.);

    m_label->show();
    m_border->show();
}

void QGIView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    if(!borderVisible){
         m_label->hide();
         m_border->hide();
    }
    QGraphicsItemGroup::paint(painter, &myOption, widget);
}

QRectF QGIView::customChildrenBoundingRect() {
    QList<QGraphicsItem*> children = childItems();
    int dimItemType = QGraphicsItem::UserType + 106;  // TODO: Magic number warning. make include file for custom types?
    int borderItemType = QGraphicsItem::UserType + 136;  // TODO: Magic number warning
    int labelItemType = QGraphicsItem::UserType + 135;  // TODO: Magic number warning
    QRectF result;
    for (QList<QGraphicsItem*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ( ((*it)->type() != dimItemType) &&
             ((*it)->type() != borderItemType) &&
             ((*it)->type() != labelItemType) ) {
            result = result.united((*it)->boundingRect());
        }
    }
    return result;
}

void _debugRect(char* text, QRectF r) {
    Base::Console().Message("TRACE - %s - rect: (%.3f,%.3f) x (%.3f,%.3f)\n",text,
                            r.left(),r.top(),r.right(),r.bottom());
}

#include "moc_QGIView.cpp"
