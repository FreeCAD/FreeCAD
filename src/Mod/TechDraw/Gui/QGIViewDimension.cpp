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
#include <Base/UnitsApi.h>
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
#include "DrawGuiUtil.h"

//TODO: hide the Qt coord system (+y down).  

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

    m_dimText = new QGCustomText();
    m_dimText->setParentItem(this);
    m_tolText = new QGCustomText();
    m_tolText->setParentItem(this);
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

    return QGraphicsItem::itemChange(change, value);
}

void QGIDatumLabel::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if(scene() && this == scene()->mouseGrabberItem()) {
        Q_EMIT dragFinished();
    }
      QGraphicsItem::mousePressEvent(event);
}

void QGIDatumLabel::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIDatumLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if(scene() && this == scene()->mouseGrabberItem()) {
        Q_EMIT dragFinished();
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIDatumLabel::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_EMIT hover(true);
    if (!isSelected()) {
        setPrettyPre();
    }
    QGraphicsItem::hoverEnterEvent(event);
}

void QGIDatumLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());
    assert(view != 0);
    Q_UNUSED(view);

    Q_EMIT hover(false);
    if (!isSelected()) {
        setPrettyNormal();
    }
    QGraphicsItem::hoverLeaveEvent(event);
}

QRectF QGIDatumLabel::boundingRect() const
{
    return childrenBoundingRect();
}

void QGIDatumLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

}

void QGIDatumLabel::setPosFromCenter(const double &xCenter, const double &yCenter)
{
    //set label's Qt position(top,left) given boundingRect center point
    setPos(xCenter - m_dimText->boundingRect().width() / 2., yCenter - m_dimText->boundingRect().height() / 2.);
    //set tolerance position
    QRectF labelBox = m_dimText->boundingRect();
    double right = labelBox.right();
    double top   = labelBox.top();
    m_tolText->setPos(right,top);
}

void QGIDatumLabel::setLabelCenter()
{
    //save label's bRect center (posX,posY) given Qt position (top,left)
    posX = x() + m_dimText->boundingRect().width() / 2.;
    posY = y() + m_dimText->boundingRect().height() / 2.;
}

void QGIDatumLabel::setFont(QFont f)
{
    m_dimText->setFont(f);
    QFont tFont(f);
    double fontSize = f.pointSizeF();
    double tolAdj = getTolAdjust();
    tFont.setPointSizeF(fontSize * tolAdj);
    m_tolText->setFont(tFont);
}

void QGIDatumLabel::setDimString(QString t)
{
    prepareGeometryChange();
    m_dimText->setPlainText(t);
} 

void QGIDatumLabel::setTolString()
{
    prepareGeometryChange();
    QGIViewDimension* qgivd = dynamic_cast<QGIViewDimension*>(parentItem());
    if( qgivd == nullptr ) {
        return;                  //tarfu
    }
    const auto dim( dynamic_cast<TechDraw::DrawViewDimension *>(qgivd->getViewObject()) );
    if( dim == nullptr ) {
        return;
    } else if (!dim->hasTolerance()) {
        return;
    }
    
    double overTol = dim->OverTolerance.getValue();
    double underTol = dim->UnderTolerance.getValue();

    int precision = getPrecision();
    QString overFormat = QString::number(overTol,'f', precision);
    QString underFormat = QString::number(underTol,'f',precision);

    QString html = QString::fromUtf8("<div>%1 <br/>%2 </div>");
    html = html.arg(overFormat).arg(underFormat);
    m_tolText->setHtml(html);

    return;
} 

int QGIDatumLabel::getPrecision(void)
{
    int precision;
    bool global = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    global = hGrp->GetBool("UseGlobalDecimals", true);
    if (global) {
        precision = Base::UnitsApi::getDecimals();
    } else {
        precision = hGrp->GetInt("AltDecimals", 2);
    }
    return precision;
}

double QGIDatumLabel::getTolAdjust(void)
{
    double adjust;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    adjust = hGrp->GetFloat("TolSizeAdjust", 0.50);
    return adjust;
}


void QGIDatumLabel::setPrettySel(void)
{
    m_dimText->setPrettySel();
    m_tolText->setPrettySel();
}

void QGIDatumLabel::setPrettyPre(void)
{
    m_dimText->setPrettyPre();
    m_tolText->setPrettyPre();
}

void QGIDatumLabel::setPrettyNormal(void)
{
    m_dimText->setPrettyNormal();
    m_tolText->setPrettyNormal();
}

//**************************************************************
QGIViewDimension::QGIViewDimension() :
    hasHover(false),
    m_lineWidth(0.0),
    m_obtuse(false)
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
         QFont font = datumLabel->getFont();
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
    (void) obtuse;
    const auto dim( dynamic_cast<TechDraw::DrawViewDimension *>(getViewObject()) );
    if( dim == nullptr ) {
        return;
    }
    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }
 
    QString labelText = QString::fromUtf8(dim->getFormatedValue(m_obtuse).c_str());
    
    QFont font = datumLabel->getFont();
    font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));

    datumLabel->setFont(font);
    prepareGeometryChange();
    datumLabel->setDimString(labelText);
    datumLabel->setTolString();
    datumLabel->setPosFromCenter(datumLabel->X(),datumLabel->Y());
}
//this is for formatting and finding centers, not display
QString QGIViewDimension::getLabelText(void)
{
    QString result;
    QString first = datumLabel->getDimText()->toPlainText();
    QString second = datumLabel->getTolText()->toPlainText();
    result = first + second;
    return result;

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

    QString labelText = getLabelText();
    Base::Vector3d lblCenter(datumLabel->X(), datumLabel->Y(), 0);    //already Qt gui coords

    const char *dimType = dim->Type.getValueAsString();

   if (strcmp(dimType, "Distance") == 0 ||
        strcmp(dimType, "DistanceX") == 0 ||
        strcmp(dimType, "DistanceY") == 0) {
        Base::Vector3d stdUp(0.0,1.0,0.0);
        Base::Vector3d stdLeft(-1.0,0.0,0.0);
        pointPair pts = dim->getLinearPoints();
        Base::Vector3d startDist, endDist, midDist;                     //start/end/mid points of distance line
        startDist = Rez::guiX(pts.first);
        endDist   = Rez::guiX(pts.second);
        if (startDist.y < endDist.y) {                                 //measure bottom to top
            Base::Vector3d temp = startDist;
            startDist = endDist;
            endDist = temp;
        }

        Base::Vector3d vecDist = (endDist - startDist);

        // +/- aligned method
        // dimension text legible from bottom or right
        // text outside arrows (not between) iso convention (asme convention)
        // text to left of vertical dims
        // text above horizontal dims
        Base::Vector3d dirDist, normDist;               //direction/normal vectors of distance line
        Base::Vector3d dirExt;
        Base::Vector3d dirDim, normDim;
        Base::Vector3d dirIso;
        dirDist = vecDist;
        dirDist.Normalize();
        normDist = Base::Vector3d (-dirDist.y,dirDist.x, 0);         //normal to distance direction
                                                                     //toward dimension line??? how to tell?
        if (strcmp(dimType, "Distance") == 0 ) {
            //distance and dimension lines parallel
            dirDim = dirDist;
            normDim = normDist;
        } else if (strcmp(dimType, "DistanceX") == 0 ) {
            //distance and dimension lines not (necessarily) parallel
            dirDim = Base::Vector3d ( ((endDist.x - startDist.x >= FLT_EPSILON) ? 1 : -1) , 0, 0);
            normDim = Base::Vector3d (-dirDim.y,dirDim.x, 0);
        } else if (strcmp(dimType, "DistanceY") == 0 ) {
            //distance and dimension lines not (necessarily) parallel
            dirDim = Base::Vector3d (0, 1, 0); 
            normDim = Base::Vector3d (-1, 0, 0);
        }

        //for ortho drawing extension lines are para to normDim, perp to dirDist
        dirExt = normDim;                           //dirExt is para or anti-parallel to real extension direction
        dirIso = normDim;
        if (refObj->isIso()) {
            //is this dimension an iso dimension? ie points +/-isoX,+/-isoY,+/-isoZ
            dirIso = findIsoDir(dirDist);
            dirExt = findIsoExt(dirIso);
        }
        dirExt.Normalize();

        // Get magnitude of angle between dimension line and horizontal
        // to determine rotation of dimension text  (iso convention. asme is always upright)
        //note qt y axis is reversed! and angles are CW!
        double angle = atan2(-dirDim.y,dirDim.x);
        if (angle < 0.0) {
            angle = 2 * M_PI + angle;          //map to +ve angle
        }

        //orient text right side up
        double angleFiddle = M_PI / 10.0;                  // 18 => 10*, 12 => 15*, ...
        if ((angle > M_PI_2 + angleFiddle) &&              // > 100CW   
                   (angle <= M_PI)) {                      // < 180CW -> Q2
            angle += M_PI;                                 // flip CW
        } else if ((angle > M_PI) &&                       // > 180CW
                   (angle <= 1.5*M_PI - angleFiddle))  {   // < 260CW -> Q3
            angle -= M_PI;                                 // flip CCW
        }

        Base::Vector3d textNorm = normDim;
        if (strcmp(dimType, "DistanceX") == 0 ) {
            textNorm = stdUp;                                          //always above
        } else if (strcmp(dimType, "DistanceY") == 0 ) {
            textNorm = stdLeft;                                        //left of dimLine
        } else if (std::abs(dirDist.x) < FLT_EPSILON) {                //this is horizontal dim line
            textNorm = stdLeft;                                        //left of dimLine
        } else if (std::abs(dirDist.y) < FLT_EPSILON) {                //this is vertical dim line
            textNorm = stdLeft;                                        //left of dimLine
        }
        // +/- pos of startDist vs endDist for vert/horiz Dims
        // distStartDelta sb zero for normal dims
        float distStartDelta = vecDist.Dot(normDim);        // component of distance vector in dim line direction
        Base::Vector3d startDistAdj = startDist + normDim * distStartDelta;

        //offset of dimLine from getDimText()
        double offsetFudge = 2.0;
        double textMult  = 1.0;
        if (dim->hasTolerance()) {
            textMult = 1.5;
        }
        double textOffset = textMult * Rez::guiX(vp->Fontsize.getValue()) + offsetFudge;

        QPointF qFigure = boundingRect().center();
        Base::Vector3d figureCenter(qFigure.x(),qFigure.y(),0.0);               
        
        QRectF  mappedRect = mapRectFromItem(datumLabel, datumLabel->boundingRect());
        lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0); 

        //find actual extension direction vector
        Base::Vector3d startIntercept = DrawUtil::Intersect2d(startDist, dirExt,
                                                              lblCenter,dirDim);
        Base::Vector3d dirExtActual = (startIntercept - startDist);
        dirExtActual.Normalize();

        midDist = (startDistAdj + endDist) / 2.0;           //middle point of distance line
        //fauxCenter is where the getDimText() would be if it was on the dimLine
        Base::Vector3d fauxCenter;
        if (strcmp(dimType, "Distance") == 0 ) {                                 //oblique line
                angle = -angle;
                fauxCenter = lblCenter + textOffset * dirExtActual;
                double slope;
                if (DrawUtil::fpCompare(dirDist.x, 0.0)) {
                    slope = std::numeric_limits<float>::max();                  //vertical line
                } else {
                    slope = fabs(dirDist.y / dirDist.x);
                }
                if (slope > 1.0) {                                              //mostly vertical
                    if (lblCenter.x > fauxCenter.x) {                           //label is to right of dimline
                        fauxCenter = lblCenter - textOffset*dirExtActual;       //move dim line closer to figure
                    }
                } else {                                                        //mostly horizontal
                    if (lblCenter.y > fauxCenter.y) {                           //label is below dimline
                        fauxCenter = lblCenter - textOffset*dirExtActual;       //move dim line closer to figure
                    }
                }

        } else if (strcmp(dimType, "DistanceX") == 0 ) {
            fauxCenter = lblCenter + textOffset * textNorm;
        } else if (strcmp(dimType, "DistanceY") == 0 ) {
            angle = - angle;
            fauxCenter = lblCenter - textOffset * textNorm;
        }

        //intersection of extension lines and dimension line
        startIntercept = DrawUtil::Intersect2d(startDist, dirExt,
                                               fauxCenter,dirDim);
        Base::Vector3d endIntercept = DrawUtil::Intersect2d(endDist, dirExt,
                                                         fauxCenter,dirDim);

        dirExtActual = (startIntercept - startDist);
        dirExtActual.Normalize();

        margin = Rez::guiX(2.f);
        float scaler = 1.;

        Base::Vector3d extStartEnd = startIntercept + dirExtActual * (margin * scaler);
        Base::Vector3d extEndEnd   = endIntercept + dirExtActual * (margin * scaler);

        //case 1: inner placement: text between extensions & fits. arros point out from inside (default)
        //case 2: inner placement2: text too big to fit. arrows point in from outside
        //case 3: outer placement: text is outside extensions.  arrows point in, 1 arrow tracks getDimText()

        Base::Vector3d  dim1Tip = startIntercept;
        Base::Vector3d  dim1Tail = fauxCenter;
        Base::Vector3d  dim2Tip =  endIntercept;
        Base::Vector3d  dim2Tail = fauxCenter;
        Base::Vector3d  a1Dir = -dirDim;
        Base::Vector3d  a2Dir = dirDim;
        if (strcmp(dimType, "DistanceY") == 0 ) {
            a1Dir = Base::Vector3d(0,1,0);
            a2Dir = Base::Vector3d(0,-1,0);
        }

        double dimSpan    = (extEndEnd - extStartEnd).Length();     //distance between extension lines
        double fauxToDim1 = (fauxCenter - dim1Tip).Length();        //label to arrow #1
        double fauxToDim2 = (fauxCenter - dim2Tip).Length();        //label to end #2
        double tailLength = Rez::guiX(10.f) * scaler;

        //case2 - innerPlacement && text > span
        double lblWidth = datumLabel->boundingRect().width();
        if ((DrawUtil::isBetween(fauxCenter, dim1Tip, dim2Tip)) &&
            (lblWidth > dimSpan) ) {
            if (strcmp(dimType, "DistanceY") == 0 ) {
                a1Dir = Base::Vector3d(0,-1,0);
                a2Dir = Base::Vector3d(0,1,0);
                dim1Tail = dim1Tip;
                dim2Tail = dim2Tip;
            } else {
                dim1Tail = dim1Tip - tailLength * dirDim;
                dim2Tail = dim2Tip + tailLength * dirDim;
                a1Dir = dirDim;
                a2Dir = -dirDim;
            }
        }

        if (!DrawUtil::isBetween(fauxCenter, dim1Tip, dim2Tip)) {
            //case3 - outerPlacement
            if (strcmp(dimType, "DistanceY") == 0 ) {
                a1Dir = Base::Vector3d(0,-1,0);
                a2Dir = Base::Vector3d(0,1,0);
            } else {
                a1Dir = dirDim;
                a2Dir = -dirDim;
            }
            if (fauxToDim1 < fauxToDim2)  {
                dim1Tail = fauxCenter;
                dim2Tail = dim2Tip;
            } else {
                dim1Tail = dim1Tip;
                dim2Tail = fauxCenter;
            }
        }

        // Extension lines
        QPainterPath path;
        Base::Vector3d gap = startDist + dirExtActual * (margin * scaler);  //space ext line a bt
        path.moveTo(gap.x, gap.y);
        path.lineTo(extStartEnd.x, extStartEnd.y);

        gap = endDist + dirExtActual * (margin * scaler);
        path.moveTo(gap.x, gap.y);
        path.lineTo(extEndEnd.x, extEndEnd.y);

        //Dimension lines (arrow shafts)
        //TODO: line tip goes just a bit too far. overlaps the arrowhead's point.
        path.moveTo(dim1Tip.x, dim1Tip.y);
        path.lineTo(dim1Tail.x, dim1Tail.y);

        path.moveTo(dim2Tip.x, dim2Tip.y);
        path.lineTo(dim2Tail.x, dim2Tail.y);

        dimLines->setPath(path);

        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        double bbX  = datumLabel->boundingRect().width();
        double bbY = datumLabel->boundingRect().height();
        datumLabel->setTransformOriginPoint(bbX / 2, bbY /2);
        double angleOption = 0.0;                                      //put lblText angle adjustments here
        datumLabel->setRotation((angle * 180 / M_PI) + angleOption);
        if (strcmp(dimType, "DistanceY") == 0 ) {
            datumLabel->setRotation(-90.0 + angleOption);
        }


        aHead1->setDirMode(true);
        aHead2->setDirMode(true);
    
        if (vp->FlipArrowheads.getValue()) {
            aHead1->setDirection(a1Dir * -1.0);
            aHead2->setDirection(a2Dir * -1.0);
        } else {
            aHead1->setDirection(a1Dir);
            aHead2->setDirection(a2Dir);
        }
        
        aHead1->setStyle(QGIArrow::getPrefArrowStyle());
        aHead1->setSize(QGIArrow::getPrefArrowSize());
        aHead1->draw();
        aHead2->setStyle(QGIArrow::getPrefArrowStyle());
        aHead2->setSize(QGIArrow::getPrefArrowSize());
        aHead2->draw();

        aHead1->setPos(dim1Tip.x, dim1Tip.y);
        aHead2->setPos(dim2Tip.x, dim2Tip.y);

        aHead1->setDirMode(false);
        aHead2->setDirMode(false);

    } else if(strcmp(dimType, "Diameter") == 0) {
        // terminology: Dimension Text, Dimension Line(s), Extension Lines, Arrowheads
        Base::Vector3d arrow1Tip, arrow2Tip;
        Base::Vector3d arrow1Tail, arrow2Tail;
        Base::Vector3d arrow1Dir,arrow2Dir;
        double radius;
        Base::Vector3d pointOnCurve,curveCenter;
        Base::Vector3d startExt1,endExt1,startExt2,endExt2;

        arcPoints pts = dim->getArcPoints();
        bool isArc = pts.isArc;
        radius = Rez::guiX(pts.radius);
        curveCenter = Rez::guiX(pts.center);
        pointOnCurve = Rez::guiX(pts.onCurve.first);
        
        Base::Vector3d startDist,endDist,dirDim;
        startDist = Rez::guiX(pts.onCurve.first);
        endDist   = Rez::guiX(pts.onCurve.second);
        dirDim    = endDist - startDist;
        if (fabs(dirDim.Length()) < Precision::Confusion())  {
            dirDim = Base::Vector3d(1.0,0.0,0.0);
        }
        dirDim.Normalize();
        Base::Vector3d fauxCenter = lblCenter;

        //default arrow endpoints
        arrow1Tip = curveCenter - dirDim * radius;
        arrow2Tip = curveCenter + dirDim * radius;
        arrow1Tail = curveCenter;
        arrow2Tail = curveCenter;

        double textWidth = datumLabel->getDimText()->boundingRect().width();
        if (dim->hasTolerance()) {
            textWidth += datumLabel->getTolText()->boundingRect().width();
        }

        double gapMargin = Rez::guiX(4.f);
        margin = Rez::guiX(2.f);
        float scaler = 1.;
        double tip = (margin * scaler);
        double gap = (gapMargin * scaler);            //sb % of radius?
        //offset of dimLine from getDimText()
        double offsetFudge = 2.0;
        
        double textMult  = 1.0;
        if (dim->hasTolerance()) {
            textMult = 1.5;
        }
        double vertOffset  = textMult * Rez::guiX(vp->Fontsize.getValue()) + offsetFudge;
        double horizOffset = (textWidth/2.0) + offsetFudge;         //a bit tight w/o more fudge

        bool outerPlacement = false;
        if ((lblCenter-curveCenter).Length() > radius) {                     //label is outside circle
            outerPlacement = true;
        }

//        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = datumLabel->boundingRect().width();
        float bbY = datumLabel->boundingRect().height();
        datumLabel->setTransformOriginPoint(bbX / 2, bbY /2);

        int posMode = NoSnap;
        bool isLeader = false;
        QPainterPath path;
        if(outerPlacement) {
            Base::Vector3d v = (lblCenter - curveCenter);
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

//            fauxCenter = Base::Vector3d(lblCenter.x,lblCenter.y + vertOffset,lblCenter.z);
            if(posMode == VerticalSnap) {
                QRectF  mappedRect = mapRectFromItem(datumLabel, datumLabel->boundingRect());
                lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0); 
                if (lblCenter.y > curveCenter.y) {             //
                    fauxCenter = Base::Vector3d(lblCenter.x,lblCenter.y + vertOffset,lblCenter.z);
                } else {
                    fauxCenter = Base::Vector3d(lblCenter.x,lblCenter.y + vertOffset,lblCenter.z);
                    tip = -tip;
                    gap = -gap;
                }

                arrow1Tip.x = curveCenter.x - radius;                       //to left, on circle cl
                arrow1Tip.y = fauxCenter.y;
                arrow1Tail.x = curveCenter.x;
                arrow1Tail.y = arrow1Tip.y;
                arrow1Dir = (arrow1Tip - arrow1Tail).Normalize();
                path.moveTo(arrow1Tail.x, arrow1Tail.y);
                path.lineTo(arrow1Tip.x, arrow1Tip.y);

                arrow2Tip.x = curveCenter.x + radius;
                arrow2Tip.y = fauxCenter.y;
                arrow2Tail.x = curveCenter.x;
                arrow2Tail.y = arrow2Tip.y;
                arrow2Dir = (arrow2Tip - arrow2Tail).Normalize();
                path.moveTo(arrow2Tail.x, arrow2Tail.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                startExt1.x = curveCenter.x - radius;
                startExt1.y = curveCenter.y + gap;
                endExt1.x   = startExt1.x;
                endExt1.y   = fauxCenter.y + tip;

                startExt2.x = curveCenter.x + radius;
                startExt2.y = curveCenter.y + gap;
                endExt2.x   = startExt2.x;
                endExt2.y   = fauxCenter.y + tip; 

                path.moveTo(startExt1.x, startExt1.y);
                path.lineTo(endExt1.x, endExt1.y);

                path.moveTo(startExt2.x, startExt2.y);
                path.lineTo(endExt2.x, endExt2.y);

                datumLabel->setRotation(0.0);

            } else if(posMode == HorizontalSnap) {
                QRectF  mappedRect = mapRectFromItem(datumLabel, datumLabel->boundingRect());
                lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0); 
             
                if (lblCenter.x > curveCenter.x) {                //label right
//                    fauxCenter = Base::Vector3d(lblCenter.x - horizOffset,lblCenter.y,lblCenter.z);  //unidirection convention
                    fauxCenter = Base::Vector3d(lblCenter.x + vertOffset,lblCenter.y,lblCenter.z);     //aligned convention
                } else {                                          //label left
                    tip = -tip;
                    gap = -gap;
//                    fauxCenter = Base::Vector3d(lblCenter.x + horizOffset,lblCenter.y,lblCenter.z);
                    fauxCenter = Base::Vector3d(lblCenter.x + vertOffset,lblCenter.y,lblCenter.z);
                }

                arrow1Tip.x = fauxCenter.x;
                arrow1Tip.y = curveCenter.y - radius;
                arrow1Tail.x = arrow1Tip.x;
                arrow1Tail.y = curveCenter.y;
                arrow1Dir = (arrow1Tip - arrow1Tail).Normalize();
                path.moveTo(arrow1Tail.x, arrow1Tail.y);
                path.lineTo(arrow1Tip.x, arrow1Tip.y);

                arrow2Tip.x = fauxCenter.x;
                arrow2Tip.y = curveCenter.y + radius;
                arrow2Tail.x = arrow2Tip.x;
                arrow2Tail.y = curveCenter.y;
                arrow2Dir = (arrow2Tip - arrow2Tail).Normalize();
                path.moveTo(arrow2Tail.x, arrow2Tail.y);
                path.lineTo(arrow2Tip.x, arrow2Tip.y);

                startExt1.x = curveCenter.x + gap;
                startExt1.y = curveCenter.y - radius;
                endExt1.x   = fauxCenter.x + tip;
                endExt1.y   = startExt1.y;

                startExt2.x = curveCenter.x + gap;
                startExt2.y = curveCenter.y + radius;
                endExt2.x   = fauxCenter.x + tip;
                endExt2.y   = startExt2.y;

                path.moveTo(startExt1.x, startExt1.y);
                path.lineTo(endExt1.x, endExt1.y);

                path.moveTo(startExt2.x, startExt2.y);
                path.lineTo(endExt2.x, endExt2.y);

                datumLabel->setRotation(-90.0);                     //aligned convention
//                datumLabel->setRotation(0.0);                       //unidirectional convention

            } else {                                                   //outer placement, NoSnap
                QRectF  mappedRect = mapRectFromItem(datumLabel, datumLabel->boundingRect());
                lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0); 
                isLeader = true;
                float spacer = (margin + textWidth / 2);
                spacer = (lblCenter.x < curveCenter.x) ? spacer : -spacer;

                arrow1Tail = lblCenter;
                arrow1Tail.x += spacer;

                Base::Vector3d kinkPoint = arrow1Tail;
                kinkPoint.x += (lblCenter.x < curveCenter.x) ? margin : - margin;

                arrow1Tip = curveCenter + (kinkPoint - curveCenter).Normalize() * radius;

                path.moveTo(arrow1Tail.x, arrow1Tail.y);
                path.lineTo(kinkPoint.x, kinkPoint.y);

                path.lineTo(arrow1Tip.x, arrow1Tip.y);
                arrow1Dir = (arrow1Tip - kinkPoint).Normalize();
                datumLabel->setRotation(0.);
            }
        } else {                                                       //NOT outerplacement ie dimLines are inside circle
            //text always rightside up inside circle
            datumLabel->setRotation(0);
            dirDim = (lblCenter - curveCenter).Normalize();
            if (fabs(dirDim.Length()) < (Precision::Confusion())) {
                dirDim = Base::Vector3d(-1.0,0.0,0.0);
            }
            
            arrow1Tip  = curveCenter - dirDim * radius;
            arrow1Tail = curveCenter;
            arrow1Dir  = (arrow1Tip - arrow1Tail).Normalize();
            arrow2Tip  = curveCenter + dirDim * radius;
            arrow2Tail = curveCenter;
            arrow2Dir  = (arrow2Tip - arrow2Tail).Normalize();

            path.moveTo(arrow1Tail.x, arrow1Tail.y);
            path.lineTo(arrow1Tip.x, arrow1Tip.y);

            path.moveTo(arrow2Tail.x, arrow2Tail.y);
            path.lineTo(arrow2Tip.x, arrow2Tip.y);
        }

        aHead1->setStyle(QGIArrow::getPrefArrowStyle());
        aHead1->setSize(QGIArrow::getPrefArrowSize());

        if (!isLeader) {
            aHead2->setStyle(QGIArrow::getPrefArrowStyle());
            aHead2->setSize(QGIArrow::getPrefArrowSize());
        }

        //handle partial arc weird cases
        //TODO: does anybody dimension Arcs with Diameter? doesn't Radius make more sense?
        Base::Vector3d dLineStart;
        Base::Vector3d kinkPoint;
        margin = Rez::guiX(5.f);
        double kinkLength = Rez::guiX(5.0);                      //sb % of horizontal dist(lblCenter,curveCenter)???
        QPainterPath arcPath;
        if (isArc) {
            if (lblCenter.x > curveCenter.x) {            // label to right of vert c/l
                fauxCenter = Base::Vector3d(lblCenter.x - horizOffset,lblCenter.y,lblCenter.z);
                kinkPoint  = Base::Vector3d(fauxCenter.x - kinkLength,fauxCenter.y,fauxCenter.z);
            } else {
                tip = -tip;
                gap = -gap;
                fauxCenter = Base::Vector3d(lblCenter.x + horizOffset,lblCenter.y,lblCenter.z);
                kinkPoint  = Base::Vector3d(fauxCenter.x + kinkLength,fauxCenter.y,fauxCenter.z);
            }
            dirDim     = (kinkPoint - curveCenter).Normalize();
            pointOnCurve = curveCenter + (dirDim * radius);
            Base::Vector3d startPt = Rez::guiX(pts.arcEnds.first);
            Base::Vector3d endPt = Rez::guiX(pts.arcEnds.second);
            if (!dim->leaderIntersectsArc(Rez::appX(kinkPoint),Rez::appX(pointOnCurve))) {   //keep point within arc
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
            arcPath.moveTo(fauxCenter.x,fauxCenter.y);
            arcPath.lineTo(kinkPoint.x,kinkPoint.y);
            arcPath.lineTo(pointOnCurve.x,pointOnCurve.y);
            arrow1Dir = (pointOnCurve - kinkPoint).Normalize();
            datumLabel->setRotation(0.);
        }

        dimLines->setPath(path);

        if (isArc) {
            dimLines->setPath(arcPath);
            aHead1->setPos(pointOnCurve.x, pointOnCurve.y);
            aHead1->setDirMode(true);
            aHead1->setDirection(arrow1Dir);
            aHead1->draw();
            aHead1->show();
            aHead2->hide();
        } else if(outerPlacement) {
            if(posMode > NoSnap) {                     // Horizontal or Vertical snap
                aHead1->setPos(arrow1Tip.x, arrow1Tip.y);
                aHead1->setDirMode(true);
                aHead1->setDirection(arrow1Dir);
                aHead1->draw();
                aHead1->show();
                aHead2->setPos(arrow2Tip.x, arrow2Tip.y);
                aHead2->setDirMode(true);
                aHead2->setDirection(arrow2Dir);
                aHead2->draw();
                aHead2->show();
            } else {                                  //leader stye
                aHead1->setPos(arrow1Tip.x, arrow1Tip.y);
                aHead1->setDirMode(true);
                aHead1->setDirection(arrow1Dir);
                aHead1->draw();
                aHead1->show();
                aHead2->hide();                             //only 1 arrowhead for NoSnap + outerplacement (ie a leader)
            }
        } else {                                    //inner placement
            aHead1->setPos(arrow1Tip.x, arrow1Tip.y);
            aHead1->setDirMode(true);
            aHead1->setDirection(arrow1Dir);
            aHead1->draw();
            aHead1->show();
            aHead2->setPos(arrow2Tip.x, arrow2Tip.y);
            aHead2->setDirMode(true);
            aHead2->setDirection(arrow2Dir);
            aHead2->draw();
            aHead2->show();
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
        QRectF  mappedRect = mapRectFromItem(datumLabel, datumLabel->boundingRect());
        lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0); 

        // Note Bounding Box size is not the same width or height as text (only used for finding center)
        float bbX  = datumLabel->boundingRect().width();
        float bbY = datumLabel->boundingRect().height();
        datumLabel->setTransformOriginPoint(bbX / 2, bbY /2);
        datumLabel->setRotation(0.0);                                                //label is always right side up & horizontal

        //if inside the arc (len(DimLine < radius)) arrow goes from center to edge away from label
        //if outside the arc arrow kinks, then goes to edge nearest label
        bool outerPlacement = false;
        if ((lblCenter - curveCenter).Length() > radius) {                     //label is outside circle
            outerPlacement = true;
        }

        Base::Vector3d dirDimLine = (lblCenter - curveCenter).Normalize();
        if (fabs(dirDimLine.Length()) < (Precision::Confusion())) {
            dirDimLine = Base::Vector3d(-1.0,0.0,0.0);
        }

        double textWidth = datumLabel->getDimText()->boundingRect().width();
        if (dim->hasTolerance()) {
            textWidth += datumLabel->getTolText()->boundingRect().width();
        }

        Base::Vector3d dLineStart;
        Base::Vector3d kinkPoint;
        margin = Rez::guiX(5.f);                                                //space around label
        double kinkLength = Rez::guiX(5.0);                                //sb % of horizontal dist(lblCenter,curveCenter)???
        if (outerPlacement) {
            double offset = (margin + textWidth / 2.0);
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
    } else if( (strcmp(dimType, "Angle") == 0) ||
               (strcmp(dimType, "Angel3Pt")) ) {
        anglePoints pts = dim->getAnglePoints();
        Base::Vector3d X(1.0,0.0,0.0);
        Base::Vector3d vertex = Rez::guiX(pts.vertex);
        Base::Vector3d legEnd0 = Rez::guiX(pts.ends.first);
        Base::Vector3d legEnd1 = Rez::guiX(pts.ends.second);
        Base::Vector3d dir0 = legEnd0 - vertex;
        Base::Vector3d dir1 = legEnd1 - vertex;
        Base::Vector3d d0 = dir0;
        d0.Normalize();
        Base::Vector3d d1 = dir1;
        d1.Normalize();
        Base::Vector3d leg0 = dir0;
        Base::Vector3d leg1 = dir1;
        // Qt y coordinates are flipped
        dir0.y *= -1.;
        dir1.y *= -1.;

        double insideAngle = dir0.GetAngle(dir1);            // [0,PI]
        double outsideAngle = 2*M_PI - insideAngle;          // [PI,2PI]
        Base::Vector3d tempCross = d0.Cross(d1);
        double insideDir = tempCross.z;
        bool ccwInner = true;
        if (insideDir > 0.0) {
            ccwInner = false;
        }

        QRectF  mappedRect = mapRectFromItem(datumLabel, datumLabel->boundingRect());
        lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0); 

        Base::Vector3d labelVec = (lblCenter - vertex);   //dir from label to vertex

        double textHeight = datumLabel->getDimText()->boundingRect().height();
        if (dim->hasTolerance()) {
            textHeight = datumLabel->getTolText()->boundingRect().height();
        }
        double offsetFudge = 2.0;
        double textOffset = textHeight/2.0 + offsetFudge;
        double radius = labelVec.Length() - textOffset;

        QRectF arcRect(vertex.x - radius, vertex.y - radius, 2. * radius, 2. * radius);
        Base::Vector3d ar0Pos = vertex + d0 * radius;
        Base::Vector3d ar1Pos = vertex + d1 * radius;

        double startangle = atan2(dir0.y,dir0.x);
        if (startangle < 0) {
            startangle += 2.0 * M_PI;
        }
        double endangle = atan2(dir1.y,dir1.x);
        if (endangle < 0) {
            endangle += 2.0 * M_PI;
        }

        Base::Vector3d startExt0 = legEnd0;
        Base::Vector3d startExt1 = legEnd1;
        // add an offset from the ends
        offsetFudge = 5.0;
        startExt0 += d0 * offsetFudge;
        startExt1 += d1 * offsetFudge;

        // Draw the path
        QPainterPath path;

        // Only draw extension lines if outside arc
        double extFudge = 5.0;
        if(radius > (startExt0-vertex).Length()) {
            path.moveTo(startExt0.x, startExt0.y);
            Base::Vector3d endExt0 = ar0Pos + d0*Rez::guiX(extFudge); 
            path.lineTo(endExt0.x, endExt0.y);
        }

        if(radius > (startExt1-vertex).Length()) {
            path.moveTo(startExt1.x, startExt1.y);
            Base::Vector3d endExt1 = ar1Pos + d1*Rez::guiX(extFudge); 
            path.lineTo(endExt1.x, endExt1.y);
        }

        // Treat zero as positive to be consistent for horizontal lines
        if(std::abs(startangle) < FLT_EPSILON)
            startangle = 0;

         if(std::abs(endangle) < FLT_EPSILON)
            endangle = 0;

        //https://stackoverflow.com/questions/13640931/how-to-determine-if-a-vector-is-between-two-other-vectors
        bool isOutside = true;
        if ( ((d0.Cross(labelVec)).Dot(d0.Cross(d1)) >= 0.0) &&
             ((d1.Cross(labelVec)).Dot(d1.Cross(d0)) >= 0.0) ) {
            isOutside = false;
        }

        path.arcMoveTo(arcRect, startangle * 180 / M_PI);
        double actualSweep = 0.0;
        m_obtuse = false;
        if(isOutside) {
            m_obtuse = true;
            if (ccwInner) {              //inner is ccw so outer is cw and sweep is -ve
                actualSweep = -outsideAngle;
            } else {                     //inner is cw so outer is ccw and sweep is +ve
                actualSweep = outsideAngle;
            }
        } else {
            if (ccwInner) {           //inner is ccw and sweep is +ve
                actualSweep = insideAngle;
            } else {             //inner is cw and sweep is -ve
                actualSweep = -insideAngle;
            }
        }
        path.arcTo(arcRect, startangle * 180 / M_PI, actualSweep*180.0/M_PI);  

        dimLines->setPath(path);

//        aHead1->setDirMode(true);
//        aHead2->setDirMode(true);
//        aHead1->setDirection(a1Dir);
//        aHead2->setDirection(a2Dir);
        aHead1->flip(true);
        aHead1->setStyle(QGIArrow::getPrefArrowStyle());
        aHead1->setSize(QGIArrow::getPrefArrowSize());
        aHead1->draw();
        aHead2->setStyle(QGIArrow::getPrefArrowStyle());
        aHead2->setSize(QGIArrow::getPrefArrowSize());
        aHead2->draw();

        aHead1->setPos(ar0Pos.x,ar0Pos.y );
        aHead2->setPos(ar1Pos.x,ar1Pos.y );

        Base::Vector3d norm1 = leg0;
        Base::Vector3d norm2 = leg1;
        Base::Vector3d avg = (norm1 + norm2) / 2.;   //midline of legs

        norm1 = norm1.ProjectToLine(avg, norm1);
        norm2 = norm2.ProjectToLine(avg, norm2);

        float ar0angle = atan2(-norm1.y, -norm1.x) * 180 / M_PI;
        float ar1angle = atan2(norm2.y, norm2.x) * 180 / M_PI;

        if(isOutside) {
            aHead1->setRotation(ar0angle + 180.);
            aHead2->setRotation(ar1angle + 180.);
        } else {
            aHead1->setRotation(ar0angle);
            aHead2->setRotation(ar1angle);
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

        float bbX  = datumLabel->boundingRect().width();
        float bbY  = datumLabel->boundingRect().height();
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

//! find the closest isometric axis given an ortho vector
Base::Vector3d QGIViewDimension::findIsoDir(Base::Vector3d ortho)
{
    std::vector<Base::Vector3d> isoDirs = { Base::Vector3d(0.866,0.5,0.0),     //iso X
                                            Base::Vector3d(-0.866,-0.5,0.0),   //iso -X
                                            Base::Vector3d(-0.866,0.5,0.0),    //iso -Y?
                                            Base::Vector3d(0.866,-0.5,0.0),    //iso +Y?
                                            Base::Vector3d(0.0,-1.0,0.0),      //iso -Z
                                            Base::Vector3d(0.0,1.0,0.0) };     //iso Z
    std::vector<double> angles;
    for (auto& iso: isoDirs) {
        angles.push_back(ortho.GetAngle(iso));
    }
    int idx = 0;
    double min = angles[0];
    for (int i = 1; i < 6; i++) {
        if (angles[i] < min) {
            idx = i;
            min = angles[i];
        }
    }
    return isoDirs[idx];
}

//! find the iso extension direction corresponding to an iso dist direction
Base::Vector3d QGIViewDimension::findIsoExt(Base::Vector3d dir)
{
    Base::Vector3d dirExt(1,0,0);
    Base::Vector3d isoX(0.866,0.5,0.0);     //iso X
    Base::Vector3d isoXr(-0.866,-0.5,0.0);  //iso -X
    Base::Vector3d isoY(-0.866,0.5,0.0);    //iso -Y?
    Base::Vector3d isoYr(0.866,-0.5,0.0);   //iso +Y?
    Base::Vector3d isoZ(0.0,1.0,0.0);       //iso Z
    Base::Vector3d isoZr(0.0,-1.0,0.0);     //iso -Z
    if (dir.IsEqual(isoX, FLT_EPSILON)) {
        dirExt = isoY;
    } else if (dir.IsEqual(-isoX, FLT_EPSILON)) {
        dirExt = -isoY;
    } else if (dir.IsEqual(isoY, FLT_EPSILON)) {
        dirExt = isoZ;
    } else if (dir.IsEqual(-isoY, FLT_EPSILON)) {
        dirExt = -isoZ;
    } else if (dir.IsEqual(isoZ, FLT_EPSILON)) {
        dirExt = isoX;
    } else if (dir.IsEqual(-isoZ, FLT_EPSILON)) {
        dirExt = -isoX;
    } else { //tarfu
        Base::Console().Message("QGIVD::findIsoExt - %s - input is not iso axis\n", getViewObject()->getNameInDocument());
    }

    return dirExt;
}

#include <Mod/TechDraw/Gui/moc_QGIViewDimension.cpp>
