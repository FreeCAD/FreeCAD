/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef FEMGUI_ABAQUSHIGHLIGHTER_H
#define FEMGUI_ABAQUSHIGHLIGHTER_H

#include <Gui/SyntaxHighlighter.h>

namespace FemGui {

/**
 * Syntax highlighter for Abaqus.
 * @author Werner Mayer
 */
class AbaqusHighlighter : public Gui::SyntaxHighlighter
{
public:
    AbaqusHighlighter(QObject* parent);
    virtual ~AbaqusHighlighter();

protected:
    void highlightBlock(const QString &text);
};

} // namespace FemGui

#endif // FEMGUI_ABAQUSHIGHLIGHTER_H
