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

  # include <QGraphicsScene>
  # include <QGraphicsSceneMouseEvent>
  # include <QPainter>
  # include <QPaintDevice>
  # include <QSvgGenerator>
//  # include <QApplication>

  # include <cmath>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <string>

#include <Mod/TechDraw/App/DrawViewGDTReference.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "Rez.h"
#include "ZVALUE.h"

#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "QGIViewGDTReference.h"
#include "ViewProviderGDTReference.h"
#include "DrawGuiUtil.h"

//TODO: hide the Qt coord system (+y down).  

using namespace TechDraw;
using namespace TechDrawGui;

QGIReferenceLabel::QGIReferenceLabel(QGIViewGDTReference * parent)
{
	this->parent = parent;

	posX = 0;
    posY = 0;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);

    m_labelText = new QGCustomText();
    m_labelText->setParentItem(this);

    m_ctrl = false;
    hasHover = false;
}

QVariant QGIReferenceLabel::itemChange(GraphicsItemChange change, const QVariant &value)
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
        Q_EMIT dragging(m_ctrl);
    }

    return QGraphicsItem::itemChange(change, value);
}

void QGIReferenceLabel::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if(event->modifiers() & Qt::ControlModifier) {
        m_ctrl = true;
    }

    if(scene() && this == scene()->mouseGrabberItem()) {
        Q_EMIT dragFinished();
    }
      QGraphicsItem::mousePressEvent(event);
}

void QGIReferenceLabel::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIReferenceLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    m_ctrl = false;
    if(scene() && this == scene()->mouseGrabberItem()) {
        Q_EMIT dragFinished();
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIReferenceLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    //Gui::Control().showDialog(new TaskDlgReference(parent));
    //QGraphicsItem::mouseDoubleClickEvent(event);
}

void QGIReferenceLabel::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
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

void QGIReferenceLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());
    assert(view != 0);
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

QRectF QGIReferenceLabel::boundingRect() const
{
    return childrenBoundingRect();
}

void QGIReferenceLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //QGraphicsObject/QGraphicsItem::paint gives link error.
}

void QGIReferenceLabel::setPosFromCenter(const double &xCenter, const double &yCenter)
{
    //set label's Qt position(top,left) given boundingRect center point
    setPos(xCenter - m_labelText->boundingRect().width() / 2., yCenter - m_labelText->boundingRect().height() / 2.);
}

void QGIReferenceLabel::setLabelCenter()
{
    //save label's bRect center (posX,posY) given Qt position (top,left)
    posX = x() + m_labelText->boundingRect().width() / 2.;
    posY = y() + m_labelText->boundingRect().height() / 2.;
}

void QGIReferenceLabel::setFont(QFont f)
{
    m_labelText->setFont(f);
}

void QGIReferenceLabel::setLabelString(QString t)
{
    prepareGeometryChange();
    m_labelText->setPlainText(t);
} 

void QGIReferenceLabel::setLabelString(QString t, qreal maxWidth)
{
    prepareGeometryChange();
    m_labelText->setPlainText(t);
    m_labelText->setTextWidth(maxWidth);
}

void QGIReferenceLabel::setPrettySel(void)
{
    m_labelText->setPrettySel();
}

void QGIReferenceLabel::setPrettyPre(void)
{
    m_labelText->setPrettyPre();
}

void QGIReferenceLabel::setPrettyNormal(void)
{
    m_labelText->setPrettyNormal();
}

void QGIReferenceLabel::setColor(QColor c)
{
    m_colNormal = c;
    m_labelText->setColor(m_colNormal);
}

//**************************************************************
// QGI VIEW REFERENCE
//**************************************************************
QGIViewGDTReference::QGIViewGDTReference() :
    hasHover(false),
    m_lineWidth(0.0)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    referenceLabel = new QGIReferenceLabel(this);

    addToGroup(referenceLabel);
    referenceLabel->setColor(getNormalColor());
    referenceLabel->setPrettyNormal();

    referenceLines = new QGIDimLines();
    addToGroup(referenceLines);
    referenceLines->setNormalColor(getNormalColor());
    referenceLines->setPrettyNormal();

    referenceShape = new QGIDimLines();
    addToGroup(referenceShape);
    referenceShape->setNormalColor(getNormalColor());
    referenceShape->setPrettyNormal();
    
    referenceArrow = new QGIArrow();
    addToGroup(referenceArrow);
    referenceArrow->setNormalColor(getNormalColor());
    referenceArrow->setFillColor(getNormalColor());
    referenceArrow->setPrettyNormal();
    // TODO change style to PYRAMID
    //referenceArrow->setStyle(PYRAMID);
    referenceArrow->setFlipped(true);

    referenceLabel->setZValue(ZVALUE::LABEL);
    referenceArrow->setZValue(ZVALUE::DIMENSION);
    referenceLines->setZValue(ZVALUE::DIMENSION);
    referenceLines->setStyle(Qt::SolidLine);

    referenceShape->setZValue(ZVALUE::DIMENSION);
    referenceShape->setStyle(Qt::SolidLine);

    oldLabelCenter = new QPointF;
    oldLabelCenter->setX(0.0);
    oldLabelCenter->setY(0.0);

    referenceLabel->setPosFromCenter(0, 0);

    // connecting the needed slots and signals
    QObject::connect(
    		referenceLabel, SIGNAL(dragging(bool)),
        this  , SLOT  (referenceLabelDragged(bool)));

    QObject::connect(
    		referenceLabel, SIGNAL(dragFinished()),
        this  , SLOT  (referenceLabelDragFinished()));

    QObject::connect(
    		referenceLabel, SIGNAL(selected(bool)),
        this  , SLOT  (select(bool)));

    QObject::connect(
    		referenceLabel, SIGNAL(hover(bool)),
        this  , SLOT  (hover(bool)));

//    toggleBorder(false);
    setZValue(ZVALUE::DIMENSION);                    //note: this won't paint dimensions over another View if it stacks
                                                     //above this Dimension's parent view.   need Layers?

}


void QGIViewGDTReference::setViewPartFeature(TechDraw::DrawViewGDTReference *reference)
{
    if(reference == 0)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(reference));

    float x = Rez::guiX(reference->X.getValue());
    float y = Rez::guiX(-reference->Y.getValue());

    referenceLabel->setColor(getNormalColor());
    referenceLabel->setPosFromCenter(x, y);

    QString labelText = QString::fromUtf8(reference->Text.getStrValue().data());
    referenceLabel->setLabelString(labelText, Rez::guiX(reference->TextWrapLen.getValue()));

    updateReference();

    draw();
}

void QGIViewGDTReference::select(bool state)
{
    setSelected(state);
    draw();
}

void QGIViewGDTReference::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewGDTReference::updateView(bool update)
{
    Q_UNUSED(update);
    auto reference( dynamic_cast<TechDraw::DrawViewGDTReference*>(getViewObject()) );
    if( reference == nullptr )
        return;

    auto vp = static_cast<ViewProviderGDTReference*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (update) {
        QString labelText = QString::fromUtf8(reference->Text.getStrValue().data());
        referenceLabel->setLabelString(labelText, Rez::guiX(reference->TextWrapLen.getValue()));
        referenceLabel->setColor(getNormalColor());
    }

    updateReference();
    draw();
}

void QGIViewGDTReference::updateReference(bool obtuse)
{
    (void) obtuse;
    const auto reference( dynamic_cast<TechDraw::DrawViewGDTReference *>(getViewObject()) );
    if( reference == nullptr ) {
        return;
    }
    auto vp = static_cast<ViewProviderGDTReference*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    QFont font = referenceLabel->getFont();
    font.setPixelSize(calculateFontPixelSize(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    referenceLabel->setFont(font);

    QString labelText = QString::fromUtf8(reference->Text.getStrValue().data());

    referenceLabel->setLabelString(labelText, Rez::guiX(reference->TextWrapLen.getValue()));
}

void QGIViewGDTReference::referenceLabelDragged(bool ctrl)
{
    draw_modifier(ctrl);
}

void QGIViewGDTReference::referenceLabelDragFinished()
{
    auto dim( dynamic_cast<TechDraw::DrawViewGDTReference *>(getViewObject()) );

    if( dim == nullptr ) {
        return;
    }

    double x = Rez::appX(referenceLabel->X()),
           y = Rez::appX(referenceLabel->Y());
    Gui::Command::openCommand("Drag Reference");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.X = %f", dim->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Y = %f", dim->getNameInDocument(), -y);
    Gui::Command::commitCommand();
}

void QGIViewGDTReference::draw()
{
    draw_modifier(false);
}

void QGIViewGDTReference::draw_modifier(bool modifier)
{
    if (!isVisible()) {                                                //should this be controlled by parent ViewPart?
        return;
    }

    TechDraw::DrawViewGDTReference *reference = dynamic_cast<TechDraw::DrawViewGDTReference *>(getViewObject());
    if((!reference) ||                                                       //nothing to draw, don't try
       (!reference->isDerivedFrom(TechDraw::DrawViewGDTReference::getClassTypeId()))) {
        referenceLabel->hide();
        hide();
        return;
    }

    referenceLabel->show();
    show();

//    const TechDraw::DrawViewPart *refObj = reference->getViewPart();
//    if(!refObj->hasGeometry()) {                                       //nothing to draw yet (restoring)
//        referenceLabel->hide();
//        hide();
//        return;
//    }

    auto vp = static_cast<ViewProviderGDTReference*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    m_colNormal = getNormalColor();
    referenceLabel->setColor(m_colNormal);

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());

    double textWidth = referenceLabel->getLabelText()->boundingRect().width();
    double textHeight = referenceLabel->getLabelText()->boundingRect().height();
    QRectF  mappedRect = mapRectFromItem(referenceLabel, referenceLabel->boundingRect());
    Base::Vector3d lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0);

    if (reference->isLocked()) {
        lblCenter.x = (oldLabelCenter->x());
        lblCenter.y = (oldLabelCenter->y());
        referenceLabel->setFlag(QGraphicsItem::ItemIsMovable, false);
    } else
        referenceLabel->setFlag(QGraphicsItem::ItemIsMovable, true);

    Base::Vector3d dLineStart;
    Base::Vector3d kinkPoint;
    double kinkLength = Rez::guiX(5.0);

    float orginX = reference->OriginX.getValue();
    float orginY = reference->OriginY.getValue();


    float scale = reference->SymbolScale.getValue();
    double offset = 0;
    QPainterPath referencePath;

    // Square symbol
    textWidth = (textWidth * scale) + Rez::guiX(2.0);
    textHeight = (textHeight * scale) + Rez::guiX(1.0);
    double max = std::max(textWidth, textHeight);
    referencePath.addRect(lblCenter.x -(max / 2.0), lblCenter.y - (max / 2.0), max, max);
    offset = (max / 2.0);


    offset = (lblCenter.x < orginX) ? offset : -offset;
    dLineStart.y = lblCenter.y;
    dLineStart.x = lblCenter.x + offset;
    kinkLength = (lblCenter.x < orginX) ? kinkLength : -kinkLength;
    kinkPoint.y = dLineStart.y;
    kinkPoint.x = dLineStart.x + kinkLength;

    QPainterPath dLinePath;
    dLinePath.moveTo(dLineStart.x, dLineStart.y);
    dLinePath.lineTo(kinkPoint.x, kinkPoint.y);

    if (modifier) {
        reference->OriginX.setValue(orginX + lblCenter.x - oldLabelCenter->x());
        reference->OriginY.setValue(orginY + lblCenter.y - oldLabelCenter->y());
    }

    orginX = reference->OriginX.getValue();
    orginY = reference->OriginY.getValue();

    dLinePath.lineTo(orginX, orginY);

    oldLabelCenter->setX(lblCenter.x);
    oldLabelCenter->setY(lblCenter.y);

    referenceLines->setPath(dLinePath);
    referenceShape->setPath(referencePath);


    referenceArrow->setStyle(QGIArrow::getPrefArrowStyle());
    referenceArrow->setSize(QGIArrow::getPrefArrowSize());
    referenceArrow->draw();

    Base::Vector3d orign(orginX, orginY, 0.0);
    Base::Vector3d dirreferenceLinesLine = (orign - kinkPoint).Normalize();
    float arAngle = atan2(dirreferenceLinesLine.y, dirreferenceLinesLine.x) * 180 / M_PI;

    referenceArrow->setPos(orginX, orginY);
    referenceArrow->setRotation(arAngle);
    referenceArrow->show();

    // redraw the Dimension and the parent View
    if (hasHover && !isSelected()) {
        referenceArrow->setPrettyPre();
        referenceLines->setPrettyPre();
        referenceShape->setPrettyPre();
    } else if (isSelected()) {
        referenceArrow->setPrettySel();
        referenceLines->setPrettySel();
        referenceShape->setPrettySel();
    } else {
        referenceArrow->setPrettyNormal();
        referenceLines->setPrettyNormal();
        referenceShape->setPrettyNormal();
    }

    update();
    if (parentItem()) {
        //TODO: parent redraw still required with new frame/label??
        parentItem()->update();
    } else {
        Base::Console().Log("INFO - QGIVD::draw - no parent to update\n");
    }

}

void QGIViewGDTReference::drawBorder(void)
{
//Dimensions have no border!
//    Base::Console().Message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
}

QVariant QGIViewGDTReference::itemChange(GraphicsItemChange change, const QVariant &value)
{
   if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            referenceLabel->setSelected(true);
        } else {
            referenceLabel->setSelected(false);
        }
        draw();
    }
    return QGIView::itemChange(change, value);
}

void QGIViewGDTReference::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    setPens();
    //double referenceLinesSaveWidth = arrow->getWidth();
    if (svg) {
        setSvgPens();
    } else {
        setPens();
    }
    QGIView::paint (painter, &myOption, widget);
    setPens();
}

void QGIViewGDTReference::setSvgPens(void)
{
    double svgLineFactor = 3.0;                     //magic number.  should be a setting somewhere.
    referenceLines->setWidth(m_lineWidth/svgLineFactor);
    referenceShape->setWidth(m_lineWidth/svgLineFactor);
    referenceArrow->setWidth(referenceArrow->getWidth()/svgLineFactor);
}

void QGIViewGDTReference::setPens(void)
{
	referenceLines->setWidth(m_lineWidth);
	referenceShape->setWidth(m_lineWidth);
    referenceArrow->setWidth(m_lineWidth);
}

QColor QGIViewGDTReference::getNormalColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();

    auto reference( dynamic_cast<TechDraw::DrawViewGDTReference*>(getViewObject()) );
    if( reference == nullptr )
        return m_colNormal;

    auto vp = static_cast<ViewProviderGDTReference*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return m_colNormal;
    }

    m_colNormal = vp->Color.getValue().asValue<QColor>();
    return m_colNormal;
}

#include <Mod/TechDraw/Gui/moc_QGIViewGDTReference.cpp>
