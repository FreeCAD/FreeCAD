/***************************************************************************
 *   Copyright (c) 2025 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QStyleHints>

#include <Base/Console.h>

#include "TagWidget.h"

using namespace Gui;

/* TRANSLATOR Gui::TagWidget */

TagWidget::TagWidget(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    QSizePolicy size_policy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    size_policy.setHeightForWidth(true);
    setSizePolicy(size_policy);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setCursor(Qt::IBeamCursor);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setupCompleter();
    setCursorVisible(hasFocus());
    updateDisplayText();

    viewport()->setContentsMargins(1, 1, 1, 1);
}

/*
 *  Destroys the object and frees any allocated resources
 */
TagWidget::~TagWidget() = default;

void TagWidget::setupCompleter()
{
    _completer->setWidget(this);
    QObject::connect(
        _completer.get(),
        qOverload<QString const&>(&QCompleter::activated),
        [this](QString const& text) { setEditorText(text); }
    );
}

void TagWidget::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    calcRectsUpdateScrollRanges();
}

void TagWidget::focusInEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusInEvent(event);
    _focusedAt = std::chrono::steady_clock::now();
    setCursorVisible(true);
    updateDisplayText();
    calcRectsUpdateScrollRanges();
    if (event->reason() != Qt::FocusReason::MouseFocusReason || _restoreCursorPositionOnFocusClick) {
        ensureCursorIsVisibleH();
        ensureCursorIsVisibleV();
    }
    viewport()->update();
}

void TagWidget::focusOutEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusOutEvent(event);
    setCursorVisible(false);
    updateDisplayText();
    calcRectsUpdateScrollRanges();
    viewport()->update();
}

void TagWidget::paintEvent(QPaintEvent* e)
{
    QAbstractScrollArea::paintEvent(e);

    QPainter painter(viewport());

    painter.setClipRect(contentsRect());

    auto const middle = _tags.cbegin() + static_cast<ptrdiff_t>(_editingIndex);

    // tags
    drawTags(painter, std::ranges::subrange(_tags.cbegin(), middle));

    if (cursorVisible()) {
        drawEditor(painter, palette(), offset());
    }
    else if (!editorText().isEmpty()) {
        drawTags(painter, std::ranges::subrange(middle, middle + 1));
    }

    // tags
    drawTags(painter, std::ranges::subrange(middle + 1, _tags.cend()));
}

void TagWidget::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == _blinkTimer) {
        _blinkStatus = !_blinkStatus;
        viewport()->update();
    }
}

void TagWidget::mousePressEvent(QMouseEvent* event)
{
    // we don't want to change cursor position if this event is part of focusIn
    using namespace std::chrono_literals;
    if (_restoreCursorPositionOnFocusClick && elapsed(_focusedAt) < 1ms) {
        return;
    }

    bool keep_cursor_visible = true;

    // remove or edit a tag
    for (size_t i = 0; i < _tags.size(); ++i) {
        if (!_tags[i].rectangle.translated(-offset()).contains(event->pos())) {
            continue;
        }

        if (inCrossArea(i, event->pos(), offset())) {
            removeTag(i);
            keep_cursor_visible = false;
        }
        else if (_editingIndex == i) {
            moveCursor(
                _textLayout.lineAt(0).xToCursor(
                    (event->pos() - (editorRect() - _pillThickness).translated(-offset()).topLeft()).x()
                ),
                false
            );
        }
        else {
            editTag(i);
        }

        updateTagDisplay(keep_cursor_visible);
        return;
    }

    // add new tag closest to the cursor
    for (auto it = begin(_tags); it != end(_tags); ++it) {
        // find the row
        if (it->rectangle.translated(-offset()).bottom() < event->pos().y()) {
            continue;
        }

        // find the closest spot
        auto const row = it->rectangle.translated(-offset()).top();
        while (it != end(_tags) && it->rectangle.translated(-offset()).top() == row
               && event->pos().x() > it->rectangle.translated(-offset()).left()) {
            ++it;
        }

        editNewTag(static_cast<size_t>(std::distance(begin(_tags), it)));
        updateTagDisplay(keep_cursor_visible);
        return;
    }

    // append a new nag
    editNewTag(_tags.size());
    updateTagDisplay(keep_cursor_visible);
}

void TagWidget::keyPressEvent(QKeyEvent* event)
{
    if (_readOnly) {
        return;
    }

    if (event == QKeySequence::SelectAll) {
        selectAll();
    }
    else if (event == QKeySequence::SelectPreviousChar) {
        moveCursor(_textLayout.previousCursorPosition(_cursor), true);
    }
    else if (event == QKeySequence::SelectNextChar) {
        moveCursor(_textLayout.nextCursorPosition(_cursor), true);
    }
    else {
        switch (event->key()) {
            case Qt::Key_Left:
                if (_cursor == 0) {
                    editPreviousTag();
                }
                else {
                    moveCursor(_textLayout.previousCursorPosition(_cursor), false);
                }
                break;
            case Qt::Key_Right:
                if (_cursor == editorText().size()) {
                    editNextTag();
                }
                else {
                    moveCursor(_textLayout.nextCursorPosition(_cursor), false);
                }
                break;
            case Qt::Key_Home:
                if (_cursor == 0) {
                    editTag(0);
                }
                else {
                    moveCursor(0, false);
                }
                break;
            case Qt::Key_End:
                if (_cursor == editorText().size()) {
                    editTag(_tags.size() - 1);
                }
                else {
                    moveCursor(editorText().length(), false);
                }
                break;
            case Qt::Key_Backspace:
                if (!editorText().isEmpty()) {
                    removeBackwardOne();
                }
                else if (_editingIndex > 0) {
                    editPreviousTag();
                }
                break;
            case Qt::Key_Comma:
            case Qt::Key_Return:
                if (!editorText().isEmpty()) {
                    editNewTag(_editingIndex + 1);
                }
                break;
            default:
                if (isAcceptableInput(*event)) {
                    if (hasSelection()) {
                        removeSelection();
                    }
                    editorText().insert(_cursor, event->text());
                    _cursor = _cursor + event->text().length();
                    break;
                }
                else {
                    event->setAccepted(false);
                    return;
                }
        }
    }

    updateTagDisplay();

    _completer->setCompletionPrefix(editorText());
    _completer->complete();

    Q_EMIT tagsEdited();
}

void TagWidget::mouseMoveEvent(QMouseEvent* event)
{
    for (size_t i = 0; i < _tags.size(); ++i) {
        if (inCrossArea(i, event->pos(), offset())) {
            viewport()->setCursor(Qt::ArrowCursor);
            return;
        }
    }
    if (contentsRect().contains(event->pos())) {
        viewport()->setCursor(Qt::IBeamCursor);
    }
    else {
        QAbstractScrollArea::mouseMoveEvent(event);
    }
}

/// Calculate the width that a tag would have with the given text width
int TagWidget::pillWidth(int textWidth, bool hasCross) const
{
    return textWidth + _pillThickness.left() + (hasCross ? (_tagCrossSpacing + _tagCrossSize) : 0)
        + _pillThickness.right();
}

/// Calculate the height that a tag would have with the given text height
int TagWidget::pillHeight(int textHeight) const
{
    return textHeight + _pillThickness.top() + _pillThickness.bottom();
}

void TagWidget::calculateRectangles(QRect rectangle, QPoint& leftTop, QFontMetrics const& metrics)
{
    auto const middle = _tags.begin() + static_cast<ptrdiff_t>(_editingIndex);

    calculateRectangles(leftTop, std::ranges::subrange(_tags.begin(), middle), metrics, rectangle);

    if (cursorVisible() || !editorText().isEmpty()) {
        calculateRectangles(leftTop, std::ranges::subrange(middle, middle + 1), metrics, rectangle);
    }

    calculateRectangles(leftTop, std::ranges::subrange(middle + 1, _tags.end()), metrics, rectangle);
}

QRect TagWidget::calculateRectangles(QRect rectangle)
{
    auto leftTop = rectangle.topLeft();
    QFontMetrics metrics = fontMetrics();
    calculateRectangles(rectangle, leftTop, metrics);
    rectangle.setBottom(leftTop.y() + pillHeight(metrics.height()) - 1);
    return rectangle;
}

QRect TagWidget::calculateRectangles()
{
    return calculateRectangles(contentsRect());
}

void TagWidget::calcRectsUpdateScrollRanges()
{
    calculateRectangles();
    updateVScrollRange();
    updateHScrollRange();
}

void TagWidget::updateVScrollRange()
{
    if (_tags.size() == 1 && _tags.front().text.isEmpty()) {
        verticalScrollBar()->setRange(0, 0);
        return;
    }

    auto const metrics = fontMetrics();
    auto const row_h = pillHeight(metrics.height()) + _pillVerticalSpacing;
    verticalScrollBar()->setPageStep(row_h);
    assert(!_tags.empty());  // Invariant-1

    int top = _tags.front().rectangle.top();
    int bottom = _tags.back().rectangle.bottom();

    if (_editingIndex == 0 && !(cursorVisible() || !editorText().isEmpty())) {
        top = _tags[1].rectangle.top();
    }
    else if (_editingIndex == _tags.size() - 1 && !(cursorVisible() || !editorText().isEmpty())) {
        bottom = _tags[_tags.size() - 2].rectangle.bottom();
    }

    auto const h = bottom - top + 1;
    auto const contents_rect = contentsRect();

    if (contents_rect.height() < h) {
        verticalScrollBar()->setRange(0, h - contents_rect.height());
    }
    else {
        verticalScrollBar()->setRange(0, 0);
    }
}

void TagWidget::updateHScrollRange()
{
    assert(!_tags.empty());  // Invariant-1
    auto const width = std::max_element(begin(_tags), end(_tags), [](auto const& x, auto const& y) {
                           return x.rectangle.width() < y.rectangle.width();
                       })->rectangle.width();

    auto const contents_rect_width = contentsRect().width();

    if (contents_rect_width < width) {
        horizontalScrollBar()->setRange(0, width - contents_rect_width);
    }
    else {
        horizontalScrollBar()->setRange(0, 0);
    }
}

void TagWidget::updateDisplayText()
{
    _textLayout.clearLayout();
    _textLayout.setText(editorText());
    _textLayout.beginLayout();
    _textLayout.createLine();
    _textLayout.endLayout();
}

void TagWidget::setCursorVisible(bool visible)
{
    if (_blinkTimer) {
        killTimer(_blinkTimer);
        _blinkTimer = 0;
    }

    if (visible) {
        _blinkStatus = true;
        int flashTime = QGuiApplication::styleHints()->cursorFlashTime();
        if (flashTime >= 2) {
            _blinkTimer = startTimer(flashTime / 2);
        }
    }
    else {
        _blinkStatus = false;
    }
}

void TagWidget::ensureCursorIsVisibleV()
{
    if (!cursorVisible()) {
        return;
    }
    auto const metrics = fontMetrics();
    auto const row_h = pillHeight(metrics.height());
    auto const vscroll = verticalScrollBar()->value();
    auto const cursor_top = editorRect().topLeft() + QPoint(qRound(cursorToX()), 0);
    auto const cursor_bottom = cursor_top + QPoint(0, row_h - 1);
    auto const contents_rect = contentsRect().translated(0, vscroll);
    if (contents_rect.bottom() < cursor_bottom.y()) {
        verticalScrollBar()->setValue(cursor_bottom.y() - row_h);
    }
    else if (cursor_top.y() < contents_rect.top()) {
        verticalScrollBar()->setValue(cursor_top.y() - 1);
    }
}

void TagWidget::ensureCursorIsVisibleH()
{
    if (!cursorVisible()) {
        return;
    }
    auto const contents_rect = contentsRect().translated(horizontalScrollBar()->value(), 0);
    auto const cursor_x = (editorRect() - _pillThickness).left() + qRound(cursorToX());
    if (contents_rect.right() < cursor_x) {
        horizontalScrollBar()->setValue(cursor_x - contents_rect.width());
    }
    else if (cursor_x < contents_rect.left()) {
        horizontalScrollBar()->setValue(cursor_x - 1);
    }
}

void TagWidget::updateTagDisplay(bool keep_cursor_visible)
{
    updateDisplayText();
    calcRectsUpdateScrollRanges();
    if (keep_cursor_visible) {
        ensureCursorIsVisibleV();
        ensureCursorIsVisibleH();
    }
    updateCursorBlinking();
    viewport()->update();
}

QSize TagWidget::sizeHint() const
{
    return minimumSizeHint();
}

QSize TagWidget::minimumSizeHint() const
{
    ensurePolished();
    QFontMetrics metrics = fontMetrics();
    QRect rectangle(0, 0, pillWidth(metrics.maxWidth(), true), pillHeight(metrics.height()));
    rectangle += contentsMargins() + viewport()->contentsMargins() + viewportMargins();
    return rectangle.size();
}

int TagWidget::heightForWidth(int width) const
{
    auto const content_width = width;
    QRect contents_rect(0, 0, content_width, 100);
    contents_rect -= contentsMargins() + viewport()->contentsMargins() + viewportMargins();
    contents_rect = const_cast<TagWidget*>(this)->calculateRectangles(contents_rect);
    contents_rect += contentsMargins() + viewport()->contentsMargins() + viewportMargins();
    return contents_rect.height();
}

void TagWidget::setTags(std::vector<QString> const& tags)
{
    _setTags(tags);
    updateTagDisplay();
}

std::vector<QString> TagWidget::getTags() const
{
    std::vector<QString> ret(_tags.size());
    std::transform(_tags.begin(), _tags.end(), ret.begin(), [](Tag const& _tags) {
        return _tags.text;
    });
    assert(!ret.empty());  // Invariant-1
    if (ret[_editingIndex].isEmpty() || (_uniqueTagsOnly && isCurrentTagADuplicate())) {
        ret.erase(ret.begin() + static_cast<std::ptrdiff_t>(_editingIndex));
    }
    return ret;
}

void TagWidget::clear()
{
    // Set tags to an empty list
    std::vector<QString> tags;
    _setTags(tags);
}

void TagWidget::setCompletions(std::vector<QString> const& completions)
{
    _completer = std::make_unique<QCompleter>([&] {
        QStringList ret;
        std::copy(completions.begin(), completions.end(), std::back_inserter(ret));
        return ret;
    }());
    setupCompleter();
}

void TagWidget::setReadOnly(bool readOnly)
{
    _readOnly = readOnly;
    updateTagDisplay();
}

void TagWidget::setUnique(bool unique)
{
    _uniqueTagsOnly = unique;
    updateTagDisplay();
}

void TagWidget::setRestoreCursorPositionOnFocusClick(bool restore)
{
    _restoreCursorPositionOnFocusClick = restore;
}

void TagWidget::setPillThickness(const QMargins& thickness)
{
    _pillThickness = thickness;
}

void TagWidget::setPillHorizontalSpacing(int spacing)
{
    _pillHorizontalSpacing = spacing;
}

void TagWidget::setPillVerticalSpacing(int spacing)
{
    _pillVerticalSpacing = spacing;
}

void TagWidget::setTagCrossSize(qreal size)
{
    _tagCrossSize = size;
}

void TagWidget::setTagCrossSpacing(int spacing)
{
    _tagCrossSpacing = spacing;
}

void TagWidget::setRoundingXRadius(qreal radius)
{
    _roundingXRadius = radius;
}

void TagWidget::setRoundingYRadius(qreal radius)
{
    _roundingYRadius = radius;
}

void TagWidget::setTagColor(const QColor& color)
{
    _tagColor = color;
}

void TagWidget::_setTags(std::vector<QString> const& tags)
{
    std::unordered_set<QString> unique_tags;
    std::vector<Tag> t;
    for (auto const& x : tags) {
        if (/* Invariant-1 */ !x.isEmpty()
            && /* Invariant-2 */ (!_uniqueTagsOnly || unique_tags.insert(x).second)) {
            t.emplace_back(x, QRect {});
        }
    }
    _tags = std::move(t);
    _tags.push_back(Tag {});
    _editingIndex = _tags.size() - 1;
    moveCursor(0, false);
}

bool TagWidget::isCurrentTagADuplicate() const
{
    assert(_editingIndex < _tags.size());
    auto const mid = _tags.begin() + static_cast<std::ptrdiff_t>(_editingIndex);
    auto const text_eq = [this](const Tag& x) {
        return x.text == editorText();
    };
    return std::find_if(_tags.begin(), mid, text_eq) != mid
        || std::find_if(mid + 1, _tags.end(), text_eq) != _tags.end();
}

qreal TagWidget::cursorToX()
{
    return _textLayout.lineAt(0).cursorToX(_cursor);
}

void TagWidget::moveCursor(int pos, bool mark)
{
    if (mark) {
        auto e = _selectStart + _selectSize;
        int anchor = _selectSize > 0 && _cursor == _selectStart ? e
            : _selectSize > 0 && _cursor == e                   ? _selectStart
                                                                : _cursor;
        _selectStart = qMin(anchor, pos);
        _selectSize = qMax(anchor, pos) - _selectStart;
    }
    else {
        deselectAll();
    }
    _cursor = pos;
}

void TagWidget::deselectAll()
{
    _selectStart = 0;
    _selectSize = 0;
}

bool TagWidget::hasSelection() const noexcept
{
    return _selectSize > 0;
}

void TagWidget::selectAll()
{
    _selectStart = 0;
    _selectSize = editorText().size();
}

void TagWidget::removeSelection()
{
    assert(_selectStart + _selectSize <= editorText().size());
    _cursor = _selectStart;
    editorText().remove(_cursor, _selectSize);
    deselectAll();
}

void TagWidget::drawEditor(QPainter& painter, QPalette const& palette, QPoint const& offset) const
{
    auto const& rectangle = editorRect();
    auto const& txt_p = rectangle.topLeft() + QPointF(_pillThickness.left(), _pillThickness.top());
    auto const f = formatting(palette);
    _textLayout.draw(&painter, txt_p - offset, f);
    if (_blinkStatus) {
        _textLayout.drawCursor(&painter, txt_p - offset, _cursor);
    }
}

QVector<QTextLayout::FormatRange> TagWidget::formatting(QPalette const& palette) const
{
    if (_selectSize == 0) {
        return {};
    }

    QTextLayout::FormatRange selection;
    selection.start = _selectStart;
    selection.length = _selectSize;
    selection.format.setBackground(palette.brush(QPalette::Highlight));
    selection.format.setForeground(palette.brush(QPalette::HighlightedText));
    return {selection};
}

/// Makes the tag at `i` currently editing, and ensures Invariant-1 and Invariant-2`.
void TagWidget::setEditorIndex(size_t i)
{
    assert(i < _tags.size());
    if (editorText().isEmpty() || (_uniqueTagsOnly && isCurrentTagADuplicate())) {
        _tags.erase(std::next(begin(_tags), static_cast<std::ptrdiff_t>(_editingIndex)));
        if (_editingIndex <= i) {  // Did we shift `i`?
            --i;
        }
    }
    _editingIndex = i;
}

// Inserts a new tag at `i`, makes the tag currently editing, and ensures Invariant-1.
void TagWidget::editNewTag(size_t i)
{
    assert(i <= _tags.size());
    _tags.insert(begin(_tags) + static_cast<std::ptrdiff_t>(i), Tag {});
    if (i <= _editingIndex) {  // Did we shift `editing_index`?
        ++_editingIndex;
    }
    setEditorIndex(i);
    moveCursor(0, false);
}

void TagWidget::editPreviousTag()
{
    if (_editingIndex > 0) {
        setEditorIndex(_editingIndex - 1);
        moveCursor(editorText().size(), false);
    }
}

void TagWidget::editNextTag()
{
    if (_editingIndex < _tags.size() - 1) {
        setEditorIndex(_editingIndex + 1);
        moveCursor(0, false);
    }
}

void TagWidget::editTag(size_t i)
{
    assert(i < _tags.size());
    setEditorIndex(i);
    moveCursor(editorText().size(), false);
}

void TagWidget::removeTag(size_t i)
{
    _tags.erase(_tags.begin() + static_cast<ptrdiff_t>(i));
    if (i <= _editingIndex) {
        --_editingIndex;
    }
}

void TagWidget::removeBackwardOne()
{
    if (hasSelection()) {
        removeSelection();
    }
    else {
        editorText().remove(--_cursor, 1);
    }
}

void TagWidget::removeDuplicates()
{
    removeDuplicates(_tags);
    auto const it = std::find_if(_tags.begin(), _tags.end(), [](auto const& x) {
        return x.text.isEmpty();  // Thanks to Invariant-1 we can track back the editing_index.
    });
    assert(it != _tags.end());
    _editingIndex = static_cast<size_t>(std::distance(_tags.begin(), it));
}

void TagWidget::removeDuplicates(std::vector<Tag>& tags)
{
    std::unordered_map<QString, size_t> unique;
    for (auto const i : std::views::iota(size_t {0}, tags.size())) {
        unique.emplace(tags[i].text, i);
    }

    for (auto b = tags.rbegin(), it = b, e = tags.rend(); it != e;) {
        if (auto const i = static_cast<size_t>(std::distance(it, e) - 1); unique.at(it->text) != i) {
            tags.erase(it++.base() - 1);
        }
        else {
            ++it;
        }
    }
}

bool TagWidget::isAcceptableInput(QKeyEvent const& event)
{
    auto const text = event.text();
    if (text.isEmpty()) {
        return false;
    }

    auto const c = text.at(0);

    if (c.category() == QChar::Other_Format) {
        return true;
    }

    if (event.modifiers() == Qt::ControlModifier
        || event.modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
        return false;
    }

    if (c.isPrint()) {
        return true;
    }

    if (c.category() == QChar::Other_PrivateUse) {
        return true;
    }

    return false;
}

void TagWidget::setEditorText(QString const& text)
{
    editorText() = text;
    moveCursor(editorText().length(), false);
    updateTagDisplay();
}

QRectF TagWidget::crossRectangle(QRectF const& rectangle, qreal crossSize) const
{
    QRectF cross(QPointF {0, 0}, QSizeF {crossSize, crossSize});
    cross.moveCenter(QPointF(rectangle.right() - crossSize, rectangle.center().y()));
    return cross;
}

QRectF TagWidget::crossRectangle(QRectF const& rectangle) const
{
    return crossRectangle(rectangle, _tagCrossSize);
}

#include "moc_TagWidget.cpp"
