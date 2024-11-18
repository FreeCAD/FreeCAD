/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_SYNTAXHIGHLIGHTER_H
#define GUI_SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <FCGlobal.h>

namespace Gui {
class SyntaxHighlighterP;
class TextEditor;

/**
 * Abstract Syntax highlighter.
 * @author Werner Mayer
 */
class GuiExport SyntaxHighlighter : public QSyntaxHighlighter
{
public:
    SyntaxHighlighter(QObject* parent);
    ~SyntaxHighlighter() override;

    int maximumUserState() const;

    void setColor(const QString& type, const QColor& col);
    QColor color(const QString& type);

protected:
    virtual void colorChanged(const QString& type, const QColor& col);

protected:
    enum TColor
    {
        Text = 0, Comment = 1, BlockComment = 2, Number = 3, String = 4, Keyword = 5,
        Classname = 6, Defname = 7, Operator = 8, Output = 9, Error = 10
    };

    QColor colorByType(TColor type);


private:
    SyntaxHighlighterP* d;
};

} // namespace Gui

#endif // GUI_SYNTAXHIGHLIGHTER_H
