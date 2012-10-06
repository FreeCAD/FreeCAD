/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRep_Builder.hxx>
# include <Handle_TDocStd_Document.hxx>
# include <Handle_XCAFApp_Application.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
# include <XCAFDoc_DocumentTool.hxx>
# include <XCAFDoc_ShapeTool.hxx>
# include <XCAFDoc_ColorTool.hxx>
# include <XCAFDoc_Location.hxx>
# include <TDF_Label.hxx>
# include <TDF_LabelSequence.hxx>
# include <TDF_ChildIterator.hxx>
# include <TDataStd_Name.hxx>
# include <Quantity_Color.hxx>
# include <STEPCAFControl_Reader.hxx>
# include <STEPCAFControl_Writer.hxx>
# include <STEPControl_Writer.hxx>
# include <IGESCAFControl_Reader.hxx>
# include <IGESCAFControl_Writer.hxx>
# include <IGESControl_Controller.hxx>
# include <Interface_Static.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_WorkSession.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Iterator.hxx>
# include <APIHeaderSection_MakeHeader.hxx>
#if OCC_VERSION_HEX >= 0x060500
# include <TDataXtd_Shape.hxx>
# else
# include <TDataStd_Shape.hxx>
# endif
#endif

#include <Base/PyObjectBase.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/ProgressIndicator.h>



class ImportOCAF
{
public:
    ImportOCAF(Handle_TDocStd_Document h, App::Document* d, const std::string& name)
        : pDoc(h), doc(d), default_name(name)
    {
        aShapeTool = XCAFDoc_DocumentTool::ShapeTool (pDoc->Main());
        aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());
    }

    void loadShapes();

private:
    void loadShapes(const TDF_Label& label, const TopLoc_Location&, const std::string& partname, bool isRef);
    void createShape(const TDF_Label& label, const TopLoc_Location&, const std::string&);
    void createShape(const TopoDS_Shape& label, const TopLoc_Location&, const std::string&);

private:
    Handle_TDocStd_Document pDoc;
    App::Document* doc;
    Handle_XCAFDoc_ShapeTool aShapeTool;
    Handle_XCAFDoc_ColorTool aColorTool;
    std::string default_name;
    std::set<int> myRefShapes;
    static const int HashUpper = INT_MAX;
};

void ImportOCAF::loadShapes()
{
    myRefShapes.clear();
    loadShapes(pDoc->Main(), TopLoc_Location(), default_name, false);
}

void ImportOCAF::loadShapes(const TDF_Label& label, const TopLoc_Location& loc, const std::string& defaultname, bool isRef)
{
    int hash = 0;
    TopoDS_Shape aShape;
    if (aShapeTool->GetShape(label,aShape)) {
        hash = aShape.HashCode(HashUpper);
    }

    Handle(TDataStd_Name) name;
    std::string part_name = defaultname;
    if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
        TCollection_ExtendedString extstr = name->Get();
        char* str = new char[extstr.LengthOfCString()+1];
        extstr.ToUTF8CString(str);
        part_name = str;
        delete [] str;
        if (part_name.empty()) {
            part_name = defaultname;
        }
        else {
            bool ws=true;
            for (std::string::iterator it = part_name.begin(); it != part_name.end(); ++it) {
                if (*it != ' ') {
                    ws = false;
                    break;
                }
            }
            if (ws)
                part_name = defaultname;
        }
    }

    TopLoc_Location part_loc = loc;
    Handle(XCAFDoc_Location) hLoc;
    if (label.FindAttribute(XCAFDoc_Location::GetID(), hLoc)) {
        if (isRef)
            part_loc = part_loc * hLoc->Get();
        else
            part_loc = hLoc->Get();
    }

#ifdef FC_DEBUG
    Base::Console().Message("H:%d, N:%s, T:%d, A:%d, S:%d, C:%d, SS:%d, F:%d, R:%d, C:%d, SS:%d\n",
        hash,
        part_name.c_str(),
        aShapeTool->IsTopLevel(label),
        aShapeTool->IsAssembly(label),
        aShapeTool->IsShape(label),
        aShapeTool->IsCompound(label),
        aShapeTool->IsSimpleShape(label),
        aShapeTool->IsFree(label),
        aShapeTool->IsReference(label),
        aShapeTool->IsComponent(label),
        aShapeTool->IsSubShape(label)
    );
#endif

    TDF_Label ref;
    if (aShapeTool->IsReference(label) && aShapeTool->GetReferredShape(label, ref)) {
        loadShapes(ref, part_loc, part_name, true);
    }

    if (isRef || myRefShapes.find(hash) == myRefShapes.end()) {
        TopoDS_Shape aShape;
        if (isRef && aShapeTool->GetShape(label, aShape))
            myRefShapes.insert(aShape.HashCode(HashUpper));

        if (aShapeTool->IsSimpleShape(label) && (isRef || aShapeTool->IsFree(label))) {
            if (isRef)
                createShape(label, loc, part_name);
            else
                createShape(label, part_loc, part_name);
        }
        else {
            for (TDF_ChildIterator it(label); it.More(); it.Next()) {
                loadShapes(it.Value(), part_loc, part_name, isRef);
            }
        }
    }
}

void ImportOCAF::createShape(const TDF_Label& label, const TopLoc_Location& loc, const std::string& name)
{
    const TopoDS_Shape& aShape = aShapeTool->GetShape(label);
    if (!aShape.IsNull() && aShape.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        int ctSolids = 0, ctShells = 0;
        for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++)
            createShape(xp.Current(), loc, name);
        for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++)
            createShape(xp.Current(), loc, name);
        if (ctSolids > 0 || ctShells > 0)
            return;
    }

    createShape(aShape, loc, name);
}

void ImportOCAF::createShape(const TopoDS_Shape& aShape, const TopLoc_Location& loc, const std::string& name)
{
    Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature"));
    if (!loc.IsIdentity())
        part->Shape.setValue(aShape.Moved(loc));
    else
        part->Shape.setValue(aShape);
    part->Label.setValue(name);

    Quantity_Color aColor;
    App::Color color(0.8f,0.8f,0.8f);
    if (aColorTool->GetColor(aShape, XCAFDoc_ColorGen, aColor) ||
        aColorTool->GetColor(aShape, XCAFDoc_ColorSurf, aColor) ||
        aColorTool->GetColor(aShape, XCAFDoc_ColorCurv, aColor)) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPart::getClassTypeId())) {
            color.r = aColor.Red();
            color.g = aColor.Green();
            color.b = aColor.Blue();
            static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.setValue(color);
        }
    }

    TopTools_IndexedMapOfShape faces;
    TopExp_Explorer xp(aShape,TopAbs_FACE);
    while (xp.More()) {
        faces.Add(xp.Current());
        xp.Next();
    }
    bool found_face_color = false;
    std::vector<App::Color> faceColors;
    faceColors.resize(faces.Extent(), color);
    xp.Init(aShape,TopAbs_FACE);
    while (xp.More()) {
        if (aColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, aColor) ||
            aColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, aColor) ||
            aColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, aColor)) {
            int index = faces.FindIndex(xp.Current());
            color.r = aColor.Red();
            color.g = aColor.Green();
            color.b = aColor.Blue();
            faceColors[index-1] = color;
            found_face_color = true;
        }
        xp.Next();
    }

    if (found_face_color) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
            static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.setValues(faceColors);
        }
    }
}

class ImportXCAF
{
public:
    ImportXCAF(Handle_TDocStd_Document h, App::Document* d, const std::string& name)
        : hdoc(h), doc(d), default_name(name)
    {
        aShapeTool = XCAFDoc_DocumentTool::ShapeTool (hdoc->Main());
        hColors = XCAFDoc_DocumentTool::ColorTool(hdoc->Main());
    }

    void loadShapes()
    {
        // collect sequence of labels to display
        TDF_LabelSequence shapeLabels, colorLabels;
        aShapeTool->GetFreeShapes (shapeLabels);
        hColors->GetColors(colorLabels);

        // set presentations and show
        for (Standard_Integer i=1; i <= shapeLabels.Length(); i++ ) {
            // get the shapes and attributes
            const TDF_Label& label = shapeLabels.Value(i);
            loadShapes(label);
        }
        std::map<Standard_Integer, TopoDS_Shape>::iterator it;
        // go through solids
        for (it = mySolids.begin(); it != mySolids.end(); ++it) {
            createShape(it->second, true, true);
        }
        // go through shells
        for (it = myShells.begin(); it != myShells.end(); ++it) {
            createShape(it->second, true, true);
        }
        // go through compounds
        for (it = myCompds.begin(); it != myCompds.end(); ++it) {
            createShape(it->second, true, true);
        }
        // do the rest
        if (!myShapes.empty()) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            for (it = myShapes.begin(); it != myShapes.end(); ++it) {
                builder.Add(comp, it->second);
            }
            createShape(comp, true, false);
        }
    }

private:
    void createShape(const TopoDS_Shape& shape, bool perface=false, bool setname=false) const
    {
        Part::Feature* part;
        part = static_cast<Part::Feature*>(doc->addObject("Part::Feature", default_name.c_str()));
        part->Shape.setValue(shape);
        std::map<Standard_Integer, Quantity_Color>::const_iterator jt;
        jt = myColorMap.find(shape.HashCode(INT_MAX));

        App::Color partColor(0.8f,0.8f,0.8f);
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPart::getClassTypeId())) {
            if (jt != myColorMap.end()) {
                App::Color color;
                color.r = jt->second.Red();
                color.g = jt->second.Green();
                color.b = jt->second.Blue();
                static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.setValue(color);
            }

            partColor = static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.getValue();
        }

        // set label name if defined
        if (setname && !myNameMap.empty()) {
            std::map<Standard_Integer, std::string>::const_iterator jt;
            jt = myNameMap.find(shape.HashCode(INT_MAX));
            if (jt != myNameMap.end()) {
                part->Label.setValue(jt->second);
            }
        }

        // check for colors per face
        if (perface && !myColorMap.empty()) {
            TopTools_IndexedMapOfShape faces;
            TopExp_Explorer xp(shape,TopAbs_FACE);
            while (xp.More()) {
                faces.Add(xp.Current());
                xp.Next();
            }

            bool found_face_color = false;
            std::vector<App::Color> faceColors;
            faceColors.resize(faces.Extent(), partColor);
            xp.Init(shape,TopAbs_FACE);
            while (xp.More()) {
                jt = myColorMap.find(xp.Current().HashCode(INT_MAX));
                if (jt != myColorMap.end()) {
                    int index = faces.FindIndex(xp.Current());
                    App::Color color;
                    color.r = jt->second.Red();
                    color.g = jt->second.Green();
                    color.b = jt->second.Blue();
                    faceColors[index-1] = color;
                    found_face_color = true;
                }
                xp.Next();
            }

            if (found_face_color) {
                Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
                if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
                    static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.setValues(faceColors);
                }
            }
        }
    }
    void loadShapes(const TDF_Label& label)
    {
        TopoDS_Shape aShape;
        if (aShapeTool->GetShape(label,aShape)) {
            //if (aShapeTool->IsReference(label)) {
            //    TDF_Label reflabel;
            //    if (aShapeTool->GetReferredShape(label, reflabel)) {
            //        loadShapes(reflabel);
            //    }
            //}
            if (aShapeTool->IsTopLevel(label)) {
                int ctSolids = 0, ctShells = 0, ctComps = 0;
                // add the shapes
                TopExp_Explorer xp;
                for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++)
                    this->mySolids[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++)
                    this->myShells[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                // if no solids and no shells were found then go for compounds
                if (ctSolids == 0 && ctShells == 0) {
                    for (xp.Init(aShape, TopAbs_COMPOUND); xp.More(); xp.Next(), ctComps++)
                        this->myCompds[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                }
                if (ctComps == 0) {
                    for (xp.Init(aShape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next())
                        this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                    for (xp.Init(aShape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next())
                        this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                    for (xp.Init(aShape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next())
                        this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                    for (xp.Init(aShape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next())
                        this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                }
            }

            // getting color
            Quantity_Color col;
            if (hColors->GetColor(label, XCAFDoc_ColorGen, col) ||
                hColors->GetColor(label, XCAFDoc_ColorSurf, col) ||
                hColors->GetColor(label, XCAFDoc_ColorCurv, col)) {
                // add defined color
                myColorMap[aShape.HashCode(INT_MAX)] = col;
            }
            else {
                // http://www.opencascade.org/org/forum/thread_17107/
                TopoDS_Iterator it;
                for (it.Initialize(aShape);it.More(); it.Next()) {
                    if (hColors->GetColor(it.Value(), XCAFDoc_ColorGen, col) ||
                        hColors->GetColor(it.Value(), XCAFDoc_ColorSurf, col) ||
                        hColors->GetColor(it.Value(), XCAFDoc_ColorCurv, col)) {
                        // add defined color
                        myColorMap[it.Value().HashCode(INT_MAX)] = col;
                    }
                }
            }

            // getting names
            Handle(TDataStd_Name) name;
            if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
                TCollection_ExtendedString extstr = name->Get();
                char* str = new char[extstr.LengthOfCString()+1];
                extstr.ToUTF8CString(str);
                std::string label(str);
                if (!label.empty())
                    myNameMap[aShape.HashCode(INT_MAX)] = label;
                delete [] str;
            }

#if 0
            // http://www.opencascade.org/org/forum/thread_15174/
            if (aShapeTool->IsAssembly(label)) {
                TDF_LabelSequence shapeLabels;
                aShapeTool->GetComponents(label, shapeLabels);
                Standard_Integer nbShapes = shapeLabels.Length();
                for (Standard_Integer i = 1; i <= nbShapes; i++) {
                    loadShapes(shapeLabels.Value(i));
                }
            }
#endif

            if (label.HasChild()) {
                TDF_ChildIterator it;
                for (it.Initialize(label); it.More(); it.Next()) {
                    loadShapes(it.Value());
                }
            }
        }
    }

private:
    Handle_TDocStd_Document hdoc;
    App::Document* doc;
    Handle_XCAFDoc_ShapeTool aShapeTool;
    Handle_XCAFDoc_ColorTool hColors;
    std::string default_name;
    std::map<Standard_Integer, TopoDS_Shape> mySolids;
    std::map<Standard_Integer, TopoDS_Shape> myShells;
    std::map<Standard_Integer, TopoDS_Shape> myCompds;
    std::map<Standard_Integer, TopoDS_Shape> myShapes;
    std::map<Standard_Integer, Quantity_Color> myColorMap;
    std::map<Standard_Integer, std::string> myNameMap;
};

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
        else if (file.hasExtension("igs") || file.hasExtension("iges")) {
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
        else {
            PyErr_SetString(PyExc_Exception, "no supported file format");
            return 0;
        }

#if 1
        ImportOCAF ocaf(hDoc, pcDoc, file.fileNamePure());
        ocaf.loadShapes();
#else
        ImportXCAF xcaf(hDoc, pcDoc, file.fileNamePure());
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
        Handle_XCAFDoc_ShapeTool hShapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
        Handle_XCAFDoc_ColorTool hColors = XCAFDoc_DocumentTool::ColorTool(hDoc->Main());

        TDF_Label rootLabel= TDF_TagSource::NewChild(hDoc->Main());

        Py::List list(object);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                    Part::Feature* part = static_cast<Part::Feature*>(obj);
                    const TopoDS_Shape& shape = part->Shape.getValue();

                    // Add shape and name
                    //TDF_Label shapeLabel = hShapeTool->AddShape(shape, Standard_False);
                    TDF_Label shapeLabel= TDF_TagSource::NewChild(rootLabel);
#if OCC_VERSION_HEX >= 0x060500
                    TDataXtd_Shape::Set(shapeLabel, shape);
#else
                    TDataStd_Shape::Set(shapeLabel, shape);
#endif
                    TDataStd_Name::Set(shapeLabel, TCollection_ExtendedString(part->Label.getValue(), 1));

                    // Add color information
                    Quantity_Color col;
                    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
                    bool per_face = false;
                    if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
                        const std::vector<App::Color>& c = static_cast<PartGui::ViewProviderPartExt*>
                            (vp)->DiffuseColor.getValues();
                        // define color per face
                        if (c.size() > 1) {
                            per_face = true;
                            std::set<int> face_index;
                            TopTools_IndexedMapOfShape faces;
                            TopExp_Explorer xp(shape,TopAbs_FACE);
                            while (xp.More()) {
                                face_index.insert(faces.Add(xp.Current()));
                                xp.Next();
                            }

                            xp.Init(shape,TopAbs_FACE);
                            while (xp.More()) {
                                int index = faces.FindIndex(xp.Current());
                                if (face_index.find(index) != face_index.end()) {
                                    face_index.erase(index);
                                    TDF_Label faceLabel= TDF_TagSource::NewChild(shapeLabel);
#if OCC_VERSION_HEX >= 0x060500
                                    TDataXtd_Shape::Set(faceLabel, xp.Current());
#else
                                    TDataStd_Shape::Set(faceLabel, xp.Current());
#endif
                                    const App::Color& color = c[index-1];
                                    Quantity_Parameter mat[3];
                                    mat[0] = color.r;
                                    mat[1] = color.g;
                                    mat[2] = color.b;
                                    col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
                                    hColors->SetColor(faceLabel, col, XCAFDoc_ColorSurf);
                                }
                                xp.Next();
                            }
                        }
                    }
                    if (!per_face && vp && vp->isDerivedFrom(PartGui::ViewProviderPart::getClassTypeId())) {
                        App::Color color = static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.getValue();
                        Quantity_Parameter mat[3];
                        mat[0] = color.r;
                        mat[1] = color.g;
                        mat[2] = color.b;
                        col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
                        hColors->SetColor(shapeLabel, col, XCAFDoc_ColorGen);
                    }
                }
                else {
                    Base::Console().Message("'%s' is not a shape, export will be ignored.\n", obj->Label.getValue());
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

#include <TDataStd.hxx>
#include <TDataStd_Integer.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDF_ChildIDIterator.hxx>
#include <TDF_Data.hxx>
#include <TDF_IDList.hxx>
#include <TDF_ListIteratorOfIDList.hxx>
#include <TDF_TagSource.hxx>
#include <TDocStd_Owner.hxx>
#include <TNaming_NamedShape.hxx>
#include <TNaming_UsedShapes.hxx>
#include <XCAFDoc.hxx>
#include <XCAFDoc_Color.hxx>
#include <XCAFDoc_LayerTool.hxx>
#include <XCAFDoc_ShapeMapTool.hxx>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>
#include <QStyle>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextStream>
#include <QHBoxLayout>
#include <QVBoxLayout>

class OCAFBrowser
{
public:
    OCAFBrowser(Handle_TDocStd_Document h)
        : pDoc(h)
    {
        myGroupIcon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);

        TDataStd::IDList(myList);
        myList.Append(TDataStd_TreeNode::GetDefaultTreeID());
        myList.Append(TDataStd_Integer::GetID());
        myList.Append(TDocStd_Owner::GetID());
        myList.Append(TNaming_NamedShape::GetID());
        myList.Append(TNaming_UsedShapes::GetID());
        myList.Append(XCAFDoc_Color::GetID());
        myList.Append(XCAFDoc_ColorTool::GetID());
        myList.Append(XCAFDoc_LayerTool::GetID());
        myList.Append(XCAFDoc_ShapeTool::GetID());
        myList.Append(XCAFDoc_ShapeMapTool::GetID());
        myList.Append(XCAFDoc_Location::GetID());
    }

    void load(QTreeWidget*);

private:
    void load(const TDF_Label& label, QTreeWidgetItem* item, const QString&);
    std::string toString(const TCollection_ExtendedString& extstr) const
    {
        char* str = new char[extstr.LengthOfCString()+1];
        extstr.ToUTF8CString(str);
        std::string text(str);
        delete [] str;
        return text;
    }

private:
    QIcon myGroupIcon;
    TDF_IDList myList;
    Handle_TDocStd_Document pDoc;
};

void OCAFBrowser::load(QTreeWidget* theTree)
{
    theTree->clear();

    QTreeWidgetItem* root = new QTreeWidgetItem();
    root->setText(0, QLatin1String("0"));
    root->setIcon(0, myGroupIcon);
    theTree->addTopLevelItem(root);

    load(pDoc->GetData()->Root(), root, QString::fromAscii("0"));
}

void OCAFBrowser::load(const TDF_Label& label, QTreeWidgetItem* item, const QString& s)
{
    Handle(TDataStd_Name) name;
    if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
        QString text = QString::fromAscii("%1 %2").arg(s).arg(QString::fromUtf8(toString(name->Get()).c_str()));
        item->setText(0, text);
    }

    for (TDF_ListIteratorOfIDList it(myList); it.More(); it.Next()) {
        Handle(TDF_Attribute) attr;
        if (label.FindAttribute(it.Value(), attr)) {
            QTreeWidgetItem* child = new QTreeWidgetItem();
            item->addChild(child);
            if (it.Value() == TDataStd_Name::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << toString(Handle_TDataStd_Name::DownCast(attr)->Get()).c_str();
                child->setText(0, text);
            }
            else if (it.Value() == TDF_TagSource::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << Handle_TDF_TagSource::DownCast(attr)->Get();
                child->setText(0, text);
            }
            else if (it.Value() == TDataStd_Integer::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << Handle_TDataStd_Integer::DownCast(attr)->Get();
                child->setText(0, text);
            }
            else if (it.Value() == TNaming_NamedShape::GetID()) {
                TopoDS_Shape shape = Handle_TNaming_NamedShape::DownCast(attr)->Get();
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name() << " = ";
                if (!shape.IsNull()) {
                    switch (shape.ShapeType()) {
                    case TopAbs_COMPOUND:
                        str << "COMPOUND PRIMITIVE";
                        break;
                    case TopAbs_COMPSOLID:
                        str << "COMPSOLID PRIMITIVE";
                        break;
                    case TopAbs_SOLID:
                        str << "SOLID PRIMITIVE";
                        break;
                    case TopAbs_SHELL:
                        str << "SHELL PRIMITIVE";
                        break;
                    case TopAbs_FACE:
                        str << "FACE PRIMITIVE";
                        break;
                    case TopAbs_WIRE:
                        str << "WIRE PRIMITIVE";
                        break;
                    case TopAbs_EDGE:
                        str << "EDGE PRIMITIVE";
                        break;
                    case TopAbs_VERTEX:
                        str << "VERTEX PRIMITIVE";
                        break;
                    case TopAbs_SHAPE:
                        str << "SHAPE PRIMITIVE";
                        break;
                    }
                }
                child->setText(0, text);
            }
            else {
                child->setText(0, QLatin1String(attr->DynamicType()->Name()));
            }
        }
    }

    //TDF_ChildIDIterator nodeIterator(label, XCAFDoc::ShapeRefGUID());
    //for (; nodeIterator.More(); nodeIterator.Next()) {
    //    Handle(TDataStd_TreeNode) node = Handle(TDataStd_TreeNode)::DownCast(nodeIterator.Value());
    //    //if (node->HasFather())
    //    //    ;
    //    QTreeWidgetItem* child = new QTreeWidgetItem();
    //    child->setText(0, QString::fromAscii("TDataStd_TreeNode"));
    //    item->addChild(child);
    //}

    int i=1;
    for (TDF_ChildIterator it(label); it.More(); it.Next(),i++) {
        QString text = QString::fromAscii("%1:%2").arg(s).arg(i);
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, text);
        child->setIcon(0, myGroupIcon);
        item->addChild(child);
        load(it.Value(), child, text);
    }
}

static PyObject * ocaf(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return 0;

    PY_TRY {
        //Base::Console().Log("Insert in Part with %s",Name);
        Base::FileInfo file(Name);

        Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
        Handle(TDocStd_Document) hDoc;
        hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

        if (file.hasExtension("stp") || file.hasExtension("step")) {
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
        else if (file.hasExtension("igs") || file.hasExtension("iges")) {
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
        else {
            PyErr_SetString(PyExc_Exception, "no supported file format");
            return 0;
        }

        static QPointer<QDialog> dlg = 0;
        if (!dlg) {
            dlg = new QDialog(Gui::getMainWindow());
            QTreeWidget* tree = new QTreeWidget();
            tree->setHeaderLabel(QString::fromAscii("OCAF Browser"));

            QVBoxLayout *layout = new QVBoxLayout;
            layout->addWidget(tree);
            dlg->setLayout(layout);

            QDialogButtonBox* btn = new QDialogButtonBox(dlg);
            btn->setStandardButtons(QDialogButtonBox::Close);
            QObject::connect(btn, SIGNAL(rejected()), dlg, SLOT(reject()));
            QHBoxLayout *boxlayout = new QHBoxLayout;
            boxlayout->addWidget(btn);
            layout->addLayout(boxlayout);
        }

        dlg->setWindowTitle(QString::fromUtf8(file.fileName().c_str()));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();

        OCAFBrowser browse(hDoc);
        browse.load(dlg->findChild<QTreeWidget*>());
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
struct PyMethodDef ImportGui_Import_methods[] = {
    {"open"     ,open  ,METH_VARARGS,
     "open(string) -- Open the file and create a new document."},
    {"insert"     ,importer  ,METH_VARARGS,
     "insert(string,string) -- Insert the file into the given document."},
    {"export"     ,exporter  ,METH_VARARGS,
     "export(list,string) -- Export a list of objects into a single file."},
    {"ocaf"       ,ocaf  ,METH_VARARGS,
     "ocaf(string) -- Browse the ocaf structure."},
    {NULL, NULL}                   /* end of table marker */
};
