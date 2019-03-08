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
#include <Mod/Import/App/ImportOCAF.h>
#include <Mod/Import/App/ExportOCAF.h>

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

class ImportOCAFGui : public Import::ImportOCAF
{
public:
    ImportOCAFGui(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
        : ImportOCAF(h, d, name)
    {
    }

private:
    void applyColors(Part::Feature* part, const std::vector<App::Color>& colors)
    {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
            static_cast<PartGui::ViewProviderPartExt*>(vp)->ShapeColor.setValue(colors.front());
            static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.setValues(colors);
        }
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
        add_varargs_method("open",&Module::open,
            "open(string) -- Open the file and create a new document."
        );
        add_varargs_method("insert",&Module::insert,
            "insert(string,string) -- Insert the file into the given document."
        );
        add_varargs_method("export",&Module::exporter,
            "export(list,string) -- Export a list of objects into a single file."
        );
        add_varargs_method("ocaf",&Module::ocaf,
            "ocaf(string) -- Browse the ocaf structure."
        );
        initialize("This module is the ImportGui module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        return insert(args);
    }
    Py::Object insert(const Py::Tuple& args)
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
            bool optionReadShapeCompoundMode = true;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Import/hSTEP");
            optionReadShapeCompoundMode = hGrp->GetBool("ReadShapeCompoundMode", optionReadShapeCompoundMode);

            if (file.hasExtension("stp") || file.hasExtension("step")) {
                try {
                    STEPCAFControl_Reader aReader;
                    aReader.SetColorMode(true);
                    aReader.SetNameMode(true);
                    aReader.SetLayerMode(true);
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

            ImportOCAFGui ocaf(hDoc, pcDoc, file.fileNamePure());
            // We must recompute the doc before loading shapes as they are going to be
            // inserted into the document and computed at the same time so we are going to
            // purge the document before recomputing it to clear it and settle it in the proper
            // way. This is drastically improving STEP rendering time on complex STEP files.
            pcDoc->recompute();
            if (file.hasExtension("stp") || file.hasExtension("step"))
                ocaf.setMerge(optionReadShapeCompoundMode);
            ocaf.loadShapes();
            pcDoc->purgeTouched();
            pcDoc->recompute();
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

            //bool keepExplicitPlacement = list.size() > 1;
            bool keepExplicitPlacement = Standard_True;
            ExportOCAFGui ocaf(hDoc, keepExplicitPlacement);

            // That stuff is exporting a list of selected objects into FreeCAD Tree
            std::vector <TDF_Label> hierarchical_label;
            std::vector <TopLoc_Location> hierarchical_loc;
            std::vector <App::DocumentObject*> hierarchical_part;

            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                    ocaf.exportObject(obj, hierarchical_label, hierarchical_loc, hierarchical_part);
                }
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
