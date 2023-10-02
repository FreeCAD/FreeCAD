/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <unordered_map>
# include <stdexcept>
#endif

#include <Base/Interpreter.h>
#include <App/Document.h>

#include "MDIViewPyWrap.h"
#include "PythonWrapper.h"


using namespace Gui;
namespace sp = std::placeholders;

namespace Gui {

class MDIViewPyWrapImp
{
public:
    MDIViewPyWrapImp(Py::Object pyobject)
        : pyobject{pyobject}
    {
        Base::PyGILStateLocker lock;
        std::vector<std::string> methods = {"widget", "onMsg", "onHasMsg", "canClose", "printDocument", "print", "printPdf", "printPreview", "redoActions", "undoActions"};

        for (const auto& it : methods) {
            if (pyobject.hasAttr(it)) {
                func[it] = pyobject.getAttr(it);
            }
        }
    }

    ~MDIViewPyWrapImp()
    {
        Base::PyGILStateLocker lock;
        pyobject = Py::None();
        func.clear();
    }

    QWidget* widget()
    {
        Base::PyGILStateLocker lock;
        PythonWrapper wrap;
        wrap.loadWidgetsModule();
        if (func.count("widget") == 0) {
            throw Py::AttributeError("Object has no attribute 'widget'");
        }
        Py::Callable target(func.at("widget"));
        Py::Object pywidget(target.apply(Py::Tuple()));
        return qobject_cast<QWidget*>(wrap.toQObject(pywidget));
    }

    bool onMsg(const char* pMsg)
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("onMsg"));
        Py::Boolean result(target.apply(Py::TupleN(Py::String(pMsg))));
        return static_cast<bool>(result);
    }

    bool onHasMsg(const char* pMsg)
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("onHasMsg"));
        Py::Boolean result(target.apply(Py::TupleN(Py::String(pMsg))));
        return static_cast<bool>(result);
    }

    bool canClose()
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("canClose"));
        Py::Boolean result(target.apply(Py::Tuple()));
        return static_cast<bool>(result);
    }

    void printDocument(QPrinter* printer)
    {
        Base::PyGILStateLocker lock;
        PythonWrapper wrap;
        wrap.loadPrintSupportModule();
        Py::Object pyprint = wrap.fromQPrinter(printer);
        Py::Callable target(func.at("printDocument"));
        target.apply(Py::TupleN(pyprint));
    }

    void print()
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("print"));
        target.apply(Py::Tuple());
    }

    void printPdf()
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("printPdf"));
        target.apply(Py::Tuple());
    }

    void printPreview()
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("printPreview"));
        target.apply(Py::Tuple());
    }

    QStringList undoActions()
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("undoActions"));
        Py::List list(target.apply(Py::Tuple()));
        QStringList actions;
        for (const auto& it : list) {
            Py::String str(it);
            actions << QString::fromStdString(str);
        }
        return actions;
    }

    QStringList redoActions()
    {
        Base::PyGILStateLocker lock;
        Py::Callable target(func.at("redoActions"));
        Py::List list(target.apply(Py::Tuple()));
        QStringList actions;
        for (const auto& it : list) {
            Py::String str(it);
            actions << QString::fromStdString(str);
        }
        return actions;
    }

private:
    std::unordered_map<std::string, Py::Object> func;
    Py::Object pyobject;
};

}


TYPESYSTEM_SOURCE_ABSTRACT(Gui::MDIViewPyWrap,Gui::MDIView)

MDIViewPyWrap::MDIViewPyWrap(const Py::Object& py, Gui::Document* pcDocument,QWidget* parent, Qt::WindowFlags wflags)
  : MDIView(pcDocument, parent, wflags)
  , ptr(std::make_unique<MDIViewPyWrapImp>(py))
{
    try {
        QWidget* widget = ptr->widget();
        if (widget) {
            setCentralWidget(widget);
        }
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
    }
}

MDIViewPyWrap::~MDIViewPyWrap()
{
    ptr.reset(nullptr);
}

PyObject* MDIViewPyWrap::getPyObject()
{
    return MDIView::getPyObject();
}

bool MDIViewPyWrap::onMsg(const char* pMsg,const char** ppReturn)
{
    try {
        if (ptr->onMsg(pMsg)) {
            return true;
        }
        return MDIView::onMsg(pMsg, ppReturn);
    }
    catch (const std::exception&) {
        return MDIView::onMsg(pMsg, ppReturn);
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
        return false;
    }
}

bool MDIViewPyWrap::onHasMsg(const char* pMsg) const
{
    try {
        if (ptr->onHasMsg(pMsg)) {
            return true;
        }
        return MDIView::onHasMsg(pMsg);
    }
    catch (const std::exception&) {
        return MDIView::onHasMsg(pMsg);
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
        return false;
    }
}

bool MDIViewPyWrap::canClose()
{
    try {
        return ptr->canClose();
    }
    catch (const std::exception&) {
        return MDIView::canClose();
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
        return false;
    }
}

void MDIViewPyWrap::print(QPrinter* printer)
{
    try {
        return ptr->printDocument(printer);
    }
    catch (const std::exception&) {
        return MDIView::print(printer);
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
    }
}

void MDIViewPyWrap::print()
{
    try {
        return ptr->print();
    }
    catch (const std::exception&) {
        return MDIView::print();
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
    }
}

void MDIViewPyWrap::printPdf()
{
    try {
        return ptr->printPdf();
    }
    catch (const std::exception&) {
        return MDIView::printPdf();
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
    }
}

void MDIViewPyWrap::printPreview()
{
    try {
        return ptr->printPreview();
    }
    catch (const std::exception&) {
        return MDIView::printPreview();
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
    }
}

QStringList MDIViewPyWrap::undoActions() const
{
    try {
        return ptr->undoActions();
    }
    catch (const std::exception&) {
        return MDIView::undoActions();
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
        return MDIView::undoActions();
    }
}

QStringList MDIViewPyWrap::redoActions() const
{
    try {
        return ptr->redoActions();
    }
    catch (const std::exception&) {
        return MDIView::redoActions();
    }
    catch (Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException exc;
        exc.ReportException();
        return MDIView::redoActions();
    }
}

#include "moc_MDIViewPyWrap.cpp"
