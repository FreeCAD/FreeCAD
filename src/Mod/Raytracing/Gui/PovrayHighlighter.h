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


#ifndef RAY_POVRAYHIGHLIGHTER_H
#define RAY_POVRAYHIGHLIGHTER_H

#include <Gui/SyntaxHighlighter.h>

namespace RaytracingGui {
class PovrayHighlighterP;

/**
 * Syntax highlighter for Povray.
 * @author Werner Mayer
 */
class PovrayHighlighter : public Gui::SyntaxHighlighter
{
public:
    PovrayHighlighter(QObject* parent);
    virtual ~PovrayHighlighter();

protected:
    void highlightBlock(const QString &text);

private:
    PovrayHighlighterP* d;
};

} // namespace Gui

#endif // RAY_POVRAYHIGHLIGHTER_H
