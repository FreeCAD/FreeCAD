/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/PythonWrapper.h>
#include <Mod/Part/App/OCCError.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawPagePy.h>
#include <Mod/TechDraw/App/DrawViewPy.h>  // generated from DrawViewPy.xml

#include "MDIViewPage.h"
#include "QGIView.h"
#include "QGSPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderDrawingView.h"
#include "PagePrinter.h"


namespace TechDrawGui {

class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("TechDrawGui")
    {
       add_varargs_method("export", &Module::exporter,
            "TechDraw hook for FC Gui exporter."
       );
       add_varargs_method("exportPageAsPdf", &Module::exportPageAsPdf,
            "exportPageAsPdf(DrawPageObject, FilePath) -- print page as Pdf to file."
        );
        add_varargs_method("exportPageAsSvg", &Module::exportPageAsSvg,
            "exportPageAsSvg(DrawPageObject, FilePath) -- print page as Svg to file."
        );
        add_varargs_method("addQGIToView", &Module::addQGIToView,
            "addQGIToView(View, QGraphicsItem) -- insert graphics item into view's graphic."
        );
        add_varargs_method("addQGObjToView", &Module::addQGObjToView,
            "addQGObjToView(View, QGraphicsObject) -- insert graphics object into view's graphic. Use for QGraphicsItems that have QGraphicsObject as base class."
        );
        add_varargs_method("addQGIToScene", &Module::addQGIToScene,
            "addQGIToScene(Page, QGraphicsItem) -- insert graphics item into Page's scene."
        );
        add_varargs_method("addQGObjToScene", &Module::addQGObjToScene,
            "addQGObjToScene(Page, QGraphicsObject) -- insert graphics object into Page's scene. Use for QGraphicsItems that have QGraphicsObject as base class."
        );
        add_varargs_method("getSceneForPage", &Module::getSceneForPage,
            "QGSPage = getSceneForPage(page) -- get the scene for a DrawPage."
        );
        initialize("This is a module for displaying drawings"); // register with Python
    }
    ~Module() override {}

private:
    Py::Object invoke_method_varargs(void *method_def, const Py::Tuple &args) override
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Standard_Failure &e) {
            std::string str;
            Standard_CString msg = e.GetMessageString();
            str += typeid(e).name();
            str += " ";
            if (msg) {str += msg;}
            else     {str += "No OCCT Exception Message";}
            Base::Console().Error("%s\n", str.c_str());
            throw Py::Exception(Part::PartExceptionOCCError, str);
        }
        catch (const Base::Exception &e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            e.ReportException();
            throw Py::RuntimeError(str);
        }
        catch (const std::exception &e) {
            std::string str;
            str += "C++ exception thrown (";
            str += e.what();
            str += ")";
            Base::Console().Error("%s\n", str.c_str());
            throw Py::RuntimeError(str);
        }
        return Py::None(); //only here to prevent warning re no return value
    }

//! hook for FC Gui export function
    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object;
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet", &object, "utf-8", &Name))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        TechDraw::DrawPage* page = nullptr;
        Py::Sequence list(object);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(TechDraw::DrawPage::getClassTypeId())) {
                    page = static_cast<TechDraw::DrawPage*>(obj);
                    Gui::Document* activeGui = Gui::Application::Instance->getDocument(page->getDocument());
                    Gui::ViewProvider* vp = activeGui->getViewProvider(obj);
                    ViewProviderPage* dvp = dynamic_cast<ViewProviderPage*>(vp);
                    if ( !(dvp  && dvp->getMDIViewPage()) ) {
                        throw Py::TypeError("TechDraw can not find Page");
                    }

                    Base::FileInfo fi_out(EncodedName.c_str());

                    if (fi_out.hasExtension("svg")) {
                        dvp->getMDIViewPage()->saveSVG(EncodedName);
                    } else if (fi_out.hasExtension("dxf")) {
                        dvp->getMDIViewPage()->saveDXF(EncodedName);
                    } else if (fi_out.hasExtension("pdf")) {
                        dvp->getMDIViewPage()->savePDF(EncodedName);
                    } else {
                        throw Py::TypeError("TechDraw can not export this file format");
                    }
                }
                else {
                    throw Py::TypeError("No Technical Drawing Page found in selection.");
                }
            }
        }

        return Py::None();
    }

//!exportPageAsPdf(PageObject, FullPath)
    Py::Object exportPageAsPdf(const Py::Tuple& args)
    {
        PyObject *pageObj;
        char* name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet", &pageObj, "utf-8", &name)) {
            throw Py::TypeError("expected (Page, path");
        }

        std::string filePath = std::string(name);
        PyMem_Free(name);

        try {
           App::DocumentObject* obj = nullptr;
           Gui::ViewProvider* vp = nullptr;
           MDIViewPage* mdi = nullptr;
           if (PyObject_TypeCheck(pageObj, &(App::DocumentObjectPy::Type))) {
               obj = static_cast<App::DocumentObjectPy*>(pageObj)->getDocumentObjectPtr();
               vp = Gui::Application::Instance->getViewProvider(obj);
               if (vp) {
                   TechDrawGui::ViewProviderPage* vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
                   if (vpp) {
                       mdi = vpp->getMDIViewPage();
                       if (mdi) {
                           mdi->savePDF(filePath);
                       } else {
                           vpp->showMDIViewPage();
                           mdi = vpp->getMDIViewPage();
                           if (mdi) {
                               mdi->savePDF(filePath);
                           } else {
                               throw Py::TypeError("Page not available! Is it Hidden?");
                           }
                       }
                   }
               }
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }

//!exportPageAsSvg(PageObject, FullPath)
    Py::Object exportPageAsSvg(const Py::Tuple& args)
    {
        PyObject *pageObj;
        char* name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet", &pageObj, "utf-8", &name)) {
            throw Py::TypeError("expected (Page, path");
        }

        std::string filePath = std::string(name);
        PyMem_Free(name);

        try {
           App::DocumentObject* obj = nullptr;
           Gui::ViewProvider* vp = nullptr;
           MDIViewPage* mdi = nullptr;
           if (PyObject_TypeCheck(pageObj, &(App::DocumentObjectPy::Type))) {
               obj = static_cast<App::DocumentObjectPy*>(pageObj)->getDocumentObjectPtr();
               vp = Gui::Application::Instance->getViewProvider(obj);
               if (vp) {
                   TechDrawGui::ViewProviderPage* vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
                   if (vpp) {
                       mdi = vpp->getMDIViewPage();
                       if (mdi) {
                           mdi->saveSVG(filePath);
                       } else {
                           vpp->showMDIViewPage();
                           mdi = vpp->getMDIViewPage();
                           if (mdi) {
                               mdi->saveSVG(filePath);
                           } else {
                               throw Py::TypeError("Page not available! Is it Hidden?");
                           }
                       }
                   }
               }
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }

        Py::Object addQGIToView(const Py::Tuple& args)
    {
        PyObject *viewPy = nullptr;
        PyObject *qgiPy = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!O", &(TechDraw::DrawViewPy::Type), &viewPy, &qgiPy)) {
            throw Py::TypeError("expected (view, item)");
        }

        try {
           App::DocumentObject* obj = nullptr;
           Gui::ViewProvider* vp = nullptr;
           QGIView* qgiv = nullptr;
           obj = static_cast<App::DocumentObjectPy*>(viewPy)->getDocumentObjectPtr();
           vp = Gui::Application::Instance->getViewProvider(obj);
           if (vp) {
               TechDrawGui::ViewProviderDrawingView* vpdv =
                            dynamic_cast<TechDrawGui::ViewProviderDrawingView*>(vp);
               if (vpdv) {
                   qgiv = vpdv->getQView();
                   if (qgiv) {
                       Gui::PythonWrapper wrap;
                       if (!wrap.loadGuiModule()) {
                           throw Py::RuntimeError("Failed to load Python wrapper for Qt::Gui");
                        }
                        QGraphicsItem* item = wrap.toQGraphicsItem(args[1]);
                        if (item) {
                            qgiv->addArbitraryItem(item);
                        }
                    }
               }
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }

    Py::Object addQGObjToView(const Py::Tuple& args)
    {
        PyObject *viewPy = nullptr;
        PyObject *qgiPy = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!O", &(TechDraw::DrawViewPy::Type), &viewPy, &qgiPy)) {
            throw Py::TypeError("expected (view, item)");
        }

        try {
           App::DocumentObject* obj = nullptr;
           Gui::ViewProvider* vp = nullptr;
           QGIView* qgiv = nullptr;
           obj = static_cast<App::DocumentObjectPy*>(viewPy)->getDocumentObjectPtr();
           vp = Gui::Application::Instance->getViewProvider(obj);
           if (vp) {
               TechDrawGui::ViewProviderDrawingView* vpdv =
                            dynamic_cast<TechDrawGui::ViewProviderDrawingView*>(vp);
               if (vpdv) {
                   qgiv = vpdv->getQView();
                   if (qgiv) {
                       Gui::PythonWrapper wrap;
                       if (!wrap.loadGuiModule()) {
                           throw Py::RuntimeError("Failed to load Python wrapper for Qt::Gui");
                        }
                        QGraphicsObject* item = wrap.toQGraphicsObject(args[1]);
                        if (item) {
                            qgiv->addArbitraryItem(item);
                        }
                    }
               }
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }


    //adds a free graphics item to a Page's scene
    Py::Object addQGIToScene(const Py::Tuple& args)
    {
        PyObject *pagePy = nullptr;
        PyObject *qgiPy = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!O", &(TechDraw::DrawPagePy::Type), &pagePy, &qgiPy)) {
            throw Py::TypeError("expected (view, item)");
        }

        try {
           App::DocumentObject* obj = nullptr;
           Gui::ViewProvider* vp = nullptr;
           QGSPage* qgsp = nullptr;
           obj = static_cast<App::DocumentObjectPy*>(pagePy)->getDocumentObjectPtr();
           vp = Gui::Application::Instance->getViewProvider(obj);
           if (vp) {
               TechDrawGui::ViewProviderPage* vpp =
                            dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
               if (vpp) {
                   qgsp = vpp->getQGSPage();
                   if (qgsp) {
                       Gui::PythonWrapper wrap;
                       if (!wrap.loadGuiModule()) {
                           throw Py::RuntimeError("Failed to load Python wrapper for Qt::Gui");
                       }
                        QGraphicsItem* item = wrap.toQGraphicsItem(args[1]);
                        if (item) {
                            qgsp->addItem(item);
                        }
                    }
               }
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }


    //adds a free graphics object to a Page's scene
//!use addQGObjToScene for QGraphics items like QGraphicsSvgItem or QGraphicsTextItem that are
//! derived from QGraphicsObject
    Py::Object addQGObjToScene(const Py::Tuple& args)
    {
        PyObject *pagePy = nullptr;
        PyObject *qgiPy = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!O", &(TechDraw::DrawPagePy::Type), &pagePy, &qgiPy)) {
            throw Py::TypeError("expected (view, item)");
        }

        try {
           App::DocumentObject* obj = nullptr;
           Gui::ViewProvider* vp = nullptr;
           QGSPage* qgsp = nullptr;
           obj = static_cast<App::DocumentObjectPy*>(pagePy)->getDocumentObjectPtr();
           vp = Gui::Application::Instance->getViewProvider(obj);
           if (vp) {
               TechDrawGui::ViewProviderPage* vpp =
                            dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
               if (vpp) {
                   qgsp = vpp->getQGSPage();
                   if (qgsp) {
                       Gui::PythonWrapper wrap;
                       if (!wrap.loadGuiModule()) {
                           throw Py::RuntimeError("Failed to load Python wrapper for Qt::Gui");
                       }
                        QGraphicsObject* item = wrap.toQGraphicsObject(args[1]);
                        if (item) {
                            qgsp->addItem(item);
                        }
                    }
               }
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }

    Py::Object getSceneForPage(const Py::Tuple& args)
    {
        PyObject *pagePy = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &(TechDraw::DrawPagePy::Type), &pagePy)) {
            throw Py::TypeError("expected (page)");
        }

        try {
           App::DocumentObject* obj = nullptr;
           Gui::ViewProvider* vp = nullptr;
           QGSPage* qgsp = nullptr;
           obj = static_cast<App::DocumentObjectPy*>(pagePy)->getDocumentObjectPtr();
           vp = Gui::Application::Instance->getViewProvider(obj);
           if (vp) {
               TechDrawGui::ViewProviderPage* vpp =
                            dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
               if (vpp) {
                   qgsp = vpp->getQGSPage();
                   if (qgsp) {
                       Gui::PythonWrapper wrap;
                       if (!wrap.loadGuiModule()) {
                           throw Py::RuntimeError("Failed to load Python wrapper for Qt::Gui");
                       }
                       return wrap.fromQObject(qgsp, "TechDrawGui::QGSPage");
                    }
               }
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace TechDrawGui
