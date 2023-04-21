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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QCoreApplication>
# include <QEvent>
#endif

#include "Debugger.h"
#include "Console.h"


using namespace Base;

Debugger::Debugger(QObject* parent)
  : QObject(parent), isAttached(false)
{
}

Debugger::~Debugger() = default;

void Debugger::attach()
{
    QCoreApplication::instance()->installEventFilter(this);
    isAttached = true;
}

void Debugger::detach()
{
    QCoreApplication::instance()->removeEventFilter(this);
    isAttached = false;
}

bool Debugger::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        if (loop.isRunning()) {
            loop.quit();
            return true;
        }
    }

    return false;
}

int Debugger::exec()
{
    if (isAttached)
        Base::Console().Message("TO CONTINUE PRESS ANY KEY...\n");
    return loop.exec();
}

void Debugger::quit()
{
    loop.quit();
}

#include "moc_Debugger.cpp"
