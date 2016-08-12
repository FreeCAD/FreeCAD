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
  # include <Precision.hxx>

  # include <QGraphicsScene>
  # include <QGraphicsSceneMouseEvent>
  # include <QPainter>
  # include <QPaintDevice>
  # include <QSvgGenerator>

  # include <strstream>
  # include <math.h>
#endif

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
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIArrow.h"
#include "QGIViewDimension.h"

using namespace TechDrawGui;

enum SnapMode{
        NoSnap,
        VerticalSnap,
        HorizontalSnap
    };

QGIDatumLabel::QGIDatumLabel()
{
    posX = 0;
    posY = 0;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);
}

QVariant QGIDatumLabel::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            Q_EMIT selected(true);
            setDefaultTextColor(getSelectColor());
        } else {
            Q_EMIT selected(false);
            setDefaultTextColor(getNormalColor());
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
    setDefaultTextColor(getPreColor());
    update();
}

void QGIDatumLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());
    assert(view != 0);

    Q_EMIT hover(false);
    if(!isSelected() && !view->isSelected()) {
        setDefaultTextColor(getNormalColor());
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

QGIViewDimension::QGIViewDimension() :
    hasHover(false)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    datumLabel = new QGIDatumLabel();
    addToGroup(datumLabel);
    dimLines = new QGraphicsPathItem();
    addToGroup(dimLines);
    centerMark = new QGraphicsPathItem();
    addToGroup(centerMark);
    aHead1 = new QGIArrow();
    addToGroup(aHead1);
    aHead2 = new QGIArrow();
    addToGroup(aHead2);

    // connecting the needed slots and signals
    QObject::connect(
        datumLabel, SIGNAL(dragging()),
        this  , SLOT  (datumLabelDragged()));

    QObject::connect(
        datumLabel, SIGNAL(dragFinished()),
        this  , SLOT  (datumLabelDragFinished()));

    QObject::connect(
        datumLabel, SIGNAL(selected(bool)),
        this  , SLOT  (select(bool)));

    QObject::connect(
        datumLabel, SIGNAL(hover(bool)),
        this  , SLOT  (hover(bool)));

    m_pen.setStyle(Qt::SolidLine);
    m_clPen.setColor(QColor(128,128,128));  // TODO: centre line colour preference?

    toggleBorder(false);
}


void QGIViewDimension::setViewPartFeature(TechDraw::DrawViewDimension *obj)
{
    if(obj == 0)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(obj));

    // Set the QGIGroup Properties based on the DrawView
    float x = obj->X.getValue();
    float y = obj->Y.getValue();

    datumLabel->setPosFromCenter(x, y);

    updateDim();
    draw();
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

    // Identify what changed to prevent complete redraw
    if(dim->Fontsize.isTouched() ||
       dim->Font.isTouched()) {
        QFont font = datumLabel->font();
        font.setPointSizeF(dim->Fontsize.getValue());          //scene units (mm), not points
        font.setFamily(QString::fromAscii(dim->Font.getValue()));

        datumLabel->setFont(font);
        datumLabel->setLabelCenter();
        updateDim();
    } else if(dim->X.isTouched() ||
              dim->Y.isTouched()) {
        datumLabel->setPosFromCenter(datumLabel->X(),datumLabel->Y());
        updateDim();
    } else {
        updateDim();
    }

    draw();
}

void QGIViewDimension::updateDim()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()))
        return;

    const TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject());
    QString labelText = QString::fromUtf8(dim->getFormatedValue().data(),dim->getFormatedValue().size());
    QFont font = datumLabel->font();
    font.setPointSizeF(dim->Fontsize.getValue());            //scene units (mm), not points
    font.setFamily(QString::fromUtf8(dim->Font.getValue()));

    datumLabel->setPlainText(labelText);
    datumLabel->setFont(font);
    datumLabel->setPosFromCenter(datumLabel->X(),datumLabel->Y());
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

    double x = datumLabel->X(),
           y = datumLabel->Y();
    Gui::Command::openCommand("Drag Dimension");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.X = %f", dim->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Y = %f", dim->getNameInDocument(), y);
    Gui::Command::commitCommand();
}


void QGIViewDimension::draw()
{
    if (!isVisible()) {                                                //should this be controlled by parent ViewPart?
        return;
    }

    TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject());
    if((!dim) ||                                                       //nothing to draw, don't try
       (!dim->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) ||
       (!dim->has2DReferences()) ) {
        return;
    }

    const TechDraw::DrawViewPart *refObj = dim->getViewPart();
    if(!refObj->hasGeometry()) {                                       //nothing to draw yet (restoring)
        return;
    }

    m_pen.setWidthF(dim->LineWidth.getValue());
    m_clPen.setWidthF(m_pen.widthF() * 0.80);                          //magic number!!!!

    // Crude method of determining state [TODO] improve
    if(isSelected()) {
        m_pen.setColor(getSelectColor());
    } else if (hasHover) {
        m_pen.setColor(getPreColor());
    } else {
        m_pen.setColor(getNormalColor());
    }

    QString labelText = datumLabel->toPlainText();
    Base::Vector3d lblCenter(datumLabel->X(), datumLabel->Y(), 0);

    //we always draw based on Projected geometry.
    //const std::vector<App::DocumentObject*> &objects = dim->References2D.getValues();
    const std::vector<std::string> &SubNames         = dim->References2D.getSubValues();

    const char *dimType = dim->Type.getValueAsString();

    if(strcmp(dimType, "Distance") == 0 ||
       strcmp(dimType, "DistanceX") == 0 ||
       strcmp(dimType, "DistanceY") == 0) {
        Base::Vector3d distStart, distEnd;                                      //start/end points of distance to dimension
        if((dim->References2D.getValues().size() == 1) &&
           (TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge")) {
            int idx = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
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
                throw Base::Exception("QGIVD::draw - Original edge not found or is invalid type (1)");
            }
        } else if(dim->References2D.getValues().size() == 2 &&
                  TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Vertex" &&
                  TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]) == "Vertex") {
            int idx0 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            int idx1 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
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
        } else if(dim->References2D.getValues().size() == 2 &&
            TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&
            TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int idx0 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            int idx1 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
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

                // figure out which end of each edge to use for drawing
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

        Base::Vector3d dir, norm;                                               //direction/normal vectors of dimLine
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
        //Vertical text should be legible from the right
        if (std::abs(angle + M_PI/2.0) < FLT_EPSILON) {
            //noop
        } else if (angle > M_PI_2+M_PI/12) {     //keeps some diagonal dims from turning upside down?
            angle -= (float)M_PI;
        } else if (angle <= -M_PI_2+M_PI/12) {
            angle += (float)M_PI;
        }

        // when the dimension line is not parallel to (distStart-distEnd) (ie DistanceY on side of a Cone) the projection of
        // (distStart-distEnd) on dimLine.norm is not zero, distEnd is considered as reference and distStart
        // is replaced by its projection distStart_
        // wf: in this case we can't use one of the Distance? end points as a reference for dim/ext lines. So we use the projection of
        // startpoint(distStart) onto dimLine
        float normproj12 = (distEnd-distStart).x * norm.x + (distEnd-distStart).y * norm.y;
        Base::Vector3d distStart_ = distStart + norm * normproj12;

        //Base::Vector3d midpos = (distStart_ + distEnd) / 2;

        QFontMetrics fm(datumLabel->font());
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

        dimLines->setPath(path);

        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = datumLabel->boundingRect().width();
        float bbY = datumLabel->boundingRect().height();
        datumLabel->setTransformOriginPoint(bbX / 2, bbY /2);
        double angleOption = 0.0;                                      //put lblText angle adjustments here
        datumLabel->setRotation((angle * 180 / M_PI) + angleOption);

        aHead1->draw();
        aHead2->flip(true);
        aHead2->draw();
        angle = atan2f(dir[1],dir[0]);
        float arrowAngle = angle * 180 / M_PI;
        arrowAngle -= 180.;
        if(flipTriang){
            aHead1->setRotation(arrowAngle + 180.);
            aHead2->setRotation(arrowAngle + 180.);
        } else {
            aHead1->setRotation(arrowAngle);
            aHead2->setRotation(arrowAngle);
        }

        aHead1->setPos(dim1Tip.x, dim1Tip.y);
        aHead2->setPos(dim2Tail.x, dim2Tail.y);

        aHead1->setHighlighted(isSelected() || hasHover);    //setPrettyxxx??
        aHead2->setHighlighted(isSelected() || hasHover);

    } else if(strcmp(dimType, "Diameter") == 0) {
        // terminology: Dimension Text, Dimension Line(s), Extension Lines, Arrowheads
        // was datumLabel, datum line/parallel line, perpendicular line, arw
        Base::Vector3d arrow1Tip, arrow2Tip, dirDimLine, centre; //was p1,p2,dir
        Base::Vector3d lblCenter(datumLabel->X(), datumLabel->Y(), 0);
        double radius;

        if(dim->References2D.getValues().size() == 1 &&
           TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {
            int idx = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
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
        float bbX  = datumLabel->boundingRect().width();
        float bbY = datumLabel->boundingRect().height();

        dirDimLine = (lblCenter - centre).Normalize();                          //if lblCenter == centre, this is (0,0,0)??
        if (fabs(dirDimLine.Length()) < (Precision::Confusion())) {
            dirDimLine = Base::Vector3d(-1.0,0.0,0.0);
        }

        //this is for inner placement only?  recalced for outer?
        arrow1Tip = centre - dirDimLine * radius;                                    //endpoint of diameter arrowhead1
        arrow2Tip = centre + dirDimLine * radius;                                    //endpoint of diameter arrowhead2

        QFontMetrics fm(datumLabel->font());

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
        datumLabel->setTransformOriginPoint(bbX / 2, bbY /2);

        int posMode = NoSnap;
        QPainterPath path;

        if(outerPlacement) {
            // Select whether to snap vertically or hoziontally given tolerance
            Base::Vector3d v = (lblCenter-arrow1Tip);

            double angle = atan2(v.y, v.x);
            double tolerance = 15.0; //deg

            tolerance *= M_PI / 180;
            if( (angle > -tolerance && angle < tolerance) ||                                       //angle = 0 or 180  (+/- 15)
                (angle > (M_PI - tolerance) || angle < (-M_PI + tolerance)) ) {                    //dim line is Horizontal
                  posMode = HorizontalSnap;
            } else if( (angle < ( M_PI / 2. + tolerance) && angle > ( M_PI / 2. - tolerance)) ||   //angle = 90 or 270 (+/- 15)
                       (angle < (-M_PI / 2. + tolerance) && angle > (-M_PI / 2. - tolerance)) ) {  //dim line is Vertical
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

                datumLabel->setRotation(0.);

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

                datumLabel->setRotation(-90.);

            } else {                                                   //outer placement, NoSnap
                float tip = (margin + w / 2);                          // spacer + 0.5*lblText.width()  tip is actually tail?
                tip = (lblCenter.x < centre.x) ? tip : -tip;           //if label on left then tip is +ve (ie to right)

                arrow1Tip = lblCenter;                                 //this tip is really tail(ie end near lblText)
                arrow1Tip.x += tip;


                Base::Vector3d p3 = arrow1Tip;
                p3.x += (lblCenter.x < centre.x) ? margin : - margin;  // p3 is a little farther away from lblText towards centre in X

                arrow2Tip = centre + (p3 - centre).Normalize() * radius; //point on curve aimed just short of label text

                path.moveTo(arrow1Tip.x, arrow1Tip.y);
                path.lineTo(p3[0], p3[1]);

                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                datumLabel->setRotation(0.);
            }
        } else {                                                       //NOT outerplacement ie dimLines are inside circle
            //text always rightside up inside circle
            datumLabel->setRotation(0);
            dLine1Tail = centre - dirDimLine * margin;
            dLine2Tail = centre + dirDimLine * margin;

            path.moveTo(arrow1Tip.x, arrow1Tip.y);
            path.lineTo(dLine1Tail.x, dLine1Tail.y);

            path.moveTo(dLine2Tail.x, dLine2Tail.y);
            path.lineTo(arrow2Tip.x, arrow2Tip.y);
        }

        dimLines->setPath(path);

        // Add or remove centre lines
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
        }

        centerMark->setPath(clpath);

        aHead1->draw();
        aHead2->flip(true);
        aHead2->draw();

        float arAngle = atan2(dirDimLine.y, dirDimLine.x) * 180 / M_PI;

        aHead1->setHighlighted(isSelected() || hasHover);
        aHead2->setHighlighted(isSelected() || hasHover);
        aHead2->show();

        if(outerPlacement) {
            if(posMode > NoSnap) {
                  aHead1->setPos(arrow2Tip.x, arrow2Tip.y);               //arrow 1's endpoint is arrow2Tip!?
                  aHead2->setPos(arrow1Tip.x, arrow1Tip.y);
                  aHead1->setRotation((posMode == HorizontalSnap) ? 90 : 0);
                  aHead2->setRotation((posMode == HorizontalSnap) ? 90 : 0);
            } else {
                Base::Vector3d vec = (arrow2Tip - centre).Normalize();
                float arAngle = atan2(-vec.y, -vec.x) * 180 / M_PI;
                aHead1->setPos(arrow2Tip.x, arrow2Tip.y);
                aHead1->setRotation(arAngle);
                aHead2->hide();                                           //only 1 arrowhead for NoSnap + outerplacement (ie a leader)
            }
        } else {
            aHead1->setRotation(arAngle);
            aHead2->setRotation(arAngle);

            aHead1->setPos(arrow2Tip.x, arrow2Tip.y);
            aHead2->show();
            aHead2->setPos(arrow1Tip.x, arrow1Tip.y);
        }

    } else if(strcmp(dimType, "Radius") == 0) {
        // preferred terminology: Dimension Text, Dimension Line(s), Extension Lines, Arrowheads
        // radius gets 1 dimension line from the dimension text to a point on the curve
        Base::Vector3d lblCenter(datumLabel->X(), datumLabel->Y(),0.0);

        Base::Vector3d pointOnCurve,curveCenter;
        double radius;
        TechDrawGeometry::AOC* geomArc = 0;
        bool isArc = false;
        if(dim->References2D.getValues().size() == 1 &&
           TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {
            int idx = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
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
                isArc = true;
                TechDrawGeometry::AOC *circ = static_cast<TechDrawGeometry::AOC *>(geom);
                geomArc = circ;
                radius = circ->radius;
                curveCenter = Base::Vector3d(circ->center.fX,circ->center.fY,0.0);
                pointOnCurve = Base::Vector3d(circ->midPnt.fX, circ->midPnt.fY,0.0);
            } else {
                throw Base::Exception("FVD::draw - Original edge not found or is invalid type (3)");
            }
        } else {
            throw Base::Exception("FVD::draw - Invalid reference for dimension type (3)");
        }

        QFontMetrics fm(datumLabel->font());
        int w = fm.width(labelText);
        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = datumLabel->boundingRect().width();
        float bbY = datumLabel->boundingRect().height();
        datumLabel->setTransformOriginPoint(bbX / 2, bbY /2);
        datumLabel->setRotation(0.0);                                                //label is always right side up & horizontal

        //if inside the arc (len(DimLine < radius)) arrow goes from center to edge away from label
        //if inside the arc arrow kinks, then goes to edge nearest label
        bool outerPlacement = false;
        if ((lblCenter - curveCenter).Length() > radius) {                     //label is outside circle
            outerPlacement = true;
        }

        Base::Vector3d dirDimLine = (lblCenter - curveCenter).Normalize();
        if (fabs(dirDimLine.Length()) < (Precision::Confusion())) {
            dirDimLine = Base::Vector3d(-1.0,0.0,0.0);
        }

        Base::Vector3d dLineStart;
        Base::Vector3d kinkPoint;
        double margin = 5.f;                                                    //space around label
        double kinkLength = 5.0;                                                //sb % of horizontal dist(lblCenter,curveCenter)???
        if (outerPlacement) {
            float offset = (margin + w / 2);
            offset = (lblCenter.x < curveCenter.x) ? offset : -offset;           //if label on left then tip is +ve (ie to right)
            dLineStart.y = lblCenter.y;
            dLineStart.x = lblCenter.x + offset;                                     //start at right or left of label
            kinkLength = (lblCenter.x < curveCenter.x) ? kinkLength : -kinkLength;
            kinkPoint.y = dLineStart.y;
            kinkPoint.x = dLineStart.x + kinkLength;
            pointOnCurve = curveCenter + (kinkPoint - curveCenter).Normalize() * radius;
            if ((kinkPoint - curveCenter).Length() < radius) {
                dirDimLine = (curveCenter - kinkPoint).Normalize();
            } else {
                dirDimLine = (kinkPoint - curveCenter).Normalize();
            }
        } else {
            dLineStart = curveCenter - dirDimLine * margin;      //just beyond centerpoint
            pointOnCurve = curveCenter - dirDimLine * radius;
            kinkPoint = dLineStart;                              //no kink
        }

        //handle partial arc weird cases
        if (isArc) {
            Base::Vector3d midPt(geomArc->midPnt.fX, geomArc->midPnt.fY,0.0);
            Base::Vector3d startPt(geomArc->startPnt.fX, geomArc->startPnt.fY,0.0);
            Base::Vector3d endPt(geomArc->endPnt.fX, geomArc->endPnt.fY,0.0);
            if (outerPlacement &&
                !geomArc->intersectsArc(curveCenter,kinkPoint) ) {
                pointOnCurve = midPt;
            } else if (!outerPlacement) {
                if ((midPt - lblCenter).Length() > (midPt - curveCenter).Length()) {     //label is farther than center
                    dirDimLine = dirDimLine * -1;
                }
                dLineStart = curveCenter + dirDimLine * margin;
                pointOnCurve = curveCenter + dirDimLine * radius;
                kinkPoint = dLineStart;
                if (!geomArc->intersectsArc(dLineStart,pointOnCurve)) {   //keep pathological case within arc
                    if ((pointOnCurve - endPt).Length() < (pointOnCurve - startPt).Length()) {
                        if (!geomArc->cw ) {
                            pointOnCurve = endPt;
                        } else {
                            pointOnCurve = startPt;
                        }
                    } else {
                        if (!geomArc->cw) {
                            pointOnCurve = startPt;
                        } else {
                            pointOnCurve = endPt;
                        }
                    }
                    dLineStart = curveCenter + (pointOnCurve - curveCenter).Normalize() * margin;
                    kinkPoint = dLineStart;
                }
            }
        }

        QPainterPath dLinePath;                                                 //radius dimension line path
        dLinePath.moveTo(dLineStart.x, dLineStart.y);
        dLinePath.lineTo(kinkPoint.x, kinkPoint.y);
        dLinePath.lineTo(pointOnCurve.x, pointOnCurve.y);

        dimLines->setPath(dLinePath);

        // Add or remove centre lines  (wf - this is centermark, not centerlines)
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
        }
        centerMark->setPath(clpath);

        aHead1->draw();

        Base::Vector3d ar1Pos = pointOnCurve;
        Base::Vector3d dirArrowLine = (pointOnCurve - kinkPoint).Normalize();
        float arAngle = atan2(dirArrowLine.y, dirArrowLine.x) * 180 / M_PI;

        aHead1->setPos(ar1Pos.x, ar1Pos.y);
        aHead1->setRotation(arAngle);
        aHead1->setHighlighted(isSelected() || hasHover);
        aHead1->show();
        aHead2->hide();
    } else if(strcmp(dimType, "Angle") == 0) {
        // Only use two straight line edeges for angle
        if(dim->References2D.getValues().size() == 2 &&
           TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&
           TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int idx0 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            int idx1 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
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
                Base::Vector3d lblCenter(datumLabel->X(), datumLabel->Y(), 0);

                float bbX  = datumLabel->boundingRect().width();
                float bbY  = datumLabel->boundingRect().height();

                // Get font height
                QFontMetrics fm(datumLabel->font());

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

                dimLines->setPath(path);

                aHead1->flip(true);
                aHead1->draw();
                aHead2->draw();

                Base::Vector3d norm1 = p1-p0; //(-dir1.y, dir1.x, 0.);
                Base::Vector3d norm2 = p2-p0; //(-dir2.y, dir2.x, 0.);

                Base::Vector3d avg = (norm1 + norm2) / 2.;

                norm1 = norm1.ProjectToLine(avg, norm1);
                norm2 = norm2.ProjectToLine(avg, norm2);

                aHead1->setPos(ar1Pos.x,ar1Pos.y );
                aHead2->setPos(ar2Pos.x,ar2Pos.y );

                float ar1angle = atan2(-norm1.y, -norm1.x) * 180 / M_PI;
                float ar2angle = atan2(norm2.y, norm2.x) * 180 / M_PI;

                if(isOutside) {
                    aHead1->setRotation(ar1angle + 180.);
                    aHead2->setRotation(ar2angle + 180.);
                } else {
                    aHead1->setRotation(ar1angle);
                    aHead2->setRotation(ar2angle);
                }

                aHead1->setHighlighted(isSelected() || hasHover);
                aHead2->setHighlighted(isSelected() || hasHover);

                // Set the angle of the datum text

                Base::Vector3d labelNorm(-labelVec.y, labelVec.x, 0.);
                double lAngle = atan2(labelNorm.y, labelNorm.x);

                if (lAngle > M_PI_2+M_PI/12) {
                    lAngle -= M_PI;
                } else if (lAngle <= -M_PI_2+M_PI/12) {
                    lAngle += M_PI;
                }

                datumLabel->setTransformOriginPoint(bbX / 2., bbY /2.);

                datumLabel->setRotation(lAngle * 180 / M_PI);

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
    } else {
        Base::Console().Log("INFO - QGIVD::draw - no parent to update\n");
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
        if(isSelected()) {
            datumLabel->setSelected(true);
        } else {
            datumLabel->setSelected(false);
        }
        draw();
    }
    return QGIView::itemChange(change, value);
}

void QGIViewDimension::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    double saveWidth = m_pen.widthF();
    double saveClWidth = m_clPen.widthF();
    if (svg) {
        setSvgPens();
    } else {
        setPens();
    }
    QGIView::paint (painter, &myOption, widget);
    m_pen.setWidthF(saveWidth);
    m_clPen.setWidthF(saveClWidth);
}

void QGIViewDimension::setSvgPens(void)
{
    double svgLineFactor = 3.0;                     //magic number.  should be a setting somewhere.
    m_pen.setWidthF(m_pen.widthF()/svgLineFactor);
    dimLines->setPen(m_pen);
    aHead1->setPen(m_pen);
    aHead2->setPen(m_pen);
    m_clPen.setWidthF(m_clPen.widthF()/svgLineFactor);
    centerMark->setPen(m_clPen);
}

void QGIViewDimension::setPens(void)
{
    dimLines->setPen(m_pen);
    aHead1->setPen(m_pen);
    aHead2->setPen(m_pen);
    centerMark->setPen(m_clPen);
}

#include <Mod/TechDraw/Gui/moc_QGIViewDimension.cpp>
