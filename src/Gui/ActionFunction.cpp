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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QAction>
# include <QMap>
#endif

#include "ActionFunction.h"

using namespace Gui;


namespace Gui {
class ActionFunctionPrivate
{
public:
    QMap<QAction*, boost::function<void()> > triggerMap;
    QMap<QAction*, boost::function<void(bool)> > toggleMap;
    QMap<QAction*, boost::function<void()> > hoverMap;
};
}

ActionFunction::ActionFunction(QObject* parent)
  : QObject(parent), d_ptr(new ActionFunctionPrivate())
{
}

ActionFunction::~ActionFunction()
{
}

void ActionFunction::trigger(QAction* action, boost::function<void()> func)
{
    Q_D(ActionFunction);

    d->triggerMap[action] = func;
    connect(action, SIGNAL(triggered()), this, SLOT(triggered()));
}

void ActionFunction::triggered()
{
    Q_D(ActionFunction);

    QAction* a = qobject_cast<QAction*>(sender());
    QMap<QAction*, boost::function<void()> >::iterator it = d->triggerMap.find(a);
    if (it != d->triggerMap.end()) {
        // invoke the class function here
        it.value()();
    }
}

void ActionFunction::toggle(QAction* action, boost::function<void(bool)> func)
{
    Q_D(ActionFunction);

    d->toggleMap[action] = func;
    connect(action, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)));
}

void ActionFunction::toggled(bool on)
{
    Q_D(ActionFunction);

    QAction* a = qobject_cast<QAction*>(sender());
    QMap<QAction*, boost::function<void(bool)> >::iterator it = d->toggleMap.find(a);
    if (it != d->toggleMap.end()) {
        // invoke the class function here
        it.value()(on);
    }
}

void ActionFunction::hover(QAction* action, boost::function<void()> func)
{
    Q_D(ActionFunction);

    d->hoverMap[action] = func;
    connect(action, SIGNAL(hovered()), this, SLOT(hovered()));
}

void ActionFunction::hovered()
{
    Q_D(ActionFunction);

    QAction* a = qobject_cast<QAction*>(sender());
    QMap<QAction*, boost::function<void()> >::iterator it = d->hoverMap.find(a);
    if (it != d->hoverMap.end()) {
        // invoke the class function here
        it.value()();
    }
}

#include "moc_ActionFunction.cpp"
