/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#endif

#include <Gui/PythonWrapper.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Fem/App/FemPostFilter.h>
#include <Mod/Fem/App/FemPostPipeline.h>

#include <QString>
#include <QTableView>
#include <QHeaderView>
#include <QDialog>
#include <QVBoxLayout>

#include "ViewProviderFemPostObject.h"
#include "TaskPostExtraction.h"

using namespace FemGui;
using namespace Gui;


// ***************************************************************************
// box to handle data extractions

TaskPostExtraction::TaskPostExtraction(ViewProviderFemPostObject* view, QWidget* parent)
    : TaskPostWidget(view, Gui::BitmapFactory().pixmap("FEM_PostHistogram"), QString(), parent)
{
    // we load the python implementation, and try to get the widget from it, to add
    // directly our widget

    setWindowTitle(tr("Data and Extractions"));

    Base::PyGILStateLocker lock;


    try {
        Py::Module mod(PyImport_ImportModule("femguiutils.data_extraction"), true);
        if (mod.isNull()) {
            Base::Console().error("Unable to import data extraction widget\n");
            return;
        }

        Py::Callable method(mod.getAttr(std::string("DataExtraction")));
        Py::Tuple args(1);
        args.setItem(0, Py::Object(view->getPyObject()));
        m_panel = Py::Object(method.apply(args));
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }

    if (m_panel.hasAttr(std::string("widget"))) {
        Py::Object pywidget(m_panel.getAttr(std::string("widget")));

        Gui::PythonWrapper wrap;
        if (wrap.loadCoreModule()) {
            if (auto* widget = qobject_cast<QWidget*>(wrap.toQObject(pywidget))) {
                // finally we have the usable QWidget. Add to us!

                auto layout = new QVBoxLayout();
                layout->addWidget(widget);
                setLayout(layout);
                return;
            }
        }
    }

    // if we are here something went wrong!
    Base::Console().error("Unable to import data extraction widget\n");
};

TaskPostExtraction::~TaskPostExtraction()
{

    Base::PyGILStateLocker lock;
    try {
        if (m_panel.hasAttr(std::string("widget"))) {
            m_panel.setAttr(std::string("widget"), Py::None());
        }
        m_panel = Py::None();
    }
    catch (Py::AttributeError& e) {
        e.clear();
    }
}

void TaskPostExtraction::onPostDataChanged(Fem::FemPostObject* obj)
{
    Base::PyGILStateLocker lock;
    try {
        if (m_panel.hasAttr(std::string("onPostDataChanged"))) {
            Py::Callable method(m_panel.getAttr(std::string("onPostDataChanged")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(obj->getPyObject()));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }
};

bool TaskPostExtraction::isGuiTaskOnly()
{
    Base::PyGILStateLocker lock;
    try {
        if (m_panel.hasAttr(std::string("isGuiTaskOnly"))) {
            Py::Callable method(m_panel.getAttr(std::string("isGuiTaskOnly")));
            auto result = Py::Boolean(method.apply());
            return result.as_bool();
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }

    return false;
};

void TaskPostExtraction::apply()
{
    Base::PyGILStateLocker lock;
    try {
        if (m_panel.hasAttr(std::string("apply"))) {
            Py::Callable method(m_panel.getAttr(std::string("apply")));
            method.apply();
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }
}

bool TaskPostExtraction::initiallyCollapsed()
{
    Base::PyGILStateLocker lock;
    try {
        if (m_panel.hasAttr(std::string("initiallyCollapsed"))) {
            Py::Callable method(m_panel.getAttr(std::string("initiallyCollapsed")));
            auto result = Py::Boolean(method.apply());
            return result.as_bool();
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }

    return false;
}

#include "moc_TaskPostExtraction.cpp"
