/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_DEBUGGER_H
#define BASE_DEBUGGER_H

#include <QObject>
#include <QEventLoop>

namespace Base {
/**
  This is a utility class to break the application at a point to inspect e.g. the result of
  an algorithm.
  You usually use it like this
  \code
    ...
    Base::Debugger dbg;
    dbg.attach();
    dbg.exec();
    ...
  \endcode
  Or you can connect it with a button and let the user click it in order to continue.
  \code
    QPushButton* btn = new QPushButton();
    btn->setText("Continue");
    btn->show();
    Base::Debugger dbg;
    connect(btn, SIGNAL(clicked()), &dbg, SLOT(quit()));
    dbg.exec();
  \endcode
 \author Werner Mayer
 */
class BaseExport Debugger : public QObject
{
    Q_OBJECT

public:
    Debugger(QObject* parent=0);
    ~Debugger();

    void attach();
    void detach();
    bool eventFilter(QObject*, QEvent*);
    int exec();

public Q_SLOTS:
    void quit();

private:
    bool isAttached;
    QEventLoop loop;
};

}

#endif // BASE_DEBUGGER_H
