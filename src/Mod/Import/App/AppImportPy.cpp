/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#if defined(__MINGW32__)
# define WNT // avoid conflict with GUID
#endif
#ifndef _PreComp_
# include <Python.h>
# include <climits>
# include <Standard_Version.hxx>
# include <Handle_TDocStd_Document.hxx>
# include <Handle_XCAFApp_Application.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
# include <STEPCAFControl_Reader.hxx>
# include <STEPCAFControl_Writer.hxx>
# include <STEPControl_Writer.hxx>
# include <IGESCAFControl_Reader.hxx>
# include <IGESCAFControl_Writer.hxx>
# include <IGESControl_Controller.hxx>
# include <IGESData_GlobalSection.hxx>
# include <IGESData_IGESModel.hxx>
# include <Interface_Static.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_WorkSession.hxx>
# include <APIHeaderSection_MakeHeader.hxx>
# include <OSD_Exception.hxx>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include "ImportOCAF.h"
#include <Base/PyObjectBase.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/ProgressIndicator.h>
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>
#include <Mod/Part/App/encodeFilename.h>


namespace Import {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Import")
    {
        add_varargs_method("open",&Module::importer,
            "open(string) -- Open the file and create a new document."
        );
        add_varargs_method("insert",&Module::importer,
            "insert(string,string) -- Insert the file into the given document."
        );
        add_varargs_method("export",&Module::exporter,
            "export(list,string) -- Export a list of objects into a single file."
        );
        initialize("This module is the Import module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object importer(const Py::Tuple& args)
    {
        char* Name;
        char* DocName=0;
        if (!PyArg_ParseTuple(args.ptr(), "et|s","utf-8",&Name,&DocName))
            throw Py::Exception();

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);
        std::string name8bit = Part::encodeFilename(Utf8Name);

        try {
            //Base::Console().Log("Insert in Part with %s",Name);
            Base::FileInfo file(Utf8Name.c_str());

            App::Document *pcDoc = 0;
            if (DocName) {
                pcDoc = App::GetApplication().getDocument(DocName);
            }
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument("Unnamed");
            }

            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

            if (file.hasExtension("stp") || file.hasExtension("step")) {
                try {
                    STEPCAFControl_Reader aReader;
                    aReader.SetColorMode(true);
                    aReader.SetNameMode(true);
                    aReader.SetLayerMode(true);
                    if (aReader.ReadFile((Standard_CString)(name8bit.c_str())) != IFSelect_RetDone) {
                        throw Py::Exception(PyExc_IOError, "cannot read STEP file");
                    }

                    Handle_Message_ProgressIndicator pi = new Part::ProgressIndicator(100);
                    aReader.Reader().WS()->MapReader()->SetProgress(pi);
                    pi->NewScope(100, "Reading STEP file...");
                    pi->Show();
                    aReader.Transfer(hDoc);
                    pi->EndScope();
                }
                catch (OSD_Exception) {
                    Handle_Standard_Failure e = Standard_Failure::Caught();
                    Base::Console().Error("%s\n", e->GetMessageString());
                    Base::Console().Message("Try to load STEP file without colors...\n");

                    Part::ImportStepParts(pcDoc,Utf8Name.c_str());
                    pcDoc->recompute();
                }
            }
            else if (file.hasExtension("igs") || file.hasExtension("iges")) {
                Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                    .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("IGES");

                try {
                    IGESControl_Controller::Init();
                    IGESCAFControl_Reader aReader;
                    // http://www.opencascade.org/org/forum/thread_20603/?forum=3
                    aReader.SetReadVisible(hGrp->GetBool("SkipBlankEntities", true)
                        ? Standard_True : Standard_False);
                    aReader.SetColorMode(true);
                    aReader.SetNameMode(true);
                    aReader.SetLayerMode(true);
                    if (aReader.ReadFile((Standard_CString)(name8bit.c_str())) != IFSelect_RetDone) {
                        throw Py::Exception(PyExc_IOError, "cannot read IGES file");
                    }

                    Handle_Message_ProgressIndicator pi = new Part::ProgressIndicator(100);
                    aReader.WS()->MapReader()->SetProgress(pi);
                    pi->NewScope(100, "Reading IGES file...");
                    pi->Show();
                    aReader.Transfer(hDoc);
                    pi->EndScope();
                }
                catch (OSD_Exception) {
                    Handle_Standard_Failure e = Standard_Failure::Caught();
                    Base::Console().Error("%s\n", e->GetMessageString());
                    Base::Console().Message("Try to load IGES file without colors...\n");

                    Part::ImportIgesParts(pcDoc,Utf8Name.c_str());
                    pcDoc->recompute();
                }
            }
            else {
                throw Py::Exception(Base::BaseExceptionFreeCADError, "no supported file format");
            }

#if 1
            Import::ImportOCAF ocaf(hDoc, pcDoc, file.fileNamePure());
            ocaf.loadShapes();
#else
            Import::ImportXCAF xcaf(hDoc, pcDoc, file.fileNamePure());
            xcaf.loadShapes();
#endif
            pcDoc->recompute();
            hApp->Close(hDoc);
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            throw Py::Exception(Base::BaseExceptionFreeCADError, e->GetMessageString());
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }
    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object;
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet",&object,"utf-8",&Name))
            throw Py::Exception();

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);
        std::string name8bit = Part::encodeFilename(Utf8Name);

        try {
            Py::Sequence list(object);
            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

            bool keepExplicitPlacement = list.size() > 1;
            Import::ExportOCAF ocaf(hDoc, keepExplicitPlacement);

            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                        Part::Feature* part = static_cast<Part::Feature*>(obj);
                        std::vector<App::Color> colors;
                        ocaf.saveShape(part, colors);
                    }
                    else {
                        Base::Console().Message("'%s' is not a shape, export will be ignored.\n", obj->Label.getValue());
                    }
                }
                else if (PyTuple_Check(item) && PyTuple_Size(item) == 2) {
                    Py::Tuple tuple(*it);
                    Py::Object item0 = tuple.getItem(0);
                    Py::Object item1 = tuple.getItem(1);
                    if (PyObject_TypeCheck(item0.ptr(), &(App::DocumentObjectPy::Type))) {
                        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item0.ptr())->getDocumentObjectPtr();
                        if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                            Part::Feature* part = static_cast<Part::Feature*>(obj);
                            App::PropertyColorList colors;
                            colors.setPyObject(item1.ptr());
                            ocaf.saveShape(part, colors.getValues());
                        }
                        else {
                            Base::Console().Message("'%s' is not a shape, export will be ignored.\n", obj->Label.getValue());
                        }
                    }
                }
            }

            Base::FileInfo file(Utf8Name.c_str());
            if (file.hasExtension("stp") || file.hasExtension("step")) {
                //Interface_Static::SetCVal("write.step.schema", "AP214IS");
                STEPCAFControl_Writer writer;
                writer.Transfer(hDoc, STEPControl_AsIs);

                // edit STEP header
#if OCC_VERSION_HEX >= 0x060500
                APIHeaderSection_MakeHeader makeHeader(writer.ChangeWriter().Model());
#else
                APIHeaderSection_MakeHeader makeHeader(writer.Writer().Model());
#endif
                Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                    .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("STEP");

                makeHeader.SetName(new TCollection_HAsciiString((const Standard_CString)Utf8Name.c_str()));
                makeHeader.SetAuthorValue (1, new TCollection_HAsciiString(hGrp->GetASCII("Author", "Author").c_str()));
                makeHeader.SetOrganizationValue (1, new TCollection_HAsciiString(hGrp->GetASCII("Company").c_str()));
                makeHeader.SetOriginatingSystem(new TCollection_HAsciiString(App::GetApplication().getExecutableName()));
                makeHeader.SetDescriptionValue(1, new TCollection_HAsciiString("FreeCAD Model"));
                IFSelect_ReturnStatus ret = writer.Write(name8bit.c_str());
                if (ret == IFSelect_RetError || ret == IFSelect_RetFail || ret == IFSelect_RetStop) {
                    PyErr_Format(PyExc_IOError, "Cannot open file '%s'", Utf8Name.c_str());
                    throw Py::Exception();
                }
            }
            else if (file.hasExtension("igs") || file.hasExtension("iges")) {
                IGESControl_Controller::Init();
                IGESCAFControl_Writer writer;
                IGESData_GlobalSection header = writer.Model()->GlobalSection();
                header.SetAuthorName(new TCollection_HAsciiString(Interface_Static::CVal("write.iges.header.author")));
                header.SetCompanyName(new TCollection_HAsciiString(Interface_Static::CVal("write.iges.header.company")));
                header.SetSendName(new TCollection_HAsciiString(Interface_Static::CVal("write.iges.header.product")));
                writer.Model()->SetGlobalSection(header);
                writer.Transfer(hDoc);
                Standard_Boolean ret = writer.Write(name8bit.c_str());
                if (!ret) {
                    PyErr_Format(PyExc_IOError, "Cannot open file '%s'", Utf8Name.c_str());
                    throw Py::Exception();
                }
            }

            hApp->Close(hDoc);
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            throw Py::Exception(Base::BaseExceptionFreeCADError, e->GetMessageString());
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Import
