/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

# include <cmath>

# include <QDialog>
# include <QGraphicsItem>
# include <QGraphicsSceneMouseEvent>
# include <QPainter>
# include <QRegularExpression>
# include <QApplication>
# include <QCursor>
# include <QRegularExpressionMatch>
# include <QTextBlock>
# include <QTextCursor>
# include <QTextDocumentFragment>
# include <QTimer>
# include <QGraphicsScene>
# include <QGraphicsView>

#include <App/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>

#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIRichAnno.h"
#include "mrichtextedit.h"
#include "PreferencesGui.h"
#include "QGCustomRect.h"
#include "QGCustomText.h"
#include "Rez.h"
#include "ViewProviderRichAnno.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"


using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

const double QGIRichAnno::HandleInteractionMargin = 30.0;  // Scene units for hover/click
const double QGIRichAnno::MinTextWidthDocument = 5.0;    // Min width in document units

//**************************************************************
QGIRichAnno::QGIRichAnno() :
    m_isExportingPdf(false),
    m_isExportingSvg(false),
    m_currentResizeHandle(ResizeHandle::NoHandle),
    m_isResizing(false),
    m_isDraggingMidResize(false),
    m_transactionOpen(false),
    m_dragStartMouseScenePos(),
    m_initialItemScenePos(),
    m_initialTextWidthScene(0.0), 
    m_frameWasHiddenOnHoverEnter(false),
    m_isEditing(false)
{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true); // Enable hover events for cursor changes
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_text = new QGCustomText();
    m_text->setTextInteractionFlags(Qt::NoTextInteraction);
    m_text->setDefaultTextColor(PreferencesGui::normalQColor());
    addToGroup(m_text);
    m_text->setZValue(ZVALUE::DIMENSION);
    m_text->centerAt(0.0, 0.0);

    m_rect = new QGCustomRect();
    addToGroup(m_rect);
    m_rect->setZValue(ZVALUE::DIMENSION - 1);
    m_rect->centerAt(0.0, 0.0);

    setZValue(ZVALUE::DIMENSION);

    connect(m_text->document(),
            &QTextDocument::contentsChanged,
            this,
            &QGIRichAnno::onContentsChanged);
    connect(m_text, &QGCustomText::selectionChanged, this, &QGIRichAnno::selectionChanged);
}

void QGIRichAnno::updateView(bool update)
{
//    Base::Console().message("QGIRA::updateView() - %s\n", getViewName());
    Q_UNUSED(update);
    auto annoFeat( dynamic_cast<TechDraw::DrawRichAnno*>(getViewObject()) );
    if (!annoFeat) {
        return;
    }

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    //allow/prevent dragging
    if (getViewObject()->isLocked()) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
    } else {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    if (annoFeat->X.isTouched() ||
        annoFeat->Y.isTouched()) {
        float x = Rez::guiX(annoFeat->X.getValue());
        float y = Rez::guiX(annoFeat->Y.getValue());
        m_text->centerAt(x, -y);
        m_rect->centerAt(x, -y);
     }

    draw();
}

void QGIRichAnno::drawBorder()
{
////Leaders have no border!
//    QGIView::drawBorder();   //good for debugging
}


void QGIRichAnno::draw()
{
//    Base::Console().log("QGIRA::draw() - %s - parent: %X\n", getFeature()->getNameInDocument(), parentItem());
    if (!isVisible())
//        Base::Console().message("QGIRA::draw - not visible\n");
        return;

    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if (!annoFeat)
//        Base::Console().message("QGIRA::draw - no feature\n");
        return;

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getFeature()));
    if (!vp) {
//        Base::Console().message("QGIRA::draw - no viewprovider\n");
        return;
    }

    setTextItem();

    QGIView::draw();
}

void QGIRichAnno::setTextItem()
{
//    Base::Console().message("QGIRA::setTextItem() - %s - exportingSvg: %d\n", getViewName(), getExportingSvg());
    TechDraw::DrawRichAnno* annoFeat = getFeature();

    updateLayout();

    // convert the text size
    if (!m_isEditing) {
        // This block now only runs on initial draw, or after an edit is finished.
        QString inHtml = QString::fromUtf8(annoFeat->AnnoText.getValue());
        QString outHtml = convertTextSizes(inHtml);
        m_text->setHtml(outHtml);
    }

    if (getExportingSvg()) {
        // lines are correctly spaced on screen or in pdf, but svg needs this
        setLineSpacing(100);
    }

    if (!getExportingSvg()) {
        // screen or pdf rendering
        m_text->centerAt(0.0, 0.0);
    }

    // align the frame rectangle to the text
    constexpr double frameMargin{10.0};
    QRectF outRect = m_text->boundingRect().adjusted(-frameMargin, -frameMargin, frameMargin, frameMargin);
    m_rect->setPen(rectPen());
    m_rect->setBrush(Qt::NoBrush);
    if (!getExportingSvg()) {
        m_rect->setRect(outRect);
        m_rect->setPos(m_text->pos().x() - frameMargin, m_text->pos().y() - frameMargin);
    }

    m_rect->setVisible(annoFeat->ShowFrame.getValue() || m_isResizing);

    if (m_isEditing) {
        Q_EMIT positionChanged(scenePos());
    }
}

// attempt to space the lines correctly after the font sizes are changed to match
// the Svg rendering of the QGraphicsTextItem.
void QGIRichAnno::setLineSpacing(int lineSpacing)
{
    // left to itself, Qt appears to space the lines according to this formula
    // DeltaY(in mm) = (1 + 2*pointSize + margin) which vastly under spaces the
    // lines
    m_text->document()->setUseDesignMetrics(true);

    QTextBlock block = m_text->document()->begin();
    for (; block.isValid(); block = block.next()) {
        QTextCursor tc = QTextCursor(block);
        QTextBlockFormat fmt = tc.blockFormat();
        QTextCharFormat cFmt = tc.charFormat();
        // this is already converted to pixels in setTextItem
        //
        double cssPixelSize = cFmt.font().pointSizeF();
        // css Pixels are treated as if they were points in the conversion to Svg
        double textHeightSU = Rez::guiX(cssPixelSize * 25.4 / 72.0);  // actual height of text in scene units
        double pointSize = cssPixelSize * 72.0 / 96.0;
        double deltaYSU = 1.0 + 2.0 * pointSize;    // how far Qt will space lines (based on samples)
        double linegap = 0.4 * cssPixelSize;  // 20% gaps above and below
        // margins will be included in Qt's calculation of spacing

        double margin = linegap * pointSize / 10.0;
        QTextBlockFormat spacerFmt = QTextBlockFormat();
        if (block.previous().isValid()) {
            // there is a block before this one, so add a top margin
            spacerFmt.setTopMargin(margin);
        }
        if (block.next().isValid()) {
            // there is another block after this, so add a bottom margin
            spacerFmt.setBottomMargin(margin);
        }
        double requiredSpacing = (textHeightSU / (deltaYSU - 1.0)) * lineSpacing;
        spacerFmt.setLineHeight(requiredSpacing, QTextBlockFormat::ProportionalHeight);
        tc.mergeBlockFormat(spacerFmt);
    }
}

//! convert the word processing font size spec (in typographic points) to scene units for the screen or
//! pdf rendering or to CSS pixels for Svg rendering
QString QGIRichAnno::convertTextSizes(const QString& inHtml)  const
{
    constexpr double mmPerPoint{0.353};                  // 25.4 mm/in / 72 points/inch
    constexpr double cssPxPerPoint{1.333333};            // CSS says 12 pt text is 16 px high
    double sceneUnitsPerPoint = Rez::getRezFactor() * mmPerPoint;      // scene units per point: 3.53

    QRegularExpression rxFontSize(QStringLiteral("font-size:([0-9]*)pt;"));
    QRegularExpressionMatch match;
    QStringList findList;
    QStringList replList;

    // find each occurrence of "font-size:..." and calculate the equivalent size in scene units
    // or CSS pixels
    int pos = 0;
    while ((pos = inHtml.indexOf(rxFontSize, pos, &match)) != -1) {
        QString found = match.captured(0);
        findList << found;
        QString qsOldSize = match.captured(1);

        QString repl = found;
        double newSize = qsOldSize.toDouble();      // in points
        // The font size in the QGraphicsTextItem html is interpreted differently
        // in QSvgRenderer rendering compared to painting the screen or pdf
        if (getExportingSvg()) {
            // scale point size to CSS pixels
            newSize = newSize * cssPxPerPoint;
        } else {
            // scale point size to scene units
            newSize = newSize * sceneUnitsPerPoint;
        }
        QString qsNewSize = QString::number(newSize, 'f', 2);
        repl.replace(qsOldSize, qsNewSize);
        replList << repl;
        pos += match.capturedLength();
    }
    QString outHtml = inHtml;
    int iRepl = 0;
    //TODO: check list for duplicates?
    for ( ; iRepl < findList.size(); iRepl++) {
        outHtml = outHtml.replace(findList[iRepl], replList[iRepl]);
    }

    return outHtml;
}

TechDraw::DrawRichAnno* QGIRichAnno::getFeature()
{
    return static_cast<TechDraw::DrawRichAnno*>(getViewObject());
}


// TODO: this rect is the right size, but not in the right place
QRectF QGIRichAnno::boundingRect() const
{
    QRectF roughRect = m_text->boundingRect() | m_rect->boundingRect();
    double halfWidth = roughRect.width() / 2.0;
    double halfHeight = roughRect.height() / 2.0;
    return { -halfWidth, - halfHeight, halfWidth * 2.0, halfHeight * 2.0 };
}

void QGIRichAnno::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->setPen(Qt::blue);
//    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}

QPen QGIRichAnno::rectPen() const
{
    const auto sym( dynamic_cast<TechDraw::DrawRichAnno*>(getViewObject()) );
    if (!sym) {
        return QPen();
    }
    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if (!vp) {
        return QPen();
    }

    double rectWeight = Rez::guiX(vp->LineWidth.getValue());
    Qt::PenStyle rectStyle = static_cast<Qt::PenStyle>(vp->LineStyle.getValue());
    Base::Color temp = vp->LineColor.getValue();
    QColor rectColor = temp.asValue<QColor>();

    QPen pen = QPen(rectStyle);
    pen.setWidthF(rectWeight);
    pen.setColor(rectColor);
    return pen;
}

QFont QGIRichAnno::prefFont()
{
    return PreferencesGui::labelFontQFont();
}

void QGIRichAnno::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if (!annoFeat) {
        QGIView::hoverEnterEvent(event);
        return;
    }

    // Store original state and show frame if it's currently hidden
    m_frameWasHiddenOnHoverEnter = !annoFeat->ShowFrame.getValue();
    if (m_frameWasHiddenOnHoverEnter) {
        if (m_rect) {
            m_rect->show();
            // Potentially update related UI elements or trigger a mini-repaint if needed,
            // though m_rect->show() might be enough if it forces a repaint of itself.
            // Forcing a repaint of the item might be safer:
            update();  // This QGraphicsItem::update() schedules a repaint for the item's bounding
                       // rect
        }
    }
    QGIView::hoverEnterEvent(event);
}

void QGIRichAnno::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if (!annoFeat || m_isResizing) {
        QGIView::hoverLeaveEvent(event);
        return;
    }

    // If the frame was originally hidden and we showed it on hover, hide it again
    if (m_frameWasHiddenOnHoverEnter) {
        if (m_rect) {
            m_rect->hide();
            update();  // Schedule a repaint
        }
    }
    // Reset the flag
    m_frameWasHiddenOnHoverEnter = false;

    // Also, ensure the cursor is reset if the mouse leaves while it was a resize cursor
    // This is important if the mouse leaves the item entirely while a resize handle was active.
    setCursor(Qt::ArrowCursor);

    QGIView::hoverLeaveEvent(event);
}

void QGIRichAnno::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if (!annoFeat || annoFeat->isLocked()) {
        setCursor(Qt::ArrowCursor);
        QGIView::hoverMoveEvent(event);
        return;
    }
    // Mouse position in QGIRichAnno's local coordinates
    QPointF localPos = mapFromScene(event->scenePos());  

    // Calculate visual edges of m_rect in QGIRichAnno's local coordinates
    double visualRectLeftEdgeX = m_rect->x() + m_rect->rect().left();
    double visualRectRightEdgeX = m_rect->x() + m_rect->rect().right();

    bool onLeftEdge = qAbs(localPos.x() - visualRectLeftEdgeX) < HandleInteractionMargin;
    bool onRightEdge = qAbs(localPos.x() - visualRectRightEdgeX) < HandleInteractionMargin;

    if (onLeftEdge || onRightEdge) {
        setCursor(Qt::SizeHorCursor);
    }
    else {
        setCursor(Qt::ArrowCursor);
    }
    QGIView::hoverMoveEvent(event);
}

void QGIRichAnno::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    // Allow resizing even if MaxWidth is initially -1 or 0, as long as frame is shown
    if (event->button() != Qt::LeftButton || !annoFeat || annoFeat->isLocked()) {
        QGIView::mousePressEvent(event);
        return;
    }

    // Mouse position in QGIRichAnno's local coordinates
    QPointF localPos = mapFromScene(event->scenePos());
    m_currentResizeHandle = ResizeHandle::NoHandle;

    // Calculate visual edges of m_rect in QGIRichAnno's local coordinates
    // m_rect's geometry should be up-to-date from the last draw/updateView
    double visualRectLeftEdgeX = m_rect->x() + m_rect->rect().left();
    double visualRectRightEdgeX = m_rect->x() + m_rect->rect().right();

    if (qAbs(localPos.x() - visualRectLeftEdgeX) < HandleInteractionMargin) {
        m_currentResizeHandle = ResizeHandle::LeftHandle;
    }
    else if (qAbs(localPos.x() - visualRectRightEdgeX) < HandleInteractionMargin) {
        m_currentResizeHandle = ResizeHandle::RightHandle;
    }

    if (m_currentResizeHandle != ResizeHandle::NoHandle) {
        m_isResizing = true;
        m_isDraggingMidResize = false;
        m_transactionOpen = false;
        m_dragStartMouseScenePos = event->scenePos();
        m_initialItemScenePos = this->scenePos();

        // Determine initial text width for resizing
        if (annoFeat->MaxWidth.getValue() > 0.0) {
            m_initialTextWidthScene = Rez::guiX(annoFeat->MaxWidth.getValue());
        }
        else {
            if (m_text) {
                m_initialTextWidthScene = m_rect->rect().width() - (2 * Rez::guiX(1.0));
                m_initialTextWidthScene = m_text->boundingRect().width();
            }
            else {
                // Fallback, should not happen
                m_initialTextWidthScene = Rez::guiX(MinTextWidthDocument * 2);
            }
        }
        event->accept();
    }
    else {
        m_isResizing = false;
        QGIView::mousePressEvent(event);
    }
}

void QGIRichAnno::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_isResizing && m_currentResizeHandle != ResizeHandle::NoHandle) {
        TechDraw::DrawRichAnno* annoFeat = getFeature();
        if (!annoFeat) {
            return;
        }

        if (!m_isDraggingMidResize) {  // First actual move during this resize op
            // Open a transaction for the entire resize operation
            if (!Gui::Control().activeDialog()) {
                Gui::Command::openCommand(
                    QObject::tr("Resize Rich Annotation").toStdString().c_str());
            }
            m_transactionOpen = true;
            m_isDraggingMidResize = true;
        }

        QPointF currentMouseScenePos = event->scenePos();
        double mouseDeltaSceneX = currentMouseScenePos.x() - m_dragStartMouseScenePos.x();

        double newTextWidthScene = 0;
        double newItemScenePosX = 0;  // New center X of the item in scene coords

        if (m_currentResizeHandle == ResizeHandle::RightHandle) {
            newTextWidthScene = m_initialTextWidthScene + mouseDeltaSceneX;
            newItemScenePosX = m_initialItemScenePos.x() + mouseDeltaSceneX / 2.0;
        }
        else {  // LeftHandle
            newTextWidthScene = m_initialTextWidthScene - mouseDeltaSceneX;
            newItemScenePosX = m_initialItemScenePos.x() + mouseDeltaSceneX / 2.0;
        }

        // Apply minimum width constraint
        double newTextWidthDoc = Rez::appX(newTextWidthScene);
        if (newTextWidthDoc < MinTextWidthDocument) {
            newTextWidthDoc = MinTextWidthDocument;
            newTextWidthScene = Rez::guiX(newTextWidthDoc);  // Recalculate scene width

            // Adjust mouseDeltaSceneX based on the clamped width to correctly position the center
            if (m_currentResizeHandle == ResizeHandle::RightHandle) {
                mouseDeltaSceneX = newTextWidthScene - m_initialTextWidthScene;
            }
            else {  // LeftHandle
                mouseDeltaSceneX = -(newTextWidthScene - m_initialTextWidthScene);
            }
            newItemScenePosX = m_initialItemScenePos.x() + mouseDeltaSceneX / 2.0;
        }

        annoFeat->MaxWidth.setValue(newTextWidthDoc);
        annoFeat->X.setValue(Rez::appX(newItemScenePosX));
        // Y position is not changed by horizontal resize.
        // The property changes will trigger QGIRichAnno::updateView via onChanged/requestPaint,
        // and the item's scene position (this->pos()) will be updated by QGIView::itemChange
        // reacting to the X property change.

        // Emit initial position. This should happen after the geometry update.
        QTimer::singleShot(0, this, [this]() {
            if (this && scene()) {
                Q_EMIT positionChanged(scenePos());
            }
        });

        event->accept();
    }
    else {
        QGIView::mouseMoveEvent(event);
    }
}

void QGIRichAnno::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_isResizing) {
        if (m_transactionOpen) {
            // Only commit if actual dragging (and thus property changes) occurred.
            // m_isDraggingMidResize flag indicates if mouseMoveEvent was processed.
            if (!Gui::Control().activeDialog()) {
                Gui::Command::commitCommand();
            }
            m_transactionOpen = false;
            widthChanged();
        }
        m_isResizing = false;
        m_isDraggingMidResize = false;
        m_currentResizeHandle = ResizeHandle::NoHandle;
        setCursor(Qt::ArrowCursor);  // Reset cursor

        Q_EMIT positionChanged(scenePos());

        if (!isUnderMouse()) {
            QGraphicsSceneHoverEvent leaveEvent(QEvent::GraphicsSceneHoverLeave);
            hoverLeaveEvent(&leaveEvent);  // Manually trigger leave event
        }

        event->accept();
    }
    else {
        QGIView::mouseReleaseEvent(event);
    }
}

void QGIRichAnno::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event);
    
    // If resizing was in progress, cancel it to avoid conflict with dialog
    if (m_isResizing) {
        if (m_transactionOpen) {
            // To avoid partial changes, might need to revert or just abort.
            // For simplicity, we commit if open. A better way would be to store original values and revert.
            if (!Gui::Control().activeDialog()) {
                Gui::Command::commitCommand();
            }
            m_transactionOpen = false;
        }
        m_isResizing = false;
        m_isDraggingMidResize = false;
        m_currentResizeHandle = ResizeHandle::NoHandle;
        setCursor(Qt::ArrowCursor);
    }

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    vp->doubleClicked();
}

void QGIRichAnno::setEditMode(bool enable)
{
    m_isEditing = enable;
    if (enable) {
        m_text->setTextInteractionFlags(Qt::TextEditorInteraction);
        // Maybe change border to indicate editing
        m_rect->setPen(QPen(Qt::DashLine));
        m_rect->show();  // Ensure frame is visible during editing

        
        QTextCursor cursor = m_text->textCursor();

        // Check if the document is empty. If so, create a default format.
        if (m_text->document()->isEmpty()) {
            // Document is empty, so we need to create a default style from scratch.
            // Let's use the default label font from preferences.
            QFont font = PreferencesGui::labelFontQFont();

            // We need to convert its point size to scene units, just like in convertTextSizes.
            constexpr double mmPerPoint {0.353};
            double sceneUnitsPerPoint = Rez::getRezFactor() * mmPerPoint;
            double sceneUnitFontSize = font.pointSizeF() * sceneUnitsPerPoint;

            QTextCharFormat defaultFormat;
            defaultFormat.setFontPointSize(sceneUnitFontSize);
            cursor.setCharFormat(defaultFormat);
        }
        else {
            // Document has content. Let's use the format of the first character
            // as the default for any new text.
            cursor.setPosition(0);
            QTextCharFormat formatAtStart = cursor.charFormat();

            // Move the cursor back to its original position (or end of document)
            cursor.movePosition(QTextCursor::End);

            // Apply the format from the start of the document to the current cursor position.
            // This sets the "default" format for subsequent typing.
            cursor.setCharFormat(formatAtStart);
        }

        // IMPORTANT: Apply the modified cursor back to the text item.
        m_text->setTextCursor(cursor);

        if (scene()) {
            if (!scene()->views().isEmpty()) {
                scene()->views().first()->setFocus();
            }

            scene()->setFocusItem(m_text, Qt::OtherFocusReason);
        }

        Q_EMIT positionChanged(scenePos());
    }
    else {
        m_text->setTextInteractionFlags(Qt::NoTextInteraction);
        m_text->clearFocus();
        clearFocus();
        // Restore normal border
        m_rect->setPen(rectPen());
        // Hide frame if it's supposed to be hidden
        if (!getFeature()->ShowFrame.getValue()) {
            m_rect->hide();
        }
    }
    update();
}

QTextDocument* QGIRichAnno::document() const
{
    return m_text->document();
}

QTextCursor QGIRichAnno::textCursor() const
{
    return m_text->textCursor();
}

void QGIRichAnno::setTextCursor(const QTextCursor& cursor)
{
    m_text->setTextCursor(cursor);
}

void QGIRichAnno::onContentsChanged()
{
    // Only process changes when in edit mode to avoid loops during setup
    if (m_isEditing) {
        // Update the feature property in real-time
        getFeature()->AnnoText.setValue(m_text->toHtml().toUtf8());
        // Emit signal for the task panel
        Q_EMIT textChanged();
    }
}

QVariant QGIRichAnno::itemChange(GraphicsItemChange change, const QVariant& value)
{

    if (change == QGraphicsItem::ItemScenePositionHasChanged
        && scene()) {
        Q_EMIT positionChanged(scenePos());
    }
    return QGIView::itemChange(change, value);
}

void QGIRichAnno::updateLayout()
{
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if (!annoFeat || !m_text) {
        return;
    }

    prepareGeometryChange();

    // This is the crucial part. We re-apply the width constraint
    // from the feature to the QGraphicsTextItem.
    double maxWidth = annoFeat->MaxWidth.getValue();
    if (maxWidth > 0.0) {
        m_text->setTextWidth(Rez::guiX(maxWidth));
    }
    else {
        m_text->setTextWidth(-1.0);
    }

    // Now that the width is correctly configured, we can trigger the geometry update.
    update();

    if (scene()) {
        Q_EMIT positionChanged(scenePos());
    }
}

#include <Mod/TechDraw/Gui/moc_QGIRichAnno.cpp>
