/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <Gui/ExpressionCompleter.h>
#include <QWidget>


namespace SpreadsheetGui
{

class LineEdit: public Gui::ExpressionLineEdit
{
    Q_OBJECT
public:
    explicit LineEdit(QWidget* parent = nullptr);

    bool event(QEvent* event) override;

Q_SIGNALS:
    void finishedWithKey(int key, Qt::KeyboardModifiers modifiers);

private:
    bool eventFilter(QObject* object, QEvent* event) override;


private:
    int lastKeyPressed;
    Qt::KeyboardModifiers lastModifiers;
};

}  // namespace SpreadsheetGui

#endif  // LINEEDIT_H
