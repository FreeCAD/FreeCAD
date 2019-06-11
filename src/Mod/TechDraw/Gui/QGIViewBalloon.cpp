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
  # include <QApplication>

  # include <math.h>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <string>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "QGIViewBalloon.h"
#include "ViewProviderBalloon.h"
#include "DrawGuiUtil.h"
#include "QGIViewPart.h"
#include "QGIViewDimension.h"
#include "QGVPage.h"
#include "MDIViewPage.h"
#include "TaskBalloon.h"

#define PI  3.14159

//TODO: hide the Qt coord system (+y down).  

using namespace TechDraw;
using namespace TechDrawGui;

void QGIBalloonLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    Gui::Control().showDialog(new TaskDlgBalloon(parent));
    QGraphicsItem::mouseDoubleClickEvent(event);
}

//**************************************************************
QGIViewBalloon::QGIViewBalloon() :
    hasHover(false),
    m_lineWidth(0.0)
{
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

    balloonShape = new QGIDimLines();
    addToGroup(balloonShape);

    arrow = new QGIArrow();
    addToGroup(arrow);

    balloonLabel->setZValue(ZVALUE::LABEL);
    arrow->setZValue(ZVALUE::DIMENSION);
    balloonLines->setZValue(ZVALUE::DIMENSION);
    balloonLines->setStyle(Qt::SolidLine);

    balloonShape->setZValue(ZVALUE::DIMENSION);
    balloonShape->setStyle(Qt::SolidLine);

    oldLabelCenter = new QPointF;
    oldLabelCenter->setX(0.0);
    oldLabelCenter->setY(0.0);

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

//    toggleBorder(false);
    setZValue(ZVALUE::DIMENSION);                    //note: this won't paint dimensions over another View if it stacks
                                                     //above this Dimension's parent view.   need Layers?

}

void QGIViewBalloon::placeBalloon(QPointF pos)
{

    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if( balloon == nullptr ) {
        return;
    }

    DrawView* balloonParent = nullptr;
    App::DocumentObject* docObj = balloon->sourceView.getValue();
    if (docObj == nullptr) {
        return;
    } else {
        balloonParent = dynamic_cast<DrawView*>(docObj);
    }
    
    auto featPage = balloonParent->findParentPage();
    if (featPage == nullptr) {
        return;
    }
    
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

        balloon->OriginX.setValue(mapFromScene(pos).x());
        balloon->OriginY.setValue(mapFromScene(pos).y());

        int idx = featPage->getNextBalloonIndex();
        QString labelText = QString::number(idx);
        balloon->Text.setValue(std::to_string(idx).c_str());
     
        QFont font = balloonLabel->getFont();
        font.setPointSizeF(calculateFontPointSizeF(vp->Fontsize.getValue()));
        font.setFamily(QString::fromUtf8(vp->Font.getValue()));
        balloonLabel->setFont(font);
        prepareGeometryChange();

        // Default label position
        balloonLabel->setPosFromCenter(mapFromScene(pos).x() + 200, mapFromScene(pos).y() -200);
        balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
//    }

    draw();
}


void QGIViewBalloon::setViewPartFeature(TechDraw::DrawViewBalloon *balloon)
{
    if(balloon == 0)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(balloon));

    float x = Rez::guiX(balloon->X.getValue());
    float y = Rez::guiX(-balloon->Y.getValue());

    balloonLabel->setColor(getNormalColor());
    balloonLabel->setPosFromCenter(x, y);

    QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
    balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));

    updateBalloon();

    draw();
}

void QGIViewBalloon::select(bool state)
{
    setSelected(state);
    draw();
}

void QGIViewBalloon::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewBalloon::updateView(bool update)
{
    Q_UNUSED(update);
    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if( balloon == nullptr )
        return;

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (update) {
        QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
        balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
        balloonLabel->setColor(getNormalColor());
    }

    updateBalloon();
    draw();
}

void QGIViewBalloon::updateBalloon(bool obtuse)
{
    (void) obtuse;
    const auto balloon( dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject()) );
    if( balloon == nullptr ) {
        return;
    }
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    QFont font = balloonLabel->getFont();
    font.setPointSizeF(calculateFontPointSizeF(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    balloonLabel->setFont(font);

    QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
    balloonLabel->verticalSep = false;

    if (strcmp(balloon->Symbol.getValueAsString(), "Rectangle") == 0) {
        while (labelText.contains(QString::fromUtf8("|"))) {
            int pos = labelText.indexOf(QString::fromUtf8("|"));
            labelText.replace(pos, 1, QString::fromUtf8("   "));
            QFontMetrics fm(balloonLabel->getFont());
            balloonLabel->seps.push_back(fm.width((labelText.left(pos + 2))));
            balloonLabel->verticalSep = true;
        }
    }

    balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
}

void QGIViewBalloon::balloonLabelDragged(bool ctrl)
{
    draw_modifier(ctrl);
}

void QGIViewBalloon::balloonLabelDragFinished()
{
    auto dim( dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject()) );

    if( dim == nullptr ) {
        return;
    }

    double x = Rez::appX(balloonLabel->X()),
           y = Rez::appX(balloonLabel->Y());
    Gui::Command::openCommand("Drag Balloon");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.X = %f", dim->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Y = %f", dim->getNameInDocument(), -y);
    Gui::Command::commitCommand();
}

void QGIViewBalloon::draw()
{
    draw_modifier(false);
}

void QGIViewBalloon::draw_modifier(bool modifier)
{
    if (!isVisible()) {                                                //should this be controlled by parent ViewPart?
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
    if(!refObj->hasGeometry()) {                                       //nothing to draw yet (restoring)
        balloonLabel->hide();
        hide();
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    m_colNormal = getNormalColor();
    balloonLabel->setColor(m_colNormal);

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());

    double textWidth = balloonLabel->getDimText()->boundingRect().width();
    double textHeight = balloonLabel->getDimText()->boundingRect().height();
    QRectF  mappedRect = mapRectFromItem(balloonLabel, balloonLabel->boundingRect());
    Base::Vector3d lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0);

    if (balloon->isLocked()) {
        lblCenter.x = (oldLabelCenter->x());
        lblCenter.y = (oldLabelCenter->y());
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, false);
    } else
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, true);

    Base::Vector3d dLineStart;
    Base::Vector3d kinkPoint;
    double kinkLength = Rez::guiX(5.0);

    float orginX = balloon->OriginX.getValue();
    float orginY = balloon->OriginY.getValue();

    const char *balloonType = balloon->Symbol.getValueAsString();

    float scale = balloon->SymbolScale.getValue();
    double offset = 0;
    QPainterPath balloonPath;

    if (strcmp(balloonType, "Circular") == 0) {
        double balloonRadius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        balloonRadius = balloonRadius * scale;
        balloonPath.moveTo(lblCenter.x, lblCenter.y);
        balloonPath.addEllipse(lblCenter.x - balloonRadius,lblCenter.y - balloonRadius, balloonRadius * 2, balloonRadius * 2);
        offset = balloonRadius;
    } else if (strcmp(balloonType, "None") == 0) {
        balloonPath = QPainterPath();
        offset = (textWidth / 2.0) + Rez::guiX(2.0);
    } else if (strcmp(balloonType, "Rectangle") == 0) {
        //Add some room
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        if (balloonLabel->verticalSep) {
            for (std::vector<int>::iterator it = balloonLabel->seps.begin() ; it != balloonLabel->seps.end(); ++it) {
                balloonPath.moveTo(lblCenter.x - (textWidth / 2.0) + *it, lblCenter.y - (textHeight / 2.0));
                balloonPath.lineTo(lblCenter.x - (textWidth / 2.0) + *it, lblCenter.y + (textHeight / 2.0));
            }
        }
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        balloonPath.addRect(lblCenter.x -(textWidth / 2.0), lblCenter.y - (textHeight / 2.0), textWidth, textHeight);
        offset = (textWidth / 2.0);
    } else if (strcmp(balloonType, "Triangle") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(3.0);
        offset = (tan(30 * PI / 180) * radius);
        QPolygonF triangle;
        double startAngle = -PI / 2;
        double angle = startAngle;
        for (int i = 0; i < 4; i++) {
            triangle += QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * PI / 3);
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
        offset = (textWidth / 2.0) + (textHeight / 2.0);
    } else if (strcmp(balloonType, "Square") == 0) {
        //Add some room
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        double max = std::max(textWidth, textHeight);
        balloonPath.addRect(lblCenter.x -(max / 2.0), lblCenter.y - (max / 2.0), max, max);
        offset = (max / 2.0);
    } else if (strcmp(balloonType, "Hexagon") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(1.0);
        offset = radius;
        QPolygonF triangle;
        double startAngle = -2 * PI / 3;
        double angle = startAngle;
        for (int i = 0; i < 7; i++) {
            triangle += QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * PI / 6);
        }
        balloonPath.moveTo(lblCenter.x + (radius * cos(startAngle)), lblCenter.y + (radius * sin(startAngle)));
        balloonPath.addPolygon(triangle);
    }

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
        balloon->OriginX.setValue(orginX + lblCenter.x - oldLabelCenter->x());
        balloon->OriginY.setValue(orginY + lblCenter.y - oldLabelCenter->y());
    }

    orginX = balloon->OriginX.getValue();
    orginY = balloon->OriginY.getValue();

    dLinePath.lineTo(orginX, orginY);

    oldLabelCenter->setX(lblCenter.x);
    oldLabelCenter->setY(lblCenter.y);

    balloonLines->setPath(dLinePath);
    balloonShape->setPath(balloonPath);

    const char *endType = balloon->EndType.getValueAsString();

    if (strcmp(endType, "Arrow") == 0) {
        arrow->setStyle(QGIArrow::getPrefArrowStyle());
    } else if (strcmp(endType, "Dot") == 0) {
        arrow->setStyle(3);
    }

    arrow->setSize(QGIArrow::getPrefArrowSize());
    arrow->draw();

    Base::Vector3d orign(orginX, orginY, 0.0);
    Base::Vector3d dirballoonLinesLine = (orign - kinkPoint).Normalize();
    float arAngle = atan2(dirballoonLinesLine.y, dirballoonLinesLine.x) * 180 / M_PI;

    arrow->setPos(orginX, orginY);
    arrow->setRotation(arAngle);
    arrow->show();

    // redraw the Dimension and the parent View
    if (hasHover && !isSelected()) {
        arrow->setPrettyPre();
        balloonLines->setPrettyPre();
        balloonShape->setPrettyPre();
    } else if (isSelected()) {
        arrow->setPrettySel();
        balloonLines->setPrettySel();
        balloonShape->setPrettySel();
    } else {
        arrow->setPrettyNormal();
        balloonLines->setPrettyNormal();
        balloonShape->setPrettyNormal();
    }

    update();
    if (parentItem()) {
        //TODO: parent redraw still required with new frame/label??
        parentItem()->update();
    } else {
        Base::Console().Log("INFO - QGIVD::draw - no parent to update\n");
    }

}

void QGIViewBalloon::drawBorder(void)
{
//Dimensions have no border!
//    Base::Console().Message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
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

void QGIViewBalloon::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    setPens();
    //double balloonLinesSaveWidth = arrow->getWidth();
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
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();

    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if( balloon == nullptr )
        return m_colNormal;

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return m_colNormal;
    }

    m_colNormal = vp->Color.getValue().asValue<QColor>();
    return m_colNormal;
}

#include <Mod/TechDraw/Gui/moc_QGIViewBalloon.cpp>
