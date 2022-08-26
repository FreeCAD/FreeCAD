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

#ifndef GUI_TEXTEDIT_H
#define GUI_TEXTEDIT_H

#include <QListWidget>
#include <QPlainTextEdit>
#include "Window.h"


namespace Gui {
class CompletionBox;
class SyntaxHighlighter;

/**
 * Completion is a means by which an editor automatically completes words that the user is typing.
 * For example, in a code editor, a programmer might type "sur", then Tab, and the editor will complete
 * the word the programmer was typing so that "sur" is replaced by "surnameLineEdit". This is very
 * useful for text that contains long words or variable names. The completion mechanism usually works
 * by looking at the existing text to see if any words begin with what the user has typed, and in most
 * editors completion is invoked by a special key sequence.
 *
 * TextEdit can detect a special key sequence to invoke the completion mechanism, and can handle three
 * different situations:
 * \li There are no possible completions.
 * \li There is a single possible completion.
 * \li There are two or more possible completions.
 *
 * \remark The original sources are taken from Qt Quarterly (Customizing for Completion).
 * @author Werner Mayer
 */
class CompletionList;
class GuiExport TextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TextEdit(QWidget *parent = nullptr);
    ~TextEdit() override;

private Q_SLOTS:
    void complete();

Q_SIGNALS:
    void showSearchBar();
    void findNext();
    void findPrevious();

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    void createListBox();

private:
    QString wordPrefix;
    int cursorPosition;
    CompletionList *listBox;
};

class SyntaxHighlighter;
class GuiExport TextEditor : public TextEdit, public WindowParameter
{
    Q_OBJECT

public:
    explicit TextEditor(QWidget *parent = nullptr);
    ~TextEditor() override;
    void setSyntaxHighlighter(SyntaxHighlighter*);

    void OnChange(Base::Subject<const char*> &rCaller,const char* rcReason) override;

    void lineNumberAreaPaintEvent(QPaintEvent* );
    int lineNumberAreaWidth();

private Q_SLOTS:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &, int);
    void highlightCurrentLine();

protected:
    void keyPressEvent (QKeyEvent * e) override;
    /** Draw a beam in the line where the cursor is. */
    void paintEvent (QPaintEvent * e) override;
    void resizeEvent(QResizeEvent* e) override;
    QWidget* getMarker() const
    { return lineNumberArea; }
    virtual void drawMarker(int line, int x, int y, QPainter*);

private:
    SyntaxHighlighter* highlighter;
    QWidget* lineNumberArea;
    struct TextEditorP* d;

    friend class SyntaxHighlighter;
};

class LineMarker : public QWidget
{
    Q_OBJECT

public:
    explicit LineMarker(TextEditor* editor);
    ~LineMarker() override;

    QSize sizeHint() const override;

protected:
    void paintEvent (QPaintEvent *) override;

private:
    TextEditor *textEditor;
};

/**
 * The CompletionList class provides a list box that pops up in a text edit if the user has pressed
 * an accelerator to complete the current word they are typing in.
 * @author Werner Mayer
 */
class CompletionList : public QListWidget
{
    Q_OBJECT

public:
    /// Construction
    explicit CompletionList(QPlainTextEdit* parent);
    /// Destruction
    ~CompletionList() override;

    void findCurrentWord(const QString&);

protected:
    bool eventFilter(QObject *, QEvent *) override;

private Q_SLOTS:
    void completionItem(QListWidgetItem *item);

private:
    QPlainTextEdit* textEdit;
};

} // namespace Gui

#endif // GUI_TEXTEDIT_H
