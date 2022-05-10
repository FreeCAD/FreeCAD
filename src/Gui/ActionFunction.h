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


#ifndef GUI_ACTIONFUNCTION_H
#define GUI_ACTIONFUNCTION_H

#include <QObject>
#include <QVariant>
#include <boost/function.hpp>

class QAction;

namespace Gui
{

class ActionFunctionPrivate;

/*!
  The class connects the triggered() signal of a QAction instance with the method
  of a usual C++ class not derived from QObject.

  The class can e.g be used to fill up the context-menu of the view provider classes.
  \code
    void MyViewProvider::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
    {
        Gui::ActionFunction* func = new Gui::ActionFunction(menu);

        QAction* a1 = menu->addAction(QObject::tr("Menu item 1..."));
        func->triggered(a1, boost::bind(&MyViewProvider::doItem1, this));

        QAction* a2 = menu->addAction(QObject::tr("Menu item 2..."));
        func->triggered(a2, boost::bind(&MyViewProvider::doItem2, this));

        QAction* a3 = menu->addAction(QObject::tr("Menu item 3..."));
        func->triggered(a3, boost::bind(&MyViewProvider::doItem3, this));
    }
  \endcode

  @author Werner Mayer

  http://www.boost.org/doc/libs/1_57_0/libs/bind/bind.html#with_boost_function
*/
class GuiExport ActionFunction : public QObject
{
    Q_OBJECT

public:
    /// Constructor
    ActionFunction(QObject*);
    virtual ~ActionFunction();

    /*!
       Connects the QAction's triggered() signal with the function \a func
     */
    void trigger(QAction* a, boost::function<void()> func);
    void toggle(QAction* a, boost::function<void(bool)> func);
    void hover(QAction* a, boost::function<void()> func);

private Q_SLOTS:
    void triggered();
    void toggled(bool);
    void hovered();

private:
    QScopedPointer<ActionFunctionPrivate> d_ptr;
    Q_DISABLE_COPY(ActionFunction)
    Q_DECLARE_PRIVATE(ActionFunction)
};

class TimerFunctionPrivate;

class GuiExport TimerFunction : public QObject
{
    Q_OBJECT

public:
    /// Constructor
    TimerFunction(QObject* = nullptr);
    virtual ~TimerFunction();

    void setFunction(boost::function<void()> func);
    void setFunction(boost::function<void(QObject*)> func, QObject* args);
    void setFunction(boost::function<void(QVariant)> func, QVariant args);
    void setAutoDelete(bool);

private Q_SLOTS:
    void timeout();

private:
    QScopedPointer<TimerFunctionPrivate> d_ptr;
    Q_DISABLE_COPY(TimerFunction)
    Q_DECLARE_PRIVATE(TimerFunction)
};

} //namespace Gui


#endif // GUI_ACTIONFUNCTION_H
