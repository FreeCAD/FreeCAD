/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QKeyEvent>
# include <QPainter>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QShortcut>
# include <QTextCursor>
#endif

#include "TextEdit.h"
#include "SyntaxHighlighter.h"
#include "Tools.h"
#include <App/Color.h>


using namespace Gui;

/**
 *  Constructs a TextEdit which is a child of 'parent'.
 */
TextEdit::TextEdit(QWidget* parent)
    : QPlainTextEdit(parent), cursorPosition(0), listBox(nullptr)
{
    //Note: Set the correct context to this shortcut as we may use several instances of this
    //class at a time
    auto shortcut = new QShortcut(this);
    shortcut->setKey(QKeySequence(QString::fromLatin1("CTRL+Space")));
    shortcut->setContext(Qt::WidgetShortcut);
    connect(shortcut, &QShortcut::activated, this, &TextEdit::complete);

    auto shortcutFind = new QShortcut(this);
    shortcutFind->setKey(QKeySequence::Find);
    shortcutFind->setContext(Qt::WidgetShortcut);
    connect(shortcutFind, &QShortcut::activated, this, &TextEdit::showSearchBar);

    auto shortcutNext = new QShortcut(this);
    shortcutNext->setKey(QKeySequence::FindNext);
    shortcutNext->setContext(Qt::WidgetShortcut);
    connect(shortcutNext, &QShortcut::activated, this, &TextEdit::findNext);

    auto shortcutPrev = new QShortcut(this);
    shortcutPrev->setKey(QKeySequence::FindPrevious);
    shortcutPrev->setContext(Qt::WidgetShortcut);
    connect(shortcutPrev, &QShortcut::activated, this, &TextEdit::findPrevious);
}

/** Destroys the object and frees any allocated resources */
TextEdit::~TextEdit() = default;

/**
 * Set the approproriate item of the completion box or hide it, if needed.
 */
void TextEdit::keyPressEvent(QKeyEvent* e)
{
    QPlainTextEdit::keyPressEvent(e);
    // This can't be done in CompletionList::eventFilter() because we must first perform
    // the event and afterwards update the list widget
    if (listBox && listBox->isVisible()) {
        // Get the word under the cursor
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfWord);
        // the cursor has moved to outside the word prefix
        if (cursor.position() < cursorPosition-wordPrefix.length() ||
            cursor.position() > cursorPosition) {
            listBox->hide();
            return;
        }
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        listBox->keyboardSearch(cursor.selectedText());
        cursor.clearSelection();
    }
}

/**
 * Completes the word.
 */
void TextEdit::complete()
{
    QTextBlock block = textCursor().block();
    if (!block.isValid())
        return;
    int cursorPos = textCursor().position()-block.position();
    QString para = block.text();
    int wordStart = cursorPos;
    while (wordStart > 0 && para[wordStart - 1].isLetterOrNumber())
        --wordStart;
    wordPrefix = para.mid(wordStart, cursorPos - wordStart);
    if (wordPrefix.isEmpty())
        return;

    QStringList list = toPlainText().split(QRegularExpression(QLatin1String("\\W+")));
    QMap<QString, QString> map;
    QStringList::Iterator it = list.begin();
    while (it != list.end()) {
        if ((*it).startsWith(wordPrefix) && (*it).length() > wordPrefix.length())
            map[(*it).toLower()] = *it;
        ++it;
    }

    if (map.count() == 1) {
        insertPlainText((*map.begin()).mid(wordPrefix.length()));
    } else if (map.count() > 1) {
        if (!listBox)
            createListBox();
        listBox->clear();
        listBox->addItems(map.values());
        listBox->setFont(QFont(font().family(), 8));

        this->cursorPosition = textCursor().position();

        // get the minimum width and height of the box
        int h = 0;
        int w = 0;
        for (int i = 0; i < listBox->count(); ++i) {
            QRect r = listBox->visualItemRect(listBox->item(i));
            w = qMax(w, r.width());
            h += r.height();
        }

        // Add an offset
        w += 2*listBox->frameWidth();
        h += 2*listBox->frameWidth();

        // get the start position of the word prefix
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfWord);
        QRect rect = cursorRect(cursor);
        int posX = rect.x();
        int posY = rect.y();
        int boxH = h;

        // Decide whether to show downstairs or upstairs
        if (posY > viewport()->height()/2) {
            h = qMin(qMin(h,posY), 250);
            if (h < boxH)
                w += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
            listBox->setGeometry(posX,posY-h, w, h);
        } else {
            h = qMin(qMin(h,viewport()->height()-fontMetrics().height()-posY), 250);
            if (h < boxH)
                w += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
            listBox->setGeometry(posX, posY+fontMetrics().height(), w, h);
        }

        listBox->setCurrentRow(0);
        listBox->show();
    }
}

/**
 * Creates the listbox containing all possibilities for the completion.
 * The listbox is closed when ESC is pressed, the text edit field loses focus or a
 * mouse button was pressed.
 */
void TextEdit::createListBox()
{
    listBox = new CompletionList(this);
    listBox->setFrameStyle(QFrame::Box);
    listBox->setFrameShadow(QFrame::Raised);
    listBox->setLineWidth(2);
    installEventFilter(listBox);
    viewport()->installEventFilter(listBox);
    listBox->setSelectionMode( QAbstractItemView::SingleSelection );
    listBox->hide();
}

// ------------------------------------------------------------------------------

namespace Gui {
struct TextEditorP
{
    QMap<QString, QColor> colormap; // Color map
    TextEditorP()
    {
        colormap[QLatin1String("Text")] = qApp->palette().windowText().color();
        colormap[QLatin1String("Bookmark")] = Qt::cyan;
        colormap[QLatin1String("Breakpoint")] = Qt::red;
        colormap[QLatin1String("Keyword")] = Qt::blue;
        colormap[QLatin1String("Comment")] = QColor(0, 170, 0);
        colormap[QLatin1String("Block comment")] = QColor(160, 160, 164);
        colormap[QLatin1String("Number")] = Qt::blue;
        colormap[QLatin1String("String")] = Qt::red;
        colormap[QLatin1String("Character")] = Qt::red;
        colormap[QLatin1String("Class name")] = QColor(255, 170, 0);
        colormap[QLatin1String("Define name")] = QColor(255, 170, 0);
        colormap[QLatin1String("Operator")] = QColor(160, 160, 164);
        colormap[QLatin1String("Python output")] = QColor(170, 170, 127);
        colormap[QLatin1String("Python error")] = Qt::red;
        colormap[QLatin1String("Current line highlight")] = QColor(224,224,224);
    }
};
} // namespace Gui

/**
 *  Constructs a TextEditor which is a child of 'parent' and does the
 *  syntax highlighting for the Python language.
 */
TextEditor::TextEditor(QWidget* parent)
  : TextEdit(parent), WindowParameter("Editor"), highlighter(nullptr)
{
    d = new TextEditorP();
    lineNumberArea = new LineMarker(this);

    QFont serifFont(QLatin1String("Courier"), 10, QFont::Normal);
    setFont(serifFont);

    ParameterGrp::handle hPrefGrp = getWindowParameter();
    hPrefGrp->Attach( this );

    // set colors and font
    hPrefGrp->NotifyAll();

    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &TextEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &TextEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &TextEditor::updateLineNumberArea);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

/** Destroys the object and frees any allocated resources */
TextEditor::~TextEditor()
{
    getWindowParameter()->Detach(this);
    delete highlighter;
    delete d;
}

int TextEditor::lineNumberAreaWidth()
{
    return QtTools::horizontalAdvance(fontMetrics(), QLatin1String("0000")) + 10;
}

void TextEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void TextEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void TextEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void TextEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = d->colormap[QLatin1String("Current line highlight")];
        unsigned int col = App::Color::asPackedRGB<QColor>(lineColor);
        ParameterGrp::handle hPrefGrp = getWindowParameter();
        auto value = static_cast<unsigned long>(col);
        value = hPrefGrp->GetUnsigned( "Current line highlight", value);
        col = static_cast<unsigned int>(value);
        lineColor.setRgb((col>>24)&0xff, (col>>16)&0xff, (col>>8)&0xff);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void TextEditor::drawMarker(int line, int x, int y, QPainter* p)
{
    Q_UNUSED(line);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(p);
}

void TextEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    //painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            QPalette pal = palette();
            QColor color = pal.windowText().color();
            painter.setPen(color);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
            drawMarker(blockNumber + 1, 1, top, &painter);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void TextEditor::setSyntaxHighlighter(SyntaxHighlighter* sh)
{
    sh->setDocument(this->document());
    this->highlighter = sh;
}

void TextEditor::keyPressEvent (QKeyEvent * e)
{
    if ( e->key() == Qt::Key_Tab ) {
        ParameterGrp::handle hPrefGrp = getWindowParameter();
        int indent = hPrefGrp->GetInt( "IndentSize", 4 );
        bool space = hPrefGrp->GetBool( "Spaces", false );
        QString ch = space ? QString(indent, QLatin1Char(' '))
                           : QString::fromLatin1("\t");

        QTextCursor cursor = textCursor();
        if (!cursor.hasSelection()) {
            // insert a single tab or several spaces
            cursor.beginEditBlock();
            cursor.insertText(ch);
            cursor.endEditBlock();
        } else {
            // for each selected block insert a tab or spaces
            int selStart = cursor.selectionStart();
            int selEnd = cursor.selectionEnd();
            QTextBlock block;
            cursor.beginEditBlock();
            for (block = document()->begin(); block.isValid(); block = block.next()) {
                int pos = block.position();
                int off = block.length()-1;
                // at least one char of the block is part of the selection
                if ( pos >= selStart || pos+off >= selStart) {
                    if ( pos+1 > selEnd )
                        break; // end of selection reached
                    cursor.setPosition(block.position());
                    cursor.insertText(ch);
                        selEnd += ch.length();
                }
            }

            cursor.endEditBlock();
        }

        return;
    }
    else if (e->key() == Qt::Key_Backtab) {
        QTextCursor cursor = textCursor();
        if (!cursor.hasSelection())
            return; // Shift+Tab should not do anything
        // If some text is selected we remove a leading tab or
        // spaces from each selected block
        ParameterGrp::handle hPrefGrp = getWindowParameter();
        int indent = hPrefGrp->GetInt( "IndentSize", 4 );

        int selStart = cursor.selectionStart();
        int selEnd = cursor.selectionEnd();
        QTextBlock block;
        cursor.beginEditBlock();
        for (block = document()->begin(); block.isValid(); block = block.next()) {
            int pos = block.position();
            int off = block.length()-1;
            // at least one char of the block is part of the selection
            if ( pos >= selStart || pos+off >= selStart) {
                if ( pos+1 > selEnd )
                    break; // end of selection reached
                // if possible remove one tab or several spaces
                QString text = block.text();
                if (text.startsWith(QLatin1String("\t"))) {
                    cursor.setPosition(block.position());
                    cursor.deleteChar();
                    selEnd--;
                }
                else {
                    cursor.setPosition(block.position());
                    for (int i=0; i<indent; i++) {
                        if (!text.startsWith(QLatin1String(" ")))
                            break;
                        text = text.mid(1);
                        cursor.deleteChar();
                        selEnd--;
                    }
                }
            }
        }

        cursor.endEditBlock();
        return;
    }

    TextEdit::keyPressEvent( e );
}

/** Sets the font, font size and tab size of the editor. */
void TextEditor::OnChange(Base::Subject<const char*> &rCaller,const char* sReason)
{
    Q_UNUSED(rCaller);
    ParameterGrp::handle hPrefGrp = getWindowParameter();
    if (strcmp(sReason, "FontSize") == 0 || strcmp(sReason, "Font") == 0) {
#ifdef FC_OS_LINUX
        int fontSize = hPrefGrp->GetInt("FontSize", 15);
#else
        int fontSize = hPrefGrp->GetInt("FontSize", 10);
#endif
        QString fontFamily = QString::fromLatin1(hPrefGrp->GetASCII( "Font", "Courier" ).c_str());

        QFont font(fontFamily, fontSize);
        setFont(font);
        lineNumberArea->setFont(font);
    }
    else {
        QMap<QString, QColor>::Iterator it = d->colormap.find(QString::fromLatin1(sReason));
        if (it != d->colormap.end()) {
            QColor color = it.value();
            unsigned int col = App::Color::asPackedRGB<QColor>(color);
            auto value = static_cast<unsigned long>(col);
            value = hPrefGrp->GetUnsigned(sReason, value);
            col = static_cast<unsigned int>(value);
            color.setRgb((col>>24)&0xff, (col>>16)&0xff, (col>>8)&0xff);
            if (this->highlighter)
                this->highlighter->setColor(QLatin1String(sReason), color);
        }
    }

    if (strcmp(sReason, "TabSize") == 0 || strcmp(sReason, "FontSize") == 0) {
        int tabWidth = hPrefGrp->GetInt("TabSize", 4);
        QFontMetrics metric(font());
        int fontSize = QtTools::horizontalAdvance(metric, QLatin1Char('0'));
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        setTabStopWidth(tabWidth * fontSize);
#else
        setTabStopDistance(tabWidth * fontSize);
#endif
    }

    // Enables/Disables Line number in the Macro Editor from Edit->Preferences->Editor menu.
    if (strcmp(sReason, "EnableLineNumber") == 0) {
        QRect cr = contentsRect();
        bool show = hPrefGrp->GetBool("EnableLineNumber", true);
        if(show)
            lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
        else
            lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), 0, cr.height()));
    }

    if (strcmp(sReason, "EnableBlockCursor") == 0 ||
        strcmp(sReason, "FontSize") == 0 ||
        strcmp(sReason, "Font") == 0) {
        bool block = hPrefGrp->GetBool("EnableBlockCursor", false);
        if (block)
            setCursorWidth(QFontMetrics(font()).averageCharWidth());
        else
            setCursorWidth(1);
    }
}

void TextEditor::paintEvent (QPaintEvent * e)
{
    TextEdit::paintEvent( e );
}

// ------------------------------------------------------------------------------

LineMarker::LineMarker(TextEditor* editor)
    : QWidget(editor), textEditor(editor)
{
}

LineMarker::~LineMarker() = default;

QSize LineMarker::sizeHint() const
{
    return {textEditor->lineNumberAreaWidth(), 0};
}

void LineMarker::paintEvent(QPaintEvent* e)
{
    textEditor->lineNumberAreaPaintEvent(e);
}

// ------------------------------------------------------------------------------

CompletionList::CompletionList(QPlainTextEdit* parent)
  :  QListWidget(parent), textEdit(parent)
{
    // make the user assume that the widget is active
    QPalette pal = parent->palette();
    pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
    pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
    parent->setPalette( pal );

    connect(this, &CompletionList::itemActivated,
            this, &CompletionList::completionItem);
}

CompletionList::~CompletionList() = default;

void CompletionList::findCurrentWord(const QString& wordPrefix)
{
    for (int i=0; i<count(); ++i) {
        QString text = item(i)->text();
        if (text.startsWith(wordPrefix)) {
            setCurrentRow(i);
            return;
        }
    }

    if (currentItem())
        currentItem()->setSelected(false);
}

/**
 * Get all incoming events of the text edit and redirect some of them, like key up and
 * down, mouse press events, ... to the widget itself.
 */
bool CompletionList::eventFilter(QObject * watched, QEvent * event)
{
    if (isVisible() && watched == textEdit->viewport()) {
        if (event->type() == QEvent::MouseButtonPress)
            hide();
    } else if (isVisible() && watched == textEdit) {
        if (event->type() == QEvent::KeyPress) {
            auto ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down) {
                keyPressEvent(ke);
                return true;
            } else if (ke->key() == Qt::Key_PageUp || ke->key() == Qt::Key_PageDown) {
                keyPressEvent(ke);
                return true;
            } else if (ke->key() == Qt::Key_Escape) {
                hide();
                return true;
            } else if (ke->key() == Qt::Key_Space) {
                hide();
                return false;
            } else if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                Q_EMIT itemActivated(currentItem());
                return true;
            }
        } else if (event->type() == QEvent::FocusOut) {
            if (!hasFocus())
                hide();
        }
    }

    return QListWidget::eventFilter(watched, event);
}

/**
 * If an item was chosen (either by clicking or pressing enter) the rest of the word is completed.
 * The listbox is closed without destroying it.
 */
void CompletionList::completionItem(QListWidgetItem *item)
{
    hide();
    QString text = item->text();
    QTextCursor cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::StartOfWord);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    cursor.insertText( text );
    textEdit->ensureCursorVisible();
}

#include "moc_TextEdit.cpp"
