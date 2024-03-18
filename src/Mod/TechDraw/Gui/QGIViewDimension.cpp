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

#ifdef FC_OS_WIN32
# define _USE_MATH_DEFINES //resolves Windows & M_PI issues
#endif

#ifndef _PreComp_
# include <cmath>

# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QPaintDevice>
# include <QPainter>
# include <QPainterPath>
# include <QSvgGenerator>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>
#include <Gui/Command.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIViewDimension.h"
#include "PreferencesGui.h"
#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "QGIVertex.h"
#include "QGCustomSvg.h"
#include "ViewProviderDimension.h"
#include "ZVALUE.h"


#define NORMAL 0
#define PRE 1
#define SEL 2


//TODO: hide the Qt coord system (+y down).

using namespace TechDraw;
using namespace TechDrawGui;

enum SnapMode
{
    NoSnap,
    VerticalSnap,
    HorizontalSnap
};

enum DragState
{
    NoDrag,
    DragStarted,
    Dragging
};


QGIDatumLabel::QGIDatumLabel() : m_dragState(NoDrag)
{
    verticalSep = false;
    posX = 0;
    posY = 0;

    parent = nullptr;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);
    setFiltersChildEvents(true);

    m_dimText = new QGCustomText();
    m_dimText->setTightBounding(true);
    m_dimText->setParentItem(this);
    m_tolTextOver = new QGCustomText();
    m_tolTextOver->setTightBounding(true);
    m_tolTextOver->setParentItem(this);
    m_tolTextUnder = new QGCustomText();
    m_tolTextUnder->setTightBounding(true);
    m_tolTextUnder->setParentItem(this);
    m_unitText = new QGCustomText();
    m_unitText->setTightBounding(true);
    m_unitText->setParentItem(this);

    m_ctrl = false;

    m_isFramed = false;
    m_lineWidth = Rez::guiX(0.5);
}

QVariant QGIDatumLabel::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            setPrettySel();
        }
        else {
            setPrettyNormal();
            if (m_dragState == Dragging) {
                //stop the drag if we are no longer selected.
                m_dragState = NoDrag;
                Q_EMIT dragFinished();
            }
        }
    }
    else if (change == ItemPositionHasChanged && scene()) {
        setLabelCenter();
        m_dragState = Dragging;
        Q_EMIT dragging(m_ctrl);
    }

    return QGraphicsItem::itemChange(change, value);
}

void QGIDatumLabel::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        m_ctrl = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void QGIDatumLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    //    Base::Console().Message("QGIDL::mouseReleaseEvent()\n");
    m_ctrl = false;
    if (m_dragState == Dragging) {
        m_dragState = NoDrag;
        Q_EMIT dragFinished();
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIDatumLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    QGIViewDimension* qgivDimension = dynamic_cast<QGIViewDimension*>(parentItem());
    if (!qgivDimension) {
        qWarning() << "QGIDatumLabel::mouseDoubleClickEvent: No parent item";
        return;
    }

    auto ViewProvider = dynamic_cast<ViewProviderDimension*>(
        qgivDimension->getViewProvider(qgivDimension->getViewObject()));
    if (!ViewProvider) {
        qWarning() << "QGIDatumLabel::mouseDoubleClickEvent: No valid view provider";
        return;
    }

    ViewProvider->startDefaultEditMode();
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void QGIDatumLabel::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_EMIT hover(true);
    if (!isSelected()) {
        setPrettyPre();
    }
    else {
        setPrettySel();
    }
    QGraphicsItem::hoverEnterEvent(event);
}

void QGIDatumLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_EMIT hover(false);
    if (!isSelected()) {
        setPrettyNormal();
    }
    else {
        setPrettySel();
    }

    QGraphicsItem::hoverLeaveEvent(event);
}

QRectF QGIDatumLabel::boundingRect() const
{
    QRectF result = childrenBoundingRect();
    result.adjust(-m_lineWidth * 4.0, 0.0, 0.0, 0.0);
    return result;
}

void QGIDatumLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                          QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //    painter->setPen(Qt::blue);
    //    painter->drawRect(boundingRect());          //good for debugging

    if (m_isFramed) {
        QPen prevPen = painter->pen();
        QPen framePen(prevPen);

        framePen.setWidthF(m_lineWidth);
        framePen.setColor(m_dimText->defaultTextColor());

        painter->setPen(framePen);
        painter->drawRect(boundingRect());
        painter->setPen(prevPen);
    }
}

void QGIDatumLabel::setPosFromCenter(const double& xCenter, const double& yCenter)
{
    prepareGeometryChange();
    QGIViewDimension* qgivd = dynamic_cast<QGIViewDimension*>(parentItem());
    if (!qgivd) {
        return;
    }
    const auto dim(dynamic_cast<TechDraw::DrawViewDimension*>(qgivd->getViewObject()));
    if (!dim) {
        return;
    }

    //set label's Qt position(top, left) given boundingRect center point
    setPos(xCenter - m_dimText->boundingRect().width() / 2.,
           yCenter - m_dimText->boundingRect().height() / 2.);

    QString uText = m_unitText->toPlainText();
    if ((uText.size() > 0) && (uText.at(0) != QChar::fromLatin1(' '))) {
        QString vText = m_dimText->toPlainText();
        vText = vText + uText;
        m_dimText->setPlainText(vText);
        m_unitText->setPlainText(QString());
    }

    QRectF labelBox = m_dimText->boundingRect();
    double right = labelBox.right();
    double top = labelBox.top();
    double bottom = labelBox.bottom();
    double middle = (top + bottom) / 2.0;

    //set unit position
    QRectF unitBox = m_unitText->boundingRect();
    double unitWidth = unitBox.width();
    double unitRight = right + unitWidth;
    // Set the m_unitText font *baseline* at same height as the m_dimText font baseline
    m_unitText->setPos(right, 0.0);

    //set tolerance position
    QRectF overBox = m_tolTextOver->boundingRect();
    double tolLeft  = unitRight;

    // Adjust for difference in tight and original bounding box sizes, note the y-coord down system
    QPointF tol_adj = m_tolTextOver->tightBoundingAdjust();
    m_tolTextOver->justifyLeftAt(tolLeft + tol_adj.x(), middle - tol_adj.y(), false);
    tol_adj = m_tolTextUnder->tightBoundingAdjust();
    m_tolTextUnder->justifyLeftAt(tolLeft + tol_adj.x(), middle + overBox.height() - tol_adj.y(),
                                   false);
}

void QGIDatumLabel::setLabelCenter()
{
    //save label's bRect center (posX, posY) given Qt position (top, left)
    posX = x() + m_dimText->boundingRect().width() / 2.;
    posY = y() + m_dimText->boundingRect().height() / 2.;
}

void QGIDatumLabel::setFont(QFont font)
{
    prepareGeometryChange();
    m_dimText->setFont(font);
    m_unitText->setFont(font);
    QFont tFont(font);
    double fontSize = font.pixelSize();
    double tolAdj = getTolAdjust();
    tFont.setPixelSize((int)(fontSize * tolAdj));
    m_tolTextOver->setFont(tFont);
    m_tolTextUnder->setFont(tFont);
}

void QGIDatumLabel::setDimString(QString text)
{
    prepareGeometryChange();
    m_dimText->setPlainText(text);
}

void QGIDatumLabel::setDimString(QString text, qreal maxWidth)
{
    prepareGeometryChange();
    m_dimText->setPlainText(text);
    m_dimText->setTextWidth(maxWidth);
}

void QGIDatumLabel::setToleranceString()
{
    prepareGeometryChange();
    QGIViewDimension* qgivd = dynamic_cast<QGIViewDimension*>(parentItem());
    if (!qgivd) {
        return;
    }
    const auto dim(dynamic_cast<TechDraw::DrawViewDimension*>(qgivd->getViewObject()));
    if (!dim) {
        return;
        // don't show if both are zero or if EqualTolerance is true
    }
    else if (!dim->hasOverUnderTolerance() || dim->EqualTolerance.getValue()
             || dim->TheoreticalExact.getValue()) {
        m_tolTextOver->hide();
        m_tolTextUnder->hide();
        // we must explicitly empty the text otherwise the frame drawn for
        // TheoreticalExact would be as wide as necessary for the text
        m_tolTextOver->setPlainText(QString());
        m_tolTextUnder->setPlainText(QString());
        return;
    }

    std::pair<std::string, std::string> labelTexts, unitTexts;

    if (dim->ArbitraryTolerances.getValue()) {
        labelTexts = dim->getFormattedToleranceValues(1);//copy tolerance spec
        unitTexts.first = "";
        unitTexts.second = "";
    }
    else {
        if (dim->isMultiValueSchema()) {
            labelTexts = dim->getFormattedToleranceValues(0);//don't format multis
            unitTexts.first = "";
            unitTexts.second = "";
        }
        else {
            labelTexts = dim->getFormattedToleranceValues(1);// prefix value [unit] postfix
            unitTexts = dim->getFormattedToleranceValues(2); //just the unit
        }
    }

    if (labelTexts.first.empty()) {
        m_tolTextUnder->hide();
    }
    else {
        m_tolTextUnder->setPlainText(QString::fromUtf8(labelTexts.first.c_str()));
        m_tolTextUnder->show();
    }
    if (labelTexts.second.empty()) {
        m_tolTextOver->hide();
    }
    else {
        m_tolTextOver->setPlainText(QString::fromUtf8(labelTexts.second.c_str()));
        m_tolTextOver->show();
    }

    return;
}

void QGIDatumLabel::setUnitString(QString text)
{
    prepareGeometryChange();
    if (text.isEmpty()) {
        m_unitText->hide();
    }
    else {
        m_unitText->setPlainText(text);
        m_unitText->show();
    }
}


int QGIDatumLabel::getPrecision()
{
    if (Preferences::useGlobalDecimals()) {
        return Base::UnitsApi::getDecimals();
    }
    return Preferences::getPreferenceGroup("Dimensions")->GetInt("AltDecimals", 2);
}

double QGIDatumLabel::getTolAdjust()
{
    return Preferences::getPreferenceGroup("Dimensions")->GetFloat("TolSizeAdjust", 0.50);
}


void QGIDatumLabel::setPrettySel()
{
    //    Base::Console().Message("QGIDL::setPrettySel()\n");
    m_dimText->setPrettySel();
    m_tolTextOver->setPrettySel();
    m_tolTextUnder->setPrettySel();
    m_unitText->setPrettySel();
    Q_EMIT setPretty(SEL);
}

void QGIDatumLabel::setPrettyPre()
{
    //    Base::Console().Message("QGIDL::setPrettyPre()\n");
    m_dimText->setPrettyPre();
    m_tolTextOver->setPrettyPre();
    m_tolTextUnder->setPrettyPre();
    m_unitText->setPrettyPre();
    Q_EMIT setPretty(PRE);
}

void QGIDatumLabel::setPrettyNormal()
{
    //    Base::Console().Message("QGIDL::setPrettyNormal()\n");
    m_dimText->setPrettyNormal();
    m_tolTextOver->setPrettyNormal();
    m_tolTextUnder->setPrettyNormal();
    m_unitText->setPrettyNormal();
    Q_EMIT setPretty(NORMAL);
}

void QGIDatumLabel::setColor(QColor color)
{
    //    Base::Console().Message("QGIDL::setColor(%s)\n", qPrintable(c.name()));
    m_colNormal = color;
    m_dimText->setColor(m_colNormal);
    m_tolTextOver->setColor(m_colNormal);
    m_tolTextUnder->setColor(m_colNormal);
    m_unitText->setColor(m_colNormal);
}

//**************************************************************
QGIViewDimension::QGIViewDimension() : dvDimension(nullptr), hasHover(false), m_lineWidth(0.0)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setAcceptHoverEvents(false);
    setCacheMode(QGraphicsItem::NoCache);

    datumLabel = new QGIDatumLabel();
    datumLabel->setQDim(this);

    addToGroup(datumLabel);

    dimLines = new QGIDimLines();
    addToGroup(dimLines);

    aHead1 = new QGIArrow();
    addToGroup(aHead1);

    aHead2 = new QGIArrow();
    addToGroup(aHead2);

    datumLabel->setZValue(ZVALUE::DIMENSION);
    aHead1->setZValue(ZVALUE::DIMENSION);
    aHead2->setZValue(ZVALUE::DIMENSION);
    dimLines->setZValue(ZVALUE::DIMENSION);
    dimLines->setStyle(Qt::SolidLine);

    // connecting the needed slots and signals
    QObject::connect(datumLabel, &QGIDatumLabel::dragging, this, &QGIViewDimension::datumLabelDragged);

    QObject::connect(datumLabel, &QGIDatumLabel::dragFinished, this, &QGIViewDimension::datumLabelDragFinished);

    QObject::connect(datumLabel, &QGIDatumLabel::selected, this, &QGIViewDimension::select);

    QObject::connect(datumLabel, &QGIDatumLabel::hover, this, &QGIViewDimension::hover);

    QObject::connect(datumLabel, &QGIDatumLabel::setPretty, this, &QGIViewDimension::onPrettyChanged);

    setZValue(ZVALUE::DIMENSION);//note: this won't paint dimensions over another View if it stacks
                                 //above this Dimension's parent view.   need Layers?
    hideFrame();

    m_refFlag = new QGCustomSvg();
    m_refFlag->setParentItem(this);
    m_refFlag->load(QString::fromUtf8(":/icons/TechDraw_RefError.svg"));
    m_refFlag->setZValue(ZVALUE::LOCK);
    m_refFlag->hide();
}

QVariant QGIViewDimension::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            datumLabel->setSelected(true);
        }
        else {
            datumLabel->setSelected(false);
        }
        draw();
        return value;
    }
    if(change == ItemPositionChange && scene()) {
        // QGIVDimension doesn't really change position the way other views do.
        // If we call QGIView::itemChange it will set the position to (0,0) instead of
        // using the label's position, and the Dimension will be in the wrong place.
        // QGIVBalloon behaves the same way.
        return QGraphicsItem::itemChange(change, value);
    }
    return QGIView::itemChange(change, value);
}

bool QGIViewDimension::getGroupSelection()
{
    return datumLabel->isSelected();
}

//Set selection state for this and its children
void QGIViewDimension::setGroupSelection(bool isSelected)
{
    //    Base::Console().Message("QGIVD::setGroupSelection(%d)\n", b);
    setSelected(isSelected);
    datumLabel->setSelected(isSelected);
    dimLines->setSelected(isSelected);
    aHead1->setSelected(isSelected);
    aHead2->setSelected(isSelected);
}

void QGIViewDimension::select(bool state)
{
    setSelected(state);
    draw();
}

//surrogate for hover enter (true), hover leave (false) events
void QGIViewDimension::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewDimension::setViewPartFeature(TechDraw::DrawViewDimension* obj)
{
    //    Base::Console().Message("QGIVD::setViewPartFeature()\n");
    if (!obj) {
        return;
    }

    setViewFeature(static_cast<TechDraw::DrawView*>(obj));
    dvDimension = obj;

    // Set the QGIGroup Properties based on the DrawView
    float x = Rez::guiX(obj->X.getValue());
    float y = Rez::guiX(-obj->Y.getValue());

    datumLabel->setPosFromCenter(x, y);

    setNormalColorAll();
    setPrettyNormal();

    updateDim();
    draw();
}

void QGIViewDimension::setNormalColorAll()
{
    QColor qc = prefNormalColor();
    datumLabel->setColor(qc);
    dimLines->setNormalColor(qc);
    aHead1->setNormalColor(qc);
    aHead1->setFillColor(qc);
    aHead2->setNormalColor(qc);
    aHead2->setFillColor(qc);
}

//QGIViewDimension does not behave the same as other QGIView derived classes
//and so mouse events need to be ignored.  Only the QGIDatumLabel mouse events are relevant.
void QGIViewDimension::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    //    Base::Console().Message("QGIVD::mousePressEvent() - %s\n", getViewName());
    QGraphicsItem::mousePressEvent(event);
}

void QGIViewDimension::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIViewDimension::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    //    Base::Console().Message("QGIVDim::mouseReleaseEvent() - %s\n", getViewName());
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIViewDimension::updateView(bool update)
{
    Q_UNUSED(update);
    auto dim(dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject()));
    if (!dim) {
        return;
    }

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    if (update || dim->X.isTouched() || dim->Y.isTouched()) {
        float x = Rez::guiX(dim->X.getValue());
        float y = Rez::guiX(dim->Y.getValue());
        datumLabel->setPosFromCenter(x, -y);
        updateDim();
    }
    else if (vp->Fontsize.isTouched() || vp->Font.isTouched()) {
        updateDim();
    }
    else if (vp->LineWidth.isTouched()) {
        m_lineWidth = vp->LineWidth.getValue();
        updateDim();
    }
    else {
        updateDim();
    }

    if (dim->hasGoodReferences()) {
        m_refFlag->hide();
    } else {
        m_refFlag->centerAt(datumLabel->pos() + datumLabel->boundingRect().center());
        m_refFlag->show();
    }

    draw();
}

void QGIViewDimension::updateDim()
{
    const auto dim(dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject()));
    if (!dim) {
        return;
    }
    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    QString labelText =
        QString::fromUtf8(dim->getFormattedDimensionValue(1).c_str());// pre value [unit] post
    if (dim->isMultiValueSchema()) {
        labelText =
            QString::fromUtf8(dim->getFormattedDimensionValue(0).c_str());//don't format multis
    }

    QFont font = datumLabel->getFont();
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    int fontSize = QGIView::exactFontSize(vp->Font.getValue(), vp->Fontsize.getValue());
    font.setPixelSize(fontSize);
    datumLabel->setFont(font);

    prepareGeometryChange();
    datumLabel->setDimString(labelText);
    datumLabel->setToleranceString();
    datumLabel->setPosFromCenter(datumLabel->X(), datumLabel->Y());

    datumLabel->setFramed(dim->TheoreticalExact.getValue());
    datumLabel->setLineWidth(m_lineWidth);
}

void QGIViewDimension::datumLabelDragged(bool ctrl)
{
    Q_UNUSED(ctrl);
    draw();
}

void QGIViewDimension::datumLabelDragFinished()
{
    auto dim(dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject()));

    if (!dim) {
        return;
    }

    double x = Rez::appX(datumLabel->X()), y = Rez::appX(datumLabel->Y());
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Drag Dimension"));
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.X = %f",
                            dim->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Y = %f",
                            dim->getNameInDocument(), -y);
    Gui::Command::commitCommand();
}

//this is for formatting and finding centers, not display
QString QGIViewDimension::getLabelText()
{
    QString first = datumLabel->getDimText()->toPlainText();
    QString second = datumLabel->getTolTextOver()->toPlainText();
    QString third = datumLabel->getTolTextUnder()->toPlainText();
    if (second.length() > third.length()) {
        return first + second;
    }
    else {
        return first + third;
    }
}

void QGIViewDimension::draw()
{
    prepareGeometryChange();
    if (!isVisible()) {
        return;
    }

    TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject());
    if (!dim ||//nothing to draw, don't try
        !dim->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())
        || !dim->has2DReferences()) {
        datumLabel->hide();
        hide();
        return;
    }

    const TechDraw::DrawViewPart* refObj = dim->getViewPart();
    if (!refObj) {
        return;
    }
    if (!refObj->hasGeometry()) {//nothing to draw yet (restoring)
        datumLabel->hide();
        hide();
        return;
    }

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if (!vp) {
        datumLabel->show();
        show();
        return;
    }

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());
    datumLabel->setRotation(0.0);
    datumLabel->show();

    resetArrows();
    show();

    if (vp->RenderingExtent.getValue() > ViewProviderDimension::REND_EXTENT_NONE) {
        // We are expected to draw something, not just display the value
        const char* dimType = dim->Type.getValueAsString();

        if (strcmp(dimType, "Distance") == 0 || strcmp(dimType, "DistanceX") == 0
            || strcmp(dimType, "DistanceY") == 0) {
            drawDistance(dim, vp);
        }
        else if (strcmp(dimType, "Diameter") == 0) {
            drawDiameter(dim, vp);
        }
        else if (strcmp(dimType, "Radius") == 0) {
            drawRadius(dim, vp);
        }
        else if (strcmp(dimType, "Angle") == 0 || strcmp(dimType, "Angle3Pt") == 0) {
            drawAngle(dim, vp);
        }
        else {
            Base::Console().Error("QGIVD::draw - this DimensionType is unknown: %s\n", dimType);
        }
    }
    else {
        // No dimension lines are drawn, the arrows are hidden
        dimLines->setPath(QPainterPath());
        drawArrows(0, nullptr, nullptr, false);
    }

    // reset the colors
    if (hasHover && !datumLabel->isSelected()) {
        setPrettyPre();
    }
    else if (datumLabel->isSelected()) {
        setPrettySel();
    }
    else {
        setPrettyNormal();
    }

    update();
    if (parentItem()) {
        //TODO: parent redraw still required with new frame/label??
        parentItem()->update();
    }
}

double QGIViewDimension::getAnglePlacementFactor(double testAngle, double endAngle,
                                                 double startRotation)
{
    if (startRotation > 0.0) {
        startRotation = -startRotation;
        endAngle -= startRotation;
        if (endAngle > M_PI) {
            endAngle -= M_2PI;
        }
    }

    if (testAngle > endAngle) {
        testAngle -= M_2PI;
    }

    if (testAngle >= endAngle + startRotation) {
        return +1.0;
    }

    testAngle += M_PI;
    if (testAngle > endAngle) {
        testAngle -= M_2PI;
    }

    if (testAngle >= endAngle + startRotation) {
        return -1.0;
    }

    return 0.0;
}

int QGIViewDimension::compareAngleStraightness(double straightAngle, double leftAngle,
                                               double rightAngle, double leftStrikeFactor,
                                               double rightStrikeFactor)
{
    double leftDelta = DrawUtil::angleComposition(M_PI, straightAngle - leftAngle);
    double rightDelta = DrawUtil::angleComposition(rightAngle, -straightAngle);

    if (fabs(leftDelta - rightDelta) <= Precision::Confusion()) {
        return 0;
    }

    if (leftStrikeFactor == rightStrikeFactor) {
        double leftBend = fabs(leftDelta);
        double rightBend = fabs(rightDelta);

        return DrawUtil::sgn(leftBend - rightBend);
    }

    return rightStrikeFactor > leftStrikeFactor ? -1 : +1;
}

double QGIViewDimension::getIsoStandardLinePlacement(double labelAngle)
{
    // According to ISO 129-1 Standard Figure 23, the bordering angle is 1/2 PI, resp. -1/2 PI
    return labelAngle < -M_PI / 2.0 || labelAngle > +M_PI / 2.0 ? +1.0 : -1.0;
}

Base::Vector2d QGIViewDimension::getIsoRefOutsetPoint(const Base::BoundBox2d& labelRectangle,
                                                      bool right) const
{
    return Base::Vector2d(right ? labelRectangle.MinX - getDefaultIsoReferenceLineOverhang()
                                : labelRectangle.MaxX + getDefaultIsoReferenceLineOverhang(),
                          labelRectangle.MinY - getIsoDimensionLineSpacing());
}

Base::Vector2d QGIViewDimension::getIsoRefJointPoint(const Base::BoundBox2d& labelRectangle,
                                                     bool right) const
{
    return getIsoRefOutsetPoint(labelRectangle, !right);
}

Base::Vector2d QGIViewDimension::getAsmeRefOutsetPoint(const Base::BoundBox2d& labelRectangle,
                                                       bool right) const
{
    return Base::Vector2d(right ? labelRectangle.MaxX : labelRectangle.MinX,
                          labelRectangle.GetCenter().y);
}

Base::Vector2d QGIViewDimension::getAsmeRefJointPoint(const Base::BoundBox2d& labelRectangle,
                                                      bool right) const
{
    return Base::Vector2d(right ? labelRectangle.MaxX + getDefaultAsmeHorizontalLeaderLength()
                                : labelRectangle.MinX - getDefaultAsmeHorizontalLeaderLength(),
                          labelRectangle.GetCenter().y);
}

//find intersection of line L (through linePoint at angle lineAngle) and a line perpendicular to L
//passing through perpendicularPoint
//tricky vector algebra note:
//a*b is the magnitude of the projection of a onto b
//so we project a vector linePoint-perpendicularPoint onto unit vector in lineAngle direction giving
//the distance from linePoint to intersection, then make a displacement vector and add it to linePoint
Base::Vector2d QGIViewDimension::computePerpendicularIntersection(
    const Base::Vector2d& linePoint, const Base::Vector2d& perpendicularPoint, double lineAngle)
{
    return linePoint
        + Base::Vector2d::FromPolar((perpendicularPoint - linePoint)
                                        * Base::Vector2d::FromPolar(1.0, lineAngle),
                                    lineAngle);
}

//calculate the end points of 1 extension line
//originPoint - a point on the distance line (end point)
//linePoint - point on dimension line that is perpendicular projection of distance line point
//            onto dimension line
//1 extension line end point is the return value
//1 extension line end point is modified parameter startPoint
Base::Vector2d QGIViewDimension::computeExtensionLinePoints(const Base::Vector2d& originPoint,
                                                            const Base::Vector2d& linePoint,
                                                            double hintAngle, double overhangSize,
                                                            double gapSize,
                                                            Base::Vector2d& startPoint)
{
    Base::Vector2d direction(linePoint - originPoint);
    double rawLength = direction.Length();

    if (rawLength <= Precision::Confusion()) {
        direction = Base::Vector2d::FromPolar(1.0, hintAngle);
    }
    else {
        direction = direction / rawLength;
    }

    if (overhangSize > rawLength - gapSize) {
        // The extension line would be smaller than extension line overhang, keep it at least so long
        startPoint = linePoint - overhangSize * direction;
    }
    else {
        startPoint = linePoint - (rawLength - gapSize) * direction;
    }

    return linePoint + overhangSize * direction;
}

double QGIViewDimension::computeLineAndLabelAngles(const Base::Vector2d& rotationCenter,
                                                   const Base::Vector2d& labelCenter,
                                                   double lineLabelDistance, double& lineAngle,
                                                   double& labelAngle)
{
    // By default horizontal line and no label rotation
    lineAngle = 0.0;
    labelAngle = 0.0;

    Base::Vector2d rawDirection(labelCenter - rotationCenter);
    double rawDistance = rawDirection.Length();
    if (rawDistance <= Precision::Confusion()) {// Almost single point, can't tell
        return 0.0;
    }

    double rawAngle = rawDirection.Angle();
    lineAngle = rawAngle;

    // If we are too close to the line origin, no further adjustments
    if (lineLabelDistance >= rawDistance) {
        return 0.0;
    }

    // Rotate the line by angle between the label rectangle center and label bottom side center
    double devAngle = getIsoStandardLinePlacement(rawAngle) * asin(lineLabelDistance / rawDistance);
    lineAngle = DrawUtil::angleComposition(lineAngle, devAngle);

    labelAngle = devAngle < 0.0 ? lineAngle : DrawUtil::angleComposition(lineAngle, M_PI);

    return devAngle;
}

double
QGIViewDimension::computeLineStrikeFactor(const Base::BoundBox2d& labelRectangle,
                                          const Base::Vector2d& lineOrigin, double lineAngle,
                                          const std::vector<std::pair<double, bool>>& drawMarking)
{
    if (drawMarking.size() < 2) {
        return 0.0;
    }

    std::vector<Base::Vector2d> intersectionPoints;
    unsigned int startIndex = 0;
    unsigned int currentIndex = 1;

    while (currentIndex < drawMarking.size()) {
        if (drawMarking[currentIndex].second != drawMarking[startIndex].second) {
            if (drawMarking[startIndex].second) {
                double segmentBase = drawMarking[startIndex].first;
                double segmentLength = drawMarking[currentIndex].first - segmentBase;

                DrawUtil::findLineSegmentRectangleIntersections(lineOrigin, lineAngle, segmentBase,
                                                                segmentLength, labelRectangle,
                                                                intersectionPoints);
            }

            startIndex = currentIndex;
        }

        ++currentIndex;
    }

    return intersectionPoints.size() >= 2 ? 1.0 : 0.0;
}

double
QGIViewDimension::computeArcStrikeFactor(const Base::BoundBox2d& labelRectangle,
                                         const Base::Vector2d& arcCenter, double arcRadius,
                                         const std::vector<std::pair<double, bool>>& drawMarking)
{
    if (drawMarking.empty()) {
        return 0.0;
    }

    unsigned int entryIndex = 0;
    while (entryIndex < drawMarking.size() && drawMarking[entryIndex].second) {
        ++entryIndex;
    }

    std::vector<Base::Vector2d> intersectionPoints;

    if (entryIndex >= drawMarking.size()) {
        DrawUtil::findCircleRectangleIntersections(arcCenter, arcRadius, labelRectangle,
                                                   intersectionPoints);
    }
    else {
        unsigned int startIndex = entryIndex;
        unsigned int currentIndex = entryIndex;
        do {
            ++currentIndex;
            currentIndex %= drawMarking.size();

            if (drawMarking[currentIndex].second != drawMarking[startIndex].second) {
                if (drawMarking[startIndex].second) {
                    double arcAngle = drawMarking[startIndex].first;
                    double arcRotation = drawMarking[currentIndex].first - arcAngle;
                    if (arcRotation < 0.0) {
                        arcRotation += M_2PI;
                    }

                    DrawUtil::findCircularArcRectangleIntersections(arcCenter, arcRadius, arcAngle,
                                                                    arcRotation, labelRectangle,
                                                                    intersectionPoints);
                }

                startIndex = currentIndex;
            }
        } while (currentIndex != entryIndex);
    }

    return intersectionPoints.size() >= 2 ? 1.0 : 0.0;
}

double QGIViewDimension::normalizeStartPosition(double& startPosition, double& lineAngle)
{
    if (startPosition > 0.0) {
        startPosition = -startPosition;
        lineAngle += M_PI;
        return -1.0;
    }

    return +1.0;
}

double QGIViewDimension::normalizeStartRotation(double& startRotation)
{
    if (copysign(1.0, startRotation) > 0.0) {
        startRotation = -startRotation;
        return -1.0;
    }

    return 1.0;
}

bool QGIViewDimension::constructDimensionLine(
    const Base::Vector2d& targetPoint, double lineAngle, double startPosition, double jointPosition,
    const Base::BoundBox2d& labelRectangle, int arrowCount, int standardStyle, bool flipArrows,
    std::vector<std::pair<double, bool>>& outputMarking) const
{
    // The start position > 0 is not expected, the caller must handle this
    if (startPosition > 0.0) {
        Base::Console().Error(
            "QGIVD::constructDimLine - Start Position must not be positive! Received: %f\n",
            startPosition);
        return false;
    }

    double labelBorder = 0.0;
    if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
        labelBorder = labelRectangle.Width() * 0.5 + getDefaultIsoReferenceLineOverhang();
    }
    else if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        std::vector<Base::Vector2d> intersectionPoints;
        DrawUtil::findLineRectangleIntersections(targetPoint, lineAngle, labelRectangle,
                                                 intersectionPoints);

        if (intersectionPoints.size() >= 2) {
            labelBorder = (intersectionPoints[0] - labelRectangle.GetCenter()).Length();
        }
    }

    bool autoFlipArrows = false;
    if (jointPosition + labelBorder > 0.0) {
        // If label sticks out, extend the dimension line beyond the end point (0.0)
        DrawUtil::intervalMarkLinear(outputMarking, 0.0, jointPosition + labelBorder, true);
        autoFlipArrows = true;
    }

    if (jointPosition - labelBorder < startPosition) {
        DrawUtil::intervalMarkLinear(outputMarking, startPosition,
                                     jointPosition - labelBorder - startPosition, true);

        // For only one arrow and zero width line skip flipping, it already points correctly
        if (arrowCount > 1 || startPosition < 0.0) {
            autoFlipArrows = true;
        }
    }

    flipArrows ^= autoFlipArrows;
    if (!flipArrows
        || (standardStyle != ViewProviderDimension::STD_STYLE_ASME_INLINED
            && standardStyle != ViewProviderDimension::STD_STYLE_ASME_REFERENCING)) {
        // If arrows point outside, or ASME standard is not followed,
        // add the line part between start and end
        DrawUtil::intervalMarkLinear(outputMarking, 0.0, startPosition, true);
    }

    // For ASME Inlined, cut out the part of line occupied by the value
    if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        DrawUtil::intervalMarkLinear(outputMarking, jointPosition - labelBorder, labelBorder * 2.0,
                                     false);
    }

    // Add the arrow tails - these are drawn always
    double placementFactor = flipArrows ? +1.0 : -1.0;
    DrawUtil::intervalMarkLinear(outputMarking, 0.0, placementFactor * getDefaultArrowTailLength(),
                                 true);
    if (arrowCount > 1) {
        DrawUtil::intervalMarkLinear(outputMarking, startPosition,
                                     -placementFactor * getDefaultArrowTailLength(), true);
    }

    return flipArrows;
}

bool QGIViewDimension::constructDimensionArc(
    const Base::Vector2d& arcCenter, double arcRadius, double endAngle, double startRotation,
    double handednessFactor, double jointRotation, const Base::BoundBox2d& labelRectangle,
    int arrowCount, int standardStyle, bool flipArrows,
    std::vector<std::pair<double, bool>>& outputMarking) const
{
    // The start rotation > 0 is not expected, the caller must handle this
    if (startRotation > 0.0) {
        Base::Console().Error(
            "QGIVD::constructDimArc - Start Rotation must not be positive! Received: %f\n",
            startRotation);
        return false;
    }

    double startDelta = 0.0;
    double endDelta = 0.0;
    if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
        double borderRadius = (labelRectangle.GetCenter() - arcCenter).Length();

        if (borderRadius > arcRadius) {
            borderRadius = arcRadius + getIsoDimensionLineSpacing();
        }
        else if (borderRadius < arcRadius) {
            borderRadius = arcRadius - getIsoDimensionLineSpacing();
        }

        // ISO oriented labels are symmetrical along their center axis
        startDelta = atan((labelRectangle.Width() * 0.5 + getDefaultIsoReferenceLineOverhang())
                          / borderRadius);
        endDelta = startDelta;
    }
    else if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        std::vector<Base::Vector2d> intersectionPoints;

        DrawUtil::findCircleRectangleIntersections(arcCenter, arcRadius, labelRectangle,
                                                   intersectionPoints);

        // We do not want to handle other cases than 2 intersection points - if so, act as if there were none
        if (intersectionPoints.size() == 2) {
            double zeroAngle = (labelRectangle.GetCenter() - arcCenter).Angle();

            startDelta =
                DrawUtil::angleDifference(zeroAngle, (intersectionPoints[0] - arcCenter).Angle());
            endDelta =
                DrawUtil::angleDifference(zeroAngle, (intersectionPoints[1] - arcCenter).Angle());

            // End delta is the angle in the end point direction, start delta in the opposite
            // Keep orientation and the computation sign in sync
            if ((endDelta < 0.0) == (handednessFactor < 0.0)) {
                std::swap(startDelta, endDelta);
            }

            startDelta = fabs(startDelta);
            endDelta = fabs(endDelta);
        }
    }

    bool autoFlipArrows = false;
    if (jointRotation + endDelta > 0.0) {
        // If label exceeds end angle ray, extend the dimension arc and flip arrows
        DrawUtil::intervalMarkCircular(outputMarking, endAngle,
                                       handednessFactor * (jointRotation + endDelta), true);
        autoFlipArrows = true;
    }

    if (jointRotation - startDelta < startRotation) {
        DrawUtil::intervalMarkCircular(
            outputMarking, endAngle + handednessFactor * startRotation,
            handednessFactor * (jointRotation - startDelta - startRotation), true);

        // For only one arrow and zero width line skip flipping, it already points correctly
        if (arrowCount > 1 || startRotation < 0.0) {
            autoFlipArrows = true;
        }
    }

    flipArrows ^= autoFlipArrows;
    if (!flipArrows
        || (standardStyle != ViewProviderDimension::STD_STYLE_ASME_INLINED
            && standardStyle != ViewProviderDimension::STD_STYLE_ASME_REFERENCING)) {
        // If arrows point outside, or ASME standard is not followed,
        // add the arc part between start and end
        DrawUtil::intervalMarkCircular(outputMarking, endAngle, handednessFactor * startRotation,
                                       true);
    }

    // For ASME Inlined, cut out the part of arc occupied by the value
    if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        DrawUtil::intervalMarkCircular(outputMarking,
                                       endAngle + handednessFactor * (jointRotation - startDelta),
                                       handednessFactor * (startDelta + endDelta), false);
    }

    // Add the arrow tails - these are drawn always
    double tailDelta =
        arcRadius >= Precision::Confusion() ? getDefaultArrowTailLength() / arcRadius : M_PI_4;
    double placementFactor = flipArrows ? +1.0 : -1.0;

    DrawUtil::intervalMarkCircular(outputMarking, endAngle,
                                   placementFactor * handednessFactor * tailDelta, true);
    if (arrowCount > 1) {
        DrawUtil::intervalMarkCircular(outputMarking, endAngle + handednessFactor * startRotation,
                                       -placementFactor * handednessFactor * tailDelta, true);
    }

    return flipArrows;
}

void QGIViewDimension::resetArrows() const
{
    aHead1->setDirMode(true);
    aHead1->setRotation(0.0);
    aHead1->setFlipped(false);

    aHead2->setDirMode(true);
    aHead2->setRotation(0.0);
    aHead2->setFlipped(false);
}

void QGIViewDimension::drawArrows(int count, const Base::Vector2d positions[], double angles[],
                                  bool flipped) const
{
    const int arrowCount = 2;
    QGIArrow* arrows[arrowCount] = {aHead1, aHead2};

    arrowPositionsToFeature(positions);

    for (int i = 0; i < arrowCount; ++i) {
        QGIArrow* arrow = arrows[i];

        if (positions && angles) {
            arrow->setPos(toQtGui(positions[i]));
            arrow->setDirection(toQtRad(angles[i]));
        }

        if (i >= count) {
            arrow->hide();
            continue;
        }

        arrow->setStyle(QGIArrow::getPrefArrowStyle());
        auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
        auto arrowSize = vp->Arrowsize.getValue();
        arrow->setSize(arrowSize);
        arrow->setFlipped(flipped);

        if (QGIArrow::getPrefArrowStyle() != 7) {// if not "None"
            arrow->draw();
            arrow->show();
        }
        else {
            arrow->hide();
        }
    }
}

void QGIViewDimension::arrowPositionsToFeature(const Base::Vector2d positions[]) const
{
    auto dim(dynamic_cast<TechDraw::DrawViewDimension*>(getViewObject()));
    if (!dim) {
        return;
    }

    dim->saveArrowPositions(positions);
}

void QGIViewDimension::drawSingleLine(QPainterPath& painterPath, const Base::Vector2d& lineOrigin,
                                      double lineAngle, double startPosition,
                                      double endPosition) const
{
    if (endPosition == startPosition) {
        return;
    }

    painterPath.moveTo(toQtGui(lineOrigin + Base::Vector2d::FromPolar(startPosition, lineAngle)));
    painterPath.lineTo(toQtGui(lineOrigin + Base::Vector2d::FromPolar(endPosition, lineAngle)));
}


//adds line segments to painterPath from lineOrigin along lineAngle
//segment length is determined by drawMarking entries
void QGIViewDimension::drawMultiLine(QPainterPath& painterPath, const Base::Vector2d& lineOrigin,
                                     double lineAngle,
                                     const std::vector<std::pair<double, bool>>& drawMarking) const
{
    if (drawMarking.size() < 2) {
        return;
    }

    unsigned int startIndex = 0;
    unsigned int currentIndex = 1;
    while (currentIndex < drawMarking.size()) {
        if (drawMarking[currentIndex].second != drawMarking[startIndex].second) {
            if (drawMarking[startIndex].second) {
                drawSingleLine(painterPath, lineOrigin, lineAngle, drawMarking[startIndex].first,
                               drawMarking[currentIndex].first);
            }

            startIndex = currentIndex;
        }

        ++currentIndex;
    }
}

void QGIViewDimension::drawSingleArc(QPainterPath& painterPath, const Base::Vector2d& arcCenter,
                                     double arcRadius, double startAngle, double endAngle) const
{
    if (endAngle == startAngle) {
        return;
    }
    if (endAngle < startAngle) {
        endAngle += M_2PI;
    }

    QRectF qtArcRectangle(
        toQtGui(Base::BoundBox2d(arcCenter.x - arcRadius, arcCenter.y - arcRadius,
                                 arcCenter.x + arcRadius, arcCenter.y + arcRadius)));

    // In arc drawing are for some reason Qt's angles counterclockwise as in our computations...
    painterPath.arcMoveTo(qtArcRectangle, toDeg(startAngle));
    painterPath.arcTo(qtArcRectangle, toDeg(startAngle), toDeg(endAngle - startAngle));
}

void QGIViewDimension::drawMultiArc(QPainterPath& painterPath, const Base::Vector2d& arcCenter,
                                    double arcRadius,
                                    const std::vector<std::pair<double, bool>>& drawMarking) const
{
    if (drawMarking.empty()) {
        return;
    }

    unsigned int entryIndex = 0;
    while (entryIndex < drawMarking.size() && drawMarking[entryIndex].second) {
        ++entryIndex;
    }

    if (entryIndex >= drawMarking.size()) {
        drawSingleArc(painterPath, arcCenter, arcRadius, 0, M_2PI);
        return;
    }

    unsigned int startIndex = entryIndex;
    unsigned int currentIndex = entryIndex;
    do {
        ++currentIndex;
        currentIndex %= drawMarking.size();

        if (drawMarking[currentIndex].second != drawMarking[startIndex].second) {
            if (drawMarking[startIndex].second) {
                drawSingleArc(painterPath, arcCenter, arcRadius, drawMarking[startIndex].first,
                              drawMarking[currentIndex].first);
            }

            startIndex = currentIndex;
        }
    } while (currentIndex != entryIndex);
}


//adds dimension line to painterPath
//dimension line starts at targetPoint and continues for a distance (startPosition?) along lineAngle
//jointPosition - distance of reference line from 1 extension line??
//lineAngle - Clockwise angle of distance line with horizontal
void QGIViewDimension::drawDimensionLine(QPainterPath& painterPath,
                                         const Base::Vector2d& targetPoint, double lineAngle,
                                         double startPosition, double jointPosition,
                                         const Base::BoundBox2d& labelRectangle, int arrowCount,
                                         int standardStyle, bool flipArrows) const
{
    // Keep the convention start position <= 0
    jointPosition *= normalizeStartPosition(startPosition, lineAngle);

    std::vector<std::pair<double, bool>> drawMarks;
    flipArrows =
        constructDimensionLine(targetPoint, lineAngle, startPosition, jointPosition, labelRectangle,
                               arrowCount, standardStyle, flipArrows, drawMarks);

    drawMultiLine(painterPath, targetPoint, lineAngle, drawMarks);

    Base::Vector2d arrowPositions[2];
    arrowPositions[0] = targetPoint;
    arrowPositions[1] = targetPoint + Base::Vector2d::FromPolar(startPosition, lineAngle);

    double arrowAngles[2];
    arrowAngles[0] = lineAngle;
    arrowAngles[1] = lineAngle + M_PI;

    drawArrows(arrowCount, arrowPositions, arrowAngles, flipArrows);
}

void QGIViewDimension::drawDimensionArc(QPainterPath& painterPath, const Base::Vector2d& arcCenter,
                                        double arcRadius, double endAngle, double startRotation,
                                        double jointAngle, const Base::BoundBox2d& labelRectangle,
                                        int arrowCount, int standardStyle, bool flipArrows) const
{
    // Keep the convention start rotation <= 0
    double handednessFactor = normalizeStartRotation(startRotation);

    // Split the rest of 2PI minus the angle and assign joint offset so > 0 is closer to end arc side
    double jointRotation = handednessFactor * (jointAngle - endAngle);
    if (fabs(jointRotation - startRotation * 0.5) > M_PI) {
        jointRotation += jointRotation < 0.0 ? +M_2PI : -M_2PI;
    }

    std::vector<std::pair<double, bool>> drawMarks;
    flipArrows = constructDimensionArc(arcCenter, arcRadius, endAngle, startRotation,
                                       handednessFactor, jointRotation, labelRectangle, arrowCount,
                                       standardStyle, flipArrows, drawMarks);

    drawMultiArc(painterPath, arcCenter, arcRadius, drawMarks);

    Base::Vector2d arrowPositions[2];
    arrowPositions[0] = arcCenter + Base::Vector2d::FromPolar(arcRadius, endAngle);
    arrowPositions[1] = arcCenter
        + Base::Vector2d::FromPolar(arcRadius, endAngle + handednessFactor * startRotation);

    double arrowAngles[2];
    arrowAngles[0] = endAngle + handednessFactor * M_PI_2;
    arrowAngles[1] = endAngle + handednessFactor * (startRotation - M_PI_2);

    drawArrows(arrowCount, arrowPositions, arrowAngles, flipArrows);
}

//draw any of 3 distance dimension types
//startPoint, endPoint - ends of actual distance line
//lineAngle - angle of actual line with horizontal
//target points - projection of reference line ends on to extension line
//startCross & endCross - real intersection of extension lines and dimension line
//dimension line - main annotation line
//reference line - line under dimension text in referenced styles
//joint points - ends of reference line
void QGIViewDimension::drawDistanceExecutive(const Base::Vector2d& startPoint,
                                             const Base::Vector2d& endPoint, double lineAngle,
                                             const Base::BoundBox2d& labelRectangle,
                                             int standardStyle, int renderExtent,
                                             bool flipArrows) const
{
    QPainterPath distancePath;

    Base::Vector2d labelCenter(labelRectangle.GetCenter());
    double labelAngle = 0.0;

    //startCross and endCross are points where extension line intersects dimension line
    Base::Vector2d startCross;
    Base::Vector2d endCross;
    int arrowCount = renderExtent >= ViewProviderDimension::REND_EXTENT_NORMAL
            || renderExtent == ViewProviderDimension::REND_EXTENT_CONFINED
        ? 2
        : 1;

    if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
        || standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING) {
        // The dimensional value text must stay horizontal in these styles

        //jointPoints are the ends of the reference line
        Base::Vector2d jointPoints[2];

        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING) {
            jointPoints[0] = getIsoRefJointPoint(labelRectangle, false);
            jointPoints[1] = getIsoRefJointPoint(labelRectangle, true);
        }
        else {
            jointPoints[0] = getAsmeRefJointPoint(labelRectangle, false);
            jointPoints[1] = getAsmeRefJointPoint(labelRectangle, true);
        }

        //targetPoints are the projection of reference line endpoints onto endPoint's extension line
        Base::Vector2d targetPoints[2];
        targetPoints[0] = computePerpendicularIntersection(jointPoints[0], endPoint, lineAngle);
        targetPoints[1] = computePerpendicularIntersection(jointPoints[1], endPoint, lineAngle);

        // Compute and normalize (i.e. make < 0) the start position
        Base::Vector2d lineDirection(Base::Vector2d::FromPolar(1.0, lineAngle));
        double startPosition =
            arrowCount > 1 ? lineDirection * (startPoint - targetPoints[0]) : 0.0;
        lineDirection *= normalizeStartPosition(startPosition, lineAngle);

        // Find the positions where the reference line attaches to the dimension line
        //jointPoints are the ends of the reference line
        //targetPoints - projection of reference line on to extension line
        //jointPositions - displacement of jointPoints from ext line
        double jointPositions[2];
        jointPositions[0] = lineDirection * (jointPoints[0] - targetPoints[0]);
        jointPositions[1] = lineDirection * (jointPoints[1] - targetPoints[1]);

        // Orient the leader line angle correctly towards the target point
        double angles[2];
        angles[0] =
            jointPositions[0] > 0.0 ? DrawUtil::angleComposition(lineAngle, M_PI) : lineAngle;
        angles[1] =
            jointPositions[1] > 0.0 ? DrawUtil::angleComposition(lineAngle, M_PI) : lineAngle;

        // Select the placement, where the label is not obscured by the leader line
        // or (if both behave the same) the one that  bends the reference line less
        double strikeFactors[2];

        std::vector<std::pair<double, bool>> lineMarking;
        constructDimensionLine(targetPoints[0], lineAngle, startPosition, jointPositions[0],
                               labelRectangle, arrowCount, standardStyle, flipArrows, lineMarking);
        strikeFactors[0] =
            computeLineStrikeFactor(labelRectangle, targetPoints[0], lineAngle, lineMarking);

        lineMarking.clear();
        constructDimensionLine(targetPoints[1], lineAngle, startPosition, jointPositions[1],
                               labelRectangle, arrowCount, standardStyle, flipArrows, lineMarking);
        strikeFactors[1] =
            computeLineStrikeFactor(labelRectangle, targetPoints[1], lineAngle, lineMarking);

        int selected =
            compareAngleStraightness(0.0, angles[0], angles[1], strikeFactors[0], strikeFactors[1]);
        if (selected == 0) {
            // Select the side closer, so the label is on the outer side of the dimension line
            Base::Vector2d perpendicularDir(lineDirection.Perpendicular());
            if (fabs((jointPoints[0] - endPoint) * perpendicularDir)
                > fabs((jointPoints[1] - endPoint) * perpendicularDir)) {
                selected = 1;
            }
        }
        else if (selected < 0) {
            selected = 0;
        }

        //find points where extension lines meet dimension line
        endCross = targetPoints[selected];
        startCross = targetPoints[selected] + Base::Vector2d::FromPolar(startPosition, lineAngle);

        drawDimensionLine(distancePath, endCross, lineAngle, startPosition,
                          jointPositions[selected], labelRectangle, arrowCount, standardStyle,
                          flipArrows);

        Base::Vector2d outsetPoint(
            standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
                ? getIsoRefOutsetPoint(labelRectangle, selected == 1)// 0 = left, 1 = right
                : getAsmeRefOutsetPoint(labelRectangle, selected == 1));

        //add the reference line to the QPainterPath
        distancePath.moveTo(toQtGui(outsetPoint));
        distancePath.lineTo(toQtGui(jointPoints[selected]));
    }

    else if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
        // We may rotate the label so no leader and reference lines are needed
        double placementFactor = getIsoStandardLinePlacement(lineAngle);
        labelAngle =
            placementFactor > 0.0 ? DrawUtil::angleComposition(lineAngle, M_PI) : lineAngle;

        // Find out the projection of label center on the line with given angle
        Base::Vector2d labelProjection(
            labelCenter
            + Base::Vector2d::FromPolar(
                placementFactor
                    * (labelRectangle.Height() * 0.5 + getIsoDimensionLineSpacing()),
                lineAngle + M_PI_2));

        // Compute the dimensional line start and end crossings with (virtual) extension lines
        //check for isometric direction and if iso compute non-perpendicular intersection of dim line and ext lines
        Base::Vector2d lineDirection(Base::Vector2d::FromPolar(1.0, lineAngle));
        startCross = computePerpendicularIntersection(labelProjection, startPoint, lineAngle);
        endCross = computePerpendicularIntersection(labelProjection, endPoint, lineAngle);

        // Find linear coefficients of crossings
        double startPosition = arrowCount > 1 ? lineDirection * (startCross - endCross) : 0.0;
        double labelPosition = lineDirection * (labelProjection - endCross);

        drawDimensionLine(distancePath, endCross, lineAngle, startPosition, labelPosition,
                          labelRectangle, arrowCount, standardStyle, flipArrows);
    }

    else if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        // Text must remain horizontal, but it may split the leader line
        Base::Vector2d lineDirection(Base::Vector2d::FromPolar(1.0, lineAngle));
        startCross = computePerpendicularIntersection(labelCenter, startPoint, lineAngle);
        endCross = computePerpendicularIntersection(labelCenter, endPoint, lineAngle);

        // Find linear coefficients of crossings
        double startPosition = arrowCount > 1 ? lineDirection * (startCross - endCross) : 0.0;
        double labelPosition = lineDirection * (labelCenter - endCross);

        drawDimensionLine(distancePath, endCross, lineAngle, startPosition, labelPosition,
                          labelRectangle, arrowCount, standardStyle, flipArrows);
    }
    else {
        Base::Console().Error(
            "QGIVD::drawDistanceExecutive - this Standard&Style is not supported: %d\n",
            standardStyle);
        arrowCount = 0;
    }

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    assert(vp);

    if (arrowCount > 0 && renderExtent >= ViewProviderDimension::REND_EXTENT_REDUCED) {
        double gapSize = 0.0;
        if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING
            || standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
            double factor = vp->GapFactorASME.getValue();
            gapSize = Rez::appX(m_lineWidth * factor);
        }
        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
            || standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
            double factor = vp->GapFactorISO.getValue();
            gapSize = Rez::appX(m_lineWidth * factor);
        }

        Base::Vector2d extensionOrigin;
        Base::Vector2d extensionTarget(computeExtensionLinePoints(
            endPoint, endCross, lineAngle + M_PI_2, getDefaultExtensionLineOverhang(), gapSize,
            extensionOrigin));
        //draw 1st extension line
        distancePath.moveTo(toQtGui(extensionOrigin));
        distancePath.lineTo(toQtGui(extensionTarget));

        if (arrowCount > 1) {
            extensionTarget = computeExtensionLinePoints(startPoint, startCross, lineAngle + M_PI_2,
                                                         getDefaultExtensionLineOverhang(), gapSize,
                                                         extensionOrigin);
            //draw second extension line
            distancePath.moveTo(toQtGui(extensionOrigin));
            distancePath.lineTo(toQtGui(extensionTarget));
        }
    }

    datumLabel->setTransformOriginPoint(datumLabel->boundingRect().center());
    datumLabel->setRotation(toQtDeg(labelAngle));

    dimLines->setPath(distancePath);
}

//draw any of 3 distance dimension types with user override of dimension and extension line directions
//startPoint, endPoint - ends of actual distance line
//lineAngle - desired angle of dimension line with horizontal
//extensionAngle - desired angle of extension lines with horizontal
void QGIViewDimension::drawDistanceOverride(const Base::Vector2d& startPoint,
                                            const Base::Vector2d& endPoint, double lineAngle,
                                            const Base::BoundBox2d& labelRectangle,
                                            int standardStyle, int renderExtent, bool flipArrows,
                                            double extensionAngle) const
{
    QPainterPath distancePath;

    Base::Vector2d labelCenter(labelRectangle.GetCenter());
    double labelAngle = 0.0;

    //startCross and endCross are points where extension lines intersect dimension line
    Base::Vector2d startCross;
    Base::Vector2d endCross;
    Base::Vector2d lineDirection(Base::Vector2d::FromPolar(1.0, lineAngle));
    Base::Vector2d extensionDirection(Base::Vector2d::FromPolar(1.0, extensionAngle));

    int arrowCount = renderExtent >= ViewProviderDimension::REND_EXTENT_NORMAL
            || renderExtent == ViewProviderDimension::REND_EXTENT_CONFINED
        ? 2
        : 1;

    if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
        || standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING) {
        // The dimensional value text must stay horizontal in these styles

        //refEndPoints are the ends of the reference line
        Base::Vector2d refEndPoints[2];

        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING) {
            refEndPoints[0] = getIsoRefJointPoint(labelRectangle, false);
            refEndPoints[1] = getIsoRefJointPoint(labelRectangle, true);
        }
        else {
            refEndPoints[0] = getAsmeRefJointPoint(labelRectangle, false);
            refEndPoints[1] = getAsmeRefJointPoint(labelRectangle, true);
        }

        //targetPoints are the projection of reference line endpoints onto endPoint's extension line
        Base::Vector2d targetPoints[2];
        targetPoints[0] =
            DrawUtil::Intersect2d(refEndPoints[0], lineDirection, endPoint, extensionDirection);
        targetPoints[1] =
            DrawUtil::Intersect2d(refEndPoints[1], lineDirection, endPoint, extensionDirection);
        Base::Vector2d pointOnStartExtension =
            DrawUtil::Intersect2d(endPoint, lineDirection, startPoint, extensionDirection);
        double startPosition =
            arrowCount > 1 ? lineDirection * (pointOnStartExtension - endPoint) : 0.0;

        // Compute and normalize (i.e. make < 0) the start position
        lineDirection *= normalizeStartPosition(startPosition, lineAngle);

        // Find the positions where the reference line attaches to the dimension line
        //refEndPoints are the ends of the reference line
        //targetPoints - projection of reference line on to extension line
        //jointPositions - displacement of refEndPoints from extension line
        double jointPositions[2];
        jointPositions[0] = lineDirection * (refEndPoints[0] - targetPoints[0]);
        jointPositions[1] = lineDirection * (refEndPoints[1] - targetPoints[1]);

        // Orient the leader line angle correctly towards the target point
        double angles[2];
        angles[0] =
            jointPositions[0] > 0.0 ? DrawUtil::angleComposition(lineAngle, M_PI) : lineAngle;
        angles[1] =
            jointPositions[1] > 0.0 ? DrawUtil::angleComposition(lineAngle, M_PI) : lineAngle;

        // Select the placement, where the label is not obscured by the leader line
        // or (if both behave the same) the one that  bends the reference line less
        double strikeFactors[2];

        std::vector<std::pair<double, bool>> lineMarking;
        constructDimensionLine(targetPoints[0], lineAngle, startPosition, jointPositions[0],
                               labelRectangle, arrowCount, standardStyle, flipArrows, lineMarking);
        strikeFactors[0] =
            computeLineStrikeFactor(labelRectangle, targetPoints[0], lineAngle, lineMarking);

        lineMarking.clear();
        constructDimensionLine(targetPoints[1], lineAngle, startPosition, jointPositions[1],
                               labelRectangle, arrowCount, standardStyle, flipArrows, lineMarking);
        strikeFactors[1] =
            computeLineStrikeFactor(labelRectangle, targetPoints[1], lineAngle, lineMarking);

        int selected =
            compareAngleStraightness(0.0, angles[0], angles[1], strikeFactors[0], strikeFactors[1]);
        if (selected == 0) {
            // Select the side closer, so the label is on the outer side of the dimension line
            Base::Vector2d perpendicularDir(lineDirection.Perpendicular());
            if (fabs((refEndPoints[0] - endPoint) * perpendicularDir)
                > fabs((refEndPoints[1] - endPoint) * perpendicularDir)) {
                selected = 1;
            }
        }
        else if (selected < 0) {
            selected = 0;
        }

        //find points where extension lines meet dimension line
        Base::Vector2d pointOnDimLine(refEndPoints[selected].x, refEndPoints[selected].y);
        startCross =
            DrawUtil::Intersect2d(startPoint, extensionDirection, pointOnDimLine, lineDirection);
        endCross =
            DrawUtil::Intersect2d(endPoint, extensionDirection, pointOnDimLine, lineDirection);

        drawDimensionLine(distancePath, endCross, lineAngle, startPosition,
                          jointPositions[selected], labelRectangle, arrowCount, standardStyle,
                          flipArrows);

        Base::Vector2d outsetPoint(
            standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
                ? getIsoRefOutsetPoint(labelRectangle, selected == 1)// 0 = left, 1 = right
                : getAsmeRefOutsetPoint(labelRectangle, selected == 1));

        //add the reference line to the QPainterPath
        distancePath.moveTo(toQtGui(outsetPoint));
        distancePath.lineTo(toQtGui(refEndPoints[selected]));
    }

    else if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
        // We may rotate the label so no leader and reference lines are needed
        double placementFactor = getIsoStandardLinePlacement(lineAngle);
        labelAngle =
            placementFactor > 0.0 ? DrawUtil::angleComposition(lineAngle, M_PI) : lineAngle;

        // Find out the projection of label center on the line with given angle
        Base::Vector2d labelProjection(
            labelCenter
            + Base::Vector2d::FromPolar(
                placementFactor
                    * (labelRectangle.Height() * 0.5 + getIsoDimensionLineSpacing()),
                lineAngle + M_PI_2));

        // Compute the dimensional line start and end crossings with (virtual) extension lines
        startCross =
            DrawUtil::Intersect2d(startPoint, extensionDirection, labelProjection, lineDirection);
        endCross =
            DrawUtil::Intersect2d(endPoint, extensionDirection, labelProjection, lineDirection);

        // Find linear coefficients of crossings
        double startPosition = arrowCount > 1 ? lineDirection * (startCross - endCross) : 0.0;
        double labelPosition = lineDirection * (labelProjection - endCross);

        drawDimensionLine(distancePath, endCross, lineAngle, startPosition, labelPosition,
                          labelRectangle, arrowCount, standardStyle, flipArrows);
    }

    else if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        // Text must remain horizontal, but it may split the leader line
        startCross =
            DrawUtil::Intersect2d(startPoint, extensionDirection, labelCenter, lineDirection);
        endCross = DrawUtil::Intersect2d(endPoint, extensionDirection, labelCenter, lineDirection);

        // Find linear coefficients of crossings
        double startPosition = arrowCount > 1 ? lineDirection * (startCross - endCross) : 0.0;
        double labelPosition = lineDirection * (labelCenter - endCross);

        drawDimensionLine(distancePath, endCross, lineAngle, startPosition, labelPosition,
                          labelRectangle, arrowCount, standardStyle, flipArrows);
    }
    else {
        Base::Console().Error(
            "QGIVD::drawDistanceExecutive - this Standard&Style is not supported: %d\n",
            standardStyle);
        arrowCount = 0;
    }

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    assert(vp);

    if (arrowCount > 0 && renderExtent >= ViewProviderDimension::REND_EXTENT_REDUCED) {
        double gapSize = 0.0;
        if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING
            || standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
            double factor = vp->GapFactorASME.getValue();
            gapSize = Rez::appX(m_lineWidth * factor);
        }
        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
            || standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
            double factor = vp->GapFactorISO.getValue();
            gapSize = Rez::appX(m_lineWidth * factor);
        }


        Base::Vector2d extensionOrigin;
        Base::Vector2d extensionTarget(computeExtensionLinePoints(
            endPoint, endCross, lineAngle + M_PI_2, getDefaultExtensionLineOverhang(), gapSize,
            extensionOrigin));
        //draw 1st extension line
        distancePath.moveTo(toQtGui(extensionOrigin));
        distancePath.lineTo(toQtGui(extensionTarget));

        if (arrowCount > 1) {
            extensionTarget = computeExtensionLinePoints(startPoint, startCross, lineAngle + M_PI_2,
                                                         getDefaultExtensionLineOverhang(), gapSize,
                                                         extensionOrigin);
            //draw second extension line
            distancePath.moveTo(toQtGui(extensionOrigin));
            distancePath.lineTo(toQtGui(extensionTarget));
        }
    }

    datumLabel->setTransformOriginPoint(datumLabel->boundingRect().center());
    datumLabel->setRotation(toQtDeg(labelAngle));

    dimLines->setPath(distancePath);
}

void QGIViewDimension::drawRadiusExecutive(const Base::Vector2d& centerPoint,
                                           const Base::Vector2d& midPoint, double radius,
                                           double endAngle, double startRotation,
                                           const Base::BoundBox2d& labelRectangle,
                                           double centerOverhang, int standardStyle,
                                           int renderExtent, bool flipArrow) const
{
    QPainterPath radiusPath;

    Base::Vector2d labelCenter(labelRectangle.GetCenter());
    double labelAngle = 0.0;

    if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
        || standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING) {
        // The dimensional value text must stay horizontal
        Base::Vector2d jointDirections[2];

        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING) {
            jointDirections[0] = getIsoRefJointPoint(labelRectangle, false) - centerPoint;
            jointDirections[1] = getIsoRefJointPoint(labelRectangle, true) - centerPoint;
        }
        else {
            jointDirections[0] = getAsmeRefJointPoint(labelRectangle, false) - centerPoint;
            jointDirections[1] = getAsmeRefJointPoint(labelRectangle, true) - centerPoint;
        }

        double lineAngles[2];
        lineAngles[0] = jointDirections[0].Angle();
        lineAngles[1] = jointDirections[1].Angle();

        // Are there points on the arc, where line from center intersects it perpendicularly?
        double angleFactors[2];
        angleFactors[0] = getAnglePlacementFactor(lineAngles[0], endAngle, startRotation);
        angleFactors[1] = getAnglePlacementFactor(lineAngles[1], endAngle, startRotation);

        // Orient the leader line angle correctly towards the point on arc
        if (angleFactors[0] < 0.0) {
            lineAngles[0] = DrawUtil::angleComposition(lineAngles[0], M_PI);
        }
        if (angleFactors[1] < 0.0) {
            lineAngles[1] = DrawUtil::angleComposition(lineAngles[1], M_PI);
        }

        // Find the positions where the reference line attaches to the dimension line
        double jointPositions[2];
        jointPositions[0] = angleFactors[0] * jointDirections[0].Length() - radius;
        jointPositions[1] = angleFactors[1] * jointDirections[1].Length() - radius;

        Base::Vector2d targetPoints[2];
        targetPoints[0] = centerPoint + Base::Vector2d::FromPolar(radius, lineAngles[0]);
        targetPoints[1] = centerPoint + Base::Vector2d::FromPolar(radius, lineAngles[1]);

        Base::Vector2d arcPoint;
        int selected = 0;
        if (angleFactors[0] || angleFactors[1]) {
            // At least from one of the reference line sides can run the leader line
            // perpendicularly to the arc, i.e. in direction to the center
            if (angleFactors[0] && angleFactors[1]) {
                // Both are acceptable, so choose the more convenient one
                double strikeFactors[2] = {0.0, 0.0};

                if (renderExtent >= ViewProviderDimension::REND_EXTENT_NORMAL) {
                    std::vector<std::pair<double, bool>> lineMarking;
                    constructDimensionLine(targetPoints[0], lineAngles[0], -radius,
                                           jointPositions[0], labelRectangle, 1, standardStyle,
                                           flipArrow, lineMarking);
                    strikeFactors[0] = computeLineStrikeFactor(labelRectangle, targetPoints[0],
                                                               lineAngles[0], lineMarking);

                    lineMarking.clear();
                    constructDimensionLine(targetPoints[1], lineAngles[1], -radius,
                                           jointPositions[1], labelRectangle, 1, standardStyle,
                                           flipArrow, lineMarking);
                    strikeFactors[1] = computeLineStrikeFactor(labelRectangle, targetPoints[1],
                                                               lineAngles[1], lineMarking);
                }

                if (compareAngleStraightness(
                        0.0,
                        jointPositions[0] > 0.0 ? DrawUtil::angleComposition(lineAngles[0], M_PI)
                                                : lineAngles[0],
                        jointPositions[1] > 0.0 ? DrawUtil::angleComposition(lineAngles[1], M_PI)
                                                : lineAngles[1],
                        strikeFactors[0], strikeFactors[1])
                    > 0) {
                    selected = 1;
                }
            }
            else if (angleFactors[1]) {
                selected = 1;
            }

            arcPoint = targetPoints[selected];
        }
        else {//  Both joint points lay outside the vertical angles
            arcPoint = midPoint;

            if (labelCenter.x < arcPoint.x) {// Place the dimensional value left
                selected = 1;
            }

            Base::Vector2d lineDirection(arcPoint - centerPoint - jointDirections[selected]);
            lineAngles[selected] = lineDirection.Angle();
            jointPositions[selected] = -lineDirection.Length();
        }

        drawDimensionLine(radiusPath, arcPoint, lineAngles[selected],
                          // If not reduced rendering and at least in one arc wedge, draw to center
                          (angleFactors[0] || angleFactors[1])
                                  && renderExtent >= ViewProviderDimension::REND_EXTENT_NORMAL
                              ? -radius - centerOverhang
                              : 0.0,
                          jointPositions[selected], labelRectangle, 1, standardStyle, flipArrow);

        Base::Vector2d outsetPoint(standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
                                       ? getIsoRefOutsetPoint(labelRectangle, selected == 1)
                                       : getAsmeRefOutsetPoint(labelRectangle, selected == 1));

        radiusPath.moveTo(toQtGui(outsetPoint));
        radiusPath.lineTo(toQtGui(centerPoint + jointDirections[selected]));
    }
    else if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
        // We may rotate the label so no reference line is needed
        double lineAngle;
        double devAngle = computeLineAndLabelAngles(centerPoint, labelCenter,
                                                    labelRectangle.Height() * 0.5
                                                        + getIsoDimensionLineSpacing(),
                                                    lineAngle, labelAngle);

        // Is there point on the arc, where line from center intersects it perpendicularly?
        double angleFactor = getAnglePlacementFactor(lineAngle, endAngle, startRotation);
        if (angleFactor < 0.0) {
            lineAngle = DrawUtil::angleComposition(lineAngle, M_PI);
        }

        Base::Vector2d arcPoint;
        double labelPosition;
        if (angleFactor) {
            arcPoint = centerPoint + Base::Vector2d::FromPolar(radius, lineAngle);

            // Correct the label center distance projected on the leader line and subtract radius
            labelPosition =
                angleFactor * cos(devAngle) * ((labelCenter - centerPoint).Length()) - radius;
        }
        else {
            // Leader line outside both arc wedges
            arcPoint = midPoint;

            devAngle = computeLineAndLabelAngles(arcPoint, labelCenter,
                                                 labelRectangle.Height() * 0.5
                                                     + getIsoDimensionLineSpacing(),
                                                 lineAngle, labelAngle);
            lineAngle = DrawUtil::angleComposition(lineAngle, M_PI);

            labelPosition = -cos(devAngle) * ((labelCenter - arcPoint).Length());
        }

        drawDimensionLine(radiusPath, arcPoint, lineAngle,
                          // If not reduced rendering and at least in one arc wedge, draw to center
                          angleFactor && renderExtent >= ViewProviderDimension::REND_EXTENT_NORMAL
                              ? -radius - centerOverhang
                              : 0.0,
                          labelPosition, labelRectangle, 1, standardStyle, flipArrow);
    }
    else if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        // Text must remain horizontal, but it may split the leader line
        Base::Vector2d labelDirection(labelCenter - centerPoint);
        double lineAngle = labelDirection.Angle();

        // Is there point on the arc, where line from center intersects it perpendicularly?
        double angleFactor = getAnglePlacementFactor(lineAngle, endAngle, startRotation);
        if (angleFactor < 0) {
            lineAngle = DrawUtil::angleComposition(lineAngle, M_PI);
        }

        Base::Vector2d arcPoint;
        double labelPosition;
        if (angleFactor) {
            arcPoint = centerPoint + Base::Vector2d::FromPolar(radius, lineAngle);

            labelPosition = angleFactor * labelDirection.Length() - radius;
        }
        else {
            // Leader line outside both arc wedges
            arcPoint = midPoint;

            labelDirection = arcPoint - labelCenter;
            lineAngle = labelDirection.Angle();
            labelPosition = -labelDirection.Length();
        }

        drawDimensionLine(radiusPath, arcPoint, lineAngle,
                          // If not reduced rendering and at least in one arc wedge, draw to center
                          angleFactor && renderExtent >= ViewProviderDimension::REND_EXTENT_NORMAL
                              ? -radius - centerOverhang
                              : 0.0,
                          labelPosition, labelRectangle, 1, standardStyle, flipArrow);
    }
    else {
        Base::Console().Error(
            "QGIVD::drawRadiusExecutive - this Standard&Style is not supported: %d\n",
            standardStyle);
    }

    datumLabel->setTransformOriginPoint(datumLabel->boundingRect().center());
    datumLabel->setRotation(toQtDeg(labelAngle));

    dimLines->setPath(radiusPath);
}

void QGIViewDimension::drawDistance(TechDraw::DrawViewDimension* dimension,
                                    ViewProviderDimension* viewProvider) const
{
    Base::BoundBox2d labelRectangle(
        fromQtGui(mapRectFromItem(datumLabel, datumLabel->boundingRect())));

    pointPair linePoints = dimension->getLinearPoints();
    const char* dimensionType = dimension->Type.getValueAsString();

    double lineAngle;
    if (strcmp(dimensionType, "DistanceX") == 0) {
        lineAngle = 0.0;
    }
    else if (strcmp(dimensionType, "DistanceY") == 0) {
        lineAngle = M_PI_2;
    }
    else {
        lineAngle = (fromQtApp(linePoints.second()) - fromQtApp(linePoints.first())).Angle();
    }

    int standardStyle = viewProvider->StandardAndStyle.getValue();
    int renderExtent = viewProvider->RenderingExtent.getValue();
    bool flipArrows = viewProvider->FlipArrowheads.getValue();


    if (dimension->AngleOverride.getValue()) {
        drawDistanceOverride(fromQtApp(linePoints.first()), fromQtApp(linePoints.second()),
                             dimension->LineAngle.getValue() * M_PI / 180.0, labelRectangle,
                             standardStyle, renderExtent, flipArrows,
                             dimension->ExtensionAngle.getValue() * M_PI / 180.0);
    }
    else {
        drawDistanceExecutive(fromQtApp(linePoints.extensionLineFirst()), fromQtApp(linePoints.extensionLineSecond()),
                              lineAngle, labelRectangle, standardStyle, renderExtent, flipArrows);
    }
}

void QGIViewDimension::drawRadius(TechDraw::DrawViewDimension* dimension,
                                  ViewProviderDimension* viewProvider) const
{
    Base::BoundBox2d labelRectangle(
        fromQtGui(mapRectFromItem(datumLabel, datumLabel->boundingRect())));
    arcPoints curvePoints = dimension->getArcPoints();

    double endAngle;
    double startRotation;
    if (curvePoints.isArc) {
        endAngle =
            (fromQtApp(curvePoints.arcEnds.second()) - fromQtApp(curvePoints.center)).Angle();
        startRotation =
            (fromQtApp(curvePoints.arcEnds.first()) - fromQtApp(curvePoints.center)).Angle()
            - endAngle;

        if (startRotation != 0.0 && ((startRotation > 0.0) != curvePoints.arcCW)) {
            startRotation += curvePoints.arcCW ? +M_2PI : -M_2PI;
        }
    }
    else {// A circle arc covers the whole plane
        endAngle = M_PI;
        startRotation = -M_2PI;
    }

    drawRadiusExecutive(
        fromQtApp(curvePoints.center), fromQtApp(curvePoints.midArc), curvePoints.radius, endAngle,
        startRotation, labelRectangle, 0.0, viewProvider->StandardAndStyle.getValue(),
        viewProvider->RenderingExtent.getValue(), viewProvider->FlipArrowheads.getValue());
}

void QGIViewDimension::drawDiameter(TechDraw::DrawViewDimension* dimension,
                                    ViewProviderDimension* viewProvider) const
{
    Base::BoundBox2d labelRectangle(
        fromQtGui(mapRectFromItem(datumLabel, datumLabel->boundingRect())));
    Base::Vector2d labelCenter(labelRectangle.GetCenter());

    arcPoints curvePoints = dimension->getArcPoints();

    Base::Vector2d curveCenter(fromQtApp(curvePoints.center));
    double curveRadius = curvePoints.radius;

    int standardStyle = viewProvider->StandardAndStyle.getValue();
    int renderExtent = viewProvider->RenderingExtent.getValue();
    bool flipArrows = viewProvider->FlipArrowheads.getValue();

    if (renderExtent == ViewProviderDimension::REND_EXTENT_NORMAL) {
        // Draw diameter as one line crossing center touching two opposite circle points
        QPainterPath diameterPath;
        double labelAngle = 0.0;

        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
            || standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING) {
            // The dimensional value text must stay horizontal
            Base::Vector2d jointDirections[2];

            if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING) {
                jointDirections[0] = getIsoRefJointPoint(labelRectangle, false) - curveCenter;
                jointDirections[1] = getIsoRefJointPoint(labelRectangle, true) - curveCenter;
            }
            else {
                jointDirections[0] = getAsmeRefJointPoint(labelRectangle, false) - curveCenter;
                jointDirections[1] = getAsmeRefJointPoint(labelRectangle, true) - curveCenter;
            }

            // Find the angles of lines from the center
            double lineAngles[2];
            lineAngles[0] = jointDirections[0].Angle();
            lineAngles[1] = jointDirections[1].Angle();

            Base::Vector2d targetPoints[2];
            targetPoints[0] = curveCenter + Base::Vector2d::FromPolar(curveRadius, lineAngles[0]);
            targetPoints[1] = curveCenter + Base::Vector2d::FromPolar(curveRadius, lineAngles[1]);

            // Find the positions where the reference line attaches to the dimension line
            double jointPositions[2];
            jointPositions[0] = jointDirections[0].Length() - curveRadius;
            jointPositions[1] = jointDirections[1].Length() - curveRadius;

            // Select the placement, where the label is not obscured by the leader line
            double strikeFactors[2];

            std::vector<std::pair<double, bool>> lineMarking;
            constructDimensionLine(targetPoints[0], lineAngles[0], -curveRadius * 2.0,
                                   jointPositions[0], labelRectangle, 2, standardStyle, flipArrows,
                                   lineMarking);
            strikeFactors[0] = computeLineStrikeFactor(labelRectangle, targetPoints[0],
                                                       lineAngles[0], lineMarking);

            lineMarking.clear();
            constructDimensionLine(targetPoints[1], lineAngles[1], -curveRadius * 2.0,
                                   jointPositions[1], labelRectangle, 2, standardStyle, flipArrows,
                                   lineMarking);
            strikeFactors[1] = computeLineStrikeFactor(labelRectangle, targetPoints[1],
                                                       lineAngles[1], lineMarking);

            int selected = 0;
            if (compareAngleStraightness(
                    0.0,
                    jointPositions[0] > 0.0 ? DrawUtil::angleComposition(lineAngles[0], M_PI)
                                            : lineAngles[0],
                    jointPositions[1] > 0.0 ? DrawUtil::angleComposition(lineAngles[1], M_PI)
                                            : lineAngles[1],
                    strikeFactors[0], strikeFactors[1])
                > 0) {
                selected = 1;
            }

            drawDimensionLine(diameterPath,
                              curveCenter
                                  + Base::Vector2d::FromPolar(curveRadius, lineAngles[selected]),
                              lineAngles[selected], -curveRadius * 2.0, jointPositions[selected],
                              labelRectangle, 2, standardStyle, flipArrows);

            Base::Vector2d outsetPoint(standardStyle
                                               == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
                                           ? getIsoRefOutsetPoint(labelRectangle, selected == 1)
                                           : getAsmeRefOutsetPoint(labelRectangle, selected == 1));

            diameterPath.moveTo(toQtGui(outsetPoint));
            diameterPath.lineTo(toQtGui(curveCenter + jointDirections[selected]));
        }
        else if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
            // We may rotate the label so no reference line is needed
            double lineAngle;
            double devAngle = computeLineAndLabelAngles(curveCenter, labelCenter,
                                                        labelRectangle.Height() * 0.5
                                                            + getIsoDimensionLineSpacing(),
                                                        lineAngle, labelAngle);

            // Correct the label center distance projected on the leader line and subtract radius
            double labelPosition =
                cos(devAngle) * ((labelCenter - curveCenter).Length()) - curveRadius;

            drawDimensionLine(diameterPath,
                              curveCenter + Base::Vector2d::FromPolar(curveRadius, lineAngle),
                              lineAngle, -curveRadius * 2.0, labelPosition, labelRectangle, 2,
                              standardStyle, flipArrows);
        }
        else if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
            // Text must remain horizontal, but it may split the leader line
            double lineAngle = (labelCenter - curveCenter).Angle();
            //Base::Vector2d lineDirection(Base::Vector2d::FromPolar(1.0, lineAngle));

            drawDimensionLine(
                diameterPath, curveCenter + Base::Vector2d::FromPolar(curveRadius, lineAngle),
                lineAngle, -curveRadius * 2.0, (labelCenter - curveCenter).Length() - curveRadius,
                labelRectangle, 2, standardStyle, flipArrows);
        }
        else {
            Base::Console().Error("QGIVD::drawRadius - this Standard&Style is not supported: %d\n",
                                  standardStyle);
        }

        datumLabel->setTransformOriginPoint(datumLabel->boundingRect().center());
        datumLabel->setRotation(toQtDeg(labelAngle));

        dimLines->setPath(diameterPath);
    }
    else if (renderExtent >= ViewProviderDimension::REND_EXTENT_EXPANDED) {
        double lineAngle = (labelCenter - curveCenter).Angle();
        Base::Vector2d startPoint(curveCenter);
        Base::Vector2d endPoint(curveCenter);

        if ((lineAngle >= M_PI_4 && lineAngle <= 3.0 * M_PI_4)
            || (lineAngle <= -M_PI_4 && lineAngle >= -3.0 * M_PI_4)) {
            // Horizontal dimension line
            startPoint.x -= curveRadius;
            endPoint.x += curveRadius;
            lineAngle = 0.0;
        }
        else {// Vertical dimension line
            startPoint.y -= curveRadius;
            endPoint.y += curveRadius;
            lineAngle = M_PI_2;
        }

        //        lineAngle = DrawUtil::angleComposition((labelCenter - curveCenter).Angle(), +M_PI_2);
        //        startPoint = curveCenter - Base::Vector2d::FromPolar(curveRadius, lineAngle);
        //        endPoint = curveCenter + Base::Vector2d::FromPolar(curveRadius, lineAngle);

        drawDistanceExecutive(startPoint, endPoint, lineAngle, labelRectangle, standardStyle,
                              ViewProviderDimension::REND_EXTENT_NORMAL, flipArrows);
    }
    else if (renderExtent <= ViewProviderDimension::REND_EXTENT_REDUCED) {
        renderExtent = renderExtent <= ViewProviderDimension::REND_EXTENT_CONFINED
            ? ViewProviderDimension::REND_EXTENT_REDUCED
            : ViewProviderDimension::REND_EXTENT_NORMAL;

        drawRadiusExecutive(curveCenter, Rez::guiX(curvePoints.midArc, true), curveRadius, M_PI,
                            -M_2PI, labelRectangle, getDefaultExtensionLineOverhang(),
                            standardStyle, renderExtent, flipArrows);
    }
}

void QGIViewDimension::drawAngle(TechDraw::DrawViewDimension* dimension,
                                 ViewProviderDimension* viewProvider) const
{
    QPainterPath anglePath;

    Base::BoundBox2d labelRectangle(
        fromQtGui(mapRectFromItem(datumLabel, datumLabel->boundingRect())));
    Base::Vector2d labelCenter(labelRectangle.GetCenter());
    double labelAngle = 0.0;

    anglePoints anglePoints = dimension->getAnglePoints();

    Base::Vector2d angleVertex = fromQtApp(anglePoints.vertex());
    Base::Vector2d startPoint = fromQtApp(anglePoints.first());
    Base::Vector2d endPoint = fromQtApp(anglePoints.second());

    double endAngle = (endPoint - angleVertex).Angle();
    double startAngle = (startPoint - angleVertex).Angle();
    double arcRadius;

    int standardStyle = viewProvider->StandardAndStyle.getValue();
    int renderExtent = viewProvider->RenderingExtent.getValue();
    bool flipArrows = viewProvider->FlipArrowheads.getValue();

    int arrowCount = renderExtent >= ViewProviderDimension::REND_EXTENT_NORMAL
            || renderExtent == ViewProviderDimension::REND_EXTENT_CONFINED
        ? 2
        : 1;

    // Inverted dimensions display reflex angles (fi > PI), regular ones oblique angles (fi <= PI/2)
    double startRotation =
        DrawUtil::angleDifference(startAngle, endAngle, dimension->Inverted.getValue());
    if (arrowCount < 2) {
        // For single arrow, the effective angle span is 0, but still we need to know
        // the angle orientation. Floating point positive/negative zero comes to rescue...
        startRotation = copysign(0.0, startRotation);
    }

    if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
        || standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING) {
        // The dimensional value text must stay horizontal
        Base::Vector2d jointDirections[2];

        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING) {
            jointDirections[0] = getIsoRefJointPoint(labelRectangle, false) - angleVertex;
            jointDirections[1] = getIsoRefJointPoint(labelRectangle, true) - angleVertex;
        }
        else {
            jointDirections[0] = getAsmeRefJointPoint(labelRectangle, false) - angleVertex;
            jointDirections[1] = getAsmeRefJointPoint(labelRectangle, true) - angleVertex;
        }

        // Get radiuses of the angle dimension arcs
        double arcRadii[2];
        arcRadii[0] = jointDirections[0].Length();
        arcRadii[1] = jointDirections[1].Length();

        // Compute the reference line joint angles
        double jointAngles[2];
        jointAngles[0] = jointDirections[0].Angle();
        jointAngles[1] = jointDirections[1].Angle();

        double handednessFactor = normalizeStartRotation(startRotation);
        double jointRotations[2];
        jointRotations[0] = handednessFactor * (jointAngles[0] - endAngle);
        jointRotations[1] = handednessFactor * (jointAngles[1] - endAngle);

        // Compare the offset with half of the rest of 2PI minus the angle and eventually fix the values
        if (fabs(jointRotations[0] - startRotation * 0.5) > M_PI) {
            jointRotations[0] += jointRotations[0] < 0.0 ? +M_2PI : -M_2PI;
        }
        if (fabs(jointRotations[1] - startRotation * 0.5) > M_PI) {
            jointRotations[1] += jointRotations[1] < 0.0 ? +M_2PI : -M_2PI;
        }

        // Compute the strike factors so we can choose the placement where value is not obscured by dimensional arc
        double strikeFactors[2];

        std::vector<std::pair<double, bool>> arcMarking;
        constructDimensionArc(angleVertex, arcRadii[0], endAngle, startRotation, handednessFactor,
                              jointRotations[0], labelRectangle, arrowCount, standardStyle,
                              flipArrows, arcMarking);
        strikeFactors[0] =
            computeArcStrikeFactor(labelRectangle, angleVertex, arcRadii[0], arcMarking);

        arcMarking.clear();
        constructDimensionArc(angleVertex, arcRadii[1], endAngle, startRotation, handednessFactor,
                              jointRotations[1], labelRectangle, arrowCount, standardStyle,
                              flipArrows, arcMarking);
        strikeFactors[1] =
            computeArcStrikeFactor(labelRectangle, angleVertex, arcRadii[1], arcMarking);

        int selected = 0;
        if (compareAngleStraightness(
                0.0,
                DrawUtil::angleComposition(
                    jointAngles[0], handednessFactor * jointRotations[0] > 0.0 ? -M_PI_2 : +M_PI_2),
                DrawUtil::angleComposition(
                    jointAngles[1], handednessFactor * jointRotations[1] > 0.0 ? -M_PI_2 : +M_PI_2),
                strikeFactors[0], strikeFactors[1])
            > 0) {
            selected = 1;
        }

        arcRadius = arcRadii[selected];
        startRotation = copysign(startRotation, -handednessFactor);

        drawDimensionArc(anglePath, angleVertex, arcRadius, endAngle, startRotation,
                         jointAngles[selected], labelRectangle, arrowCount, standardStyle,
                         flipArrows);

        Base::Vector2d outsetPoint(standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
                                       ? getIsoRefOutsetPoint(labelRectangle, selected == 1)
                                       : getAsmeRefOutsetPoint(labelRectangle, selected == 1));

        anglePath.moveTo(toQtGui(outsetPoint));
        anglePath.lineTo(toQtGui(angleVertex + jointDirections[selected]));
    }
    else if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
        // We may rotate the label so no leader and reference lines are needed
        Base::Vector2d labelDirection(labelCenter - angleVertex);
        double radiusAngle = labelDirection.Angle();

        labelAngle = DrawUtil::angleComposition(radiusAngle, M_PI_2);
        double placementFactor = getIsoStandardLinePlacement(labelAngle);
        labelAngle =
            placementFactor > 0.0 ? DrawUtil::angleComposition(labelAngle, M_PI) : labelAngle;

        arcRadius = labelDirection.Length()
            - placementFactor
                * (labelRectangle.Height() * 0.5 + getIsoDimensionLineSpacing());
        if (arcRadius < 0.0) {
            arcRadius = labelDirection.Length();
        }

        drawDimensionArc(anglePath, angleVertex, arcRadius, endAngle, startRotation, radiusAngle,
                         labelRectangle, arrowCount, standardStyle, flipArrows);
    }
    else if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
        // Text must remain horizontal, but it may split the leader line
        Base::Vector2d labelDirection(labelCenter - angleVertex);
        arcRadius = labelDirection.Length();

        drawDimensionArc(anglePath, angleVertex, arcRadius, endAngle, startRotation,
                         labelDirection.Angle(), labelRectangle, arrowCount, standardStyle,
                         flipArrows);
    }
    else {
        Base::Console().Error("QGIVD::drawAngle - this Standard&Style is not supported: %d\n",
                              standardStyle);
        arrowCount = 0;
    }

    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    assert(vp);

    if (arrowCount > 0 && renderExtent >= ViewProviderDimension::REND_EXTENT_REDUCED) {
        double gapSize = 0.0;
        if (standardStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING
            || standardStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED) {
            double factor = vp->GapFactorASME.getValue();
            gapSize = Rez::appX(m_lineWidth * factor);
        }
        if (standardStyle == ViewProviderDimension::STD_STYLE_ISO_REFERENCING
            || standardStyle == ViewProviderDimension::STD_STYLE_ISO_ORIENTED) {
            double factor = vp->GapFactorISO.getValue();
            gapSize = Rez::appX(m_lineWidth * factor);
        }


        Base::Vector2d extensionOrigin;
        Base::Vector2d extensionTarget(computeExtensionLinePoints(
            endPoint, angleVertex + Base::Vector2d::FromPolar(arcRadius, endAngle), endAngle,
            getDefaultExtensionLineOverhang(), gapSize, extensionOrigin));
        anglePath.moveTo(toQtGui(extensionOrigin));
        anglePath.lineTo(toQtGui(extensionTarget));

        if (arrowCount > 1) {
            extensionTarget = computeExtensionLinePoints(
                startPoint, angleVertex + Base::Vector2d::FromPolar(arcRadius, startAngle),
                startAngle, getDefaultExtensionLineOverhang(), gapSize, extensionOrigin);
            anglePath.moveTo(toQtGui(extensionOrigin));
            anglePath.lineTo(toQtGui(extensionTarget));
        }
    }

    datumLabel->setTransformOriginPoint(datumLabel->boundingRect().center());
    datumLabel->setRotation(toQtDeg(labelAngle));

    dimLines->setPath(anglePath);
}

QColor QGIViewDimension::prefNormalColor()
{
    setNormalColor(PreferencesGui::getAccessibleQColor(PreferencesGui::dimQColor()));
    ViewProviderDimension* vpDim = nullptr;
    Gui::ViewProvider* vp = getViewProvider(getDimFeat());
    if (vp) {
        vpDim = dynamic_cast<ViewProviderDimension*>(vp);
        if (vpDim) {
            App::Color fcColor = vpDim->Color.getValue();
            fcColor = Preferences::getAccessibleColor(fcColor);
            setNormalColor(fcColor.asValue<QColor>());
        }
    }
    return getNormalColor();
}

//! find the closest isometric axis given an ortho vector
Base::Vector3d QGIViewDimension::findIsoDir(Base::Vector3d ortho) const
{
    std::vector<Base::Vector3d> isoDirs = {Base::Vector3d(0.866, 0.5, 0.0),  //iso X
                                           Base::Vector3d(-0.866, -0.5, 0.0),//iso -X
                                           Base::Vector3d(-0.866, 0.5, 0.0), //iso -Y?
                                           Base::Vector3d(0.866, -0.5, 0.0), //iso +Y?
                                           Base::Vector3d(0.0, -1.0, 0.0),   //iso -Z
                                           Base::Vector3d(0.0, 1.0, 0.0)};   //iso Z
    std::vector<double> angles;
    for (auto& iso : isoDirs) {
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
Base::Vector3d QGIViewDimension::findIsoExt(Base::Vector3d dir) const
{
    Base::Vector3d isoX(0.866, 0.5, 0.0);   //iso X
    Base::Vector3d isoXr(-0.866, -0.5, 0.0);//iso -X
    Base::Vector3d isoY(-0.866, 0.5, 0.0);  //iso -Y?
    Base::Vector3d isoYr(0.866, -0.5, 0.0); //iso +Y?
    Base::Vector3d isoZ(0.0, 1.0, 0.0);     //iso Z
    Base::Vector3d isoZr(0.0, -1.0, 0.0);   //iso -Z
    if (dir.IsEqual(isoX, FLT_EPSILON)) {
        return isoY;
    }
    else if (dir.IsEqual(-isoX, FLT_EPSILON)) {
        return -isoY;
    }
    else if (dir.IsEqual(isoY, FLT_EPSILON)) {
        return isoZ;
    }
    else if (dir.IsEqual(-isoY, FLT_EPSILON)) {
        return -isoZ;
    }
    else if (dir.IsEqual(isoZ, FLT_EPSILON)) {
        return isoX;
    }
    else if (dir.IsEqual(-isoZ, FLT_EPSILON)) {
        return -isoX;
    }

    //tarfu
    Base::Console().Message("QGIVD::findIsoExt - %s - input is not iso axis\n",
                            getViewObject()->getNameInDocument());
    return Base::Vector3d(1, 0, 0);
}

void QGIViewDimension::onPrettyChanged(int state)
{
    //    Base::Console().Message("QGIVD::onPrettyChange(%d)\n", state);
    if (state == NORMAL) {
        setPrettyNormal();
    }
    else if (state == PRE) {
        setPrettyPre();
    }
    else {//if state = SEL
        setPrettySel();
    }
}

void QGIViewDimension::setPrettyPre()
{
    aHead1->setPrettyPre();
    aHead2->setPrettyPre();
    dimLines->setPrettyPre();
}

void QGIViewDimension::setPrettySel()
{
    aHead1->setPrettySel();
    aHead2->setPrettySel();
    dimLines->setPrettySel();
}

void QGIViewDimension::setPrettyNormal()
{
    aHead1->setPrettyNormal();
    aHead2->setPrettyNormal();
    dimLines->setPrettyNormal();
}

void QGIViewDimension::drawBorder()
{
    //Dimensions have no border!
    //    Base::Console().Message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
}

double QGIViewDimension::getDefaultExtensionLineOverhang() const
{
    // 8x Line Width according to ISO 129-1 Standard section 5.4, not specified by ASME Y14.5M
    return Rez::appX(m_lineWidth * 8.0);
}

double QGIViewDimension::getDefaultArrowTailLength() const
{
    // Arrow length shall be equal to font height and both ISO and ASME seem
    // to have arrow tail twice the arrow length, so let's make it twice arrow size
    auto arrowSize = PreferencesGui::dimArrowSize();
    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    if (vp) {
        arrowSize = vp->Arrowsize.getValue();

    }
    return  arrowSize * 2.0;
}

double QGIViewDimension::getDefaultIsoDimensionLineSpacing() const
{
    // Not specified directly, but seems to be 2x Line Width according to ISO 129-1 Annex A
    return Rez::appX(m_lineWidth * 2.0);
}

// Returns the line spacing for ISO dimension based on the user inputted factor
double QGIViewDimension::getIsoDimensionLineSpacing() const
{
    auto vp = static_cast<ViewProviderDimension*>(getViewProvider(getViewObject()));
    return Rez::appX(m_lineWidth * vp->LineSpacingFactorISO.getValue());
}

double QGIViewDimension::getDefaultIsoReferenceLineOverhang() const
{
    // Not specified directly but seems to be exactly Line Width according to ISO 129-1 Annex A
    return Rez::appX(m_lineWidth * 1.0);
}

double QGIViewDimension::getDefaultAsmeHorizontalLeaderLength() const
{
    // Not specified by ASME Y14.5M, this is a best guess
    return Rez::appX(m_lineWidth * 12);
}

//frame, border, caption are never shown in QGIVD, so shouldn't be in bRect
QRectF QGIViewDimension::boundingRect() const
{
    QRectF labelRect = mapFromItem(datumLabel, datumLabel->boundingRect()).boundingRect();
    QRectF linesRect = mapFromItem(dimLines, dimLines->boundingRect()).boundingRect();
    QRectF aHead1Rect = mapFromItem(aHead1, aHead1->boundingRect()).boundingRect();
    QRectF aHead2Rect = mapFromItem(aHead2, aHead2->boundingRect()).boundingRect();
    QRectF result(labelRect);
    result = result.united(linesRect);
    result = result.united(aHead1Rect);
    result = result.united(aHead2Rect);
    return result;
}

void QGIViewDimension::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                             QWidget* widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    setPens();
    //double arrowSaveWidth = aHead1->getWidth();
    if (svg) {
        setSvgPens();
    }
    else {
        setPens();
    }

    //    painter->setPen(Qt::red);
    //    painter->drawRect(boundingRect());          //good for debugging

    //    QGIView::paint (painter, &myOption, widget);
    QGraphicsItemGroup::paint(painter, &myOption, widget);
    setPens();
}

void QGIViewDimension::setSvgPens()
{
    double svgLineFactor = 3.0;//magic number.  should be a setting somewhere.
    dimLines->setWidth(m_lineWidth / svgLineFactor);
    aHead1->setWidth(aHead1->getWidth() / svgLineFactor);
    aHead2->setWidth(aHead2->getWidth() / svgLineFactor);
}

void QGIViewDimension::setPens()
{
    dimLines->setWidth(m_lineWidth);
    aHead1->setWidth(m_lineWidth);
    aHead2->setWidth(m_lineWidth);
}

double QGIViewDimension::toDeg(double angle) { return angle * 180 / M_PI; }

double QGIViewDimension::toQtRad(double angle) { return -angle; }

double QGIViewDimension::toQtDeg(double angle) { return -angle * 180.0 / M_PI; }

void QGIViewDimension::makeMarkC(double xPos, double yPos, QColor color) const
{
    QGIVertex* vItem = new QGIVertex(-1);
    vItem->setParentItem(const_cast<QGIViewDimension*>(this));
    vItem->setPos(xPos, yPos);
    vItem->setWidth(2.0);
    vItem->setRadius(20.0);
    vItem->setNormalColor(color);
    vItem->setFillColor(color);
    vItem->setPrettyNormal();
    vItem->setZValue(ZVALUE::VERTEX);
}

#include <Mod/TechDraw/Gui/moc_QGIViewDimension.cpp>
