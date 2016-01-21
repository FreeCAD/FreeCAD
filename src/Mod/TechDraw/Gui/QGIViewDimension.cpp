/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

  # include <QAction>
  # include <QApplication>
  # include <QContextMenuEvent>
  # include <QGraphicsScene>
  # include <QGraphicsSceneMouseEvent>
  # include <QGridLayout>
  # include <QScopedPointer>
  # include <QMenu>
  # include <QMessageBox>
  # include <QMouseEvent>

  # include <QPainterPathStroker>
  # include <QPainter>
  # include <strstream>
  # include <math.h>
  # include <QGraphicsPathItem>
  # include <QGraphicsTextItem>
#endif

#include <Precision.hxx>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIViewDimension.h"
#include "QGIArrow.h"

using namespace TechDrawGui;

enum SnapMode{
        NoSnap,
        VerticalSnap,
        HorizontalSnap
    };

QGIDatumLabel::QGIDatumLabel(int ref, QGraphicsScene *scene  ) : reference(ref)
{
    if(scene) {
        scene->addItem(this);
    }
    posX = 0;
    posY = 0;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("NormalColor", 0x00000000));
    m_colNormal = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x0000FF00));
    m_colSel = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0x00080800));
    m_colPre = fcColor.asQColor();
}

QVariant QGIDatumLabel::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            Q_EMIT selected(true);
            setDefaultTextColor(m_colSel);
        } else {
            Q_EMIT selected(false);
            setDefaultTextColor(m_colNormal);
        }
        update();
    } else if(change == ItemPositionHasChanged && scene()) {
        setLabelCenter();
        Q_EMIT dragging();
    }

    return QGraphicsItem::itemChange(change, value);
}

void QGIDatumLabel::setPosFromCenter(const double &xCenter, const double &yCenter)
{
    //set label's Qt position(top,left) given boundingRect center point
    setPos(xCenter - boundingRect().width() / 2., yCenter - boundingRect().height() / 2.);
}

void QGIDatumLabel::setLabelCenter()
{
    //save label's bRect center (posX,posY) given Qt position (top,left)
    posX = x() + boundingRect().width() / 2.;
    posY = y() + boundingRect().height() / 2.;
}

void QGIDatumLabel::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_EMIT hover(true);
    setDefaultTextColor(m_colPre);
    update();
}

void QGIDatumLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());
    assert(view != 0);

    Q_EMIT hover(false);
    if(!isSelected() && !view->isSelected()) {
        setDefaultTextColor(m_colNormal);
        update();
    }
}

void QGIDatumLabel::mouseReleaseEvent( QGraphicsSceneMouseEvent * event)
{
    if(scene() && this == scene()->mouseGrabberItem()) {
        Q_EMIT dragFinished();
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

QGIViewDimension::QGIViewDimension(const QPoint &pos, QGraphicsScene *scene) :
    QGIView(pos, scene),
    hasHover(false)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    QGIDatumLabel *dlabel = new QGIDatumLabel();
    QGraphicsPathItem *arrws        = new QGraphicsPathItem();
    QGraphicsPathItem *clines       = new QGraphicsPathItem();

    datumLabel  = dlabel;
    arrows      = arrws;
    centreLines = clines;

    // connecting the needed slots and signals
    QObject::connect(
        dlabel, SIGNAL(dragging()),
        this  , SLOT  (datumLabelDragged()));

    QObject::connect(
        dlabel, SIGNAL(dragFinished()),
        this  , SLOT  (datumLabelDragFinished()));

    QObject::connect(
        dlabel, SIGNAL(selected(bool)),
        this  , SLOT  (select(bool)));

    QObject::connect(
        dlabel, SIGNAL(hover(bool)),
        this  , SLOT  (hover(bool)));

    pen.setCosmetic(true);
    pen.setWidthF(1.);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("NormalColor", 0x00000000));
    m_colNormal = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x0000FF00));
    m_colSel = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0x00080800));
    m_colPre = fcColor.asQColor();

    addToGroup(arrows);
    addToGroup(datumLabel);
    addToGroup(centreLines);

    toggleBorder(false);
}

QGIViewDimension::~QGIViewDimension()
{
}

void QGIViewDimension::setViewPartFeature(TechDraw::DrawViewDimension *obj)
{
    if(obj == 0)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(obj));

    // Set the QGIGroup Properties based on the DrawView
    float x = obj->X.getValue();
    float y = obj->Y.getValue();

    QGIDatumLabel *dLabel = static_cast<QGIDatumLabel *>(datumLabel);
    dLabel->setPosFromCenter(x, y);

    updateDim();
    draw();
    Q_EMIT dirty();
}

void QGIViewDimension::select(bool state)
{
    setSelected(state);
    draw();
}

void QGIViewDimension::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewDimension::updateView(bool update)
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()))
        return;
    TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject());

    std::vector<App::DocumentObject *> refs = dim->References.getValues();

    QGIDatumLabel *dLabel = dynamic_cast<QGIDatumLabel *>(datumLabel);

    // Identify what changed to prevent complete redraw
    if(dim->Fontsize.isTouched() ||
       dim->Font.isTouched()) {


        QFont font = dLabel->font();
        font.setPointSizeF(dim->Fontsize.getValue());          //scene units (mm), not points
        font.setFamily(QString::fromAscii(dim->Font.getValue()));

        dLabel->setFont(font);
        dLabel->setLabelCenter();

    } else if(dim->X.isTouched() ||
              dim->Y.isTouched()) {
        dLabel->setPosFromCenter(dim->X.getValue(), dim->Y.getValue());
        updateDim();

    } else {
        updateDim();
    }

    draw();

    Q_EMIT dirty();
}

void QGIViewDimension::updateDim()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()))
        return;

    const TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject());

    QString labelText = QString::fromStdString(dim->getFormatedValue());

    QGIDatumLabel *dLabel = dynamic_cast<QGIDatumLabel *>(datumLabel);

    QFont font = dLabel->font();
    font.setPointSizeF(dim->Fontsize.getValue());            //scene units (mm), not points
    font.setFamily(QString::fromAscii(dim->Font.getValue()));

    dLabel->setPlainText(labelText);
    dLabel->setFont(font);
    dLabel->setPosFromCenter(dLabel->X(),dLabel->Y());
}

void QGIViewDimension::datumLabelDragged()
{
    draw();
}

void QGIViewDimension::datumLabelDragFinished()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()))
        return;

    TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject());
    QGIDatumLabel *datumLbl = dynamic_cast<QGIDatumLabel *>(datumLabel);

    double x = datumLbl->X(),
           y = datumLbl->Y();
    Gui::Command::openCommand("Drag Dimension");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.X = %f", dim->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Y = %f", dim->getNameInDocument(), y);
    Gui::Command::commitCommand();
}


void QGIViewDimension::draw()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()))
        return;

    TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject());
    QGIDatumLabel *lbl = dynamic_cast<QGIDatumLabel *>(datumLabel);
    const TechDraw::DrawViewPart *refObj = dim->getViewPart();
    if(!refObj->hasGeometry()) {                                       //nothing to draw yet
        return;
    }
    pen.setStyle(Qt::SolidLine);

    // Crude method of determining state [TODO] improve
    if(isSelected()) {
        pen.setColor(m_colSel);
    } else if (hasHover) {
        pen.setColor(m_colPre);
    } else {
        pen.setColor(m_colNormal);
    }

    QString labelText = lbl->toPlainText();
    Base::Vector3d lblCenter(lbl->X(), lbl->Y(), 0);

    //we always draw based on Projected geometry.
    //const std::vector<App::DocumentObject*> &objects = dim->References.getValues();
    const std::vector<std::string> &SubNames         = dim->References.getSubValues();

    const char *dimType = dim->Type.getValueAsString();

    if(strcmp(dimType, "Distance") == 0 ||
       strcmp(dimType, "DistanceX") == 0 ||
       strcmp(dimType, "DistanceY") == 0) {
        Base::Vector3d distStart, distEnd;
        if((dim->References.getValues().size() == 1) &&
           (DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge")) {
            int idx = DrawUtil::getIndexFromName(SubNames[0]);
            TechDrawGeometry::BaseGeom* geom = refObj->getProjEdgeByIndex(idx);
            if (!geom) {
                Base::Console().Log("INFO - qgivd::draw - no geom for projected edge: %d of %d\n",
                                    idx,refObj->getEdgeGeometry().size());
                return;
            }
            if (geom->geomType == TechDrawGeometry::GENERIC) {
                TechDrawGeometry::Generic *gen = static_cast<TechDrawGeometry::Generic *>(geom);
                Base::Vector2D pnt1 = gen->points.at(0);
                Base::Vector2D pnt2 = gen->points.at(1);
                distStart = Base::Vector3d(pnt1.fX, pnt1.fY, 0.);
                distEnd = Base::Vector3d(pnt2.fX, pnt2.fY, 0.);
            } else {
                throw Base::Exception("FVD::draw - Original edge not found or is invalid type (1)");
            }
        } else if(dim->References.getValues().size() == 2 &&
                  DrawUtil::getGeomTypeFromName(SubNames[0]) == "Vertex" &&
                  DrawUtil::getGeomTypeFromName(SubNames[1]) == "Vertex") {
            int idx0 = DrawUtil::getIndexFromName(SubNames[0]);
            int idx1 = DrawUtil::getIndexFromName(SubNames[1]);
            TechDrawGeometry::Vertex *v0 = refObj->getProjVertexByIndex(idx0);
            TechDrawGeometry::Vertex *v1 = refObj->getProjVertexByIndex(idx1);
            if (!v0 || !v1) {
                //Ugh. this is probably because the document is restoring. check log.
                Base::Console().Log("INFO - qgivd::draw - no geom for projected edge: %d or %d of %d\n",
                                    idx0,idx1,refObj->getEdgeGeometry().size());
                return;
            }
            distStart = Base::Vector3d (v0->pnt.fX, v0->pnt.fY, 0.);
            distEnd = Base::Vector3d (v1->pnt.fX, v1->pnt.fY, 0.);
        } else if(dim->References.getValues().size() == 2 &&
            DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&
            DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int idx0 = DrawUtil::getIndexFromName(SubNames[0]);
            int idx1 = DrawUtil::getIndexFromName(SubNames[1]);
            TechDrawGeometry::BaseGeom* geom0 = refObj->getProjEdgeByIndex(idx0);
            TechDrawGeometry::BaseGeom* geom1 = refObj->getProjEdgeByIndex(idx1);
            if (!geom0 || !geom1) {
                Base::Console().Log("INFO - qgivd::draw - no geom for projected edge: %d or %d of %d\n",
                                    idx0,idx1,refObj->getEdgeGeometry().size());
                return;
            }
            if ( (geom0->geomType == TechDrawGeometry::GENERIC) &&
                 (geom1->geomType == TechDrawGeometry::GENERIC) ){
                TechDrawGeometry::Generic *gen0 = static_cast<TechDrawGeometry::Generic *>(geom0);
                TechDrawGeometry::Generic *gen1 = static_cast<TechDrawGeometry::Generic *>(geom1);
                Base::Vector2D pnt1, pnt2;
                Base::Vector3d edge1Start, edge1End, edge2Start, edge2End;
                pnt1 = gen0->points.at(0);
                pnt2 = gen0->points.at(1);
                edge1Start = Base::Vector3d(pnt1.fX, pnt1.fY, 0);
                edge1End = Base::Vector3d(pnt2.fX, pnt2.fY, 0);
                pnt1 = gen1->points.at(0);
                pnt2 = gen1->points.at(1);
                edge2Start = Base::Vector3d(pnt1.fX, pnt1.fY, 0);
                edge2End = Base::Vector3d(pnt2.fX, pnt2.fY, 0);

                // figure out which end of each edge to use for distance calculation (wf- calculation? sb drawing?)
                Base::Vector3d lin1 = edge1End - edge1Start;                    //vector from edge1Start to edge2End
                Base::Vector3d lin2 = edge2End - edge2Start;

                Base::Vector3d labelV1 = lblCenter - edge1Start;                //vector from edge1Start to lblCenter
                Base::Vector3d labelV2 = lblCenter - edge2Start;

                if(lin1.x * labelV1.x + lin1.y * labelV1.y > 0.)       //dotprod > 0 ==> angle(lin1,labelV1) < PI/2??
                    distStart = edge1End;
                else
                    distStart = edge1Start;

                if(lin2.x * labelV2.x + lin2.y * labelV2.y > 0.)
                    distEnd = edge2End;
                else
                    distEnd = edge2Start;
            } else {
                //TODO: Exception here seems drastic. Can we fail more gracefully?
                throw Base::Exception("FVD::draw -Invalid reference for dimension type (1)");
            }
        }

        Base::Vector3d dir, norm;
        if (strcmp(dimType, "Distance") == 0 ) {
            dir = (distEnd-distStart);
        } else if (strcmp(dimType, "DistanceX") == 0 ) {
            dir = Base::Vector3d ( ((distEnd.x - distStart.x >= FLT_EPSILON) ? 1 : -1) , 0, 0);
        } else if (strcmp(dimType, "DistanceY") == 0 ) {
            dir = Base::Vector3d (0, ((distEnd.y - distStart.y >= FLT_EPSILON) ? 1 : -1) , 0);
        }

        dir.Normalize();
        norm = Base::Vector3d (-dir.y,dir.x, 0);

        // Get magnitude of angle between dir and horizontal
        float angle = atan2f(dir.y,dir.x);
        if (angle > M_PI_2+M_PI/12) {
            angle -= (float)M_PI;
        } else if (angle <= -M_PI_2+M_PI/12) {
            angle += (float)M_PI;
        }

        // when the datum line(dimension line??) is not parallel to (distStart-distEnd) the projection of
        // (distStart-distEnd) on norm is not zero, distEnd is considered as reference and distStart
        // is replaced by its projection distStart_
        float normproj12 = (distEnd-distStart).x * norm.x + (distEnd-distStart).y * norm.y;
        Base::Vector3d distStart_ = distStart + norm * normproj12;

        //Base::Vector3d midpos = (distStart_ + distEnd) / 2;

        QFontMetrics fm(lbl->font());
        int w = fm.width(labelText);
        //int h = fm.height();

        Base::Vector3d vec = lblCenter - distEnd;
        float length = vec.x * norm.x + vec.y * norm.y;

        float margin = 3.f;
        float scaler = 1.;

        float offset1 = (length + normproj12 < 0) ? -margin : margin;
        float offset2 = (length < 0) ? -margin : margin;

        Base::Vector3d ext1End = distStart_ + norm * (length + offset1 * scaler);   //extension line 1 end
        Base::Vector3d ext2End = distEnd  + norm * (length + offset2 * scaler);

        // Calculate the start/end for the Dimension lines
        Base::Vector3d  dim1Tip = distStart_ + norm * length;              //dim line 1 tip
        Base::Vector3d  dim1Tail = lblCenter - dir * (w / 2 + margin);     //dim line 1 tail
        Base::Vector3d  dim2Tip = lblCenter + dir * (w / 2 + margin);
        Base::Vector3d  dim2Tail = distEnd  + norm * length;

        // Add a small margin
        //distStart_ += norm * margin * 0.5;
       // distEnd  += norm * margin * 0.5;

        bool flipTriang = false;

        Base::Vector3d del1 = (dim2Tip-dim1Tip);
        Base::Vector3d del2 = (dim1Tail-dim1Tip);
        float dot1 = del1.x * dir.x + del1.y * dir.y;
        float dot2 = del2.x * dir.x + del2.y * dir.y;

        //Compare to see if Dimension text is larger than dimension
        if (dot1 > (dim2Tail - dim1Tip).Length()) {
            // Increase Margin to improve visability
            float tmpMargin = 10.f * scaler;
            dim2Tip = dim2Tail;
            if(dot2 > (dim2Tail - dim1Tip).Length()) {
                dim2Tip = dim1Tail;
                dim1Tail = dim1Tip - dir * tmpMargin;
                flipTriang = true;
            }
        } else if (dot2 < 0.f) {
            float tmpMargin = 10.f * scaler;
            dim1Tail = dim1Tip;
            if(dot1 < 0.f) {
                dim1Tail = dim2Tip;
                dim2Tip = dim2Tail + dir * tmpMargin;
                flipTriang = true;
            }
        }

        // Extension lines
        QPainterPath path;
        path.moveTo(distStart.x, distStart.y);
        path.lineTo(ext1End.x, ext1End.y);

        path.moveTo(distEnd.x, distEnd.y);
        path.lineTo(ext2End.x, ext2End.y);

        //Dimension lines
        path.moveTo(dim1Tip.x, dim1Tip.y);
        path.lineTo(dim1Tail.x, dim1Tail.y);

        path.moveTo(dim2Tip.x, dim2Tip.y);
        path.lineTo(dim2Tail.x, dim2Tail.y);

        QGraphicsPathItem *arrw = dynamic_cast<QGraphicsPathItem *> (arrows);
        arrw->setPath(path);
        arrw->setPen(pen);

        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = lbl->boundingRect().width();
        float bbY = lbl->boundingRect().height();
        lbl->setTransformOriginPoint(bbX / 2, bbY /2);
        lbl->setRotation(angle * 180 / M_PI);


        if(arw.size() != 2) {
            prepareGeometryChange();
            for(std::vector<QGraphicsItem*>::iterator it = arw.begin(); it != arw.end(); ++it) {
                removeFromGroup(*it);
                delete (*it);
            }
            arw.clear();

            // These items are added to the scene-graph so should be handled by the canvas
            QGIArrow *ar1 = new QGIArrow();        //arrowhead
            QGIArrow *ar2 = new QGIArrow();
            arw.push_back(ar1);
            arw.push_back(ar2);

            ar1->draw();
            ar2->flip(true);
            ar2->draw();

            addToGroup(arw.at(0));
            addToGroup(arw.at(1));
        }

        QGIArrow *ar1 = dynamic_cast<QGIArrow *>(arw.at(0));
        QGIArrow *ar2 = dynamic_cast<QGIArrow *>(arw.at(1));

        angle = atan2f(dir[1],dir[0]);
        float arrowAngle = angle * 180 / M_PI;
        arrowAngle -= 180.;
        if(flipTriang){
            ar1->setRotation(arrowAngle + 180.);
            ar2->setRotation(arrowAngle + 180.);
        } else {
            ar1->setRotation(arrowAngle);
            ar2->setRotation(arrowAngle);
        }

        ar1->setPos(dim1Tip.x, dim1Tip.y);
        ar2->setPos(dim2Tail.x, dim2Tail.y);

        ar1->setHighlighted(isSelected() || hasHover);
        ar2->setHighlighted(isSelected() || hasHover);

    } else if(strcmp(dimType, "Diameter") == 0) {
        // terminology: Dimension Text, Dimension Line(s), Extension Lines, Arrowheads
        // was datumLabel, datum line/parallel line, perpendicular line, arw
        Base::Vector3d arrow1Tip, arrow2Tip, dirDimLine, centre; //was p1,p2,dir
        QGIDatumLabel *label = dynamic_cast<QGIDatumLabel *>(datumLabel);
        Base::Vector3d lblCenter(label->X(), label->Y(), 0);
        double radius;

        if(dim->References.getValues().size() == 1 &&
           DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {
            int idx = DrawUtil::getIndexFromName(SubNames[0]);
            TechDrawGeometry::BaseGeom *geom = refObj->getProjEdgeByIndex(idx);
            if(!geom)  {
                Base::Console().Log("INFO - qgivd::draw - no geom for projected edge: %d of %d\n",
                                        idx,refObj->getEdgeGeometry().size());
                return;
                //throw Base::Exception("Edge couldn't be found for diameter dimension");
            }
            if( (geom->geomType == TechDrawGeometry::CIRCLE) ||
                (geom->geomType == TechDrawGeometry::ARCOFCIRCLE) ) {
                TechDrawGeometry::Circle *circ = static_cast<TechDrawGeometry::Circle *>(geom);
                radius = circ->radius;
                centre = Base::Vector3d (circ->center.fX, circ->center.fY, 0);
            } else {
                throw Base::Exception("FVD::draw - Original edge not found or is invalid type (2)");
            }
        } else {
            throw Base::Exception("FVD ::draw - Invalid reference for dimension type (2)");
        }
        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = label->boundingRect().width();
        float bbY = label->boundingRect().height();

        dirDimLine = (lblCenter - centre).Normalize();                          //if lblCenter == centre, this is (0,0,0)??
        if (fabs(dirDimLine.Length()) < (Precision::Confusion())) {
            dirDimLine = Base::Vector3d(-1.0,0.0,0.0);
        }

        //this is for inner placement only?  recalced for outer?
        arrow1Tip = centre - dirDimLine * radius;                                    //endpoint of diameter arrowhead1
        arrow2Tip = centre + dirDimLine * radius;                                    //endpoint of diameter arrowhead2

        QFontMetrics fm(label->font());

        int w = fm.width(labelText);
        //int h = fm.height();

        float margin = 5.f;

        // Calculate the dimension line endpoints
        // recalced for vert & horizontal snap & inner placement.  not used for nosnap outer?
        Base::Vector3d dLine1Tail = lblCenter - dirDimLine * (margin + w / 2);   //position of tail of 1st dimension line
        Base::Vector3d dLine2Tail = lblCenter + dirDimLine * (margin + w / 2);

        bool outerPlacement = false;
        if ((lblCenter-centre).Length() > radius) {                     //label is outside circle
            outerPlacement = true;
        }

        // Reset transformation origin for datum label
        label->setTransformOriginPoint(bbX / 2, bbY /2);

        int posMode = NoSnap;
        QPainterPath path;

        if(outerPlacement) {
            // Select whether to snap vertically or hoziontally given tolerance
            Base::Vector3d v = (lblCenter-arrow1Tip);

            double angle = atan2(v.y, v.x);
            double tolerance = 15.0; //deg

            tolerance *= M_PI / 180;
            if( (angle > -tolerance && angle < tolerance) ||           //angle = 0 or 180  (+/- 15)
                (angle > (M_PI - tolerance) || angle < (-M_PI + tolerance)) ) {
                  posMode = HorizontalSnap;
            } else if( (angle < ( M_PI / 2. + tolerance) && angle > ( M_PI / 2. - tolerance)) ||   //angle = 90 or 270 (+/- 15)
                       (angle < (-M_PI / 2. + tolerance) && angle > (-M_PI / 2. - tolerance)) ) {
                posMode = VerticalSnap;
            }

            if(posMode == VerticalSnap) {
                float tip = (lblCenter.y > centre.y) ? margin: -margin;
                tip *= 0.5;

                arrow1Tip.x = centre.x - radius;                       //to left, on circle cl
                arrow1Tip.y = lblCenter.y;

                arrow2Tip.x = centre.x + radius;
                arrow2Tip.y = lblCenter.y;

                dLine1Tail = lblCenter;
                dLine1Tail.x -= (margin + w / 2);                      //to left, on label cl

                dLine2Tail = lblCenter;
                dLine2Tail.x += (margin + w / 2);

                // Extension line 1
                path.moveTo(centre.x - radius, centre.y);
                path.lineTo(arrow1Tip.x, arrow1Tip.y + tip);

                // Left Arrow
                path.moveTo(arrow1Tip.x, arrow1Tip.y);                //dimension line, not arrowhead
                path.lineTo(dLine1Tail.x, dLine1Tail.y);

                // Extension line 2
                path.moveTo(centre.x + radius, centre.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y + tip);

                // Right arrow
                path.moveTo(dLine2Tail.x, dLine2Tail.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                label->setRotation(0.);

            } else if(posMode == HorizontalSnap) {
                // Snapped Horizontally

                float tip = (lblCenter.x > centre.x) ? margin: -margin;
                tip *= 0.5;

                arrow1Tip.y = centre.y - radius;
                arrow1Tip.x = lblCenter.x;

                arrow2Tip.y = centre.y + radius;
                arrow2Tip.x = lblCenter.x;

                dLine1Tail = lblCenter;
                dLine1Tail.y -= (margin + w / 2);

                dLine2Tail = lblCenter;
                dLine2Tail.y += (margin + w / 2);

                // Extension lines
                path.moveTo(centre.x, centre.y  - radius);
                path.lineTo(arrow1Tip.x + tip, arrow1Tip.y);

                path.moveTo(arrow1Tip.x, arrow1Tip.y);
                path.lineTo(dLine1Tail.x, dLine1Tail.y);

                // Extension lines
                path.moveTo(centre.x, centre.y  + radius);
                path.lineTo(arrow2Tip.x + tip, arrow2Tip.y);

                path.moveTo(dLine2Tail.x, dLine2Tail.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                label->setRotation(90.);

            } else {                                                   //NoSnap
                float tip = (margin + w / 2);
                tip = (lblCenter.x < centre.x) ? tip : -tip;

                arrow1Tip = lblCenter;
                arrow1Tip.x += tip;


                Base::Vector3d p3 = arrow1Tip;
                p3.x += (lblCenter.x < centre.x) ? margin : - margin;

                arrow2Tip = centre + (p3 - centre).Normalize() * radius;

                path.moveTo(arrow1Tip.x, arrow1Tip.y);
                path.lineTo(p3[0], p3[1]);

                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                label->setRotation(0.);
            }
        } else {                                                       //NOT outerplacement ie dimLines are inside circle
            //text always rightside up inside circle
            label->setRotation(0);
            dLine1Tail = centre - dirDimLine * margin;
            dLine2Tail = centre + dirDimLine * margin;

            path.moveTo(arrow1Tip.x, arrow1Tip.y);
            path.lineTo(dLine1Tail.x, dLine1Tail.y);

            path.moveTo(dLine2Tail.x, dLine2Tail.y);
            path.lineTo(arrow2Tip.x, arrow2Tip.y);
        }

        QGraphicsPathItem *arrw = dynamic_cast<QGraphicsPathItem *> (arrows);
        arrw->setPath(path);
        arrw->setPen(pen);

        // Add or remove centre lines
        QGraphicsPathItem *clines = dynamic_cast<QGraphicsPathItem *> (centreLines);
        QPainterPath clpath;

        if(dim->CentreLines.getValue()) {
            // Add centre lines to the circle

            double clDist = margin; // Centre Line Size
            if( margin / radius  > 0.2) {
                // Tolerance if centre line is greater than 0.3x radius then set to limit
                clDist = radius * 0.2;
            }
            // Vertical Line
            clpath.moveTo(centre.x, centre.y + clDist);
            clpath.lineTo(centre.x, centre.y - clDist);

            // Vertical Line
            clpath.moveTo(centre.x - clDist, centre.y);
            clpath.lineTo(centre.x + clDist, centre.y);

            QPen clPen(QColor(128,128,128));  // TODO: centre line colour preference?
            clines->setPen(clPen);
        }

        clines->setPath(clpath);

        // Create Two Arrows always (but sometimes hide one!)
        if(arw.size() != 2) {
            prepareGeometryChange();
            for(std::vector<QGraphicsItem*>::iterator it = arw.begin(); it != arw.end(); ++it) {
                removeFromGroup(*it);
                delete (*it);
            }
            arw.clear();

            // These items are added to the scene-graph so should be handled by the canvas
            QGIArrow *ar1 = new QGIArrow();
            QGIArrow *ar2 = new QGIArrow();
            arw.push_back(ar1);
            arw.push_back(ar2);

            ar1->draw();
            ar2->flip(true);
            ar2->draw();
            addToGroup(arw.at(0));
            addToGroup(arw.at(1));
        }

        QGIArrow *ar1 = dynamic_cast<QGIArrow *>(arw.at(0));
        QGIArrow *ar2 = dynamic_cast<QGIArrow *>(arw.at(1));

        float arAngle = atan2(dirDimLine.y, dirDimLine.x) * 180 / M_PI;

        ar1->setHighlighted(isSelected() || hasHover);
        ar2->setHighlighted(isSelected() || hasHover);
        ar2->show();

        if(outerPlacement) {
            if(posMode > NoSnap) {
                  ar1->setPos(arrow2Tip.x, arrow2Tip.y);               //arrow 1's endpoint is arrow2Tip!?
                  ar2->setPos(arrow1Tip.x, arrow1Tip.y);
                  ar1->setRotation((posMode == HorizontalSnap) ? 90 : 0);
                  ar2->setRotation((posMode == HorizontalSnap) ? 90 : 0);
            } else {
                Base::Vector3d vec = (arrow2Tip - centre).Normalize();
                float arAngle = atan2(-vec.y, -vec.x) * 180 / M_PI;
                ar1->setPos(arrow2Tip.x, arrow2Tip.y);
                ar1->setRotation(arAngle);
                ar2->hide();                                           //only 1 arrowhead for NoSnap + outerplacement (ie a leader)
            }
        } else {
            ar1->setRotation(arAngle);
            ar2->setRotation(arAngle);

            ar1->setPos(arrow2Tip.x, arrow2Tip.y);
            ar2->show();
            ar2->setPos(arrow1Tip.x, arrow1Tip.y);
        }

    } else if(strcmp(dimType, "Radius") == 0) {
        // preferred terminology: Dimension Text, Dimension Line(s), Extension Lines, Arrowheads
        // radius gets 1 dimension line from the dimension text to a point on the curve
        QGIDatumLabel *label = dynamic_cast<QGIDatumLabel *>(datumLabel);
        Base::Vector3d lblCenter(label->X(), label->Y(),0.0);

        Base::Vector3d pointOnCurve,curveCenter;
        double radius;
        if(dim->References.getValues().size() == 1 &&
           DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {
            int idx = DrawUtil::getIndexFromName(SubNames[0]);
            TechDrawGeometry::BaseGeom* geom = refObj->getProjEdgeByIndex(idx);
            if(!geom)  {
                Base::Console().Log("INFO - qgivd::draw - no geom for projected edge: %d of %d\n",
                                        idx,refObj->getEdgeGeometry().size());
                return;
            }
            if (geom->geomType == TechDrawGeometry::CIRCLE) {
                TechDrawGeometry::Circle *circ = static_cast<TechDrawGeometry::Circle *>(geom);
                radius = circ->radius;
                curveCenter = Base::Vector3d(circ->center.fX,circ->center.fY,0.0);
                pointOnCurve = Base::Vector3d(curveCenter.x + radius, curveCenter.y,0.0);
            } else if (geom->geomType == TechDrawGeometry::ARCOFCIRCLE) {
                TechDrawGeometry::AOC *circ = static_cast<TechDrawGeometry::AOC *>(geom);
                radius = circ->radius;
                curveCenter = Base::Vector3d(circ->center.fX,circ->center.fY,0.0);
                pointOnCurve = Base::Vector3d(circ->midPnt.fX, circ->midPnt.fY,0.0);
            } else {
                throw Base::Exception("FVD::draw - Original edge not found or is invalid type (3)");
            }
        } else {
            throw Base::Exception("FVD::draw - Invalid reference for dimension type (3)");
        }

        Base::Vector3d dirDimLine = (lblCenter - pointOnCurve).Normalize();
        if (fabs(dirDimLine.Length()) < (Precision::Confusion())) {
            dirDimLine = Base::Vector3d(-1.0,0.0,0.0);
        }

        QFontMetrics fm(label->font());
        int w = fm.width(labelText);
        float margin = 5.f;
        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = label->boundingRect().width();
        float bbY = label->boundingRect().height();
        label->setTransformOriginPoint(bbX / 2, bbY /2);
        label->setRotation(0.0);                                                //label is always right side up

        Base::Vector3d dLineStart = lblCenter - dirDimLine * (margin + w / 2);  //startpoint of radius dimension line
        QPainterPath dLinePath;                                                 //radius dimension line path
        dLinePath.moveTo(dLineStart.x, dLineStart.y);
        dLinePath.lineTo(pointOnCurve.x, pointOnCurve.y);

        QGraphicsPathItem *arrw = dynamic_cast<QGraphicsPathItem *> (arrows);
        arrw->setPath(dLinePath);
        arrw->setPen(pen);

        // Add or remove centre lines  (wf - this is centermark, not centerlines)
        QGraphicsPathItem *clines = dynamic_cast<QGraphicsPathItem *> (centreLines);
        QPainterPath clpath;
        if(dim->CentreLines.getValue()) {
            // Add centre lines to the circle
            double clDist = margin; // Centre Line Size
            if( margin / radius  > 0.2) {
                // Tolerance if centre line is greater than 0.3x radius then set to limit
                clDist = radius * 0.2;
            }
            // Vertical Line
            clpath.moveTo(curveCenter.x, curveCenter.y + clDist);
            clpath.lineTo(curveCenter.x, curveCenter.y - clDist);
            // Horizontal Line
            clpath.moveTo(curveCenter.x - clDist, curveCenter.y);
            clpath.lineTo(curveCenter.x + clDist, curveCenter.y);
            QPen clPen(QColor(128,128,128));  // TODO: centre line preference?
            clines->setPen(clPen);
        }
        clines->setPath(clpath);

        // Always create Two Arrows (but sometimes hide 1!)
        QGIArrow *ar1;
        QGIArrow *ar2;
        if(arw.size() != 2) {
            prepareGeometryChange();
            for(std::vector<QGraphicsItem*>::iterator it = arw.begin(); it != arw.end(); ++it) {
                removeFromGroup(*it);
                delete (*it);
            }
            arw.clear();

            // These items are added to the scene-graph so should be handled by the canvas
            ar1 = new QGIArrow();
            ar2 = new QGIArrow();
            arw.push_back(ar1);
            arw.push_back(ar2);

            ar1->flip(true);
            ar1->draw();
            ar2->draw();
            addToGroup(arw.at(0));
            addToGroup(arw.at(1));
        }

        ar1 = dynamic_cast<QGIArrow *>(arw.at(0));
        ar2 = dynamic_cast<QGIArrow *>(arw.at(1));

        Base::Vector3d ar1Pos = pointOnCurve;
        float arAngle = atan2(dirDimLine.y, dirDimLine.x) * 180 / M_PI;

        ar1->setPos(ar1Pos.x, ar1Pos.y);
        ar1->setRotation(arAngle);
        ar1->setHighlighted(isSelected() || hasHover);
        ar1->show();
        ar2->setRotation(arAngle);
        ar2->setHighlighted(isSelected() || hasHover);
        ar2->hide();

    } else if(strcmp(dimType, "Angle") == 0) {
        // Only use two straight line edeges for angle
        if(dim->References.getValues().size() == 2 &&
           DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&
           DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int idx0 = DrawUtil::getIndexFromName(SubNames[0]);
            int idx1 = DrawUtil::getIndexFromName(SubNames[1]);
            TechDrawGeometry::BaseGeom* geom0 = refObj->getProjEdgeByIndex(idx0);
            TechDrawGeometry::BaseGeom* geom1 = refObj->getProjEdgeByIndex(idx1);
            if (!geom0 || !geom1) {
                Base::Console().Log("INFO - qgivd::draw - no geom for projected edge: %d or %d of %d\n",
                                        idx0,idx1,refObj->getEdgeGeometry().size());
                return;
            }
            if ( (geom0->geomType == TechDrawGeometry::GENERIC) &&
                 (geom1->geomType == TechDrawGeometry::GENERIC) ) {
                TechDrawGeometry::Generic *gen0 = static_cast<TechDrawGeometry::Generic *>(geom0);
                TechDrawGeometry::Generic *gen1 = static_cast<TechDrawGeometry::Generic *>(geom1);

                // Get Points for line
                Base::Vector2D pnt1, pnt2;
                Base::Vector3d p1S, p1E, p2S, p2E;
                pnt1 = gen0->points.at(0);
                pnt2 = gen0->points.at(1);

                p1S = Base::Vector3d(pnt1.fX, pnt1.fY, 0);
                p1E = Base::Vector3d(pnt2.fX, pnt2.fY, 0);

                pnt1 = gen1->points.at(0);
                pnt2 = gen1->points.at(1);

                p2S = Base::Vector3d(pnt1.fX, pnt1.fY, 0);
                p2E = Base::Vector3d(pnt2.fX, pnt2.fY, 0);

                Base::Vector3d dir1 = p1E - p1S;
                Base::Vector3d dir2 = p2E - p2S;

                double det = dir1.x*dir2.y - dir1.y*dir2.x;
                if ((det > 0 ? det : -det) < 1e-10)
                    return;
                double c1 = dir1.y*p1S.x - dir1.x*p1S.y;
                double c2 = dir2.y*p2S.x - dir2.x*p2S.y;
                double x = (dir1.x*c2 - dir2.x*c1)/det;
                double y = (dir1.y*c2 - dir2.y*c1)/det;

                Base::Vector3d p0(x,y,0);

                // Get directions with outwards orientation and check if coincident
                dir1 = ((p1E - p0).Length() > (p1S - p0).Length()) ? p1E - p0 : p1S - p0;
                dir2 = ((p2E - p0).Length() > (p2S - p0).Length()) ? p2E - p0 : p2S - p0;

                // Qt y coordinates are flipped
                dir1.y *= -1.;
                dir2.y *= -1.;

                Base::Vector3d labelVec = (lblCenter - p0);

                double labelangle = atan2(-labelVec.y, labelVec.x);

                double startangle = atan2(dir1.y,dir1.x);
                double range      = atan2(-dir1.y*dir2.x+dir1.x*dir2.y,
                                           dir1.x*dir2.x+dir1.y*dir2.y);

                double endangle = startangle + range;

                // Obtain the Label Position and measure the length between intersection
                QGIDatumLabel *label = dynamic_cast<QGIDatumLabel *>(datumLabel);
                Base::Vector3d lblCenter(label->X(), label->Y(), 0);

                float bbX  = label->boundingRect().width();
                float bbY  = label->boundingRect().height();

                // Get font height
                QFontMetrics fm(label->font());

                int h = fm.height();
                double length = labelVec.Length();
                length -= h * 0.6; // Adjust the length so the label isn't over the line

                // Find the end points for dim lines
                Base::Vector3d p1 = ((p1E - p0).Length() > (p1S - p0).Length()) ? p1E : p1S;
                Base::Vector3d p2 = ((p2E - p0).Length() > (p2S - p0).Length()) ? p2E : p2S;

                // add an offset from the ends (add 1mm from end)
                p1 += (p1-p0).Normalize() * 5.;
                p2 += (p2-p0).Normalize() * 5.;

                Base::Vector3d ar1Pos = p0;
                Base::Vector3d ar2Pos = p0;

                ar1Pos += Base::Vector3d(cos(startangle) * length, -sin(startangle) * length, 0.);
                ar2Pos += Base::Vector3d(cos(endangle) * length  , -sin(endangle) * length, 0.);

                // Draw the path
                QPainterPath path;

                // Only draw extension lines if outside arc
                if(length > (p1-p0).Length()) {
                    path.moveTo(p1.x, p1.y);
                    p1 = ar1Pos + (p1-p0).Normalize() * 5.;
                    path.lineTo(p1.x, p1.y);
                }

                if(length > (p2-p0).Length()) {
                    path.moveTo(p2.x, p2.y);
                    p2 = ar2Pos + (p2-p0).Normalize() * 5.;
                    path.lineTo(p2.x, p2.y);
                }


                bool isOutside = true;

                // TODO find a better solution for this. Addmitedely not tidy
                // ###############
                // Treat zero as positive to be consistent for horizontal lines
                if(std::abs(startangle) < FLT_EPSILON)
                    startangle = 0;

                 if(std::abs(endangle) < FLT_EPSILON)
                    endangle = 0;

                if(startangle >= 0 && endangle >= 0) {
                    // Both are in positive side
                  double langle = labelangle;
                  if(labelangle < 0)
                    langle += M_PI * 2;
                    if(endangle - startangle > 0) {
                        if(langle > startangle && langle < endangle)
                            isOutside = false;
                    } else {
                        if(langle < startangle && langle > endangle)
                            isOutside = false;
                    }
                } else if(startangle < 0 && endangle < 0) {
                    // Both are in positive side
                   double langle = labelangle;
                    if(labelangle > 0)
                        langle -= M_PI * 2;
                    if(endangle - startangle < 0) {
                        if(langle > endangle && langle < startangle) // clockwise
                            isOutside = false;
                    } else {
                        if(langle < endangle && langle > startangle) // anticlockwise
                            isOutside = false;
                    }
                } else if(startangle >= 0 && endangle < 0) {
                    if(labelangle < startangle && labelangle > endangle) // clockwise
                        isOutside = false;

                } else if(startangle < 0 && endangle >= 0) {
                   // Both are in positive side

                    if(labelangle > startangle && labelangle < endangle) // clockwise
                        isOutside = false;
                }

                // ###############
//                 Base::Console().Log("<%f, %f, %f>\n", startangle, endangle, labelangle);

                QRectF arcRect(p0.x - length, p0.y - length, 2. * length, 2. * length);
                path.arcMoveTo(arcRect, endangle * 180 / M_PI);
                if(isOutside) {
                    if(labelangle > endangle)
                    {
                        path.arcTo(arcRect, endangle * 180 / M_PI, (labelangle  - endangle) * 180 / M_PI); // chosen a nominal value for 10 degrees
                        path.arcMoveTo(arcRect,startangle * 180 / M_PI);
                        path.arcTo(arcRect, startangle * 180 / M_PI, -10);
                    } else {
                        path.arcTo(arcRect, endangle * 180 / M_PI, 10); // chosen a nominal value for 10 degrees
                        path.arcMoveTo(arcRect,startangle * 180 / M_PI);
                        path.arcTo(arcRect, startangle * 180 / M_PI, (labelangle - startangle) * 180 / M_PI);
                    }


                } else {
                    path.arcTo(arcRect, endangle * 180 / M_PI, -range * 180 / M_PI);
                }

                QGraphicsPathItem *arrw = dynamic_cast<QGraphicsPathItem *> (arrows);
                arrw->setPath(path);
                arrw->setPen(pen);

                // Add the arrows
                if(arw.size() != 2) {
                    prepareGeometryChange();
                    for(std::vector<QGraphicsItem*>::iterator it = arw.begin(); it != arw.end(); ++it) {
                        removeFromGroup(*it);
                        delete (*it);
                    }
                    arw.clear();

                    // These items are added to the scene-graph so should be handled by the canvas
                    QGIArrow *ar1 = new QGIArrow();
                    QGIArrow *ar2 = new QGIArrow();
                    arw.push_back(ar1);
                    arw.push_back(ar2);

                    ar1->flip(true);
                    ar1->draw();
                    ar2->draw();

                    addToGroup(arw.at(0));
                    addToGroup(arw.at(1));
                }

                QGIArrow *ar1 = dynamic_cast<QGIArrow *>(arw.at(0));
                QGIArrow *ar2 = dynamic_cast<QGIArrow *>(arw.at(1));

                Base::Vector3d norm1 = p1-p0; //(-dir1.y, dir1.x, 0.);
                Base::Vector3d norm2 = p2-p0; //(-dir2.y, dir2.x, 0.);

                Base::Vector3d avg = (norm1 + norm2) / 2.;

                norm1 = norm1.ProjToLine(avg, norm1);
                norm2 = norm2.ProjToLine(avg, norm2);

                ar1->setPos(ar1Pos.x,ar1Pos.y );
                ar2->setPos(ar2Pos.x,ar2Pos.y );

                float ar1angle = atan2(-norm1.y, -norm1.x) * 180 / M_PI;
                float ar2angle = atan2(norm2.y, norm2.x) * 180 / M_PI;

                if(isOutside) {
                    ar1->setRotation(ar1angle + 180.);
                    ar2->setRotation(ar2angle + 180.);
                } else {
                    ar1->setRotation(ar1angle);
                    ar2->setRotation(ar2angle);
                }

                ar1->setHighlighted(isSelected() || hasHover);
                ar2->setHighlighted(isSelected() || hasHover);

                // Set the angle of the datum text

                Base::Vector3d labelNorm(-labelVec.y, labelVec.x, 0.);
                double lAngle = atan2(labelNorm.y, labelNorm.x);

                if (lAngle > M_PI_2+M_PI/12) {
                    lAngle -= M_PI;
                } else if (lAngle <= -M_PI_2+M_PI/12) {
                    lAngle += M_PI;
                }

                label->setTransformOriginPoint(bbX / 2., bbY /2.);

                label->setRotation(lAngle * 180 / M_PI);

            } else {
                throw Base::Exception("FVD::draw - Invalid reference for dimension type (4)");
            }
        } //endif 2 Edges
    }  //endif Distance/Diameter/Radius/Angle

    // redraw the Dimension and the parent View
    update();
    if (parentItem()) {
        //TODO: parent redraw still required with new frame/label??
        parentItem()->update();
    }

}

void QGIViewDimension::drawBorder(void)
{
//Dimensions have no border!
//    Base::Console().Message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
}

QVariant QGIViewDimension::itemChange(GraphicsItemChange change, const QVariant &value)
{
   if (change == ItemSelectedHasChanged && scene()) {
        QGIDatumLabel *dLabel = dynamic_cast<QGIDatumLabel *>(datumLabel);

        if(isSelected()) {
            dLabel->setSelected(true);
        } else {
            dLabel->setSelected(false);
        }
        draw();
    }
    return QGIView::itemChange(change, value);
}

void QGIViewDimension::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;
    QGIView::paint(painter, &myOption, widget);
}

#include "moc_QGIViewDimension.cpp"
