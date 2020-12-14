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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QEvent>
# include <QKeyEvent>
#endif

#include "LineEdit.h"

using namespace SpreadsheetGui;

LineEdit::LineEdit(QWidget *parent)
    : Gui::ExpressionLineEdit(parent)
    , current()
    , deltaCol(0)
    , deltaRow(0)
{
}

bool LineEdit::event(QEvent *event)
{
    if (event && event->type() == QEvent::KeyPress) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);

        if (kevent->key() == Qt::Key_Tab) {
            if (kevent->modifiers() == 0) {
                deltaCol = 1;
                deltaRow = 0;
                Q_EMIT returnPressed();
                return true;
            }
        }
        else if (kevent->key() == Qt::Key_Backtab) {
            if (kevent->modifiers() == Qt::ShiftModifier) {
                deltaCol = -1;
                deltaRow = 0;
                Q_EMIT returnPressed();
                return true;
            }
        }
        else if (kevent->key() == Qt::Key_Enter || kevent->key() == Qt::Key_Return) {
            if (kevent->modifiers() == 0) {
                deltaCol = 0;
                deltaRow = 1;
                Q_EMIT returnPressed();
                return true;
            }
            else if (kevent->modifiers() == Qt::ShiftModifier) {
                deltaCol = 0;
                deltaRow = -1;
                Q_EMIT returnPressed();
                return true;
            }
        }
    }
    return Gui::ExpressionLineEdit::event(event);
}

void LineEdit::setIndex(QModelIndex _current)
{
    current = _current;
}

QModelIndex LineEdit::next() const
{
    const QAbstractItemModel * m = current.model();

    return m->index(qMin(qMax(0, current.row() + deltaRow), m->rowCount() - 1 ),
                    qMin(qMax(0, current.column() + deltaCol), m->columnCount() - 1 ) );
}

#include "moc_LineEdit.cpp"
