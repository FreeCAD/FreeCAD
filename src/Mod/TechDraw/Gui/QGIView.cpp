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
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"
#include "QGCustomBorder.h"
#include "QGCustomLabel.h"
#include "QGCustomBorder.h"
#include "QGCustomLabel.h"
#include "QGCustomText.h"
#include "QGICaption.h"
#include "QGCustomClip.h"
#include "QGCustomImage.h"
#include "QGIViewClip.h"
#include "ViewProviderDrawingView.h"
#include "MDIViewPage.h"
#include "QGICMark.h"

#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIView.h"

using namespace TechDrawGui;

const float labelCaptionFudge = 0.2f;   // temp fiddle for devel

QGIView::QGIView()
    :QGraphicsItemGroup(),
     viewObj(nullptr),
     m_locked(false),
     borderVisible(true),
     m_innerView(false)
{
    setCacheMode(QGraphicsItem::NoCache);
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);


    m_colCurrent = getNormalColor();
    m_pen.setColor(m_colCurrent);

    //Border/Label styling
    m_font.setPointSize(getPrefFontSize());     //scene units (mm), not points

    m_decorPen.setStyle(Qt::DashLine);
    m_decorPen.setWidth(0); // 0 => 1px "cosmetic pen"

    m_label = new QGCustomLabel();
    addToGroup(m_label);
    m_border = new QGCustomBorder();
    addToGroup(m_border);
    m_caption = new QGICaption();
    addToGroup(m_caption);
    m_lock = new QGCustomImage();
    m_lock->setParentItem(m_label);
    m_lock->load(QString::fromUtf8(":/icons/techdraw-lock.png"));
    QSize sizeLock = m_lock->imageSize();
    m_lockWidth = (double) sizeLock.width();
    m_lockHeight = (double) sizeLock.height();
    m_lock->hide();

    isVisible(true);
}

QGIView::~QGIView()
{
    signalSelectPoint.disconnect_all_slots();
}

void QGIView::alignTo(QGraphicsItem*item, const QString &alignment)
{
    alignHash.clear();
    alignHash.insert(alignment, item);
}

QVariant QGIView::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QPointF newPos(0.0,0.0);
    if(change == ItemPositionChange && scene()) {
//    if(change == ItemPositionHasChanged && scene()) {
        newPos = value.toPointF();            //position within parent!
        if(m_locked){
            newPos.setX(pos().x());
            newPos.setY(pos().y());
            return newPos;
        }
        
        // TODO  find a better data structure for this
        // this is just a pair isn't it?
        if (getViewObject()->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(getViewObject());
            TechDraw::DrawProjGroup* dpg = dpgi->getPGroup();
            if (dpg != nullptr) {
                if(alignHash.size() == 1) {   //if aligned.
                    QGraphicsItem* item = alignHash.begin().value();
                    QString alignMode   = alignHash.begin().key();
                    if(alignMode == QString::fromLatin1("Vertical")) {
                        newPos.setX(item->pos().x());
                    } else if(alignMode == QString::fromLatin1("Horizontal")) {
                        newPos.setY(item->pos().y());
                    } else if(alignMode == QString::fromLatin1("45slash")) {
                         //this logic is wrong since the constained movement direction is not necessarily 45*
//                         Base::Console().Message("QGIV::itemChange - oblique BL-TR\n");
//                        double dist = ( (newPos.x() - item->pos().x()) +
//                                        (item->pos().y() - newPos.y()) ) / 2.0;

//                        newPos.setX( item->pos().x() + dist);
//                        newPos.setY( item->pos().y() - dist );
                    } else if(alignMode == QString::fromLatin1("45backslash")) {
                         //this logic is wrong since the constained movement direction is not necessarily 45*
//                         Base::Console().Message("QGIV::itemChange - oblique TL-BR\n");
//                        double dist = ( (newPos.x() - item->pos().x()) +
//                                        (newPos.y() - item->pos().y()) ) / 2.0;

//                        newPos.setX( item->pos().x() + dist);
//                        newPos.setY( item->pos().y() + dist );
                    }
                }
            }
        }
        return newPos;
    }

    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            m_colCurrent = getSelectColor();
        } else {
            m_colCurrent = getNormalColor();
        }
        drawBorder();
    }

    return QGraphicsItemGroup::itemChange(change, value);
}

void QGIView::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    signalSelectPoint(this, event->pos());

    if(m_locked) {
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
    if(!m_locked) {
        if (!isInnerView()) {
            double tempX = x(),
                   tempY = getY();
            getViewObject()->setPosition(Rez::appX(tempX),Rez::appX(tempY));
        } else {
            getViewObject()->setPosition(Rez::appX(x()),Rez::appX(getYInClip(y())));
        }
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIView::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    // TODO don't like this but only solution at the minute (MLP)
    if (isSelected()) {
        m_colCurrent = getSelectColor();
    } else {
        m_colCurrent = getPreColor();
    }
    drawBorder();
}

void QGIView::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    if(isSelected()) {
        m_colCurrent = getSelectColor();
    } else {
        m_colCurrent = getNormalColor();
    }
    drawBorder();
}

void QGIView::setPosition(qreal x, qreal y)
{
    double newX = x;
    double newY;
    double oldX = pos().x();
    double oldY = pos().y();
    if (!isInnerView()) {
        newY = -y;
    } else {
        newY = getYInClip(y);
    }
    if ( (TechDraw::DrawUtil::fpCompare(newX,oldX)) &&
         (TechDraw::DrawUtil::fpCompare(newY,oldY)) ) {
        return;
    } else {
        setPos(newX,newY);
    }
}

//is this needed anymore???
double QGIView::getYInClip(double y)
{
    return -y;
}

QGIViewClip* QGIView::getClipGroup(void)
{
    if (!getViewObject()->isInClip()) {
        Base::Console().Log( "Logic Error - getClipGroup called for child "
                         "(%s) not in Clip\n", getViewName() );
        return nullptr;
    }
    
    QGIViewClip* result = nullptr;
    auto parentClip( dynamic_cast<QGCustomClip*>( parentItem() ) );
    if (parentClip) {
        auto parentView( dynamic_cast<QGIViewClip*>( parentClip->parentItem() ) );
        if (parentView) {
            result = parentView;
        }
    }
    return result;
}

void QGIView::updateView(bool update)
{
//    Base::Console().Message("QGIV::updateView() - %s\n",getViewObject()->getNameInDocument());
    (void) update;
    if (getViewObject()->isLocked()) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
    } else {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    if (getViewObject()->X.isTouched() ||                   //change in feat position
        getViewObject()->Y.isTouched()) {
        double featX = Rez::guiX(getViewObject()->X.getValue());
        double featY = Rez::guiX(getViewObject()->Y.getValue());
        setPosition(featX,featY);
    }

    double appRotation = getViewObject()->Rotation.getValue();
    double guiRotation = rotation();
    if (!TechDraw::DrawUtil::fpCompare(appRotation,guiRotation)) {
        rotateView();
    }

    QGIView::draw();
}

//QGIVP derived classes do not need a rotate view method as rotation is handled on App side.
void QGIView::rotateView(void)
{
//NOTE: QPainterPaths have to be rotated individually. This transform handles Rotation for everything else.
//Scale is handled in GeometryObject for DVP & descendents
//Objects not descended from DVP must setScale for themselves
//note that setTransform(,,rotation,,) is not the same as setRotation!!!
    double rot = getViewObject()->Rotation.getValue();
    QPointF centre = boundingRect().center();
    setTransform(QTransform().translate(centre.x(), centre.y()).rotate(-rot).translate(-centre.x(), -centre.y()));
}

const char * QGIView::getViewName() const
{
    return viewName.c_str();
}
const std::string QGIView::getViewNameAsString() const
{
    return viewName;
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

    //mark the actual QGraphicsItem so we can check what's in the scene later
    setData(0,QString::fromUtf8("QGIV"));
    setData(1,QString::fromUtf8(obj->getNameInDocument()));
}

void QGIView::toggleCache(bool state)
{
    // temp for devl. chaching was hiding problems WF
    //setCacheMode((state)? NoCache : NoCache);
    Q_UNUSED(state);
    setCacheMode(NoCache);
}

void QGIView::toggleBorder(bool state)
{
    borderVisible = state;
    QGIView::draw();
}

void QGIView::draw()
{
    if (isVisible()) {
        drawBorder();
        show();
    } else {
        hide();
    }
}

void QGIView::drawCaption()
{
    prepareGeometryChange();
    QRectF displayArea = customChildrenBoundingRect();
    m_caption->setDefaultTextColor(m_colCurrent);
    m_font.setFamily(getPrefFont());
    m_font.setPointSize(getPrefFontSize());     //scene units (0.1 mm), not points
    m_caption->setFont(m_font);
    QString captionStr = QString::fromUtf8(getViewObject()->Caption.getValue());
    m_caption->setPlainText(captionStr);
    QRectF captionArea = m_caption->boundingRect();
    QPointF displayCenter = displayArea.center();
    m_caption->setX(displayCenter.x() - captionArea.width()/2.);
    double labelHeight = (1 - labelCaptionFudge) * m_label->boundingRect().height();
    auto vp = static_cast<ViewProviderDrawingView*>(getViewProvider(getViewObject()));
    if (borderVisible || vp->KeepLabel.getValue()) {            //place below label if label visible
        m_caption->setY(displayArea.bottom() + labelHeight);
    } else {
        m_caption->setY(displayArea.bottom() + labelCaptionFudge * getPrefFontSize());
    }
    m_caption->show();
}

void QGIView::drawBorder()
{
    auto feat = getViewObject();
    if (feat == nullptr) {
        return;
    }
    
    drawCaption();
    //show neither
    auto vp = static_cast<ViewProviderDrawingView*>(getViewProvider(getViewObject()));
    if (!borderVisible && !vp->KeepLabel.getValue()) {
         m_label->hide();
         m_border->hide();
         m_lock->hide();
        return;
    }

    //show both or show label
    //double margin = 2.0;
    m_label->hide();
    m_border->hide();
    m_lock->hide();

    m_label->setDefaultTextColor(m_colCurrent);
    m_font.setFamily(getPrefFont());
    m_font.setPointSize(getPrefFontSize());     //scene units (0.1 mm), not points
    m_label->setFont(m_font);
    QString labelStr = QString::fromUtf8(getViewObject()->Label.getValue());
    m_label->setPlainText(labelStr);
    QRectF labelArea = m_label->boundingRect();
    double labelWidth = m_label->boundingRect().width();
    double labelHeight = (1 - labelCaptionFudge) * m_label->boundingRect().height();

    QBrush b(Qt::NoBrush);
    m_border->setBrush(b);
    m_decorPen.setColor(m_colCurrent);
    m_border->setPen(m_decorPen);

    QRectF displayArea = customChildrenBoundingRect();
    double displayWidth = displayArea.width();
    double displayHeight = displayArea.height();
    QPointF displayCenter = displayArea.center();
    m_label->setX(displayCenter.x() - labelArea.width()/2.);
    m_label->setY(displayArea.bottom());

    double frameWidth = displayWidth;
    if (labelWidth > displayWidth) {
        frameWidth = labelWidth;
    }
    double frameHeight = labelHeight + displayHeight;

    QRectF frameArea = QRectF(displayCenter.x() - frameWidth/2.,
                              displayArea.top(),
                              frameWidth,
                              frameHeight);
 
    double lockX = labelArea.left();
    double lockY = labelArea.bottom() - (2 * m_lockHeight);
    if (feat->isLocked()) {
        m_lock->setZValue(ZVALUE::LOCK);
        m_lock->setPos(lockX,lockY);
        m_lock->show();
    } else {
        m_lock->hide();
    }

    prepareGeometryChange();
    m_border->setRect(frameArea.adjusted(-2,-2,2,2));
    m_border->setPos(0.,0.);

    m_label->show();
    if (borderVisible) {
        m_border->show();
    }
}

void QGIView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGraphicsItemGroup::paint(painter, &myOption, widget);
}

QRectF QGIView::customChildrenBoundingRect() const
{
    QList<QGraphicsItem*> children = childItems();
    int dimItemType = QGraphicsItem::UserType + 106;  // TODO: Magic number warning.
    int borderItemType = QGraphicsItem::UserType + 136;  // TODO: Magic number warning
    int labelItemType = QGraphicsItem::UserType + 135;  // TODO: Magic number warning
    int captionItemType = QGraphicsItem::UserType + 180;  // TODO: Magic number warning
    QRectF result;
    for (QList<QGraphicsItem*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ( ((*it)->type() != dimItemType) &&
             ((*it)->type() != borderItemType) &&
             ((*it)->type() != labelItemType)  &&
             ((*it)->type() != captionItemType) ) {
            QRectF childRect = mapFromItem(*it,(*it)->boundingRect()).boundingRect();
            result = result.united(childRect);
            //result = result.united((*it)->boundingRect());
        }
    }
    return result;
}

QRectF QGIView::boundingRect() const
{
    return m_border->rect().adjusted(-2.,-2.,2.,2.);     //allow for border line width  //TODO: fiddle brect if border off?
}

QGIView* QGIView::getQGIVByName(std::string name)
{
    QList<QGraphicsItem*> qgItems = scene()->items();
    QList<QGraphicsItem*>::iterator it = qgItems.begin();
    for (; it != qgItems.end(); it++) {
        QGIView* qv = dynamic_cast<QGIView*>((*it));
        if (qv) {
            const char* qvName = qv->getViewName();
            if(name.compare(qvName) == 0) {
                return (qv);
            }
        }
    }
    return 0;
}

/* static */
Gui::ViewProvider* QGIView::getViewProvider(App::DocumentObject* obj)
{
    Gui::ViewProvider* result = nullptr;
    if (obj != nullptr) {
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(obj->getDocument());
        result = guiDoc->getViewProvider(obj);
    }
    return result;
}

MDIViewPage* QGIView::getMDIViewPage(void) const
{
    MDIViewPage* result = nullptr;
    QGraphicsScene* s = scene();
    QObject* parent = nullptr;
    if (s != nullptr) {
        parent = s->parent();
    }
    if (parent != nullptr) {
        MDIViewPage* mdi = dynamic_cast<MDIViewPage*>(parent);
        if (mdi != nullptr) {
            result = mdi;
        }
    }
    return result;
}

QColor QGIView::getNormalColor()
{
    Base::Reference<ParameterGrp> hGrp = getParmGroupCol();
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();
    return m_colNormal;
}

QColor QGIView::getPreColor()
{
    Base::Reference<ParameterGrp> hGrp = getParmGroupCol();
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0xFFFF0000));
    m_colPre = fcColor.asValue<QColor>();
    return m_colPre;
}

QColor QGIView::getSelectColor()
{
    Base::Reference<ParameterGrp> hGrp = getParmGroupCol();
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x00FF0000));
    m_colSel = fcColor.asValue<QColor>();
    return m_colSel;
}

Base::Reference<ParameterGrp> QGIView::getParmGroupCol()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    return hGrp;
}

QString QGIView::getPrefFont()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Labels");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");
    return QString::fromStdString(fontName);
}

double QGIView::getPrefFontSize()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Labels");
    double fontSize = hGrp->GetFloat("LabelSize", 3.5);
    return Rez::guiX(fontSize);
}

void QGIView::dumpRect(char* text, QRectF r) {
    Base::Console().Message("DUMP - %s - rect: (%.3f,%.3f) x (%.3f,%.3f)\n",text,
                            r.left(),r.top(),r.right(),r.bottom());
}

void QGIView::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x,y);
    cmItem->setThick(1.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

void QGIView::makeMark(Base::Vector3d v)
{
    makeMark(v.x,v.y);
}

#include <Mod/TechDraw/Gui/moc_QGIView.cpp>
