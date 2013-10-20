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
# include <Interface_Static.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_WorkSession.hxx>
# include <APIHeaderSection_MakeHeader.hxx>
# include <OSD_Exception.hxx>
#endif

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



/* module functions */

static PyObject * importer(PyObject *self, PyObject *args)
{
    char* Name;
    char* DocName=0;
    if (!PyArg_ParseTuple(args, "s|s",&Name,&DocName))
        return 0;

    PY_TRY {
        //Base::Console().Log("Insert in Part with %s",Name);
        Base::FileInfo file(Name);

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
                if (aReader.ReadFile((Standard_CString)Name) != IFSelect_RetDone) {
                    PyErr_SetString(PyExc_Exception, "cannot read STEP file");
                    return 0;
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

                Part::ImportStepParts(pcDoc,Name);
                pcDoc->recompute();
            }
        }
        else if (file.hasExtension("igs") || file.hasExtension("iges")) {
            try {
                IGESControl_Controller::Init();
                Interface_Static::SetIVal("read.surfacecurve.mode",3);
                IGESCAFControl_Reader aReader;
                aReader.SetColorMode(true);
                aReader.SetNameMode(true);
                aReader.SetLayerMode(true);
                if (aReader.ReadFile((Standard_CString)Name) != IFSelect_RetDone) {
                    PyErr_SetString(PyExc_Exception, "cannot read IGES file");
                    return 0;
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

                Part::ImportIgesParts(pcDoc,Name);
                pcDoc->recompute();
            }
        }
        else {
            PyErr_SetString(PyExc_Exception, "no supported file format");
            return 0;
        }

#if 1
        Import::ImportOCAF ocaf(hDoc, pcDoc, file.fileNamePure());
        ocaf.loadShapes();
#else
        Import::ImportXCAF xcaf(hDoc, pcDoc, file.fileNamePure());
        xcaf.loadShapes();
#endif
        pcDoc->recompute();

    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
    PY_CATCH

    Py_Return;
}

static PyObject * open(PyObject *self, PyObject *args)
{
    return importer(self, args);
}

static PyObject * exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    const char* filename;
    if (!PyArg_ParseTuple(args, "Os",&object,&filename))
        return NULL;

    PY_TRY {
        Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
        Handle(TDocStd_Document) hDoc;
        hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);
        Import::ExportOCAF ocaf(hDoc);

        Py::Sequence list(object);
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

        Base::FileInfo file(filename);
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
            makeHeader.SetName(new TCollection_HAsciiString((const Standard_CString)filename));
            makeHeader.SetAuthorValue (1, new TCollection_HAsciiString("FreeCAD"));
            makeHeader.SetOrganizationValue (1, new TCollection_HAsciiString("FreeCAD"));
            makeHeader.SetOriginatingSystem(new TCollection_HAsciiString("FreeCAD"));
            makeHeader.SetDescriptionValue(1, new TCollection_HAsciiString("FreeCAD Model"));
            writer.Write(filename);
        }
        else if (file.hasExtension("igs") || file.hasExtension("iges")) {
            IGESControl_Controller::Init();
            IGESCAFControl_Writer writer;
            writer.Transfer(hDoc);
            writer.Write(filename);
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
    PY_CATCH

    Py_Return;
}

/* registration table  */
struct PyMethodDef Import_Import_methods[] = {
    {"open"     ,open  ,METH_VARARGS,
     "open(string) -- Open the file and create a new document."},
    {"insert"     ,importer  ,METH_VARARGS,
     "insert(string,string) -- Insert the file into the given document."},
    {"export"     ,exporter  ,METH_VARARGS,
     "export(list,string) -- Export a list of objects into a single file."},
    {NULL, NULL}                   /* end of table marker */
};
