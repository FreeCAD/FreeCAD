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

#pragma once

#include <Python.h>
#include <QTimer>
#include "PythonEditor.h"


class QPlainTextEdit;
class QPushButton;

namespace Gui
{

/**
 * This class implements an interactive Python interpreter.
 * @author Werner Mayer
 */
class GuiExport InteractiveInterpreter
{
public:
    InteractiveInterpreter();
    ~InteractiveInterpreter();

    bool isOccupied() const;
    bool interrupt() const;

    bool push(const char*);
    int compileCommand(const char*) const;
    bool hasPendingInput() const;
    void setBuffer(const QStringList&);
    QStringList getBuffer() const;
    void clearBuffer();

private:
    bool runSource(const char*) const;
    PyObject* compile(const char*) const;
    void runCode(PyCodeObject*) const;
    void setPrompt();

private:
    struct InteractiveInterpreterP* d;
};

/**
 * This class implements the history for the Python console.
 * @author Werner Mayer
 */
class GuiExport ConsoleHistory
{
public:
    ConsoleHistory();
    ~ConsoleHistory();

    void first();
    bool more();
    bool next();
    bool prev(const QString& prefix = QString());
    bool isEmpty() const;
    const QString& value() const;
    void append(const QString& inputLine);
    const QStringList& values() const;
    void restart();
    void markScratch();
    void doScratch();

private:
    QStringList _history;
    QStringList::ConstIterator _it;
    int _scratchBegin;
    QString _prefix;
};

/**
 * Python text console with syntax highlighting.
 * @author Werner Mayer
 */
class PythonConsoleHighlighter;
class GuiExport PythonConsole: public PythonTextEditor
{
    Q_OBJECT

public:
    enum Prompt
    {
        Complete = 0,
        Incomplete = 1,
        Flush = 2,
        Special = 3
    };

    explicit PythonConsole(QWidget* parent = nullptr);
    ~PythonConsole() override;

    void OnChange(Base::Subject<const char*>& rCaller, const char* rcReason) override;
    void printStatement(const QString& cmd);
    QString readline();
    int getInputStringPosition() override;
    QString getInputString() override;

public Q_SLOTS:
    void onSaveHistoryAs();
    void onInsertFileName();
    void onCopyHistory();
    void onCopyCommand();
    void onClearConsole();
    void onFlush();

private Q_SLOTS:
    void visibilityChanged(bool visible);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void changeEvent(QEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void overrideCursor(const QString& txt);

    /** Pops up the context menu with some extensions */
    void contextMenuEvent(QContextMenuEvent* e) override;
    bool canInsertFromMimeData(const QMimeData* source) const override;
    QMimeData* createMimeDataFromSelection() const override;
    void insertFromMimeData(const QMimeData* source) override;
    QTextCursor inputBegin() const;

private:
    void runSource(const QString&);
    bool isComment(const QString&) const;
    void printPrompt(Prompt);
    void insertPythonOutput(const QString&);
    void insertPythonError(const QString&);
    void runSourceFromMimeData(const QString&);
    void appendOutput(const QString&, int);
    void loadHistory() const;
    void saveHistory() const;
    void flushOutput();

Q_SIGNALS:
    void pendingSource();

private:
    struct PythonConsoleP* d;

    PythonConsoleHighlighter* pythonSyntax {nullptr};
    QString* _sourceDrain {nullptr};
    QString _historyFile;
    QTimer* flusher {nullptr};

    friend class PythonStdout;
    friend class PythonStderr;
};

/**
 * Syntax highlighter for Python console.
 * @author Werner Mayer
 */
class GuiExport PythonConsoleHighlighter: public PythonSyntaxHighlighter
{
public:
    explicit PythonConsoleHighlighter(QObject* parent);
    ~PythonConsoleHighlighter() override;

    void highlightBlock(const QString& text) override;

protected:
    void colorChanged(const QString& type, const QColor& col) override;
};

}  // namespace Gui
