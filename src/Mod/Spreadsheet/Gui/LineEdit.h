// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <Gui/ExpressionCompleter.h>
#include <QListView>


namespace SpreadsheetGui
{

class LineEdit: public Gui::ExpressionLineEdit
{
    Q_OBJECT
public:
    explicit LineEdit(QWidget* parent = nullptr);
    void setDocumentObject(const App::DocumentObject* currentDocObj, bool checkInList = true);

Q_SIGNALS:
    void finishedWithKey(int key, Qt::KeyboardModifiers modifiers);

private:
    int lastKeyPressed;
    Qt::KeyboardModifiers lastModifiers;

protected:
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

/* QCompleter uses a parentless QListView as a popup, whose geometry
 * is corrected using its own algorithm, which does not take into account QGraphicsScene,
 * therefore we have to use our own widget to adjust the geometry. */
class XListView: public QListView
{
    Q_OBJECT
public:
    explicit XListView(LineEdit* parent);

Q_SIGNALS:
    void geometryChanged(void);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void updateGeometries(void) override;
};

}  // namespace SpreadsheetGui
