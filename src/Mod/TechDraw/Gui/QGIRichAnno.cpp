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

# include <QAbstractTextDocumentLayout>
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
# include <QTextDocument>
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
    m_isEditing(false),
    m_textScaleFactor(1.0),
    m_lastGoodWidthScene(0.0)
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

    m_rect = new QGCustomRect();
    addToGroup(m_rect);
    m_rect->setZValue(ZVALUE::DIMENSION - 1);

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

    // Convert the word processing font size spec (in typographic points) to scene units for
    // the screen or pdf rendering
    constexpr double mmPerPoint {25.4 / 72};  //  mm/in / points/inch
    m_textScaleFactor = Rez::getRezFactor() * mmPerPoint;  // scene units per point: 3.53
    m_text->setScale(m_textScaleFactor);

    draw();
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
        m_text->setHtml(QString::fromUtf8(annoFeat->AnnoText.getValue()));
    }

    // 1. Get the bounding rectangle of the text in its own local coordinates.
    QRectF textParentRect = m_text->mapRectToParent(m_text->boundingRect());

    QPointF offset(0.0, 0.0);
    if (annoFeat->OriginCentered.getValue()) {
        offset = QPointF(-textParentRect.width() / 2.0, -textParentRect.height() / 2.0);
    }
    m_text->setPos(offset);
    textParentRect = m_text->mapRectToParent(m_text->boundingRect());

    if (annoFeat->OriginCentered.getValue() || !getExportingSvg()) {
        m_rect->setRect(textParentRect);
    }

    m_rect->setPen(rectPen());
    m_rect->setBrush(Qt::NoBrush);
    m_rect->setVisible(annoFeat->ShowFrame.getValue());

    if (getExportingSvg()) {
        // Convert the word processing font size spec (in typographic points) to CSS pixels
        // for Svg rendering
        constexpr double mmPerPoint {25.4 / 72};
        constexpr double cssPxPerPoint {16 / 12};  // CSS says 12 pt text is 16 px high
        m_text->setScale(cssPxPerPoint);

        // QSvgRenderer places the text's top edge flush with the item's origin.
        // We must manually shift the QGraphicsTextItem down to create padding.
        QTextBlock firstBlock = m_text->document()->begin();
        if (firstBlock.isValid()) {
            QTextCursor cursor(firstBlock);
            double fontSizePx = cursor.charFormat().fontPointSize();
            m_text->setY(m_text->pos().y() + (fontSizePx * mmPerPoint));
        }

        // lines are correctly spaced on screen or in pdf, but svg needs this
        setLineSpacing(100);
    }

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

TechDraw::DrawRichAnno* QGIRichAnno::getFeature()
{
    return static_cast<TechDraw::DrawRichAnno*>(getViewObject());
}


// TODO: this rect is the right size, but not in the right place
QRectF QGIRichAnno::boundingRect() const
{
    return childrenBoundingRect();
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
            if (m_rect) {
                m_initialTextWidthScene = m_rect->rect().width();
            }
            else {
                // Fallback, should not happen
                m_initialTextWidthScene = Rez::guiX(MinTextWidthDocument * 2);
            }
        }

        m_lastGoodWidthScene = m_initialTextWidthScene;

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
            if (!Gui::Control().activeDialog()) {
                Gui::Command::openCommand(
                    QObject::tr("Resize Rich Annotation").toStdString().c_str());
            }
            m_transactionOpen = true;
            m_isDraggingMidResize = true;
        }

        QPointF currentMouseScenePos = event->scenePos();
        double mouseDeltaSceneX = currentMouseScenePos.x() - m_dragStartMouseScenePos.x();

        // 1. Calculate the raw target width based on mouse movement
        double targetVisualWidthScene = (m_currentResizeHandle == ResizeHandle::RightHandle)
            ? m_initialTextWidthScene + mouseDeltaSceneX
            : m_initialTextWidthScene - mouseDeltaSceneX;

        // Clamp against the geometric minimum
        double geometricMinWidthScene = Rez::guiX(MinTextWidthDocument);
        if (targetVisualWidthScene < geometricMinWidthScene) {
            targetVisualWidthScene = geometricMinWidthScene;
        }

        // --- 2. Perform Synchronous "What-If" Analysis ---
        double finalVisualWidthScene = 0;
        const double originalDocWidth = m_text->document()->textWidth();
        const double targetWidthLocal = targetVisualWidthScene / m_textScaleFactor;
        m_text->document()->setTextWidth(targetWidthLocal);
        QSizeF actualSizeLocal = m_text->document()->documentLayout()->documentSize();
        m_text->document()->setTextWidth(originalDocWidth);

        constexpr double tolerance = 1e-5;
        if (actualSizeLocal.width() > targetWidthLocal + tolerance) {
            finalVisualWidthScene = m_lastGoodWidthScene;
        }
        else {
            finalVisualWidthScene = targetVisualWidthScene;
            m_lastGoodWidthScene = finalVisualWidthScene;
        }

        if (annoFeat->OriginCentered.getValue()) {
            // --- 3. Calculate Final CENTER Position based on Validated Width ---
            // Calculate the *actual* change in width that resulted from the what-if analysis.
            const double actualWidthChange = finalVisualWidthScene - m_initialTextWidthScene;

            // The center of the item moves by exactly half of this actual change.
            double positionDeltaX = 0;
            if (m_currentResizeHandle == ResizeHandle::RightHandle) {
                // Width was added to the right side, so center moves right.
                positionDeltaX = actualWidthChange / 2.0;
            }
            else {  // LeftHandle
                // Width was added to the left side, so center moves left.
                positionDeltaX = -actualWidthChange / 2.0;
            }

            const double newItemScenePosX = m_initialItemScenePos.x() + positionDeltaX;

            annoFeat->MaxWidth.setValue(Rez::appX(finalVisualWidthScene));
            annoFeat->X.setValue(Rez::appX(newItemScenePosX));

            updateView(true);
        }
        else {
            // --- 3. Calculate Final Position based on Validated Width ---
            double newItemScenePosX = 0;
            if (m_currentResizeHandle == ResizeHandle::RightHandle) {
                newItemScenePosX = m_initialItemScenePos.x();
            }
            else {  // LeftHandle
                newItemScenePosX =
                    m_initialItemScenePos.x() + (m_initialTextWidthScene - finalVisualWidthScene);
            }

            // --- 4. Commit Final, Validated State to the Feature ---
            annoFeat->MaxWidth.setValue(Rez::appX(finalVisualWidthScene));
            annoFeat->X.setValue(Rez::appX(newItemScenePosX));
        }

        // The property changes will trigger QGIRichAnno::updateView, which handles the visual
        // update.

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

        // Ensure focus returns to the text item after the resize handle is released
        refocusAnnotation();

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
        
        QTextCursor cursor = m_text->textCursor();

        // Check if the document is empty. If so, create a default format.
        if (m_text->document()->isEmpty()) {
            // Document is empty, so we need to create a default style from scratch.
            // Let's use the default label font from preferences.
            QFont font = PreferencesGui::labelFontQFont();

            QTextCharFormat defaultFormat;
            defaultFormat.setFontPointSize(font.pointSizeF());
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

        refocusAnnotation();

        Q_EMIT positionChanged(scenePos());
    }
    else {
        m_text->setTextInteractionFlags(Qt::NoTextInteraction);
        m_text->clearFocus();
        clearFocus();
    }
    update();
}

void QGIRichAnno::refocusAnnotation()
{
    if (scene()) {
        if (!scene()->views().isEmpty()) {
            scene()->views().first()->setFocus();
        }

        scene()->setFocusItem(m_text, Qt::OtherFocusReason);
    }
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
        drawBorder(); // Make sure view frame is updated.
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
    if (!annoFeat || !m_text || m_textScaleFactor <= 0.0) {
        return;
    }
    prepareGeometryChange();

    double maxWidthDoc = annoFeat->MaxWidth.getValue();
    if (maxWidthDoc > 0.0) {
        m_text->setTextWidth(Rez::guiX(maxWidthDoc) / m_textScaleFactor);
    }
    else {
        m_text->setTextWidth(-1.0);
    }

    update();

    if (scene()) {
        Q_EMIT positionChanged(scenePos());
    }
    drawBorder();
}

#include <Mod/TechDraw/Gui/moc_QGIRichAnno.cpp>
