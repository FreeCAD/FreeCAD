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
# include <iostream>
# include <climits>
# include <QString>
# include <Standard_Version.hxx>
# include <NCollection_Vector.hxx>
# include <BRep_Builder.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
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
# include <IGESData_GlobalSection.hxx>
# include <IGESData_IGESModel.hxx>
# include <IGESToBRep_Actor.hxx>
# include <Interface_Static.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_WorkSession.hxx>
# include <XSControl_TransferReader.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Iterator.hxx>
# include <APIHeaderSection_MakeHeader.hxx>
# include <OSD_Exception.hxx>
#if OCC_VERSION_HEX >= 0x060500
# include <TDataXtd_Shape.hxx>
# else
# include <TDataStd_Shape.hxx>
# endif
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

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
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>
#include <Mod/Part/App/encodeFilename.h>
#include <Mod/Import/App/ImportOCAF2.h>

#include <TDataStd.hxx>
#include <TDataStd_Integer.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDF_ChildIDIterator.hxx>
#include <TDF_AttributeIterator.hxx>
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

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderLink.h>


FC_LOG_LEVEL_INIT("Import", true, true)

class OCAFBrowser
{
public:
    OCAFBrowser(Handle(TDocStd_Document) h)
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
    Handle(TDocStd_Document) pDoc;
};

void OCAFBrowser::load(QTreeWidget* theTree)
{
    theTree->clear();

    QTreeWidgetItem* root = new QTreeWidgetItem();
    root->setText(0, QLatin1String("0"));
    root->setIcon(0, myGroupIcon);
    theTree->addTopLevelItem(root);

    load(pDoc->GetData()->Root(), root, QString::fromLatin1("0"));
}

void OCAFBrowser::load(const TDF_Label& label, QTreeWidgetItem* item, const QString& s)
{
    label.Dump(std::cout);

    Handle(TDataStd_Name) name;
    if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
        QString text = QString::fromLatin1("%1 %2").arg(s).arg(QString::fromUtf8(toString(name->Get()).c_str()));
        item->setText(0, text);
    }

#if 0
    TDF_IDList localList = myList;
#else
    TDF_IDList localList;
    TDF_AttributeIterator itr (label);
    for ( ; itr.More(); itr.Next()) {
        localList.Append(itr.Value()->ID());
    }
#endif

    for (TDF_ListIteratorOfIDList it(localList); it.More(); it.Next()) {
        Handle(TDF_Attribute) attr;
        if (label.FindAttribute(it.Value(), attr)) {
            QTreeWidgetItem* child = new QTreeWidgetItem();
            item->addChild(child);
            if (it.Value() == TDataStd_Name::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << toString(Handle(TDataStd_Name)::DownCast(attr)->Get()).c_str();
                child->setText(0, text);
            }
            else if (it.Value() == TDF_TagSource::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << Handle(TDF_TagSource)::DownCast(attr)->Get();
                child->setText(0, text);
            }
            else if (it.Value() == TDataStd_Integer::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << Handle(TDataStd_Integer)::DownCast(attr)->Get();
                child->setText(0, text);
            }
            else if (it.Value() == TNaming_NamedShape::GetID()) {
                TopoDS_Shape shape = Handle(TNaming_NamedShape)::DownCast(attr)->Get();
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
    //    child->setText(0, QString::fromLatin1("TDataStd_TreeNode"));
    //    item->addChild(child);
    //}

    int i=1;
    for (TDF_ChildIterator it(label); it.More(); it.Next(),i++) {
        QString text = QString::fromLatin1("%1:%2").arg(s).arg(i);
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, text);
        child->setIcon(0, myGroupIcon);
        item->addChild(child);
        load(it.Value(), child, text);
    }
}

class ImportOCAFExt : public Import::ImportOCAF2
{
public:
    ImportOCAFExt(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
        : ImportOCAF2(h, d, name)
    {
    }

private:
    virtual void applyFaceColors(Part::Feature* part, const std::vector<App::Color>& colors) override {
        auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(part));
        if (!vp) return;
        if(colors.empty()) {
            vp->MapFaceColor.setValue(true);
            vp->MapLineColor.setValue(true);
            vp->updateColors(part,0,true);
            return;
        }
        vp->MapFaceColor.setValue(false);
        if(colors.size() == 1)
            vp->ShapeColor.setValue(colors.front());
        else 
            vp->DiffuseColor.setValues(colors);
    }
    virtual void applyEdgeColors(Part::Feature* part, const std::vector<App::Color>& colors) override {
        auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(part));
        if (!vp) return;
        vp->MapLineColor.setValue(false);
        if(colors.size() == 1)
            vp->LineColor.setValue(colors.front());
        else
            vp->LineColorArray.setValues(colors);
    }
    virtual void applyLinkColor(App::DocumentObject *obj, int index, App::Color color) override {
        auto vp = dynamic_cast<Gui::ViewProviderLink*>(Gui::Application::Instance->getViewProvider(obj));
        if(!vp)
            return;
        if(index<0) {
            vp->OverrideMaterial.setValue(true);
            vp->ShapeMaterial.setDiffuseColor(color);
            return;
        }
        if(vp->OverrideMaterialList.getSize()<=index)
            vp->OverrideMaterialList.setSize(index+1);
        vp->OverrideMaterialList.set1Value(index,true,false);
        App::Material mat(App::Material::DEFAULT);
        if(vp->MaterialList.getSize()<=index)
            vp->MaterialList.setSize(index+1,mat);
        mat.diffuseColor = color;
        vp->MaterialList.set1Value(index,mat,true);
    }
    virtual void applyElementColors(App::DocumentObject *obj, 
            const std::map<std::string,App::Color> &colors) override 
    {
        auto vp = Gui::Application::Instance->getViewProvider(obj);
        if(!vp)
            return;
        vp->setElementColors(colors);
    }
};

class ExportOCAFGui : public Import::ExportOCAF
{
public:
    ExportOCAFGui(Handle(TDocStd_Document) h, bool explicitPlacement)
        : ExportOCAF(h, explicitPlacement)
    {
    }
    virtual void findColors(Part::Feature* part, std::vector<App::Color>& colors) const
    {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
            colors = static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.getValues();
            if (colors.empty())
                colors.push_back(static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.getValue());
        }
    }
};

namespace ImportGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("ImportGui")
    {
        add_keyword_method("open",&Module::insert,
            "open(string) -- Open the file and create a new document."
        );
        add_keyword_method("insert",&Module::insert,
            "insert(string,string) -- Insert the file into the given document."
        );
        add_keyword_method("export",&Module::exporter,
            "export(list,string) -- Export a list of objects into a single file."
        );
        add_varargs_method("ocaf",&Module::ocaf,
            "ocaf(string) -- Browse the ocaf structure."
        );
        initialize("This module is the ImportGui module."); // register with Python
    }

    virtual ~Module() {}

private:
    class Signaler {
    public:
        Signaler() {
            App::GetApplication().signalStartOpenDocument();
        }
        ~Signaler() {
            App::GetApplication().signalFinishOpenDocument();
        }
    };
    Py::Object insert(const Py::Tuple& args, const Py::Dict &kwds)
    {
        char* Name;
        char* DocName=0;
        PyObject *importHidden = Py_None;
        PyObject *merge = Py_None;
        PyObject *useLinkGroup = Py_None;
        static char* kwd_list[] = {"name","docName","importHidden","merge","useLinkGroup",0};
        if(!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "et|sOOO", 
                    kwd_list,"utf-8",&Name,&DocName,&importHidden,&merge,&useLinkGroup))
            throw Py::Exception();

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);
        std::string name8bit = Part::encodeFilename(Utf8Name);

        Signaler signaler;

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
            bool optionReadShapeCompoundMode = true;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Import/hSTEP");
            optionReadShapeCompoundMode = hGrp->GetBool("ReadShapeCompoundMode", optionReadShapeCompoundMode);
            ImportOCAFExt ocaf(hDoc, pcDoc, file.fileNamePure());
            FC_TIME_INIT(t);
            FC_DURATION_DECL_INIT2(d1,d2);

            if (file.hasExtension("stp") || file.hasExtension("step")) {
                try {
                    STEPCAFControl_Reader aReader;
                    aReader.SetColorMode(true);
                    aReader.SetNameMode(true);
                    aReader.SetLayerMode(true);
                    aReader.SetSHUOMode(true);
                    if (aReader.ReadFile((const char*)name8bit.c_str()) != IFSelect_RetDone) {
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
                    if (aReader.ReadFile((const char*)name8bit.c_str()) != IFSelect_RetDone) {
                        throw Py::Exception(Base::BaseExceptionFreeCADError, "cannot read IGES file");
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

            FC_DURATION_PLUS(d1,t);
            if(merge!=Py_None)
                ocaf.setMerge(PyObject_IsTrue(merge));
            if(importHidden!=Py_None)
                ocaf.setImportHiddenObject(PyObject_IsTrue(importHidden));
            if(useLinkGroup!=Py_None)
                ocaf.setUseLinkGroup(PyObject_IsTrue(useLinkGroup));
            ocaf.loadShapes();
            pcDoc->purgeTouched();
            pcDoc->recompute();
            hApp->Close(hDoc);
            FC_DURATION_PLUS(d2,t);
            FC_DURATION_MSG(d1,"file read");
            FC_DURATION_MSG(d2,"import");
            FC_DURATION_MSG((d1+d2),"total");
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    static std::map<std::string, App::Color> getShapeColors(App::DocumentObject *obj, const char *subname) {
        auto vp = Gui::Application::Instance->getViewProvider(obj);
        if(vp)
            return vp->getElementColors(subname);
        return std::map<std::string,App::Color>();
    }

    Py::Object exporter(const Py::Tuple& args, const Py::Dict &kwds)
    {
        PyObject* object;
        char* Name;
        PyObject *exportHidden = Py_None;
        static char* kwd_list[] = {"obj", "name", "exportHidden",0};
        if(!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "Oet|O",
                    kwd_list,&object,"utf-8",&Name,&exportHidden))
            throw Py::Exception();

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);
        std::string name8bit = Part::encodeFilename(Utf8Name);

        try {
            Py::Sequence list(object);
            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

            std::vector<App::DocumentObject *> objs;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type)))
                    objs.push_back(static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr());
            }

            Import::ExportOCAF2 ocaf(hDoc, &getShapeColors);
            if(exportHidden!=Py_None)
                ocaf.setExportHiddenObject(PyObject_IsTrue(exportHidden));
            if(!ocaf.canFallback(objs)) 
                ocaf.exportObjects(objs);
            else {
                bool keepExplicitPlacement = objs.size() > 1;
                keepExplicitPlacement = Standard_True;
                ExportOCAFGui ocaf(hDoc, keepExplicitPlacement);
                // That stuff is exporting a list of selected objects into FreeCAD Tree
                std::vector <TDF_Label> hierarchical_label;
                std::vector <TopLoc_Location> hierarchical_loc;
                std::vector <App::DocumentObject*> hierarchical_part;
                for(auto obj : objs)
                    ocaf.exportObject(obj,hierarchical_label, hierarchical_loc,hierarchical_part);

                // Free Shapes must have absolute placement and not explicit
                std::vector <TDF_Label> FreeLabels;
                std::vector <int> part_id;
                ocaf.getFreeLabels(hierarchical_label,FreeLabels, part_id);
                // Got issue with the colors as they are coming from the View Provider they can't be determined into
                // the App Code.
                std::vector< std::vector<App::Color> > Colors;
                ocaf.getPartColors(hierarchical_part,FreeLabels,part_id,Colors);
                ocaf.reallocateFreeShape(hierarchical_part,FreeLabels,part_id,Colors);

#if OCC_VERSION_HEX >= 0x070200
            // Update is not performed automatically anymore: https://tracker.dev.opencascade.org/view.php?id=28055
                XCAFDoc_DocumentTool::ShapeTool(hDoc->Main())->UpdateAssemblies();
#endif
            }

            Base::FileInfo file(Utf8Name.c_str());
            if (file.hasExtension("stp") || file.hasExtension("step")) {
                ParameterGrp::handle hGrp_stp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part/STEP");
                std::string scheme = hGrp_stp->GetASCII("Scheme", "AP214IS");
                if (scheme == "AP203")
                    Interface_Static::SetCVal("write.step.schema", "AP203");
                else if (scheme == "AP214IS")
                    Interface_Static::SetCVal("write.step.schema", "AP214IS");

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
                Standard_Boolean ret = writer.Write((const char*)name8bit.c_str());
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
    Py::Object ocaf(const Py::Tuple& args)
    {
        const char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "s",&Name))
            throw Py::Exception();

        try {
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
                    aReader.SetSHUOMode(true);
                if (aReader.ReadFile((Standard_CString)Name) != IFSelect_RetDone) {
                    throw Py::Exception(PyExc_IOError, "cannot read STEP file");
                }

                Handle(Message_ProgressIndicator) pi = new Part::ProgressIndicator(100);
                aReader.Reader().WS()->MapReader()->SetProgress(pi);
                pi->NewScope(100, "Reading STEP file...");
                pi->Show();
                aReader.Transfer(hDoc);
                pi->EndScope();
            }
            else if (file.hasExtension("igs") || file.hasExtension("iges")) {
                Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                    .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("IGES");
                IGESControl_Controller::Init();
                IGESCAFControl_Reader aReader;
                // http://www.opencascade.org/org/forum/thread_20603/?forum=3
                aReader.SetReadVisible(hGrp->GetBool("SkipBlankEntities", true)
                    ? Standard_True : Standard_False);
                aReader.SetColorMode(true);
                aReader.SetNameMode(true);
                aReader.SetLayerMode(true);
                if (aReader.ReadFile((Standard_CString)Name) != IFSelect_RetDone) {
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
            else {
                throw Py::Exception(Base::BaseExceptionFreeCADError, "no supported file format");
            }

            static QPointer<QDialog> dlg = 0;
            if (!dlg) {
                dlg = new QDialog(Gui::getMainWindow());
                QTreeWidget* tree = new QTreeWidget();
                tree->setHeaderLabel(QString::fromLatin1("OCAF Browser"));

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
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace ImportGui
