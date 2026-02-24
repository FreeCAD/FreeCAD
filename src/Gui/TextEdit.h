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

/* Text completion mechanism */

#pragma once

#include <QListWidget>
#include <QPlainTextEdit>

#include "CallTips.h"
#include "Window.h"


namespace Gui
{
class CompletionBox;
class SyntaxHighlighter;

/**
 * Completion is a means by which an editor automatically completes words that the user is typing.
 * For example, in a code editor, a programmer might type "sur", then Tab, and the editor will
 * complete the word the programmer was typing so that "sur" is replaced by "surnameLineEdit". This
 * is very useful for text that contains long words or variable names. The completion mechanism
 * usually works by looking at the existing text to see if any words begin with what the user has
 * typed, and in most editors completion is invoked by a special key sequence.
 *
 * TextEdit can detect a special key sequence to invoke the completion mechanism, and can handle
 * three different situations:
 * \li There are no possible completions.
 * \li There is a single possible completion.
 * \li There are two or more possible completions.
 *
 * \remark The original sources are taken from Qt Quarterly (Customizing for Completion).
 * @author Werner Mayer
 */
class CompletionList;
class GuiExport TextEdit: public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TextEdit(QWidget* parent = nullptr);
    ~TextEdit() override;

    //! Get the cursor position of the current line being edited
    virtual int getInputStringPosition();
    //! Get the text of the current line being edited
    virtual QString getInputString();

private Q_SLOTS:
    void complete();

Q_SIGNALS:
    void showSearchBar();
    void findNext();
    void findPrevious();

protected:
    void keyPressEvent(QKeyEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    CallTipsList* callTipsList = nullptr;

private:
    void createListBox();

private:
    QString wordPrefix;
    int cursorPosition;
    CompletionList* listBox;
};

class SyntaxHighlighter;
class GuiExport TextEditor: public TextEdit, public WindowParameter
{
    Q_OBJECT

public:
    explicit TextEditor(QWidget* parent = nullptr);
    ~TextEditor() override;
    void setSyntaxHighlighter(SyntaxHighlighter*);

    void OnChange(Base::Subject<const char*>& rCaller, const char* rcReason) override;

    /** Draw a beam in the line where the cursor is. */
    void lineNumberAreaPaintEvent(QPaintEvent*);
    int lineNumberAreaWidth();
    void setVisibleLineNumbers(bool value);
    bool isVisibleLineNumbers() const;
    void setEnabledHighlightCurrentLine(bool value);
    bool isEnabledHighlightCurrentLine() const;

private Q_SLOTS:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect&, int);
    void highlightCurrentLine();

protected:
    void resizeEvent(QResizeEvent* e) override;
    QWidget* getMarker() const
    {
        return lineNumberArea;
    }
    virtual void drawMarker(int line, int x, int y, QPainter*);

private:
    SyntaxHighlighter* highlighter;
    QWidget* lineNumberArea;
    struct TextEditorP* d;

    friend class SyntaxHighlighter;
};

/** subclass of TextEditor that serves as base class for the
 *  python editor and the python console where we handle
 *  the tab key conversion to spaces, depending on user settings
 */
class GuiExport PythonTextEditor: public TextEditor
{
    Q_OBJECT
public:
    explicit PythonTextEditor(QWidget* parent = nullptr);
    ~PythonTextEditor() override;

public Q_SLOTS:
    /** Inserts \a str at the beginning of each selected line or the current line if
     * nothing is selected
     */
    void prepend(const QString& str);
    /** Removes \a str from the beginning of each selected line or the current line if
     * nothing is selected
     */
    void remove(const QString& str);

protected:
    void keyPressEvent(QKeyEvent*) override;
};

/**
 * @brief Line number widget (left margin gutter).
 *
 * Handles line number and mouse-based line range selections.
 */
class LineMarker: public QWidget
{
    Q_OBJECT

public:
    explicit LineMarker(TextEditor* editor);
    ~LineMarker() override = default;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    TextEditor* const textEditor;
    int anchorLine = -1;
    bool dragging = false;

    QTextBlock blockAtPosition(int y) const;
    void selectBlocks(int startLine, int endLine);
};

/**
 * The CompletionList class provides a list box that pops up in a text edit if the user has pressed
 * an accelerator to complete the current word they are typing in.
 * @author Werner Mayer
 */
class CompletionList: public QListWidget
{
    Q_OBJECT

public:
    /// Construction
    explicit CompletionList(QPlainTextEdit* parent);
    /// Destruction
    ~CompletionList() override;

    void findCurrentWord(const QString&);

protected:
    bool eventFilter(QObject*, QEvent*) override;

private Q_SLOTS:
    void completionItem(QListWidgetItem* item);

private:
    QPlainTextEdit* textEdit;
};

}  // namespace Gui
