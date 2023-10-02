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

#ifndef GUI_PYTHONEDITOR_H
#define GUI_PYTHONEDITOR_H

#include "SyntaxHighlighter.h"
#include "TextEdit.h"


namespace Gui {

class PythonSyntaxHighlighter;
class PythonSyntaxHighlighterP;

/**
 * Python text editor with syntax highlighting.
 * \author Werner Mayer
 */
class GuiExport PythonEditor : public TextEditor
{
    Q_OBJECT

public:
    explicit PythonEditor(QWidget *parent = nullptr);
    ~PythonEditor() override;

    void toggleBreakpoint();
    void showDebugMarker(int line);
    void hideDebugMarker();

public Q_SLOTS:
    /** Inserts a '#' at the beginning of each selected line or the current line if
     * nothing is selected
     */
    void onComment();
    /**
     * Removes the leading '#' from each selected line or the current line if
     * nothing is selected. In case a line hasn't a leading '#' then
     * this line is skipped.
     */
    void onUncomment();
    void setFileName(const QString&);
    void startDebug();

protected:
    /** Pops up the context menu with some extensions */
    void contextMenuEvent ( QContextMenuEvent* e ) override;
    void drawMarker(int line, int x, int y, QPainter*) override;

private:
    //PythonSyntaxHighlighter* pythonSyntax;
    struct PythonEditorP* d;
};

/**
 * Syntax highlighter for Python.
 * \author Werner Mayer
 */
class GuiExport PythonSyntaxHighlighter : public SyntaxHighlighter
{
public:
    explicit PythonSyntaxHighlighter(QObject* parent);
    ~PythonSyntaxHighlighter() override;

    void highlightBlock (const QString & text) override;

private:
    PythonSyntaxHighlighterP* d;
};

} // namespace Gui

#endif // GUI_PYTHONEDITOR_H
