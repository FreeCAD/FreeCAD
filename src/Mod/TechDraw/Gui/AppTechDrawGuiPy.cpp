/***************************************************************************
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
*                                                                          *
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
# include <Python.h>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>


#include <Mod/Part/App/OCCError.h>
#include <Mod/TechDraw/App/DrawPage.h>

#include "MDIViewPage.h"
#include "ViewProviderPage.h"

namespace TechDrawGui {
//module level static C++ functions go here
}

namespace TechDrawGui {

class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("TechDrawGui")
    {
       add_varargs_method("export",&Module::exporter,
            "TechDraw hook for FC Gui exporter."
       );
       add_varargs_method("exportPageAsPdf",&Module::exportPageAsPdf,
            "exportPageAsPdf(DrawPageObject,FilePath) -- print page as Pdf to file."
        );
        add_varargs_method("exportPageAsSvg",&Module::exportPageAsSvg,
            "exportPageAsSvg(DrawPageObject,FilePath) -- print page as Svg to file."
        );
        initialize("This is a module for displaying drawings"); // register with Python
    }
    virtual ~Module() {}

private:
    virtual Py::Object invoke_method_varargs(void *method_def, const Py::Tuple &args)
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
    }

//! hook for FC Gui export function
    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object;
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet",&object,"utf-8",&Name))
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
                    throw Py::TypeError("Export of this object type is not supported by TechDraw module");
                }
            }
        }

        return Py::None();
    }

//!exportPageAsPdf(PageObject,FullPath)
    Py::Object exportPageAsPdf(const Py::Tuple& args)
    {
        PyObject *pageObj;
        char* name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet", &pageObj, "utf-8",&name)) {
            throw Py::TypeError("expected (Page,path");
        } 
        
        std::string filePath = std::string(name);
        PyMem_Free(name);
        
        try {
           App::DocumentObject* obj = 0;
           Gui::ViewProvider* vp = 0;
           MDIViewPage* mdi = 0;
           if (PyObject_TypeCheck(pageObj, &(App::DocumentObjectPy::Type))) {
               obj = static_cast<App::DocumentObjectPy*>(pageObj)->getDocumentObjectPtr();
               vp = Gui::Application::Instance->getViewProvider(obj);
               if (vp) {
                   TechDrawGui::ViewProviderPage* vpp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
                   if (vpp) {
                       mdi = vpp->getMDIViewPage();
                       if (mdi) {
                           mdi->printPdf(filePath);
                       } else {
                           vpp->showMDIViewPage();
                           mdi = vpp->getMDIViewPage();
                           mdi->printPdf(filePath);
                       }
                   }
               }
           }
        }
        catch (Base::Exception &e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
        }

        return Py::None();
    }

//!exportPageAsSvg(PageObject,FullPath)
    Py::Object exportPageAsSvg(const Py::Tuple& args)
    {
        PyObject *pageObj;
        char* name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet", &pageObj, "utf-8",&name)) {
            throw Py::TypeError("expected (Page,path");
        } 
        
        std::string filePath = std::string(name);
        PyMem_Free(name);
        
        try {
           App::DocumentObject* obj = 0;
           Gui::ViewProvider* vp = 0;
           MDIViewPage* mdi = 0;
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
                           mdi->saveSVG(filePath);
                       }
                   }
               }
           }
        }
        catch (Base::Exception &e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
        }

        return Py::None();
    }

 };

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace TechDrawGui
