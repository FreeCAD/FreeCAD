/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "qglobal.h"
#include <Base/PyObjectBase.h>
#include <FCGlobal.h>

QT_BEGIN_NAMESPACE
class QAction;
class QDir;
class QIcon;
class QImage;
class QGraphicsObject;
class QGraphicsItem;
class QObject;
class QPrinter;
class QWidget;
QT_END_NAMESPACE

namespace Gui
{

class GuiExport PythonWrapper
{
public:
    PythonWrapper();
    bool loadCoreModule();
    bool loadGuiModule();
    bool loadWidgetsModule();
    bool loadPrintSupportModule();
    bool loadUiToolsModule();

    bool toCString(const Py::Object&, std::string&);
    QObject* toQObject(const Py::Object&);
    qsizetype toEnum(PyObject* pyPtr);
    qsizetype toEnum(const Py::Object& pyobject);
    Py::Object toStandardButton(qsizetype);
    QGraphicsItem* toQGraphicsItem(PyObject* ptr);
    QGraphicsItem* toQGraphicsItem(const Py::Object& pyObject);
    QGraphicsObject* toQGraphicsObject(PyObject* pyPtr);
    QGraphicsObject* toQGraphicsObject(const Py::Object& pyObject);

    Py::Object fromQAction(QAction*);
    Py::Object fromQPrinter(QPrinter*);
    Py::Object fromQObject(QObject*, const char* className = nullptr);
    Py::Object fromQWidget(QWidget*, const char* className = nullptr);
    const char* getWrapperName(QObject*) const;

    Py::Object fromQImage(const QImage&);
    QImage* toQImage(PyObject* pyobj);
    /*!
      Create a Python wrapper for the icon. The icon must be created on the heap
      and the Python wrapper takes ownership of it.
     */
    Py::Object fromQIcon(const QIcon*);
    QIcon* toQIcon(PyObject* pyobj);
    Py::Object fromQDir(const QDir&);
    QDir* toQDir(PyObject* pyobj);
    static void createChildrenNameAttributes(PyObject* root, QObject* object);
    static void setParent(PyObject* pyWdg, QObject* parent);

private:
    qsizetype tryEnum(PyObject* pyPtr);
    Py::Object tryToStandardButton(qsizetype value);
};

}  // namespace Gui
