/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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

  # include <QDebug>
  # include <QGraphicsScene>
  # include <QGraphicsSceneMouseEvent>
  # include <QPainter>
  # include <QPainterPath>
  # include <QPaintDevice>
  # include <QSvgGenerator>
  # include <QApplication>

  # include <cmath>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>
#include <Gui/Tools.h>
#include <string>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
//#include <Mod/TechDraw/App/Preferences.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "PreferencesGui.h"
#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "QGIViewBalloon.h"
#include "ViewProviderBalloon.h"
#include "DrawGuiUtil.h"
#include "QGIViewPart.h"
#include "ViewProviderViewPart.h"
#include "QGIViewDimension.h"
#include "QGVPage.h"
#include "MDIViewPage.h"

//TODO: hide the Qt coord system (+y down).

using namespace TechDraw;
using namespace TechDrawGui;

QGIBalloonLabel::QGIBalloonLabel()
{
    posX = 0;
    posY = 0;
    m_ctrl = false;
    m_drag = false;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);

    m_labelText = new QGCustomText();
    m_labelText->setParentItem(this);

    verticalSep = false;
    hasHover = false;
    parent = nullptr;
}

QVariant QGIBalloonLabel::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            Q_EMIT selected(true);
            setPrettySel();
        } else {
            Q_EMIT selected(false);
            setPrettyNormal();
        }
        update();
    } else if(change == ItemPositionHasChanged && scene()) {
        setLabelCenter();
        if (m_drag) {
            Q_EMIT dragging(m_ctrl);
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void QGIBalloonLabel::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    m_ctrl = false;
    m_drag = true;
    if(event->modifiers() & Qt::ControlModifier) {
        m_ctrl = true;
    }
    QGraphicsItem::mousePressEvent(event);
}

void QGIBalloonLabel::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIBalloonLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton))
        .length() > 0) {
        if (scene() && this == scene()->mouseGrabberItem()) {
            Q_EMIT dragFinished();
        }
    }
    m_ctrl = false;  
    m_drag = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIBalloonLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    QGIViewBalloon* qgivBalloon = dynamic_cast<QGIViewBalloon*>(parentItem());
    if (!qgivBalloon) {
        qWarning() << "QGIBalloonLabel::mouseDoubleClickEvent: No parent item";
        return;
    }

    auto ViewProvider = dynamic_cast<ViewProviderBalloon*>(qgivBalloon->getViewProvider(qgivBalloon->getViewObject()));
    if (!ViewProvider) {
        qWarning() << "QGIBalloonLabel::mouseDoubleClickEvent: No valid view provider";
        return;
    }

    ViewProvider->startDefaultEditMode();
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void QGIBalloonLabel::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_EMIT hover(true);
    hasHover = true;
    if (!isSelected()) {
        setPrettyPre();
    } else {
        setPrettySel();
    }
    QGraphicsItem::hoverEnterEvent(event);
}

void QGIBalloonLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());
    assert(view != nullptr);
    Q_UNUSED(view);

    Q_EMIT hover(false);
    hasHover = false;
    if (!isSelected()) {
        setPrettyNormal();
    } else {
        setPrettySel();
    }
    QGraphicsItem::hoverLeaveEvent(event);
}

QRectF QGIBalloonLabel::boundingRect() const
{
    return childrenBoundingRect();
}

void QGIBalloonLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //QGraphicsObject/QGraphicsItem::paint gives link error.
}

void QGIBalloonLabel::setPosFromCenter(const double &xCenter, const double &yCenter)
{
    //set label's Qt position(top,left) given boundingRect center point
    setPos(xCenter - m_labelText->boundingRect().width() / 2., yCenter - m_labelText->boundingRect().height() / 2.);
}

void QGIBalloonLabel::setLabelCenter()
{
    //save label's bRect center (posX,posY) given Qt position (top,left)
    posX = x() + m_labelText->boundingRect().width() / 2.;
    posY = y() + m_labelText->boundingRect().height() / 2.;
}

void QGIBalloonLabel::setFont(QFont f)
{
    m_labelText->setFont(f);
}

void QGIBalloonLabel::setDimString(QString t)
{
    prepareGeometryChange();
    m_labelText->setPlainText(t);
} 

void QGIBalloonLabel::setDimString(QString t, qreal maxWidth)
{
    prepareGeometryChange();
    m_labelText->setPlainText(t);
    m_labelText->setTextWidth(maxWidth);
}

void QGIBalloonLabel::setPrettySel(void)
{
    m_labelText->setPrettySel();
}

void QGIBalloonLabel::setPrettyPre(void)
{
    m_labelText->setPrettyPre();
}

void QGIBalloonLabel::setPrettyNormal(void)
{
    m_labelText->setPrettyNormal();
}

void QGIBalloonLabel::setColor(QColor c)
{
    m_colNormal = c;
    m_labelText->setColor(m_colNormal);
}

//**************************************************************
QGIViewBalloon::QGIViewBalloon() :
    dvBalloon(nullptr),
    hasHover(false),
    m_lineWidth(0.0),
    m_obtuse(false),
    parent(nullptr),
    m_dragInProgress(false)
{
    m_ctrl = false;

    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    balloonLabel = new QGIBalloonLabel();
    balloonLabel->parent = this;

    addToGroup(balloonLabel);
    balloonLabel->setColor(getNormalColor());
    balloonLabel->setPrettyNormal();

    balloonLines = new QGIDimLines();
    addToGroup(balloonLines);
    balloonLines->setNormalColor(getNormalColor());
    balloonLines->setPrettyNormal();

    balloonShape = new QGIDimLines();
    addToGroup(balloonShape);
    balloonShape->setNormalColor(getNormalColor());
    balloonShape->setFill(Qt::white, Qt::SolidPattern);
    balloonShape->setFillOverride(true);
    balloonShape->setPrettyNormal();
    
    arrow = new QGIArrow();
    addToGroup(arrow);
    arrow->setNormalColor(getNormalColor());
    arrow->setFillColor(getNormalColor());
    arrow->setPrettyNormal();
    arrow->setStyle(prefDefaultArrow());

    balloonLabel->setZValue(ZVALUE::LABEL);
    arrow->setZValue(ZVALUE::DIMENSION);

    balloonLines->setZValue(ZVALUE::DIMENSION);
    balloonLines->setStyle(Qt::SolidLine);

    balloonShape->setZValue(ZVALUE::DIMENSION + 1);    //above balloonLines!
    balloonShape->setStyle(Qt::SolidLine);

    balloonLabel->setPosFromCenter(0, 0);

    // connecting the needed slots and signals
    QObject::connect(
        balloonLabel, SIGNAL(dragging(bool)),
        this  , SLOT  (balloonLabelDragged(bool)));

    QObject::connect(
        balloonLabel, SIGNAL(dragFinished()),
        this  , SLOT  (balloonLabelDragFinished()));

    QObject::connect(
        balloonLabel, SIGNAL(selected(bool)),
        this  , SLOT  (select(bool)));

    QObject::connect(
        balloonLabel, SIGNAL(hover(bool)),
        this  , SLOT  (hover(bool)));

    setZValue(ZVALUE::DIMENSION);
}

QVariant QGIViewBalloon::itemChange(GraphicsItemChange change, const QVariant &value)
{
   if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            balloonLabel->setSelected(true);
        } else {
            balloonLabel->setSelected(false);
        }
        draw();
    }
    return QGIView::itemChange(change, value);
}

//QGIViewBalloon does not behave the same as other QGIView derived classes
//and so mouse events need to be ignored.  Only the QGIBalloonLabel mouse events are relevant.
void QGIViewBalloon::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGIVB::mousePressEvent() - %s\n",getViewName());
    QGraphicsItem::mousePressEvent(event);
}

void QGIViewBalloon::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIViewBalloon::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGIVB::mouseReleaseEvent() - %s\n",getViewName());
    QGraphicsItem::mouseReleaseEvent(event);
}

//Set selection state for this and it's children
void QGIViewBalloon::setGroupSelection(bool b)
{
//    Base::Console().Message("QGIVB::setGroupSelection(%d)\n",b);
    setSelected(b);
    balloonLabel->setSelected(b);
    balloonLines->setSelected(b);
    arrow->setSelected(b);
}

void QGIViewBalloon::select(bool state)
{
//    Base::Console().Message("QGIVBall::select(%d)\n", state);
    setSelected(state);
    draw();
}

void QGIViewBalloon::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewBalloon::setViewPartFeature(TechDraw::DrawViewBalloon *balloon)
{
//    Base::Console().Message("QGIVB::setViewPartFeature()\n");
    if(balloon == nullptr)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(balloon));

    DrawView* balloonParent = nullptr;
    double scale = 1.0;
    App::DocumentObject* docObj = balloon->SourceView.getValue();
    if (docObj) {
        balloonParent = dynamic_cast<DrawView*>(docObj);
        if (balloonParent)
            scale = balloonParent->getScale();
    }

    float x = Rez::guiX(balloon->X.getValue() * scale) ;
    float y = Rez::guiX(-balloon->Y.getValue() * scale);

    balloonLabel->setColor(getNormalColor());
    balloonLabel->setPosFromCenter(x, y);

    QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
    balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));

    updateBalloon();

    draw();
}

void QGIViewBalloon::updateView(bool update)
{
//    Base::Console().Message("QGIVB::updateView()\n");
    Q_UNUSED(update);
    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if (!balloon)
        return;

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp)
        return;

    if (update) {
        QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
        balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
        balloonLabel->setColor(getNormalColor());
        balloonLines->setNormalColor(getNormalColor());
        balloonShape->setNormalColor(getNormalColor());
        arrow->setNormalColor(getNormalColor());
        arrow->setFillColor(getNormalColor());

    }

    updateBalloon();
    draw();
}

//update the bubble contents
void QGIViewBalloon::updateBalloon(bool obtuse)
{
//    Base::Console().Message("QGIVB::updateBalloon()\n");
    (void) obtuse;
    const auto balloon( dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject()) );
    if (!balloon) {
        return;
    }
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    const TechDraw::DrawViewPart *refObj = balloon->getViewPart();
    if (!refObj) {
        return;
    }

    QFont font = balloonLabel->getFont();
    font.setPixelSize(calculateFontPixelSize(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    balloonLabel->setFont(font);

    QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
    balloonLabel->verticalSep = false;
    balloonLabel->seps.clear();

    if (strcmp(balloon->BubbleShape.getValueAsString(), "Rectangle") == 0) {
        while (labelText.contains(QString::fromUtf8("|"))) {
            int pos = labelText.indexOf(QString::fromUtf8("|"));
            labelText.replace(pos, 1, QString::fromUtf8("   "));
            QFontMetrics fm(balloonLabel->getFont());
            balloonLabel->seps.push_back(Gui::QtTools::horizontalAdvance(fm, labelText.left(pos + 2)));
            balloonLabel->verticalSep = true;
        }
    }

    balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
    float x = Rez::guiX(balloon->X.getValue() * refObj->getScale());
    float y = Rez::guiX(balloon->Y.getValue() * refObj->getScale());
    balloonLabel->setPosFromCenter(x, -y);
}

void QGIViewBalloon::balloonLabelDragged(bool ctrl)
{
    m_ctrl = ctrl;
    auto dvb( dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject()) );
    if (!dvb)
        return;

    if (!m_dragInProgress) {           //first drag movement
        m_dragInProgress = true;
        if (ctrl) {             //moving whole thing, remember Origin offset from Bubble
            m_saveOffset = dvb->getOriginOffset();
        }
    }

    // store if origin is also moving to be able to later calc new origin and update feature
    if (ctrl)
        m_originDragged = true;

    DrawView* balloonParent = getSourceView();
    if (balloonParent)
        // redraw the balloon at the new position
        // note that we don't store the new position to the X/Y properties
        // since the dragging is not yet finished
        drawBalloon(true); 
}

void QGIViewBalloon::balloonLabelDragFinished()
{
    // stores the final drag position for undo

    auto dvb(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!dvb)
        return;

    double scale = 1.0;
    DrawView* balloonParent = getSourceView();
    if (balloonParent)
        scale = balloonParent->getScale();

    //set feature position (x,y) from graphic position
    double x = Rez::appX(balloonLabel->X() / scale),
           y = Rez::appX(balloonLabel->Y() / scale);
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Drag Balloon"));
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.X = %f", dvb->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Y = %f", dvb->getNameInDocument(), -y);

    // for the case that origin was also dragged, calc new origin and update feature
    if (m_originDragged) {
        Base::Vector3d pos(x, -y, 0.0);
        Base::Vector3d newOrg = pos - m_saveOffset;
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.OriginX = %f",
                                dvb->getNameInDocument(), newOrg.x);
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.OriginY = %f",
                                dvb->getNameInDocument(), newOrg.y);
    }
    
    Gui::Command::commitCommand();

    m_dragInProgress = false;
    m_originDragged = false;
}

//from QGVP::mouseReleaseEvent - pos = eventPos in scene coords?
void QGIViewBalloon::placeBalloon(QPointF pos)
{
//    Base::Console().Message("QGIVB::placeBalloon(%s)\n",
//                            DrawUtil::formatVector(pos).c_str());
    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if (!balloon) {
        return;
    }

    DrawView* balloonParent = dynamic_cast<DrawView*>(balloon->SourceView.getValue());
    if (!balloonParent) {
        return;
    }
    
    auto featPage = balloonParent->findParentPage();
    if (!featPage) {
        return;
    }
    
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    
    QGIView* qgivParent = nullptr;
    QPointF viewPos;
    Gui::ViewProvider* objVp = QGIView::getViewProvider(balloonParent);
    auto partVP = dynamic_cast<ViewProviderViewPart*>(objVp);
    if (partVP) {
        qgivParent = partVP->getQView();
        if (qgivParent) {
        //tip position is mouse release pos in parentView coords ==> OriginX, OriginY
        //bubble pos is some arbitrary shift from tip position ==> X,Y
            viewPos = qgivParent->mapFromScene(pos);
            balloon->OriginX.setValue(Rez::appX(viewPos.x()) / balloonParent->getScale());
            balloon->OriginY.setValue(-Rez::appX(viewPos.y()) / balloonParent->getScale());
            balloon->X.setValue(Rez::appX((viewPos.x() +  200.0) / balloonParent->getScale() ));
            balloon->Y.setValue(- Rez::appX((viewPos.y() - 200.0) / balloonParent->getScale() ));
        }
    }

    int idx = featPage->getNextBalloonIndex();
    QString labelText = QString::number(idx);
    balloon->Text.setValue(std::to_string(idx).c_str());
 
    QFont font = balloonLabel->getFont();
    font.setPixelSize(calculateFontPixelSize(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    balloonLabel->setFont(font);
    prepareGeometryChange();

    // Default label position
    balloonLabel->setPosFromCenter(viewPos.x() + 200, viewPos.y() - 200);
    balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));

    draw();
}

void QGIViewBalloon::draw()
{
    // just redirect
    drawBalloon(false);
}

void QGIViewBalloon::drawBalloon(bool dragged)
{
//    Base::Console().Message("QGIVB::draw()\n");
    if (!isVisible()) {
        return;
    }

    TechDraw::DrawViewBalloon *balloon = dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject());
    if((!balloon) ||                                                       //nothing to draw, don't try
       (!balloon->isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId()))) {
        balloonLabel->hide();
        hide();
        return;
    }

    balloonLabel->show();
    show();

    const TechDraw::DrawViewPart *refObj = balloon->getViewPart();
    if (!refObj) {
        return;
    }
    if(!refObj->hasGeometry()) { // nothing to draw yet (restoring)
        balloonLabel->hide();
        hide();
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());

    double textWidth = balloonLabel->getDimText()->boundingRect().width();
    double textHeight = balloonLabel->getDimText()->boundingRect().height();
    float x, y, arrowTipX, arrowTipY;
    // when not dragging take the X/Y properties otherwise the current label position
    if (!dragged) {
        x = Rez::guiX(balloon->X.getValue() * refObj->getScale());
        y = Rez::guiX(balloon->Y.getValue() * refObj->getScale());
        arrowTipX = Rez::guiX(balloon->OriginX.getValue() * refObj->getScale());
        arrowTipY = -Rez::guiX(balloon->OriginY.getValue() * refObj->getScale());
    }
    else {
        x = balloonLabel->X();
        y = -balloonLabel->Y();
        if (m_originDragged) {
            double scale = Rez::guiX(refObj->getScale());
            Base::Vector3d pos(x / scale, y / scale, 0.0);
            Base::Vector3d newOrg = pos - m_saveOffset;
            arrowTipX = newOrg.x * scale;
            arrowTipY = -newOrg.y * scale;
        }
        else {
            arrowTipX = Rez::guiX(balloon->OriginX.getValue() * refObj->getScale());
            arrowTipY = -Rez::guiX(balloon->OriginY.getValue() * refObj->getScale());
        }
    }
    Base::Vector3d lblCenter(x, -y, 0.0);

    if (balloon->isLocked()) {
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, false);
    } else
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, true);

    Base::Vector3d dLineStart;
    Base::Vector3d kinkPoint;
    double kinkLength = Rez::guiX(balloon->KinkLength.getValue());

    const char *balloonType = balloon->BubbleShape.getValueAsString();

    float scale = balloon->ShapeScale.getValue();
    double offsetLR     = 0;
    double offsetUD     = 0;
    QPainterPath balloonPath;

    if (strcmp(balloonType, "Circular") == 0) {
        double balloonRadius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        balloonRadius = balloonRadius * scale;
        balloonPath.moveTo(lblCenter.x, lblCenter.y);
        balloonPath.addEllipse(lblCenter.x - balloonRadius,lblCenter.y - balloonRadius, balloonRadius * 2, balloonRadius * 2);
        offsetLR     = balloonRadius;
    } else if (strcmp(balloonType, "None") == 0) {
        balloonPath = QPainterPath();
        offsetLR     = (textWidth / 2.0) + Rez::guiX(2.0);
    } else if (strcmp(balloonType, "Rectangle") == 0) {
        //Add some room
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        // we add some textWidth later because we first need to handle the text separators 
        if (balloonLabel->verticalSep) {
            for (std::vector<int>::iterator it = balloonLabel->seps.begin() ; it != balloonLabel->seps.end(); ++it) {
                balloonPath.moveTo(lblCenter.x - (textWidth / 2.0) + *it, lblCenter.y - (textHeight / 2.0));
                balloonPath.lineTo(lblCenter.x - (textWidth / 2.0) + *it, lblCenter.y + (textHeight / 2.0));
            }
        }
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        balloonPath.addRect(lblCenter.x - (textWidth / 2.0), lblCenter.y - (textHeight / 2.0), textWidth, textHeight);
        offsetLR     = (textWidth / 2.0);
    } else if (strcmp(balloonType, "Triangle") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(3.0);
        offsetLR     = (tan(30 * M_PI / 180) * radius);
        QPolygonF triangle;
        double startAngle = -M_PI / 2;
        double angle = startAngle;
        for (int i = 0; i < 4; i++) {
            triangle += QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * M_PI / 3);
        }
        balloonPath.moveTo(lblCenter.x + (radius * cos(startAngle)), lblCenter.y + (radius * sin(startAngle)));
        balloonPath.addPolygon(triangle);
    } else if (strcmp(balloonType, "Inspection") == 0) {
        //Add some room
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        QPointF textBoxCorner(lblCenter.x - (textWidth / 2.0), lblCenter.y - (textHeight / 2.0));
        balloonPath.moveTo(textBoxCorner);
        balloonPath.lineTo(textBoxCorner.x() + textWidth, textBoxCorner.y());
        balloonPath.arcTo(textBoxCorner.x() + textWidth - (textHeight / 2.0), textBoxCorner.y(), textHeight, textHeight, 90, -180);
        balloonPath.lineTo(textBoxCorner.x(), textBoxCorner.y() + textHeight);
        balloonPath.arcTo(textBoxCorner.x() - (textHeight / 2), textBoxCorner.y(), textHeight, textHeight, -90, -180);
        offsetLR     = (textWidth / 2.0) + (textHeight / 2.0);
    } else if (strcmp(balloonType, "Square") == 0) {
        //Add some room
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        double max = std::max(textWidth, textHeight);
        balloonPath.addRect(lblCenter.x -(max / 2.0), lblCenter.y - (max / 2.0), max, max);
        offsetLR     = (max / 2.0);
    } else if (strcmp(balloonType, "Hexagon") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(1.0);
        offsetLR     = radius;
        QPolygonF triangle;
        double startAngle = -2 * M_PI / 3;
        double angle = startAngle;
        for (int i = 0; i < 7; i++) {
            triangle += QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * M_PI / 6);
        }
        balloonPath.moveTo(lblCenter.x + (radius * cos(startAngle)), lblCenter.y + (radius * sin(startAngle)));
        balloonPath.addPolygon(triangle);
    }
    else if (strcmp(balloonType, "Line") == 0) {
        textHeight = textHeight*scale + Rez::guiX(0.5);
        textWidth = textWidth*scale + Rez::guiX(1.0);

        offsetLR = textWidth/2.0;
        offsetUD = textHeight/2.0;

        balloonPath.moveTo(lblCenter.x - textWidth/2.0, lblCenter.y + offsetUD);
        balloonPath.lineTo(lblCenter.x + textWidth/2.0, lblCenter.y + offsetUD);
    }

    balloonShape->setPath(balloonPath);

    offsetLR     = (lblCenter.x < arrowTipX) ? offsetLR     : -offsetLR    ;

    if (DrawUtil::fpCompare(kinkLength, 0.0) && strcmp(balloonType, "Line")) {   //if no kink, then dLine start sb on line from center to arrow
        dLineStart = lblCenter;
        kinkPoint = dLineStart;
    } else {
        dLineStart.y = lblCenter.y + offsetUD;
        dLineStart.x = lblCenter.x + offsetLR    ;
        kinkLength = (lblCenter.x < arrowTipX) ? kinkLength : -kinkLength;
        kinkPoint.y = dLineStart.y;
        kinkPoint.x = dLineStart.x + kinkLength;
    }

    QPainterPath dLinePath;
    dLinePath.moveTo(dLineStart.x, dLineStart.y);
    dLinePath.lineTo(kinkPoint.x, kinkPoint.y);

    double xAdj = 0.0;
    double yAdj = 0.0;
    int endType = balloon->EndType.getValue();
    double arrowAdj = QGIArrow::getOverlapAdjust(endType,
                                                 balloon->EndTypeScale.getValue()*QGIArrow::getPrefArrowSize());

    if (endType == ArrowType::NONE) {
        arrow->hide();
    } else {
        arrow->setStyle(endType);

        arrow->setSize(balloon->EndTypeScale.getValue()*QGIArrow::getPrefArrowSize());
        arrow->draw();

        Base::Vector3d arrowTipPos(arrowTipX, arrowTipY, 0.0);
        Base::Vector3d dirballoonLinesLine;
        if (!DrawUtil::fpCompare(kinkLength, 0.0)) {
            dirballoonLinesLine = (arrowTipPos - kinkPoint).Normalize();
        } else {
            dirballoonLinesLine = (arrowTipPos - dLineStart).Normalize();
        }

        float arAngle = atan2(dirballoonLinesLine.y, dirballoonLinesLine.x) * 180 / M_PI;

        arrow->setPos(arrowTipX, arrowTipY);
        if ( (endType == ArrowType::FILLED_TRIANGLE) &&
             (prefOrthoPyramid()) ) {
            if (arAngle < 0.0) {
                arAngle += 360.0;
            }
            //set the angle to closest cardinal direction
            if ( (45.0 < arAngle) && (arAngle < 135.0) ) {
                arAngle = 90.0;
            } else if ( (135.0 < arAngle) && (arAngle < 225.0) ) {
                arAngle = 180.0;
            } else if ( (225.0 < arAngle) && (arAngle < 315.0) ) {
                arAngle = 270.0;
            } else {
                arAngle = 0;
            }
            double radAngle = arAngle * M_PI / 180.0;
            double sinAngle = sin(radAngle);
            double cosAngle = cos(radAngle);
            xAdj = Rez::guiX(arrowAdj * cosAngle);
            yAdj = Rez::guiX(arrowAdj * sinAngle);
        }
        arrow->setRotation(arAngle);
        arrow->show();
    }
    dLinePath.lineTo(arrowTipX - xAdj, arrowTipY - yAdj);
    balloonLines->setPath(dLinePath);

    // This overwrites the previously created QPainterPath with empty one, in case it should be hidden.  Should be refactored.
    if (!vp->LineVisible.getValue()) {
        arrow->hide();
        balloonLines->setPath(QPainterPath());
    }

    // redraw the Balloon and the parent View
    if (hasHover && !isSelected()) {
        setPrettyPre();
    } else if (isSelected()) {
        setPrettySel();
    } else {
        setPrettyNormal();
    }

    if (parentItem()) {
        parentItem()->update();
    } else {
        Base::Console().Log("INFO - QGIVB::draw - no parent to update\n");
    }
}

void QGIViewBalloon::setPrettyPre(void)
{
    arrow->setPrettyPre();
    //TODO: primPath needs override for fill
    //balloonShape->setFillOverride(true);   //don't fill with pre or select colours.
//    balloonShape->setFill(Qt::white, Qt::NoBrush);
    balloonShape->setPrettyPre();
    balloonLines->setPrettyPre();
}

void QGIViewBalloon::setPrettySel(void)
{
//    Base::Console().Message("QGIVBal::setPrettySel()\n");
    arrow->setPrettySel();
//    balloonShape->setFill(Qt::white, Qt::NoBrush);
    balloonShape->setPrettySel();
    balloonLines->setPrettySel();
}

void QGIViewBalloon::setPrettyNormal(void)
{
    arrow->setPrettyNormal();
//    balloonShape->setFill(Qt::white, Qt::SolidPattern);
    balloonShape->setPrettyNormal();
    balloonLines->setPrettyNormal();
}


void QGIViewBalloon::drawBorder(void)
{
//Dimensions have no border!
//    Base::Console().Message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
}

void QGIViewBalloon::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    setPens();
    if (svg) {
        setSvgPens();
    } else {
        setPens();
    }
    QGIView::paint (painter, &myOption, widget);
    setPens();
}

void QGIViewBalloon::setSvgPens(void)
{
    double svgLineFactor = 3.0;                     //magic number.  should be a setting somewhere.
    balloonLines->setWidth(m_lineWidth/svgLineFactor);
    balloonShape->setWidth(m_lineWidth/svgLineFactor);
    arrow->setWidth(arrow->getWidth()/svgLineFactor);
}

void QGIViewBalloon::setPens(void)
{
    balloonLines->setWidth(m_lineWidth);
    balloonShape->setWidth(m_lineWidth);
    arrow->setWidth(m_lineWidth);
}

QColor QGIViewBalloon::getNormalColor()
{
    m_colNormal = PreferencesGui::dimQColor();

    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if(!balloon)
        return m_colNormal;

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return m_colNormal;
    }

    m_colNormal = vp->Color.getValue().asValue<QColor>();
    return m_colNormal;
}

int QGIViewBalloon::prefDefaultArrow() const
{
    return Preferences::balloonArrow();
}


//should this be an object property or global preference?
//when would you want a crooked pyramid?
bool QGIViewBalloon::prefOrthoPyramid() const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->
                                        GetGroup("Mod/TechDraw/Decorations");
    bool ortho = hGrp->GetBool("PyramidOrtho", true);
    return ortho;
}

DrawView* QGIViewBalloon::getSourceView() const
{
    DrawView* balloonParent = nullptr;
    App::DocumentObject* docObj = getViewObject();
    DrawViewBalloon* dvb = dynamic_cast<DrawViewBalloon*>(docObj);
    if (dvb) {
        balloonParent = dynamic_cast<DrawView*>(dvb->SourceView.getValue());
    }
    return balloonParent;
}

#include <Mod/TechDraw/Gui/moc_QGIViewBalloon.cpp>
