// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <QEventLoop>
#include <QObject>
#include <FCGlobal.h>

namespace Base
{
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
    connect(btn, &QPushButton::clicked, &dbg, &Debugger::quit);
    dbg.exec();
  \endcode
 \author Werner Mayer
 */
class BaseExport Debugger: public QObject
{
    Q_OBJECT

public:
    explicit Debugger(QObject* parent = nullptr);
    ~Debugger() override;

    Debugger(const Debugger&) = delete;
    Debugger(Debugger&&) = delete;
    Debugger& operator=(const Debugger&) = delete;
    Debugger& operator=(Debugger&&) = delete;

    void attach();
    void detach();
    bool eventFilter(QObject* obj, QEvent* event) override;
    int exec();

public Q_SLOTS:
    void quit();

private:
    bool isAttached {false};
    QEventLoop loop;
};

}  // namespace Base
