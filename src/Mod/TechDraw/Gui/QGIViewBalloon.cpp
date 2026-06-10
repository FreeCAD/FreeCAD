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

# include <algorithm>
# include <cmath>
# include <numeric>
# include <string>

# include <QGuiApplication>
# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QPaintDevice>
# include <QPainter>
# include <QPainterPath>
# include <QStringList>
# include <QSvgGenerator>
# include <QTextDocument>
# include <QTextOption>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/Tools.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIViewBalloon.h"
#include "PreferencesGui.h"
#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "Rez.h"
#include "ViewProviderBalloon.h"
#include "ViewProviderViewPart.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"
#include "QGSPage.h"


//TODO: hide the Qt coord system (+y down).

using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;
using DGU = DrawGuiUtil;

namespace
{
struct BalloonTableText
{
    QString text;
    QString html;
    std::vector<double> separators;
    int rowCount = 1;
    double width = 0.0;
};

BalloonTableText makeBalloonTableText(const QString& source, const QFont& font)
{
    BalloonTableText result;
    result.text = source;

    if (!source.contains(QLatin1Char('|'))) {
        return result;
    }

    const QFontMetrics fm(font);
    const QStringList lines = source.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    std::vector<QStringList> rows;
    rows.reserve(lines.size());

    qsizetype columnCount = 0;
    for (const QString& line : lines) {
        rows.push_back(line.split(QLatin1Char('|'), Qt::KeepEmptyParts));
        columnCount = std::max(columnCount, rows.back().size());
    }

    if (columnCount <= 1) {
        return result;
    }

    std::vector<double> widths(static_cast<std::size_t>(columnCount), 0.0);
    const double symbolCellPadding = Gui::QtTools::horizontalAdvance(fm, QStringLiteral(" "));
    const double valueCellPadding = Gui::QtTools::horizontalAdvance(fm, QStringLiteral("    "));
    for (const auto& row : rows) {
        for (qsizetype column = 0; column < columnCount; ++column) {
            const QString cell = column < row.size() ? row.at(column).trimmed() : QString();
            const double padding = column == 0 ? symbolCellPadding : valueCellPadding;
            widths[static_cast<std::size_t>(column)] = std::max(
                widths[static_cast<std::size_t>(column)],
                static_cast<double>(Gui::QtTools::horizontalAdvance(fm, cell)) + padding);
        }
    }

    // GD&T/ISO-GPS feature control frames conventionally use a square symbol cell.
    widths.front() = std::max(widths.front(), static_cast<double>(fm.lineSpacing()));

    result.text.clear();
    QString html = QStringLiteral("<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\">");
    for (std::size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        const auto& row = rows[rowIndex];
        QString displayRow;
        html += QStringLiteral("<tr>");
        for (qsizetype column = 0; column < columnCount; ++column) {
            const QString cell = column < row.size() ? row.at(column).trimmed() : QString();
            displayRow += cell;
            html += QStringLiteral("<td width=\"%1\" align=\"center\" valign=\"middle\">%2</td>")
                    .arg(std::ceil(widths[static_cast<std::size_t>(column)]))
                    .arg(cell.toHtmlEscaped());
        }
        html += QStringLiteral("</tr>");
        if (rowIndex + 1 < rows.size()) {
            displayRow += QLatin1Char('\n');
        }
        result.text += displayRow;
    }
    html += QStringLiteral("</table>");

    result.html = html;
    result.rowCount = static_cast<int>(std::max<qsizetype>(1, rows.size()));
    result.width = std::accumulate(widths.begin(), widths.end(), 0.0);

    double separator = 0.0;
    for (qsizetype column = 0; column < columnCount - 1; ++column) {
        separator += widths[static_cast<std::size_t>(column)];
        result.separators.push_back(separator);
    }

    return result;
}
}

QGIBalloonLabel::QGIBalloonLabel()
{
    m_originDrag = false;
    m_dragging = false;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);

    m_labelText = new QGCustomText();
    m_labelText->setTightBounding(true);
    m_labelText->document()->setDocumentMargin(0.0);
    QTextOption textOption = m_labelText->document()->defaultTextOption();
    textOption.setAlignment(Qt::AlignCenter);
    m_labelText->document()->setDefaultTextOption(textOption);
    m_labelText->setParentItem(this);

    verticalSep = false;
    rowCount = 1;
    tableWidth = 0.0;
    hasHover = false;
    parent = nullptr;
}

QVariant QGIBalloonLabel::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            Q_EMIT selected(true);
            setPrettySel();
        }
        else {
            Q_EMIT selected(false);
            setPrettyNormal();
        }
        update();
    }
    else if (change == ItemPositionHasChanged && scene()) {
        if (m_dragging) {
            Q_EMIT dragging(m_originDrag);
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void QGIBalloonLabel::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_originDrag = false;
    m_dragging = true;

    if (event->button() != Qt::LeftButton) {
        QGraphicsItem::mousePressEvent(event);
        return;
    }

    if (QGSPage::cleanModifierList(event->modifiers()) == Preferences::balloonDragModifiers()) {
        if (!PreferencesGui::multiSelection() ||
            Preferences::multiselectModifiers() != Preferences::balloonDragModifiers()) {
            // multiselect does not apply or does not conflict, so treat this is an origin drag
            m_originDrag = true;
        }
    }

    QGraphicsItem::mousePressEvent(event);
}

void QGIBalloonLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() > 0) {
        if (scene() && this == scene()->mouseGrabberItem()) {
            Q_EMIT dragFinished();
        }
    }
    m_originDrag = false;
    m_dragging = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIBalloonLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    QGIViewBalloon* qgivBalloon = dynamic_cast<QGIViewBalloon*>(parentItem());
    if (!qgivBalloon) {
        qWarning() << "QGIBalloonLabel::mouseDoubleClickEvent: No parent item";
        return;
    }

    auto ViewProvider = freecad_cast<ViewProviderBalloon*>(
        qgivBalloon->getViewProvider(qgivBalloon->getViewObject()));
    if (!ViewProvider) {
        qWarning() << "QGIBalloonLabel::mouseDoubleClickEvent: No valid view provider";
        return;
    }

    ViewProvider->startDefaultEditMode();
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void QGIBalloonLabel::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_EMIT hover(true);
    hasHover = true;
    if (!isSelected()) {
        setPrettyPre();
    }
    else {
        setPrettySel();
    }
    QGraphicsItem::hoverEnterEvent(event);
}

void QGIBalloonLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGIView* view = dynamic_cast<QGIView*>(parentItem());
    assert(view);
    Q_UNUSED(view);

    Q_EMIT hover(false);
    hasHover = false;
    if (!isSelected()) {
        setPrettyNormal();
    }
    else {
        setPrettySel();
    }
    QGraphicsItem::hoverLeaveEvent(event);
}

QRectF QGIBalloonLabel::boundingRect() const { return childrenBoundingRect(); }

void QGIBalloonLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //QGraphicsObject/QGraphicsItem::paint gives link error.
}

void QGIBalloonLabel::setPosFromCenter(const double& xCenter, const double& yCenter)
{
    //set label's Qt position(top, left) given boundingRect center point
    setPos(xCenter - m_labelText->boundingRect().center().x(),
           yCenter - m_labelText->boundingRect().center().y());
}

Base::Vector3d QGIBalloonLabel::getLabelCenter() const
{
    return Base::Vector3d(getCenterX(), getCenterY(), 0.0);
}

void QGIBalloonLabel::setFont(QFont font) { m_labelText->setFont(font); }

void QGIBalloonLabel::setDimString(QString text)
{
    prepareGeometryChange();
    m_labelText->setPlainText(text);
}

void QGIBalloonLabel::setDimString(QString text, qreal maxWidth)
{
    prepareGeometryChange();
    m_labelText->setPlainText(text);
    m_labelText->setTextWidth(maxWidth);
}

void QGIBalloonLabel::setDimHtml(QString html)
{
    prepareGeometryChange();
    m_labelText->setTextWidth(-1);
    m_labelText->setHtml(html);
}

void QGIBalloonLabel::setPrettySel() { m_labelText->setPrettySel(); }

void QGIBalloonLabel::setPrettyPre() { m_labelText->setPrettyPre(); }

void QGIBalloonLabel::setPrettyNormal() { m_labelText->setPrettyNormal(); }

void QGIBalloonLabel::setColor(QColor color)
{
    m_colNormal = color;
    m_labelText->setColor(m_colNormal);
}

//**************************************************************
QGIViewBalloon::QGIViewBalloon()
    : dvBalloon(nullptr), hasHover(false), m_lineWidth(0.0), m_obtuse(false), parent(nullptr),
      m_dragInProgress(false)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    balloonLabel = new QGIBalloonLabel();
    balloonLabel->setQBalloon(this);

    addToGroup(balloonLabel);
    balloonLabel->setColor(prefNormalColor());
    balloonLabel->setPrettyNormal();

    balloonLines = new QGIDimLines();
    addToGroup(balloonLines);
    balloonLines->setNormalColor(prefNormalColor());
    balloonLines->setPrettyNormal();

    balloonShape = new QGIDimLines();
    addToGroup(balloonShape);
    balloonShape->setNormalColor(prefNormalColor());
    balloonShape->setFill(Qt::transparent, Qt::SolidPattern);
    balloonShape->setPrettyNormal();

    arrow = new QGIArrow();
    addToGroup(arrow);
    arrow->setNormalColor(prefNormalColor());
    arrow->setFillColor(prefNormalColor());
    arrow->setPrettyNormal();
    arrow->setStyle(prefDefaultArrow());

    balloonLabel->setZValue(ZVALUE::BALLOON);
    arrow->setZValue(ZVALUE::DIMENSION);

    balloonLines->setZValue(ZVALUE::DIMENSION);
    balloonLines->setStyle(Qt::SolidLine);

    balloonShape->setZValue(ZVALUE::DIMENSION + 1);//above balloonLines!
    balloonShape->setStyle(Qt::SolidLine);

    balloonLabel->setPosFromCenter(0, 0);

    // connecting the needed slots and signals
    QObject::connect(balloonLabel, &QGIBalloonLabel::dragging, this, &QGIViewBalloon::balloonLabelDragged);

    QObject::connect(balloonLabel, &QGIBalloonLabel::dragFinished, this, &QGIViewBalloon::balloonLabelDragFinished);

    QObject::connect(balloonLabel, &QGIBalloonLabel::selected, this, &QGIViewBalloon::select);

    QObject::connect(balloonLabel, &QGIBalloonLabel::hover, this, &QGIViewBalloon::hover);

    setZValue(ZVALUE::DIMENSION);
}

QVariant QGIViewBalloon::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            balloonLabel->setSelected(true);
        }
        else {
            balloonLabel->setSelected(false);
        }
        draw();
        return value;
    }

    if(change == ItemPositionChange && scene()) {
        // QGIVBalloon doesn't really change position the way other views do.
        // If we call QGIView::itemChange it will set the position to (0,0) instead of
        // using the label's position, and the Balloon will be in the wrong place.
        // QGIVDimension behaves the same way.
        return QGraphicsItem::itemChange(change, value);
    }

    return QGIView::itemChange(change, value);
}

bool QGIViewBalloon::getGroupSelection()
{
    return balloonLabel->isSelected();
}

//Set selection state for this and its children
void QGIViewBalloon::setGroupSelection(bool isSelected)
{
    //    Base::Console().message("QGIVB::setGroupSelection(%d)\n", b);
    setSelected(isSelected);
    balloonLabel->setSelected(isSelected);
    balloonLines->setSelected(isSelected);
    balloonShape->setSelected(isSelected);
    arrow->setSelected(isSelected);
}

void QGIViewBalloon::select(bool state)
{
    //    Base::Console().message("QGIVBall::select(%d)\n", state);
    setSelected(state);
    draw();
}

void QGIViewBalloon::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewBalloon::setViewPartFeature(TechDraw::DrawViewBalloon* balloonFeat)
{
    //    Base::Console().message("QGIVB::setViewPartFeature()\n");
    if (!balloonFeat) {
        return;
    }

    setViewFeature(static_cast<TechDraw::DrawView*>(balloonFeat));
    dvBalloon = balloonFeat;

    DrawView* balloonParent = nullptr;
    double scale = 1.0;
    App::DocumentObject* docObj = balloonFeat->SourceView.getValue();
    if (docObj) {
        balloonParent = freecad_cast<DrawView*>(docObj);
        if (balloonParent) {
            scale = balloonParent->getScale();
        }
    }

    float x = Rez::guiX(balloonFeat->X.getValue() * scale);
    float y = Rez::guiX(-balloonFeat->Y.getValue() * scale);

    balloonLabel->setColor(prefNormalColor());
    balloonLabel->setPosFromCenter(x, y);

    QString labelText = QString::fromUtf8(balloonFeat->Text.getStrValue().data());
    balloonLabel->setDimString(labelText, Rez::guiX(balloonFeat->TextWrapLen.getValue()));

    updateBalloon();

    draw();
}

void QGIViewBalloon::updateView(bool update)
{
    Q_UNUSED(update);
    auto balloon(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloon) {
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    if (update) {
        QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
        balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
        setNormalColorAll();
    }

    updateBalloon();
    draw();
}

//update the bubble contents
void QGIViewBalloon::updateBalloon(bool obtuse)
{
    (void)obtuse;
    const auto balloon(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloon) {
        return;
    }
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    const TechDraw::DrawView* refObj = balloon->getParentView();
    if (!refObj) {
        return;
    }

    QFont font;
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    font.setPixelSize(exactFontSize(vp->Font.getValue(), vp->Fontsize.getValue()));
    balloonLabel->setFont(font);

    QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
    QString labelHtml;
    balloonLabel->setVerticalSep(false);
    balloonLabel->setSeps(std::vector<double>());
    balloonLabel->setRowCount(1);
    balloonLabel->setTableWidth(0.0);

    if (strcmp(balloon->BubbleShape.getValueAsString(), "Rectangle") == 0) {
        const auto tableText = makeBalloonTableText(labelText, balloonLabel->getFont());
        if (!tableText.separators.empty()) {
            labelText = tableText.text;
            labelHtml = tableText.html;
            balloonLabel->setVerticalSep(true);
            balloonLabel->setSeps(tableText.separators);
            balloonLabel->setRowCount(tableText.rowCount);
            balloonLabel->setTableWidth(tableText.width);
        }
    }

    if (labelHtml.isEmpty()) {
        balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
    }
    else {
        balloonLabel->setDimHtml(labelHtml);
    }
    float x = Rez::guiX(balloon->X.getValue() * refObj->getScale());
    float y = Rez::guiX(balloon->Y.getValue() * refObj->getScale());
    balloonLabel->setPosFromCenter(x, -y);
}

void QGIViewBalloon::balloonLabelDragged(bool originDrag)
{
    auto dvb(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!dvb) {
        return;
    }

    if (!m_dragInProgress) {//first drag movement
        m_dragInProgress = true;
        if (originDrag) {//moving whole thing, remember Origin offset from Bubble
            m_saveOriginOffset = dvb->getOriginOffset();
            m_saveOrigin = DU::toVector3d(arrow->pos());
            m_savePosition = DU::toVector3d(balloonLabel->pos());
        }
    }

    // store if origin is also moving to be able to later calc new origin and update feature
    if (originDrag) {
        m_originDragged = true;
    }

    DrawView* balloonParent = getSourceView();
    if (balloonParent) {
        // redraw the balloon at the new position
        // note that we don't store the new position to the X/Y properties
        // since the dragging is not yet finished
        // TODO: we don't need to redraw if the graphic is not changing (other than location).
        //       all of the balloon components (box, text, line, arrow, etc) would have to be placed
        //       into a QGraphicsItemGroup (or made children of a single QGI) in order to move them in
        //       sync
        drawBalloon(true);
    }
}

void QGIViewBalloon::balloonLabelDragFinished()
{
    // stores the final drag position for undo

    auto dvb(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!dvb) {
        return;
    }

    double scale = 1.0;
    DrawView* balloonParent = getSourceView();
    if (!balloonParent) {
        return;
    }
    scale = balloonParent->getScale();

    //set feature position (x, y) from graphic position
    double x = Rez::appX(balloonLabel->getCenterX() / scale);
    double y = Rez::appX(balloonLabel->getCenterY() / scale);

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Drag Balloon"));

    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.X = %f",
                            dvb->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Y = %f",
                            dvb->getNameInDocument(), -y);

    // for the case that origin was also dragged, calc new origin and update feature
    if (m_originDragged) {
        auto originGui = arrowPosInDrag();                  // Qt rotated
        auto originApp = originGui / scale;                 // Qt rotated unscaled
        originApp = Rez::appX(DU::invertY(originApp));      // App rotated

        auto originAppUnrotated = originApp;
        auto rotationDeg = balloonParent->Rotation.getValue();
        if (rotationDeg != 0) {
            originAppUnrotated.RotateZ(Base::toRadians(-rotationDeg));
        }

        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.OriginX = %f",
                                dvb->getNameInDocument(), originAppUnrotated.x);
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.OriginY = %f",
                                dvb->getNameInDocument(), originAppUnrotated.y);
    }

    Gui::Command::commitCommand(tid);

    m_dragInProgress = false;
    m_originDragged = false;
    drawBalloon(false);
}

//from QGVP::mouseReleaseEvent - pos = eventPos in scene coords?
void QGIViewBalloon::placeBalloon(QPointF pos)
{
    //    Base::Console().message("QGIVB::placeBalloon(%s)\n",
    //                            DrawUtil::formatVector(pos).c_str());
    auto balloon(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloon) {
        return;
    }

    DrawView* balloonParent = freecad_cast<DrawView*>(balloon->SourceView.getValue());
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
    auto partVP = freecad_cast<ViewProviderViewPart*>(objVp);
    if (partVP) {
        qgivParent = partVP->getQView();
        if (qgivParent) {
            //tip position is mouse release pos in parentView coords ==> OriginX, OriginY
            //bubble pos is some arbitrary shift from tip position ==> X, Y
            viewPos = qgivParent->mapFromScene(pos);
            balloon->OriginX.setValue(Rez::appX(viewPos.x()) / balloonParent->getScale());
            balloon->OriginY.setValue(-Rez::appX(viewPos.y()) / balloonParent->getScale());
            balloon->X.setValue(Rez::appX((viewPos.x() + 200.0) / balloonParent->getScale()));
            balloon->Y.setValue(-Rez::appX((viewPos.y() - 200.0) / balloonParent->getScale()));
        }
    }

    int idx = featPage->getNextBalloonIndex();
    QString labelText = QString::number(idx);
    balloon->Text.setValue(std::to_string(idx).c_str());

    QFont font = balloonLabel->getFont();
    font.setPixelSize(calculateFontPixelSize(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    font.setPixelSize(exactFontSize(vp->Font.getValue(), vp->Fontsize.getValue()));
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

void QGIViewBalloon::drawBalloon(bool originDrag)
{
    using std::numbers::pi;

    if ((!originDrag) && m_dragInProgress) {
        // TODO there are 2 drag status variables.  m_draggingInProgress appears to be the one to use?
        // dragged shows false while drag is still in progress.
        return;
    }

    prepareGeometryChange();

    TechDraw::DrawViewBalloon* balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject());
    if ((!balloon) ||
        (!balloon->isDerivedFrom<TechDraw::DrawViewBalloon>())) {
        //nothing to draw, don't try
        return;
    }
    if (balloon->isRestoring()) {
        // don't try to draw yet
        return;
    }

    TechDraw::DrawView* refObj = balloon->getParentView();
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!refObj || !vp) {
        // can't draw this.  probably restoring.
        return;
    }

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());

    double textWidth = balloonLabel->getDimText()->boundingRect().width();
    double textHeight = balloonLabel->getDimText()->boundingRect().height();

    float arrowTipX;
    Base::Vector3d arrowTipPosInParent;
    bool isDragging = originDrag || m_dragInProgress;
    Base::Vector3d labelPos;
    getBalloonPoints(balloon, refObj, isDragging, labelPos, arrowTipPosInParent);
    arrowTipX = arrowTipPosInParent.x;
    Base::Vector3d lblCenter(labelPos.x, -labelPos.y, 0.0);

    if (balloon->isLocked()) {
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, false);
    }
    else {
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    Base::Vector3d dLineStart;
    Base::Vector3d kinkPoint;
    double kinkLength = Rez::guiX(balloon->KinkLength.getValue());

    const char* balloonType = balloon->BubbleShape.getValueAsString();

    float scale = balloon->ShapeScale.getValue();
    double offsetLR = 0;
    double offsetUD = 0;
    bool useRectAnchor = false;
    bool useCircleAnchor = false;
    double anchorHalfWidth = 0.0;
    double anchorHalfHeight = 0.0;
    double anchorRadius = 0.0;
    QPainterPath balloonPath;

    if (strcmp(balloonType, "Circular") == 0) {
        double balloonRadius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        balloonRadius = balloonRadius * scale;
        balloonPath.moveTo(lblCenter.x, lblCenter.y);
        balloonPath.addEllipse(lblCenter.x - balloonRadius, lblCenter.y - balloonRadius,
                               balloonRadius * 2, balloonRadius * 2);
        offsetLR = balloonRadius;
        useCircleAnchor = true;
        anchorRadius = balloonRadius;
    }
    else if (strcmp(balloonType, "None") == 0) {
        balloonPath = QPainterPath();
        offsetLR = (textWidth / 2.0) + Rez::guiX(2.0);
        useRectAnchor = true;
        anchorHalfWidth = offsetLR;
        anchorHalfHeight = (textHeight / 2.0) + Rez::guiX(1.0);
    }
    else if (strcmp(balloonType, "Rectangle") == 0) {
        //Add some room
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        const double tableWidth = balloonLabel->getTableWidth() * scale;
        const double frameWidth = std::max((textWidth * scale) + Rez::guiX(2.0),
                                           tableWidth + Rez::guiX(2.0));
        if (balloonLabel->getVerticalSep()) {
            const double frameLeft = lblCenter.x - (frameWidth / 2.0);
            const double frameTop = lblCenter.y - (textHeight / 2.0);
            const double tableOffset = (frameWidth - tableWidth) / 2.0;
            for (const auto sep : balloonLabel->getSeps()) {
                const double sepX = frameLeft + tableOffset + (sep * scale);
                balloonPath.moveTo(sepX, frameTop);
                balloonPath.lineTo(sepX, frameTop + textHeight);
            }
            const int rowCount = balloonLabel->getRowCount();
            for (int row = 1; row < rowCount; ++row) {
                const double sepY = frameTop + (textHeight * row / rowCount);
                balloonPath.moveTo(frameLeft, sepY);
                balloonPath.lineTo(frameLeft + frameWidth, sepY);
            }
        }
        textWidth = frameWidth;
        balloonPath.addRect(lblCenter.x - (textWidth / 2.0), lblCenter.y - (textHeight / 2.0),
                            textWidth, textHeight);
        offsetLR = (textWidth / 2.0);
        useRectAnchor = true;
        anchorHalfWidth = textWidth / 2.0;
        anchorHalfHeight = textHeight / 2.0;
    }
    else if (strcmp(balloonType, "Triangle") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(3.0);
        offsetLR = tan(Base::toRadians(30.0)) * radius;
        QPolygonF triangle;
        double startAngle = -pi / 2;
        double angle = startAngle;
        for (int i = 0; i < 4; i++) {
            triangle +=
                QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * pi / 3);
        }
        balloonPath.moveTo(lblCenter.x + (radius * cos(startAngle)),
                           lblCenter.y + (radius * sin(startAngle)));
        balloonPath.addPolygon(triangle);
    }
    else if (strcmp(balloonType, "Inspection") == 0) {
        //Add some room
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        QPointF textBoxCorner(lblCenter.x - (textWidth / 2.0), lblCenter.y - (textHeight / 2.0));
        balloonPath.moveTo(textBoxCorner);
        balloonPath.lineTo(textBoxCorner.x() + textWidth, textBoxCorner.y());
        balloonPath.arcTo(textBoxCorner.x() + textWidth - (textHeight / 2.0), textBoxCorner.y(),
                          textHeight, textHeight, 90, -180);
        balloonPath.lineTo(textBoxCorner.x(), textBoxCorner.y() + textHeight);
        balloonPath.arcTo(textBoxCorner.x() - (textHeight / 2), textBoxCorner.y(), textHeight,
                          textHeight, -90, -180);
        offsetLR = (textWidth / 2.0) + (textHeight / 2.0);
        useRectAnchor = true;
        anchorHalfWidth = offsetLR;
        anchorHalfHeight = textHeight / 2.0;
    }
    else if (strcmp(balloonType, "Square") == 0) {
        //Add some room
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        double max = std::max(textWidth, textHeight);
        balloonPath.addRect(lblCenter.x - (max / 2.0), lblCenter.y - (max / 2.0), max, max);
        offsetLR = (max / 2.0);
        useRectAnchor = true;
        anchorHalfWidth = max / 2.0;
        anchorHalfHeight = max / 2.0;
    }
    else if (strcmp(balloonType, "Hexagon") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(1.0);
        offsetLR = radius;
        QPolygonF triangle;
        double startAngle = -2 * pi / 3;
        double angle = startAngle;
        for (int i = 0; i < 7; i++) {
            triangle +=
                QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * pi / 6);
        }
        balloonPath.moveTo(lblCenter.x + (radius * cos(startAngle)),
                           lblCenter.y + (radius * sin(startAngle)));
        balloonPath.addPolygon(triangle);
    }
    else if (strcmp(balloonType, "Line") == 0) {
        textHeight = textHeight * scale + Rez::guiX(0.5);
        textWidth = textWidth * scale + Rez::guiX(1.0);

        offsetLR = textWidth / 2.0;
        offsetUD = textHeight / 2.0;

        balloonPath.moveTo(lblCenter.x - textWidth / 2.0, lblCenter.y + offsetUD);
        balloonPath.lineTo(lblCenter.x + textWidth / 2.0, lblCenter.y + offsetUD);
    }

    balloonShape->setPath(balloonPath);

    offsetLR = (lblCenter.x < arrowTipX) ? offsetLR : -offsetLR;

    // avoid starting the line inside the balloon
    dLineStart.y = lblCenter.y + offsetUD;
    dLineStart.x = lblCenter.x + offsetLR;
    Base::Vector3d centerToArrow = arrowTipPosInParent - lblCenter;
    if (useCircleAnchor) {
        double length = centerToArrow.Length();
        if (!DrawUtil::fpCompare(length, 0.0)) {
            dLineStart = lblCenter + (centerToArrow / length * anchorRadius);
        }
    }
    else if (useRectAnchor) {
        const double dx = centerToArrow.x;
        const double dy = centerToArrow.y;
        if (!DrawUtil::fpCompare(dx, 0.0) || !DrawUtil::fpCompare(dy, 0.0)) {
            if (std::abs(dx) * anchorHalfHeight > std::abs(dy) * anchorHalfWidth) {
                const double side = (dx >= 0.0) ? anchorHalfWidth : -anchorHalfWidth;
                dLineStart.x = lblCenter.x + side;
                dLineStart.y = lblCenter.y + dy * std::abs(side / dx);
            }
            else {
                const double side = (dy >= 0.0) ? anchorHalfHeight : -anchorHalfHeight;
                dLineStart.x = lblCenter.x + dx * std::abs(side / dy);
                dLineStart.y = lblCenter.y + side;
            }
        }
    }

    if (DrawUtil::fpCompare(kinkLength, 0.0)
        && strcmp(balloonType,
                  "Line")) {//if no kink, then dLine start sb on line from center to arrow
        kinkPoint = dLineStart;
    }
    else {
        kinkLength = (lblCenter.x < arrowTipX) ? kinkLength : -kinkLength;
        kinkPoint.y = dLineStart.y;
        kinkPoint.x = dLineStart.x + kinkLength;
    }

    QPainterPath dLinePath;
    dLinePath.moveTo(dLineStart.x, dLineStart.y);
    dLinePath.lineTo(kinkPoint.x, kinkPoint.y);

    double xAdj = 0.0;
    double yAdj = 0.0;
    ArrowType endType = static_cast<ArrowType>(balloon->EndType.getValue());
    double arrowAdj = QGIArrow::getOverlapAdjust(
        endType, balloon->EndTypeScale.getValue() * QGIArrow::getPrefArrowSize());

    if (endType == ArrowType::NONE) {
        arrow->hide();
    }
    else {
        arrow->setStyle(endType);
        arrow->setSize(balloon->EndTypeScale.getValue() * QGIArrow::getPrefArrowSize());
        arrow->draw();
        arrow->setPos(DU::toQPointF(arrowTipPosInParent));

        Base::Vector3d dirballoonLinesLine;
        if (!DrawUtil::fpCompare(kinkLength, 0.0)) {
            dirballoonLinesLine = (arrowTipPosInParent - kinkPoint).Normalize();
        }
        else {
            dirballoonLinesLine = (arrowTipPosInParent - dLineStart).Normalize();
        }

        float arAngle = Base::toDegrees(atan2(dirballoonLinesLine.y, dirballoonLinesLine.x));

        if ((endType == ArrowType::FILLED_TRIANGLE) && (prefOrthoPyramid())) {
            if (arAngle < 0.0) {
                arAngle += 360.0;
            }
            //set the angle to closest cardinal direction
            if ((45.0 < arAngle) && (arAngle < 135.0)) {
                arAngle = 90.0;
            }
            else if ((135.0 < arAngle) && (arAngle < 225.0)) {
                arAngle = 180.0;
            }
            else if ((225.0 < arAngle) && (arAngle < 315.0)) {
                arAngle = 270.0;
            }
            else {
                arAngle = 0;
            }
            double radAngle = Base::toRadians(arAngle);
            double sinAngle = sin(radAngle);
            double cosAngle = cos(radAngle);
            xAdj = Rez::guiX(arrowAdj * cosAngle);
            yAdj = Rez::guiX(arrowAdj * sinAngle);
        }
        arrow->setRotation(arAngle);
        arrow->show();
    }
    dLinePath.lineTo(arrowTipPosInParent.x - xAdj, arrowTipPosInParent.y - yAdj);
    balloonLines->setPath(dLinePath);

    // This overwrites the previously created QPainterPath with empty one, in case it should be hidden.  Should be refactored.
    if (!vp->LineVisible.getValue()) {
        arrow->hide();
        balloonLines->setPath(QPainterPath());
    }

    // redraw the Balloon and the parent View
    if (hasHover && !isSelected()) {
        setPrettyPre();
    }
    else if (isSelected()) {
        setPrettySel();
    }
    else {
        setPrettyNormal();
    }

    update();
    if (parentItem()) {
        parentItem()->update();
    }
}

void QGIViewBalloon::setPrettyPre(void)
{
    arrow->setPrettyPre();
    balloonShape->setPrettyPre();
    balloonLines->setPrettyPre();
}

void QGIViewBalloon::setPrettySel(void)
{
    //    Base::Console().message("QGIVBal::setPrettySel()\n");
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
    //    Base::Console().message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
}

void QGIViewBalloon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                           QWidget* widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    setPens();
    if (svg) {
        setSvgPens();
    }
    else {
        setPens();
    }
    QGIView::paint(painter, &myOption, widget);
    setPens();
}

void QGIViewBalloon::setSvgPens(void)
{
    double svgLineFactor = 3.0;//magic number.  should be a setting somewhere.
    balloonLines->setWidth(m_lineWidth / svgLineFactor);
    balloonShape->setWidth(m_lineWidth / svgLineFactor);
    arrow->setWidth(arrow->getWidth() / svgLineFactor);
}

void QGIViewBalloon::setPens(void)
{
    balloonLines->setWidth(m_lineWidth);
    balloonShape->setWidth(m_lineWidth);
    balloonShape->setFillColor(PreferencesGui::pageQColor());
    arrow->setWidth(m_lineWidth);
}

void QGIViewBalloon::setNormalColorAll()
{
    QColor qc = prefNormalColor();
    balloonLabel->setColor(qc);
    balloonLines->setNormalColor(qc);
    balloonShape->setNormalColor(qc);
    arrow->setNormalColor(qc);
    arrow->setFillColor(qc);
}

QColor QGIViewBalloon::prefNormalColor()
{
    setNormalColor(PreferencesGui::getAccessibleQColor(PreferencesGui::dimQColor()));

    ViewProviderBalloon* vpBalloon = nullptr;
    Gui::ViewProvider* vp = getViewProvider(getBalloonFeat());
    if (vp) {
        vpBalloon = freecad_cast<ViewProviderBalloon*>(vp);
        if (vpBalloon) {
            Base::Color fcColor = Preferences::getAccessibleColor(vpBalloon->Color.getValue());
            setNormalColor(fcColor.asValue<QColor>());
        }
    }
    return getNormalColor();
}

ArrowType QGIViewBalloon::prefDefaultArrow() const {
    return Preferences::balloonArrow();
}

//should this be an object property or global preference?
//when would you want a crooked pyramid?
bool QGIViewBalloon::prefOrthoPyramid() const
{
    return Preferences::getPreferenceGroup("Decorations")->GetBool("PyramidOrtho", true);
}

DrawView* QGIViewBalloon::getSourceView() const
{
    DrawView* balloonParent = nullptr;
    App::DocumentObject* docObj = getViewObject();
    DrawViewBalloon* dvb = freecad_cast<DrawViewBalloon*>(docObj);
    if (dvb) {
        balloonParent = freecad_cast<DrawView*>(dvb->SourceView.getValue());
    }
    return balloonParent;
}

//! Calculate the required position of the arrow tip during drag operations.  Uses the current
//! label position and relative positions of the label and tip at the start of the drag.
//! Note this returns the Gui position of the arrow, not the App position.
Base::Vector3d QGIViewBalloon::arrowPosInDrag()
{
    auto offsetGui = m_savePosition - m_saveOrigin;
    auto arrowPosGui = DU::toVector3d(balloonLabel->pos()) - offsetGui;
    return arrowPosGui;
}


//! retrieves the appropriate label position and origin (arrow) point
void QGIViewBalloon::getBalloonPoints(TechDraw::DrawViewBalloon* balloon, DrawView *refObj,
                                      bool isDragging,
                                      Base::Vector3d& labelPos,
                                      Base::Vector3d& arrowPos)
{
    float x, y;
    Base::Vector3d originApp{balloon->OriginX.getValue(), balloon->OriginY.getValue(), 0.0};
    Base::Vector3d arrowTipPosInParent;

    // when not dragging take the X/Y properties otherwise the current label position
    if (!isDragging) {
        x = Rez::guiX(balloon->X.getValue() * refObj->getScale());
        y = Rez::guiX(balloon->Y.getValue() * refObj->getScale());
        arrowTipPosInParent = DGU::toGuiPoint(refObj, originApp);
    }
    else {
        x =  balloonLabel->getCenterX();
        y = -balloonLabel->getCenterY();     // invert from Qt scene units to R2 mm
        if (m_originDragged) {
            // moving the whole bubble object. do not adjust origin point.
            arrowTipPosInParent = arrowPosInDrag();
        } else {
            // this is a bubble drag, so the origin must remain in the same position on the view.
            // if the parent view is rotated, the origin scene position must be rotated to match
            arrowTipPosInParent = DGU::toGuiPoint(refObj, originApp);
        }
    }
    labelPos = Base::Vector3d(x, y, 0.0);
    arrowPos = arrowTipPosInParent;
}

#include <Mod/TechDraw/Gui/moc_QGIViewBalloon.cpp>
