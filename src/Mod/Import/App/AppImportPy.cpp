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
# include <NCollection_Vector.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
# include <XCAFDoc_DocumentTool.hxx>
# include <XCAFDoc_ShapeTool.hxx>
# include <STEPCAFControl_Reader.hxx>
# include <STEPCAFControl_Writer.hxx>
# include <STEPControl_Writer.hxx>
# include <IGESCAFControl_Reader.hxx>
# include <IGESCAFControl_Writer.hxx>
# include <IGESControl_Controller.hxx>
# include <IGESData_GlobalSection.hxx>
# include <IGESData_IGESModel.hxx>
# include <IGESToBRep_Actor.hxx>
# include <Interface_Static.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_WorkSession.hxx>
# include <XSControl_TransferReader.hxx>
# include <APIHeaderSection_MakeHeader.hxx>
# include <OSD_Exception.hxx>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include "ImportOCAF.h"
#include "ExportOCAF.h"
//#include "ImportOCAFAssembly.h"
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
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PartFeaturePy.h>

#include "ImpExpDxf.h"

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
//        add_varargs_method("openAssembly",&Module::importAssembly,
//            "openAssembly(string) -- Open the assembly file and create a new document."
//        );
        add_varargs_method("export",&Module::exporter,
            "export(list,string) -- Export a list of objects into a single file."
        );
         add_varargs_method("readDXF",&Module::readDXF,
            "readDXF(filename,[document,ignore_errors]): Imports a DXF file into the given document. ignore_errors is True by default."
        );
        add_varargs_method("writeDXFShape",&Module::writeDXFShape,
            "writeDXFShape([shape],filename [version,usePolyline,optionSource]): Exports Shape(s) to a DXF file."
        );
        add_varargs_method("writeDXFObject",&Module::writeDXFObject,
            "writeDXFObject([objects],filename [,version,usePolyline,optionSource]): Exports DocumentObject(s) to a DXF file."
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

                    Handle(Message_ProgressIndicator) pi = new Part::ProgressIndicator(100);
                    aReader.Reader().WS()->MapReader()->SetProgress(pi);
                    pi->NewScope(100, "Reading STEP file...");
                    pi->Show();
                    aReader.Transfer(hDoc);
                    pi->EndScope();
                }
                catch (OSD_Exception& e) {
                    Base::Console().Error("%s\n", e.GetMessageString());
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

                    Handle(Message_ProgressIndicator) pi = new Part::ProgressIndicator(100);
                    aReader.WS()->MapReader()->SetProgress(pi);
                    pi->NewScope(100, "Reading IGES file...");
                    pi->Show();
                    aReader.Transfer(hDoc);
                    pi->EndScope();
                    // http://opencascade.blogspot.de/2009/03/unnoticeable-memory-leaks-part-2.html
                    Handle(IGESToBRep_Actor)::DownCast(aReader.WS()->TransferReader()->Actor())
                            ->SetModel(new IGESData_IGESModel);
                }
                catch (OSD_Exception& e) {
                    Base::Console().Error("%s\n", e.GetMessageString());
                    Base::Console().Message("Try to load IGES file without colors...\n");

                    Part::ImportIgesParts(pcDoc,Utf8Name.c_str());
                    pcDoc->recompute();
                }
            }
            else {
                throw Py::Exception(Base::BaseExceptionFreeCADError, "no supported file format");
            }

#if 1
            Import::ImportOCAFCmd ocaf(hDoc, pcDoc, file.fileNamePure());
            ocaf.loadShapes();
#else
            Import::ImportXCAF xcaf(hDoc, pcDoc, file.fileNamePure());
            xcaf.loadShapes();
#endif
            pcDoc->recompute();
            hApp->Close(hDoc);

            std::map<Part::Feature*, std::vector<App::Color> > colorMap = ocaf.getPartColorsMap();
            if (!colorMap.empty()) {
                Py::List list;
                for (auto it : colorMap) {
                    Py::Tuple tuple(2);
                    tuple.setItem(0, Py::asObject(it.first->getPyObject()));

                    App::PropertyColorList colors;
                    colors.setValues(it.second);
                    tuple.setItem(1, Py::asObject(colors.getPyObject()));

                    list.append(tuple);
                }

                return list;
            }
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.GetMessageString());
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
            keepExplicitPlacement = Standard_True;
            Import::ExportOCAFCmd ocaf(hDoc, keepExplicitPlacement);

            std::map<Part::Feature*, std::vector<App::Color> > partColors;
            std::vector<Part::Feature*> partObjects;

            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                        Part::Feature* part = static_cast<Part::Feature*>(obj);
                        partObjects.push_back(part);
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

                            partObjects.push_back(part);
                            partColors[part] = colors.getValues();
                        }
                        else {
                            Base::Console().Message("'%s' is not a shape, export will be ignored.\n", obj->Label.getValue());
                        }
                    }
                }
            }

            ocaf.setPartColorsMap(partColors);

            // That stuff is exporting a list of selected objects into FreeCAD Tree
            std::vector <TDF_Label> hierarchical_label;
            std::vector <TopLoc_Location> hierarchical_loc;
            std::vector <App::DocumentObject*> hierarchical_part;

            for (auto it : partObjects) {
                ocaf.exportObject(it, hierarchical_label, hierarchical_loc, hierarchical_part);
            }

            // Free Shapes must have absolute placement and not explicit
            std::vector <TDF_Label> FreeLabels;
            std::vector <int> part_id;
            ocaf.getFreeLabels(hierarchical_label, FreeLabels, part_id);
            // Got issue with the colors as they are coming from the View Provider they can't be determined into
            // the App Code.
            std::vector< std::vector<App::Color> > Colors;
            ocaf.getPartColors(hierarchical_part, FreeLabels, part_id, Colors);
            ocaf.reallocateFreeShape(hierarchical_part, FreeLabels, part_id, Colors);

#if OCC_VERSION_HEX >= 0x070200
            // Update is not performed automatically anymore: https://tracker.dev.opencascade.org/view.php?id=28055
            XCAFDoc_DocumentTool::ShapeTool(hDoc->Main())->UpdateAssemblies();
#endif

            Base::FileInfo file(Utf8Name.c_str());
            if (file.hasExtension("stp") || file.hasExtension("step")) {
                //Interface_Static::SetCVal("write.step.schema", "AP214IS");
                STEPCAFControl_Writer writer;
                Interface_Static::SetIVal("write.step.assembly",1);
                // writer.SetColorMode(Standard_False);
                writer.Transfer(hDoc, STEPControl_AsIs);

                // edit STEP header
#if OCC_VERSION_HEX >= 0x060500
                APIHeaderSection_MakeHeader makeHeader(writer.ChangeWriter().Model());
#else
                APIHeaderSection_MakeHeader makeHeader(writer.Writer().Model());
#endif
                Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                    .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("STEP");

                makeHeader.SetName(new TCollection_HAsciiString((Standard_CString)Utf8Name.c_str()));
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
        catch (Standard_Failure& e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object readDXF(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName=0;
        const char* optionSource = nullptr;
        char* defaultOptions = "User parameter:BaseApp/Preferences/Mod/Draft";
        char* useOptionSource = nullptr;
        bool IgnoreErrors=true;
        if (!PyArg_ParseTuple(args.ptr(), "et|sbs","utf-8",&Name,&DocName,&IgnoreErrors,&optionSource))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Base::FileInfo file(EncodedName.c_str());
        if (!file.exists())
            throw Py::RuntimeError("File doesn't exist");


        App::Document *pcDoc;
        if (DocName)
            pcDoc = App::GetApplication().getDocument(DocName);
        else
            pcDoc = App::GetApplication().getActiveDocument();
        if (!pcDoc) 
            pcDoc = App::GetApplication().newDocument(DocName);

        if (optionSource) {
            strcpy(useOptionSource,optionSource);
        } else {
            useOptionSource = defaultOptions;
        }

        try {
            // read the DXF file
            ImpExpDxfRead dxf_file(EncodedName,pcDoc);
            dxf_file.setOptionSource(useOptionSource);
            dxf_file.setOptions();
            dxf_file.DoRead(IgnoreErrors);
            pcDoc->recompute();
        }
        catch (const Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        return Py::None();
    }

    Py::Object writeDXFShape(const Py::Tuple& args)
    {
        PyObject *shapeObj;
        char* fname;
        std::string filePath;
        std::string layerName;
        const char* optionSource = nullptr;
        char* defaultOptions = "User parameter:BaseApp/Preferences/Mod/Import";
        char* useOptionSource = nullptr;
        int   versionParm = -1;
        bool  versionOverride = false;
        bool  polyOverride = false;
        PyObject *usePolyline = Py_False;

        //handle list of shapes
        if (PyArg_ParseTuple(args.ptr(), "O!et|iOs",  &(PyList_Type) ,
                                                      &shapeObj, 
                                                      "utf-8",
                                                      &fname, 
                                                      &versionParm,
                                                      &usePolyline,
                                                      &optionSource)) {
            filePath = std::string(fname);
            layerName = "none";
            PyMem_Free(fname);

            if ((versionParm == 12) ||
               (versionParm == 14)) {
               versionOverride = true;
            }
            if (usePolyline == Py_True) {
               polyOverride = true; 
            }
            if (optionSource != nullptr) {
                strcpy(useOptionSource,optionSource);
            } else {
                useOptionSource = defaultOptions;
            }

            try {
                ImpExpDxfWrite writer(filePath);
                writer.setOptionSource(useOptionSource);
                writer.setOptions();
                if (versionOverride) {
                    writer.setVersion(versionParm);
                }
                writer.setPolyOverride(polyOverride);
                writer.setLayerName(layerName);
                writer.init();
                Py::Sequence list(shapeObj);
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                        Part::TopoShape* ts = static_cast<Part::TopoShapePy*>((*it).ptr())->getTopoShapePtr();
                        TopoDS_Shape shape = ts->getShape();
                        writer.exportShape(shape);
                    }
                }
                writer.endRun();
            }
            catch (const Base::Exception& e) {
                throw Py::RuntimeError(e.what());
            }

        } else if (PyArg_ParseTuple(args.ptr(), "O!et|iOs",
                                                &(Part::TopoShapePy::Type) ,
                                                &shapeObj, 
                                                "utf-8",
                                                &fname, 
                                                &versionParm,
                                                &usePolyline,
                                                &optionSource)) {
            filePath = std::string(fname);
            layerName = "none";
            PyMem_Free(fname);

            if ((versionParm == 12) ||
               (versionParm == 14)) {
               versionOverride = true;
            }
            if (usePolyline == Py_True) {
               polyOverride = true; 
            }
            if (optionSource != nullptr) {
                strcpy(useOptionSource,optionSource);
            } else {
                useOptionSource = defaultOptions;
            }

            try {
                ImpExpDxfWrite writer(filePath);
                writer.setOptionSource(useOptionSource);
                writer.setOptions();
                if (versionOverride) {
                    writer.setVersion(versionParm);
                }
                writer.setPolyOverride(polyOverride);
                writer.setLayerName(layerName);
                writer.init();
                Part::TopoShape* obj = static_cast<Part::TopoShapePy*>(shapeObj)->getTopoShapePtr();
                TopoDS_Shape shape = obj->getShape();
                writer.exportShape(shape);
                writer.endRun();
            }
            catch (const Base::Exception& e) {
                throw Py::RuntimeError(e.what());
            }
        } else {
            throw Py::TypeError("expected ([Shape],path");
        } 
        return Py::None();
    }

    Py::Object writeDXFObject(const Py::Tuple& args)
    {
        PyObject *docObj;
        char* fname;
        std::string filePath;
        std::string layerName;
        const char* optionSource = nullptr;
        char* defaultOptions = "User parameter:BaseApp/Preferences/Mod/Import";
        char* useOptionSource = nullptr;
        int   versionParm = -1;
        bool  versionOverride = false;
        bool  polyOverride = false;
        PyObject *usePolyline = Py_False;

        if (PyArg_ParseTuple(args.ptr(), "O!et|iOs",  &(PyList_Type) ,
                                                      &docObj, 
                                                      "utf-8",
                                                      &fname, 
                                                      &versionParm,
                                                      &usePolyline,
                                                      &optionSource)) {
           filePath = std::string(fname);
            layerName = "none";
            PyMem_Free(fname);

            if ((versionParm == 12) ||
               (versionParm == 14)) {
               versionOverride = true;
            }
            if (usePolyline == Py_True) {
               polyOverride = true; 
            }

            if (optionSource != nullptr) {
                strcpy(useOptionSource,optionSource);
            } else {
                useOptionSource = defaultOptions;
            }

            try {
                ImpExpDxfWrite writer(filePath);
                writer.setOptionSource(useOptionSource);
                writer.setOptions();
                if (versionOverride) {
                    writer.setVersion(versionParm);
                }
                writer.setPolyOverride(polyOverride);
                writer.setLayerName(layerName);
                writer.init();
                Py::Sequence list(docObj);
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    if (PyObject_TypeCheck((*it).ptr(), &(Part::PartFeaturePy::Type))) {
                        PyObject* item = (*it).ptr();
                        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                        Part::Feature* part = static_cast<Part::Feature*>(obj);
                        layerName = part->getNameInDocument();
                        writer.setLayerName(layerName);
                        const TopoDS_Shape& shape = part->Shape.getValue();
                        writer.exportShape(shape);
                    }
                }
                writer.endRun();
            }
            catch (const Base::Exception& e) {
                throw Py::RuntimeError(e.what());
            }
        } else if (PyArg_ParseTuple(args.ptr(), "O!et|iOs",
                                                &(App::DocumentObjectPy::Type) ,
                                                &docObj, 
                                                "utf-8",
                                                &fname, 
                                                &versionParm,
                                                &usePolyline,
                                                &optionSource)) {
            filePath = std::string(fname);
            layerName = "none";
            PyMem_Free(fname);

            if ((versionParm == 12) ||
               (versionParm == 14)) {
               versionOverride = true;
            }
            if (usePolyline == Py_True) {
               polyOverride = true; 
            }

            if (optionSource != nullptr) {
                strcpy(useOptionSource,optionSource);
            } else {
                useOptionSource = defaultOptions;
            }
            
            try {
                ImpExpDxfWrite writer(filePath);
                writer.setOptionSource(useOptionSource);
                writer.setOptions();
                if (versionOverride) {
                    writer.setVersion(versionParm);
                }
                writer.setPolyOverride(polyOverride);
                writer.setLayerName(layerName);
                writer.init();
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(docObj)->getDocumentObjectPtr();
                Part::Feature* part = static_cast<Part::Feature*>(obj);
                layerName = part->getNameInDocument();
                writer.setLayerName(layerName);
                const TopoDS_Shape& shape = part->Shape.getValue();
                writer.exportShape(shape);
                writer.endRun();
            }
            catch (const Base::Exception& e) {
                throw Py::RuntimeError(e.what());
            }
        } else {
            throw Py::TypeError("expected ([DocObject],path");
        } 
        return Py::None();
    }


};
/*
static PyObject * importAssembly(PyObject *self, PyObject *args)
{
    char* Name;
    PyObject* TargetObjectPy=0;
    if (!PyArg_ParseTuple(args, "et|O!","utf-8",&Name,&(App::DocumentObjectPy::Type),&TargetObjectPy))
        return 0;
    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);
    std::string name8bit = Part::encodeFilename(Utf8Name);

    PY_TRY {
        //Base::Console().Log("Insert in Part with %s",Name);
        Base::FileInfo file(name8bit);

        App::DocumentObject* target = nullptr;

        if(TargetObjectPy)
            target = static_cast<App::DocumentObjectPy*>(TargetObjectPy)->getDocumentObjectPtr();
       

        App::Document *pcDoc = 0;
            
		pcDoc = App::GetApplication().getActiveDocument();
        
        if (!pcDoc) 
            pcDoc = App::GetApplication().newDocument("ImportedAssembly");
        

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
                    PyErr_SetString(PyExc_IOError, "cannot read STEP file");
                    return 0;
                }

                Handle(Message_ProgressIndicator) pi = new Part::ProgressIndicator(100);
                aReader.Reader().WS()->MapReader()->SetProgress(pi);
                pi->NewScope(100, "Reading STEP file...");
                pi->Show();
                aReader.Transfer(hDoc);
                pi->EndScope();
            }
            catch (OSD_Exception& e) {
                Base::Console().Error("%s\n", e.GetMessageString());
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
                if (aReader.ReadFile((Standard_CString)(name8bit.c_str())) != IFSelect_RetDone) {
                    PyErr_SetString(PyExc_IOError, "cannot read IGES file");
                    return 0;
                }

                Handle(Message_ProgressIndicator) pi = new Part::ProgressIndicator(100);
                aReader.WS()->MapReader()->SetProgress(pi);
                pi->NewScope(100, "Reading IGES file...");
                pi->Show();
                aReader.Transfer(hDoc);
                pi->EndScope();
            }
            catch (OSD_Exception& e) {
                Base::Console().Error("%s\n", e.GetMessageString());
                Base::Console().Message("Try to load IGES file without colors...\n");

                Part::ImportIgesParts(pcDoc,Name);
                pcDoc->recompute();
            }
        }
        else {
            PyErr_SetString(PyExc_RuntimeError, "no supported file format");
            return 0;
        }

        Import::ImportOCAFAssembly ocaf(hDoc, pcDoc, file.fileNamePure(),target);
        ocaf.loadAssembly();
        pcDoc->recompute();

    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return 0;
    }
    PY_CATCH

    Py_Return;
}*/

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Import
