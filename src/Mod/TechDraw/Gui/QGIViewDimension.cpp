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

#include "Rez.h"
#include "ZVALUE.h"
#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "QGIViewDimension.h"
#include "ViewProviderDimension.h"

using namespace TechDraw;
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
            setPrettySel();
        } else {
            Q_EMIT selected(false);
            setPrettyNormal();
        }
        update();
    } else if(change == ItemPositionHasChanged && scene()) {
        setLabelCenter();
        Q_EMIT dragging();
    }

    return QGCustomText::itemChange(change, value);
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
    QGCustomText::hoverEnterEvent(event);
}

void QGIDatumLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());
    assert(view != 0);

    Q_EMIT hover(false);
    if(!isSelected() && !view->isSelected()) {
        setPrettyNormal();
    }
    QGraphicsTextItem::hoverLeaveEvent(event);
}

void QGIDatumLabel::mouseReleaseEvent( QGraphicsSceneMouseEvent * event)
{
    if(scene() && this == scene()->mouseGrabberItem()) {
        Q_EMIT dragFinished();
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

QGIViewDimension::QGIViewDimension() :
    hasHover(false),
    m_lineWidth(0.0)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    datumLabel = new QGIDatumLabel();
    addToGroup(datumLabel);
    datumLabel->setPrettyNormal();
    dimLines = new QGIDimLines();
    addToGroup(dimLines);
    aHead1 = new QGIArrow();
    addToGroup(aHead1);
    aHead2 = new QGIArrow();
    addToGroup(aHead2);

    datumLabel->setZValue(ZVALUE::DIMENSION);
    dimLines->setZValue(ZVALUE::DIMENSION);
    aHead1->setZValue(ZVALUE::DIMENSION);
    aHead2->setZValue(ZVALUE::DIMENSION);

    //centerMark = new QGICMark();
    //addToGroup(centerMark);


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

    dimLines->setStyle(Qt::SolidLine);

    toggleBorder(false);
    setZValue(ZVALUE::DIMENSION);                    //note: this won't paint dimensions over another View if it stacks
                                                     //above this Dimension's parent view.   need Layers?

}


void QGIViewDimension::setViewPartFeature(TechDraw::DrawViewDimension *obj)
{
    if(obj == 0)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(obj));

    // Set the QGIGroup Properties based on the DrawView
    float x = Rez::guiX(obj->X.getValue());
    float y = Rez::guiX(-obj->Y.getValue());

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
    Q_UNUSED(update);
    auto dim( dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject()) );
    if( dim == nullptr )
        return;

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    // Identify what changed to prevent complete redraw
    if (update||
        dim->X.isTouched() ||
        dim->Y.isTouched()) {
        float x = Rez::guiX(dim->X.getValue());
        float y = Rez::guiX(dim->Y.getValue());
        datumLabel->setPosFromCenter(x,-y);
        updateDim();
     }
     else if(vp->Fontsize.isTouched() ||
               vp->Font.isTouched()) {
         QFont font = datumLabel->font();
         font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
         font.setFamily(QString::fromLatin1(vp->Font.getValue()));
         datumLabel->setFont(font);
         updateDim();
    } else if (vp->LineWidth.isTouched()) {           //never happens!!
        m_lineWidth = vp->LineWidth.getValue();
        updateDim();
    } else {
        updateDim();
    }

    draw();
}

void QGIViewDimension::updateDim(bool obtuse)
{
    const auto dim( dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject()) );
    if( dim == nullptr ) {
        return;
    }
    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    QString labelText = QString::fromUtf8(dim->getFormatedValue(obtuse).data(),dim->getFormatedValue().size());
    QFont font = datumLabel->font();
    font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));

    datumLabel->setFont(font);
    prepareGeometryChange();
    datumLabel->setPlainText(labelText);
    datumLabel->setPosFromCenter(datumLabel->X(),datumLabel->Y());
}

void QGIViewDimension::datumLabelDragged()
{
    draw();
}

void QGIViewDimension::datumLabelDragFinished()
{
    auto dim( dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject()) );

    if( dim == nullptr ) {
        return;
    }

    double x = Rez::appX(datumLabel->X()),
           y = Rez::appX(datumLabel->Y());
    Gui::Command::openCommand("Drag Dimension");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.X = %f", dim->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Y = %f", dim->getNameInDocument(), -y);
    Gui::Command::commitCommand();
}


void QGIViewDimension::draw()
{
    if (!isVisible()) {                                                //should this be controlled by parent ViewPart?
        return;
    }
    
    datumLabel->show();
    show();

    TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject());
    if((!dim) ||                                                       //nothing to draw, don't try
       (!dim->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) ||
       (!dim->has2DReferences()) ) {
        datumLabel->hide();
        hide();
        return;
    }

    const TechDraw::DrawViewPart *refObj = dim->getViewPart();
    if(!refObj->hasGeometry()) {                                       //nothing to draw yet (restoring)
        datumLabel->hide();
        hide();
        return;
    }

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());
    float margin = Rez::guiX(5.f);

    QString labelText = datumLabel->toPlainText();
    Base::Vector3d lblCenter(datumLabel->X(), datumLabel->Y(), 0);    //already gui coords

    //const std::vector<App::DocumentObject*> &objects = dim->References2D.getValues();
    //const std::vector<std::string> &SubNames         = dim->References2D.getSubValues();

    const char *dimType = dim->Type.getValueAsString();

    if (strcmp(dimType, "Distance") == 0 ||
        strcmp(dimType, "DistanceX") == 0 ||
        strcmp(dimType, "DistanceY") == 0) {
        pointPair pts = dim->getLinearPoints();
        Base::Vector3d distStart, distEnd;                                      //start/end points of distance to measure
        distStart = Rez::guiX(pts.first);
        distEnd   = Rez::guiX(pts.second);

        // +/- aligned method
        // dimension text legible from bottom or right
        // text outside arrows (not between)
        // text to left of vertical dims
        // text above horizontal dims
        double offsetFudge = 2.0;
        double textOffset = 1.0 * Rez::guiX(vp->Fontsize.getValue()) + offsetFudge;
        Base::Vector3d dir, norm;               //direction/normal vectors of distance line (not dimension Line)
        if (strcmp(dimType, "Distance") == 0 ) {
            dir = (distEnd-distStart);
        } else if (strcmp(dimType, "DistanceX") == 0 ) {
            dir = Base::Vector3d ( ((distEnd.x - distStart.x >= FLT_EPSILON) ? 1 : -1) , 0, 0);
        } else if (strcmp(dimType, "DistanceY") == 0 ) {
            dir = Base::Vector3d (0, ((distEnd.y - distStart.y >= FLT_EPSILON) ? 1 : -1) , 0);
        }
        dir.Normalize();
        norm = Base::Vector3d (-dir.y,dir.x, 0);         //normal to dimension direction

        // Get magnitude of angle between dir and horizontal
        double angle = atan2f(dir.y,dir.x);
        if (angle < 0.0) {
            angle = 2 * M_PI + angle;          //map to +ve angle
        }

        //orient text right side up
        bool isFlipped = false;
        double angleFiddle = M_PI / 18.0;                  // 18 => 10*, 12 => 15*, ...
        if ((angle > M_PI_2 - angleFiddle) &&              // > 80CW
                   (angle <= M_PI)) {                      // < 180CW  +/-Q3
            angle += M_PI;                                 // flip CW
            isFlipped = true;
        } else if ((angle > M_PI) &&                       // > 180CW  +/-Q4
                   (angle <= 1.5*M_PI - angleFiddle))  {   // < 260CW
            angle -= M_PI;                                 // flip CCW
            isFlipped = true;
        }

        Base::Vector3d textNorm = norm;
        if (std::abs(dir.x) < FLT_EPSILON) {
            textNorm = Base::Vector3d(1.0,0.0,0.0);                    //force text to left of dim line
        } else if (std::abs(dir.y) < FLT_EPSILON) {
            textNorm = Base::Vector3d(0.0,1.0,0.0);                    //force text above dim line
        } else {
            if (isFlipped) {
                textNorm = -norm;
            }
        }

        // when the dimension line is not parallel to (distStart-distEnd) (ie DistanceY on side of a Cone) the projection of
        // (distStart-distEnd) on dimLine.norm is not zero, distEnd is considered as reference and distStart
        // is replaced by its projection distStart_
        // wf: in this case we can't use one of the Distance? end points as a reference for dim/ext lines. So we use the projection of
        // startpoint(distStart) onto dimLine
        // m = |proj(A on B)| = dot(A,unit(B)
        // m = |proj(dimLine on normal)| = dot(dimLine,normal)
        // newstartpt = oldstart + m*normal
        float normproj12 = (distEnd-distStart).x * norm.x + (distEnd-distStart).y * norm.y;   //dot(dirDimline, normal)
        Base::Vector3d distStart_ = distStart + norm * normproj12;
        Base::Vector3d distMid = (distStart_ + distEnd) / 2.0;

//        QFont font = datumLabel->font();                         //font metrics gives answers in pixels, not mm
//        QFontMetrics fm(font);
//        int w = fm.width(labelText);                             //why is this different than diameter/radius?
//        int h = fm.height();
        double lblWidth = datumLabel->boundingRect().width();

        Base::Vector3d fauxCenter = lblCenter + textOffset * textNorm;
        Base::Vector3d vec = fauxCenter - distEnd;              //endof dist line to center of dimline
        float perpDistance = vec.x * norm.x + vec.y * norm.y;   //dot(vec,norm) the perp distance between distance & dimension lines.
        margin = Rez::guiX(2.f);
        float scaler = 1.;

        float offset1 = (perpDistance + normproj12 < 0) ? -margin : margin;
        float offset2 = (perpDistance < 0) ? -margin : margin;

        Base::Vector3d ext1End = distStart_ + norm * (perpDistance + offset1 * scaler);   //extension line 1 end
        Base::Vector3d ext2End = distEnd  + norm * (perpDistance + offset2 * scaler);

        // Calculate the start/end for the Dimension lines
        //dim1Tip is the position of 1 arrow point (lhs on a horizontal)
        //dim2Tail is the position of the other arrow point (rhs)
        //case 1: inner placement: text between extensions & fits. arros point out from inside
        //case 2: inner placement2: text too big to fit. arrows point in from outside
        //case 3: outer placement: text is outside extensions.  arrows point in, 1 arrow tracks dimText

        //case1 - inner placement, text fits within extension lines
        Base::Vector3d  dim1Tip = distStart_ + norm * perpDistance;
        Base::Vector3d  dim1Tail = distMid + norm * perpDistance;
        Base::Vector3d  dim2Tip = distMid + norm * perpDistance;
        Base::Vector3d  dim2Tail = distEnd  + norm * perpDistance;

        bool flipTriang = false;

        double dimSpan = (dim2Tail - dim1Tip).Length();
        double fauxToDim1 = (fauxCenter - dim1Tip).Length();     //label to end #1
        double fauxToDim2 = (fauxCenter - dim2Tail).Length();
        double tailLength = Rez::guiX(10.f) * scaler;

        //case2 - innerPlacement * text > span
        if ((lblWidth > dimSpan)  &&
            (fauxToDim1 < dimSpan) &&
            (fauxToDim2 < dimSpan)) {   //fauxcenter is between extensions
            dim1Tail = dim1Tip - tailLength * dir;
            dim2Tip = dim2Tail + tailLength * dir;
            flipTriang = true;
        }

        //case3 - outerPlacement
        if ((fauxToDim1 < fauxToDim2) &&
            (dimSpan < fauxToDim2) ) {
            dim1Tail = fauxCenter;
            dim2Tip = dim2Tail + tailLength * dir;
            flipTriang = true;
        } else if ((fauxToDim2 < fauxToDim1) &&
            (dimSpan < fauxToDim1) ) {
            dim1Tail = dim1Tip - tailLength * dir;
            dim2Tip = fauxCenter;
            flipTriang = true;
        } else {
            //a different case
        }

        // Extension lines
        QPainterPath path;
        path.moveTo(distStart.x, distStart.y);
        path.lineTo(ext1End.x, ext1End.y);

        path.moveTo(distEnd.x, distEnd.y);
        path.lineTo(ext2End.x, ext2End.y);

        //Dimension lines
        //TODO: line tip goes just a bit too far. overlaps the arrowhead's point
        //default arrow perpDistance is 5.0

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

        aHead1->setStyle(QGIArrow::getPrefArrowStyle());
        aHead1->setSize(QGIArrow::getPrefArrowSize());
        aHead1->draw();
        aHead2->flip(true);
        aHead2->setStyle(QGIArrow::getPrefArrowStyle());
        aHead2->setSize(QGIArrow::getPrefArrowSize());
        aHead2->draw();
        angle = atan2f(dir.y,dir.x);
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

    } else if(strcmp(dimType, "Diameter") == 0) {
        // terminology: Dimension Text, Dimension Line(s), Extension Lines, Arrowheads
        // was datumLabel, datum line/parallel line, perpendicular line, arw
        Base::Vector3d arrow1Tip, arrow2Tip, dirDimLine; //was p1,p2,dir
        double radius;
        Base::Vector3d pointOnCurve,curveCenter;

        arcPoints pts = dim->getArcPoints();
        bool isArc = pts.isArc;
        radius = Rez::guiX(pts.radius);
        curveCenter = Rez::guiX(pts.center);
        pointOnCurve = Rez::guiX(pts.onCurve.first);

        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = datumLabel->boundingRect().width();
        float bbY = datumLabel->boundingRect().height();

        dirDimLine = (lblCenter - curveCenter).Normalize();                          //if lblCenter == curveCenter, this is (0,0,0)??
        if (fabs(dirDimLine.Length()) < (Precision::Confusion())) {
            dirDimLine = Base::Vector3d(-1.0,0.0,0.0);
        }

        //this is for inner placement only?  recalced for outer?
        arrow1Tip = curveCenter - dirDimLine * radius;                                    //endpoint of diameter arrowhead1
        arrow2Tip = curveCenter + dirDimLine * radius;                                    //endpoint of diameter arrowhead2

        QFontMetrics fm(datumLabel->font());

        int w = fm.width(labelText);
        //int h = fm.height();

        margin = Rez::guiX(5.f);

        // Calculate the dimension line endpoints
        // recalced for vert & horizontal snap & inner placement.  not used for nosnap outer?
        Base::Vector3d dLine1Tail = lblCenter - dirDimLine * (margin + w / 2);   //position of tail of 1st dimension line
        Base::Vector3d dLine2Tail = lblCenter + dirDimLine * (margin + w / 2);

        bool outerPlacement = false;
        if ((lblCenter-curveCenter).Length() > radius) {                     //label is outside circle
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
                float tip = (lblCenter.y > curveCenter.y) ? margin: -margin;
                //tip *= 0.5;

                arrow1Tip.x = curveCenter.x - radius;                       //to left, on circle cl
                arrow1Tip.y = lblCenter.y;

                arrow2Tip.x = curveCenter.x + radius;
                arrow2Tip.y = lblCenter.y;

                dLine1Tail = lblCenter;
                dLine1Tail.x -= (margin + w / 2);                      //to left, on label cl

                dLine2Tail = lblCenter;
                dLine2Tail.x += (margin + w / 2);

                // Extension line 1
                path.moveTo(curveCenter.x - radius, curveCenter.y);
                path.lineTo(arrow1Tip.x, arrow1Tip.y + tip);

                path.moveTo(arrow1Tip.x, arrow1Tip.y);                //dimension line, not arrowhead
                path.lineTo(dLine1Tail.x, dLine1Tail.y);

                // Extension line 2
                path.moveTo(curveCenter.x + radius, curveCenter.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y + tip);

                path.moveTo(dLine2Tail.x, dLine2Tail.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                datumLabel->setRotation(0.);

            } else if(posMode == HorizontalSnap) {
                // Snapped Horizontally

                float tip = (lblCenter.x > curveCenter.x) ? margin: -margin;
                //tip *= 0.5;

                arrow1Tip.y = curveCenter.y - radius;
                arrow1Tip.x = lblCenter.x;

                arrow2Tip.y = curveCenter.y + radius;
                arrow2Tip.x = lblCenter.x;

                dLine1Tail = lblCenter;
                dLine1Tail.y -= (margin + w / 2);

                dLine2Tail = lblCenter;
                dLine2Tail.y += (margin + w / 2);

                // Extension lines
                path.moveTo(curveCenter.x, curveCenter.y  - radius);
                path.lineTo(arrow1Tip.x + tip, arrow1Tip.y);

                path.moveTo(arrow1Tip.x, arrow1Tip.y);
                path.lineTo(dLine1Tail.x, dLine1Tail.y);

                // Extension lines
                path.moveTo(curveCenter.x, curveCenter.y  + radius);
                path.lineTo(arrow2Tip.x + tip, arrow2Tip.y);

                path.moveTo(dLine2Tail.x, dLine2Tail.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                datumLabel->setRotation(-90.);

            } else {                                                   //outer placement, NoSnap
                float tip = (margin + w / 2);                          // spacer + 0.5*lblText.width()  tip is actually tail?
                tip = (lblCenter.x < curveCenter.x) ? tip : -tip;           //if label on left then tip is +ve (ie to right)

                arrow1Tip = lblCenter;                                 //this tip is really tail(ie end near lblText)
                arrow1Tip.x += tip;


                Base::Vector3d p3 = arrow1Tip;
                p3.x += (lblCenter.x < curveCenter.x) ? margin : - margin;  // p3 is a little farther away from lblText towards curveCenter in X

                arrow2Tip = curveCenter + (p3 - curveCenter).Normalize() * radius; //point on curve aimed just short of label text

                path.moveTo(arrow1Tip.x, arrow1Tip.y);
                path.lineTo(p3[0], p3[1]);

                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                datumLabel->setRotation(0.);
            }
        } else {                                                       //NOT outerplacement ie dimLines are inside circle
            //text always rightside up inside circle
            datumLabel->setRotation(0);
            dLine1Tail = curveCenter - dirDimLine * margin;
            dLine2Tail = curveCenter + dirDimLine * margin;

            path.moveTo(arrow1Tip.x, arrow1Tip.y);
            path.lineTo(dLine1Tail.x, dLine1Tail.y);

            path.moveTo(dLine2Tail.x, dLine2Tail.y);
            path.lineTo(arrow2Tip.x, arrow2Tip.y);
        }

        aHead1->setStyle(QGIArrow::getPrefArrowStyle());
        aHead1->setSize(QGIArrow::getPrefArrowSize());
        aHead1->draw();
        aHead2->flip(true);
        aHead2->setStyle(QGIArrow::getPrefArrowStyle());
        aHead2->setSize(QGIArrow::getPrefArrowSize());
        aHead2->draw();

        float arAngle = atan2(dirDimLine.y, dirDimLine.x) * 180 / M_PI;

        aHead2->show();

        //handle partial arc weird cases
        Base::Vector3d dLineStart;
        Base::Vector3d kinkPoint;
        margin = Rez::guiX(5.f);                                                    //space around label
        double kinkLength = Rez::guiX(5.0);                                                //sb % of horizontal dist(lblCenter,curveCenter)???
        if (isArc) {

            aHead2->hide();
            aHead1->flip(true);
            aHead1->draw();
            Base::Vector3d startPt = Rez::guiX(pts.arcEnds.first);
            Base::Vector3d endPt = Rez::guiX(pts.arcEnds.second);
            kinkLength = (lblCenter.x < curveCenter.x) ? kinkLength : -kinkLength;
            dirDimLine = lblCenter - curveCenter;
            dirDimLine.Normalize();

            Base::Vector3d labelEndDir(1.0,0.0,0.0);   
            if (lblCenter.x > pointOnCurve.x) {         //label is to right of point
                labelEndDir = -1.0 * labelEndDir;
            }
            dLineStart = lblCenter + labelEndDir * (margin + w / 2);
            kinkPoint.y = dLineStart.y;
            kinkPoint.x = dLineStart.x + kinkLength;
            pointOnCurve = curveCenter + dirDimLine * radius;
            if (!dim->leaderIntersectsArc(Rez::appX(dLineStart),Rez::appX(pointOnCurve))) {   //keep pathological case within arc
                if ((pointOnCurve - endPt).Length() < (pointOnCurve - startPt).Length()) {
                    if (!pts.arcCW ) {
                        pointOnCurve = endPt;
                    } else {
                        pointOnCurve = startPt;
                    }
                } else {
                    if (!pts.arcCW) {
                        pointOnCurve = startPt;
                    } else {
                        pointOnCurve = endPt;
                    }
                }
            }
            Base::Vector3d arVector = (kinkPoint - pointOnCurve);
            arVector.Normalize();
            arAngle = atan2(arVector.y, arVector.x) * 180 / M_PI;
            path = QPainterPath();
            path.moveTo(dLineStart.x,dLineStart.y);
            path.lineTo(kinkPoint.x,kinkPoint.y);
            path.lineTo(pointOnCurve.x,pointOnCurve.y);
            datumLabel->setRotation(0.);
        }

        dimLines->setPath(path);

        if (isArc) {
            aHead1->setPos(pointOnCurve.x, pointOnCurve.y);
            aHead1->setRotation(arAngle);
            aHead2->hide();
        } else if(outerPlacement) {
            if(posMode > NoSnap) {
                  aHead1->setPos(arrow2Tip.x, arrow2Tip.y);               //arrow 1's endpoint is arrow2Tip!?
                  aHead2->setPos(arrow1Tip.x, arrow1Tip.y);
                  aHead1->setRotation((posMode == HorizontalSnap) ? 90 : 0);
                  aHead2->setRotation((posMode == HorizontalSnap) ? 90 : 0);
            } else {
                Base::Vector3d vec = (arrow2Tip - curveCenter).Normalize();
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

//        if (dim->CentreLines.getValue()) {
//            curveCenterMark->setPos(curveCenter.x,curveCenter.y);
//            centerMark->show();
//            dim->getViewPart()->addVertex(curveCenter,true);
//        }
    } else if(strcmp(dimType, "Radius") == 0) {
        // preferred terminology: Dimension Text, Dimension Line(s), Extension Lines, Arrowheads
        // radius gets 1 dimension line from the dimension text to a point on the curve

        Base::Vector3d pointOnCurve,curveCenter;
        double radius;
        arcPoints pts = dim->getArcPoints();
        bool isArc = pts.isArc;
        radius = Rez::guiX(pts.radius);
        curveCenter = Rez::guiX(pts.center);
        pointOnCurve = Rez::guiX(pts.onCurve.first);

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
        margin = Rez::guiX(5.f);                                                    //space around label
        double kinkLength = Rez::guiX(5.0);                                                //sb % of horizontal dist(lblCenter,curveCenter)???
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
            Base::Vector3d midPt = Rez::guiX(pts.midArc);
            Base::Vector3d startPt = Rez::guiX(pts.arcEnds.first);
            Base::Vector3d endPt = Rez::guiX(pts.arcEnds.second);
            if (outerPlacement &&
                !dim->leaderIntersectsArc(Rez::appX(curveCenter),Rez::appX(kinkPoint))) {   //keep pathological case within arc
                pointOnCurve = midPt;
            } else if (!outerPlacement) {
                if ((midPt - lblCenter).Length() > (midPt - curveCenter).Length()) {     //label is farther than center
                    dirDimLine = dirDimLine * -1;
                }
                dLineStart = curveCenter + dirDimLine * margin;
                pointOnCurve = curveCenter + dirDimLine * radius;
                kinkPoint = dLineStart;
                if (!dim->leaderIntersectsArc(Rez::appX(dLineStart),Rez::appX(pointOnCurve))) {   //keep pathological case within arc
                    if ((pointOnCurve - endPt).Length() < (pointOnCurve - startPt).Length()) {
                        if (!pts.arcCW ) {
                            pointOnCurve = endPt;
                        } else {
                            pointOnCurve = startPt;
                        }
                    } else {
                        if (!pts.arcCW ) {
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

        aHead1->setStyle(QGIArrow::getPrefArrowStyle());
        aHead1->setSize(QGIArrow::getPrefArrowSize());
        aHead1->draw();

        Base::Vector3d ar1Pos = pointOnCurve;
        Base::Vector3d dirArrowLine = (pointOnCurve - kinkPoint).Normalize();
        float arAngle = atan2(dirArrowLine.y, dirArrowLine.x) * 180 / M_PI;

        aHead1->setPos(ar1Pos.x, ar1Pos.y);
        aHead1->setRotation(arAngle);
        aHead1->show();
        aHead2->hide();
//        if (dim->CentreLines.getValue()) {
//            curveCenterMark->setPos(curveCenter.x,curveCenter.y);
//            centerMark->show();
//            dim->getViewPart()->addVertex(curveCenter,true);
//        }
    } else if(strcmp(dimType, "Angle") == 0) {
        // Only use two straight line edeges for angle
        anglePoints pts = dim->getAnglePoints();
        Base::Vector3d vertex = Rez::guiX(pts.vertex);
        Base::Vector3d legEnd0 = Rez::guiX(pts.ends.first);
        Base::Vector3d legEnd1 = Rez::guiX(pts.ends.second);
        Base::Vector3d p0 = vertex;
        Base::Vector3d dir1 = legEnd0 - vertex;
        Base::Vector3d dir2 = legEnd1 - vertex;
        // Qt y coordinates are flipped
        dir1.y *= -1.;
        dir2.y *= -1.;

        Base::Vector3d labelVec = (lblCenter - p0);

        double labelangle = atan2(-labelVec.y, labelVec.x);    //angle with +X axis on [-PI,+PI]

        double startangle = atan2(dir1.y,dir1.x);
        double range      = atan2(-dir1.y*dir2.x+dir1.x*dir2.y,
                                   dir1.x*dir2.x+dir1.y*dir2.y);

        double endangle = startangle + range;

        float bbX  = datumLabel->boundingRect().width();
        float bbY  = datumLabel->boundingRect().height();

        // Get font height
        QFontMetrics fm(datumLabel->font());

        int h = fm.height();
        double length = labelVec.Length();
        length -= h * 0.6; // Adjust the length so the label isn't over the line

        Base::Vector3d p1 = legEnd0;
        Base::Vector3d p2 = legEnd1;

        // add an offset from the ends
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
            p1 = ar1Pos + (p1-p0).Normalize() * Rez::guiX(5.);    //a bit past arrow head on leg 1
            path.lineTo(p1.x, p1.y);
        }

        if(length > (p2-p0).Length()) {
            path.moveTo(p2.x, p2.y);
            p2 = ar2Pos + (p2-p0).Normalize() * Rez::guiX(5.);    //a bit past leg 2 arrow head on leg 2
            path.lineTo(p2.x, p2.y);
        }


        bool isOutside = true;        //label is outside angle end-vertex-end?

        // TODO find a better solution for this. Addmitedely not tidy
        // ###############
        // Treat zero as positive to be consistent for horizontal lines
        if(std::abs(startangle) < FLT_EPSILON)
            startangle = 0;

         if(std::abs(endangle) < FLT_EPSILON)
            endangle = 0;

        if(startangle >= 0 && endangle >= 0) {   //Both are in positive side
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
        } else if(startangle < 0 && endangle < 0) {  //both are in negative side
           double langle = labelangle;
            if(labelangle > 0)
                langle -= M_PI * 2;
            if(endangle - startangle < 0) {
                if(langle > endangle && langle < startangle)  //clockwise
                    isOutside = false;
            } else {
                if(langle < endangle && langle > startangle)  //anticlockwise
                    isOutside = false;
            }
        } else if(startangle >= 0 && endangle < 0) {
            if(labelangle < startangle && labelangle > endangle)  //clockwise
                isOutside = false;

        } else if(startangle < 0 && endangle >= 0) {
            //Both are in positive side

            if(labelangle > startangle && labelangle < endangle)  //clockwise
                isOutside = false;
        }

        QRectF arcRect(p0.x - length, p0.y - length, 2. * length, 2. * length);
        path.arcMoveTo(arcRect, endangle * 180 / M_PI);
        double innerAngle = fabs(endangle - startangle) * 180.0/M_PI;
        double outerAngle = 360.0 - innerAngle;
        if(isOutside) {
            updateDim(true);
            if(labelangle > endangle)
            {
                path.arcMoveTo(arcRect, endangle * 180 / M_PI);
                path.arcTo(arcRect, endangle * 180 / M_PI, - outerAngle);
            } else {
                path.arcMoveTo(arcRect, startangle * 180 / M_PI);
                path.arcTo(arcRect, startangle * 180 / M_PI, outerAngle);  
            }
        } else {
            updateDim(false);
            path.arcTo(arcRect, endangle * 180 / M_PI, -range * 180 / M_PI);
        }

        dimLines->setPath(path);

        aHead1->flip(true);
        aHead1->setStyle(QGIArrow::getPrefArrowStyle());
        aHead1->setSize(QGIArrow::getPrefArrowSize());
        aHead1->draw();
        aHead2->setStyle(QGIArrow::getPrefArrowStyle());
        aHead2->setSize(QGIArrow::getPrefArrowSize());
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

        // Set the angle of the dimension text

        Base::Vector3d labelNorm(-labelVec.y, labelVec.x, 0.);
        double lAngle = atan2(labelNorm.y, labelNorm.x);

        //if label is more/less vertical, make it vertical
        if (lAngle > M_PI_2+M_PI/12) {    // label norm angle > 90 + 15 = 105
            lAngle -= M_PI;               // lAngle - 180   Flip
        } else if (lAngle <= -M_PI_2+M_PI/12) {  // <  -90 + 15 = - 85
            lAngle += M_PI;               // langle + 180   Flip
        }

        datumLabel->setTransformOriginPoint(bbX / 2., bbY /2.);

        datumLabel->setRotation(lAngle * 180 / M_PI);

    }  //endif Distance/Diameter/Radius/Angle

    // redraw the Dimension and the parent View
    if (hasHover && !isSelected()) {
        aHead1->setPrettyPre();
        aHead2->setPrettyPre();
        dimLines->setPrettyPre();
    } else if (isSelected()) {
        aHead1->setPrettySel();
        aHead2->setPrettySel();
        dimLines->setPrettySel();
    } else {
        aHead1->setPrettyNormal();
        aHead2->setPrettyNormal();
        dimLines->setPrettyNormal();
    }

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
    setPens();
    //double arrowSaveWidth = aHead1->getWidth();
    if (svg) {
        setSvgPens();
    } else {
        setPens();
    }
    QGIView::paint (painter, &myOption, widget);
    setPens();
}

void QGIViewDimension::setSvgPens(void)
{
    double svgLineFactor = 3.0;                     //magic number.  should be a setting somewhere.
    dimLines->setWidth(m_lineWidth/svgLineFactor);
    aHead1->setWidth(aHead1->getWidth()/svgLineFactor);
    aHead2->setWidth(aHead2->getWidth()/svgLineFactor);
}

void QGIViewDimension::setPens(void)
{
    dimLines->setWidth(m_lineWidth);
    aHead1->setWidth(m_lineWidth);
    aHead2->setWidth(m_lineWidth);
}

QColor QGIViewDimension::getNormalColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();

    auto dim( dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject()) );
    if( dim == nullptr )
        return m_colNormal;

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return m_colNormal;
    }

    m_colNormal = vp->Color.getValue().asValue<QColor>();
    return m_colNormal;
}

#include <Mod/TechDraw/Gui/moc_QGIViewDimension.cpp>
