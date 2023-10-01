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
#define WNT  // avoid conflict with GUID
#endif
#ifndef _PreComp_
#include <climits>
#include <iostream>

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPointer>
#include <QString>
#include <QStyle>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#endif

#include <Interface_Static.hxx>
#include <OSD_Exception.hxx>
#include <Standard_Version.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#include <TDF_AttributeIterator.hxx>
#include <TDF_ChildIDIterator.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_IDList.hxx>
#include <TDF_Label.hxx>
#include <TDF_ListIteratorOfIDList.hxx>
#include <TDF_TagSource.hxx>
#include <TDataStd.hxx>
#include <TDataStd_Integer.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDataXtd_Shape.hxx>
#include <TDocStd_Document.hxx>
#include <TDocStd_Owner.hxx>
#include <TNaming_NamedShape.hxx>
#include <TNaming_UsedShapes.hxx>
#include <Transfer_TransientProcess.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_Color.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_LayerTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <XCAFDoc_ShapeMapTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>

#if OCC_VERSION_HEX >= 0x070500
#include <Message_ProgressRange.hxx>
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif

#include "ExportOCAFGui.h"

#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <Base/Console.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderLink.h>
#include <Mod/Import/App/ImportOCAF2.h>
#include <Mod/Import/App/ReaderGltf.h>
#include <Mod/Import/App/ReaderIges.h>
#include <Mod/Import/App/ReaderStep.h>
#include <Mod/Import/App/WriterGltf.h>
#include <Mod/Import/App/WriterIges.h>
#include <Mod/Import/App/WriterStep.h>
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>
#include <Mod/Part/App/Interface.h>
#include <Mod/Part/App/OCAF/ImportExportSettings.h>
#include <Mod/Part/App/ProgressIndicator.h>
#include <Mod/Part/App/encodeFilename.h>
#include <Mod/Part/Gui/DlgExportStep.h>
#include <Mod/Part/Gui/ViewProvider.h>


FC_LOG_LEVEL_INIT("Import", true, true)

namespace ImportGui
{
class OCAFBrowser
{
public:
    explicit OCAFBrowser(Handle(TDocStd_Document) h)
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
        char* str = new char[extstr.LengthOfCString() + 1];
        extstr.ToUTF8CString(str);
        std::string text(str);
        delete[] str;
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
    if (label.FindAttribute(TDataStd_Name::GetID(), name)) {
        QString text =
            QString::fromLatin1("%1 %2").arg(s, QString::fromUtf8(toString(name->Get()).c_str()));
        item->setText(0, text);
    }


    TDF_IDList localList;
    TDF_AttributeIterator itr(label);
    for (; itr.More(); itr.Next()) {
        localList.Append(itr.Value()->ID());
    }

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


    int i = 1;
    for (TDF_ChildIterator it(label); it.More(); it.Next(), i++) {
        QString text = QString::fromLatin1("%1:%2").arg(s).arg(i);
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, text);
        child->setIcon(0, myGroupIcon);
        item->addChild(child);
        load(it.Value(), child, text);
    }
}

class ImportOCAFExt: public Import::ImportOCAF2
{
public:
    ImportOCAFExt(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
        : ImportOCAF2(h, d, name)
    {}

private:
    void applyFaceColors(Part::Feature* part, const std::vector<App::Color>& colors) override
    {
        auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
            Gui::Application::Instance->getViewProvider(part));
        if (!vp) {
            return;
        }
        if (colors.empty()) {
            return;
        }

        if (colors.size() == 1) {
            vp->ShapeColor.setValue(colors.front());
            vp->Transparency.setValue(100 * colors.front().a);
        }
        else {
            vp->DiffuseColor.setValues(colors);
        }
    }
    void applyEdgeColors(Part::Feature* part, const std::vector<App::Color>& colors) override
    {
        auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
            Gui::Application::Instance->getViewProvider(part));
        if (!vp) {
            return;
        }
        if (colors.size() == 1) {
            vp->LineColor.setValue(colors.front());
        }
        else {
            vp->LineColorArray.setValues(colors);
        }
    }
    void applyLinkColor(App::DocumentObject* obj, int index, App::Color color) override
    {
        auto vp =
            dynamic_cast<Gui::ViewProviderLink*>(Gui::Application::Instance->getViewProvider(obj));
        if (!vp) {
            return;
        }
        if (index < 0) {
            vp->OverrideMaterial.setValue(true);
            vp->ShapeMaterial.setDiffuseColor(color);
            return;
        }
        if (vp->OverrideMaterialList.getSize() <= index) {
            vp->OverrideMaterialList.setSize(index + 1);
        }
        vp->OverrideMaterialList.set1Value(index, true);
        App::Material mat(App::Material::DEFAULT);
        if (vp->MaterialList.getSize() <= index) {
            vp->MaterialList.setSize(index + 1, mat);
        }
        mat.diffuseColor = color;
        vp->MaterialList.set1Value(index, mat);
    }
    void applyElementColors(App::DocumentObject* obj,
                            const std::map<std::string, App::Color>& colors) override
    {
        auto vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            return;
        }
        (void)colors;
    }
};

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("ImportGui")
    {
        add_keyword_method("open",
                           &Module::insert,
                           "open(string) -- Open the file and create a new document.");
        add_keyword_method("insert",
                           &Module::insert,
                           "insert(string,string) -- Insert the file into the given document.");
        add_varargs_method("exportOptions",
                           &Module::exportOptions,
                           "exportOptions(string) -- Return the export options of a file type.");
        add_keyword_method("export",
                           &Module::exporter,
                           "export(list,string) -- Export a list of objects into a single file.");
        add_varargs_method("ocaf", &Module::ocaf, "ocaf(string) -- Browse the ocaf structure.");
        initialize("This module is the ImportGui module.");  // register with Python
    }

private:
    Py::Object insert(const Py::Tuple& args, const Py::Dict& kwds)
    {
        char* Name;
        char* DocName = nullptr;
        PyObject* importHidden = Py_None;
        PyObject* merge = Py_None;
        PyObject* useLinkGroup = Py_None;
        int mode = -1;
        static const std::array<const char*, 7>
            kwd_list {"name", "docName", "importHidden", "merge", "useLinkGroup", "mode", nullptr};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(),
                                                 kwds.ptr(),
                                                 "et|sO!O!O!i",
                                                 kwd_list,
                                                 "utf-8",
                                                 &Name,
                                                 &DocName,
                                                 &PyBool_Type,
                                                 &importHidden,
                                                 &PyBool_Type,
                                                 &merge,
                                                 &PyBool_Type,
                                                 &useLinkGroup,
                                                 &mode)) {
            throw Py::Exception();
        }

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);
        std::string name8bit = Part::encodeFilename(Utf8Name);

        try {
            Base::FileInfo file(Utf8Name.c_str());

            App::Document* pcDoc = nullptr;
            if (DocName) {
                pcDoc = App::GetApplication().getDocument(DocName);
            }
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument();
            }

            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);
            ImportOCAFExt ocaf(hDoc, pcDoc, file.fileNamePure());
            ocaf.setImportOptions(ImportOCAFExt::customImportOptions());
            FC_TIME_INIT(t);
            FC_DURATION_DECL_INIT2(d1, d2);

            if (file.hasExtension({"stp", "step"})) {

                if (mode < 0) {
                    mode = ocaf.getMode();
                }
                if (mode && !pcDoc->isSaved()) {
                    auto gdoc = Gui::Application::Instance->getDocument(pcDoc);
                    if (!gdoc->save()) {
                        return Py::Object();
                    }
                }

                try {
                    Import::ReaderStep reader(file);
                    reader.read(hDoc);
                }
                catch (OSD_Exception& e) {
                    Base::Console().Error("%s\n", e.GetMessageString());
                    Base::Console().Message("Try to load STEP file without colors...\n");

                    Part::ImportStepParts(pcDoc, Utf8Name.c_str());
                    pcDoc->recompute();
                }
            }
            else if (file.hasExtension({"igs", "iges"})) {
                try {
                    Import::ReaderIges reader(file);
                    reader.read(hDoc);
                }
                catch (OSD_Exception& e) {
                    Base::Console().Error("%s\n", e.GetMessageString());
                    Base::Console().Message("Try to load IGES file without colors...\n");

                    Part::ImportIgesParts(pcDoc, Utf8Name.c_str());
                    pcDoc->recompute();
                }
            }
            else if (file.hasExtension({"glb", "gltf"})) {
                Import::ReaderGltf reader(file);
                reader.read(hDoc);
            }
            else {
                throw Py::Exception(PyExc_IOError, "no supported file format");
            }

            FC_DURATION_PLUS(d1, t);
            if (merge != Py_None) {
                ocaf.setMerge(Base::asBoolean(merge));
            }
            if (importHidden != Py_None) {
                ocaf.setImportHiddenObject(Base::asBoolean(importHidden));
            }
            if (useLinkGroup != Py_None) {
                ocaf.setUseLinkGroup(Base::asBoolean(useLinkGroup));
            }
            if (mode >= 0) {
                ocaf.setMode(mode);
            }
            auto ret = ocaf.loadShapes();
            hApp->Close(hDoc);
            FC_DURATION_PLUS(d2, t);
            FC_DURATION_LOG(d1, "file read");
            FC_DURATION_LOG(d2, "import");
            FC_DURATION_LOG((d1 + d2), "total");

            if (ret) {
                App::GetApplication().setActiveDocument(pcDoc);
                auto gdoc = Gui::Application::Instance->getDocument(pcDoc);
                if (gdoc) {
                    gdoc->setActiveView();
                    Gui::Application::Instance->commandManager().runCommandByName("Std_ViewFitAll");
                }
                return Py::asObject(ret->getPyObject());
            }
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(Base::PyExc_FC_GeneralError, e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }

    static std::map<std::string, App::Color> getShapeColors(App::DocumentObject* obj,
                                                            const char* subname)
    {
        auto vp = Gui::Application::Instance->getViewProvider(obj);
        if (vp) {
            return vp->getElementColors(subname);
        }
        return {};
    }

    Py::Object exportOptions(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);
        std::string name8bit = Part::encodeFilename(Utf8Name);

        Py::Dict options;
        Base::FileInfo file(name8bit.c_str());

        if (file.hasExtension({"stp", "step"})) {
            PartGui::TaskExportStep dlg(Gui::getMainWindow());
            if (!dlg.showDialog() || dlg.exec()) {
                auto stepSettings = dlg.getSettings();
                options.setItem("exportHidden", Py::Boolean(stepSettings.exportHidden));
                options.setItem("keepPlacement", Py::Boolean(stepSettings.keepPlacement));
                options.setItem("legacy", Py::Boolean(stepSettings.exportLegacy));
            }
        }

        return options;
    }

    Py::Object exporter(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject* object;
        char* Name;
        PyObject* pyoptions = nullptr;
        PyObject* pyexportHidden = Py_None;
        PyObject* pylegacy = Py_None;
        PyObject* pykeepPlacement = Py_None;
        static const std::array<const char*, 7>
            kwd_list {"obj", "name", "options", "exportHidden", "legacy", "keepPlacement", nullptr};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(),
                                                 kwds.ptr(),
                                                 "Oet|O!O!O!O!",
                                                 kwd_list,
                                                 &object,
                                                 "utf-8",
                                                 &Name,
                                                 &PyDict_Type,
                                                 &pyoptions,
                                                 &PyBool_Type,
                                                 &pyexportHidden,
                                                 &PyBool_Type,
                                                 &pylegacy,
                                                 &PyBool_Type,
                                                 &pykeepPlacement)) {
            throw Py::Exception();
        }

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);

        // clang-format off
        // determine export options
        Part::OCAF::ImportExportSettings settings;

        // still support old way
        bool legacyExport = (pylegacy         == Py_None ? settings.getExportLegacy()
                                                         : Base::asBoolean(pylegacy));
        bool exportHidden = (pyexportHidden   == Py_None ? settings.getExportHiddenObject()
                                                         : Base::asBoolean(pyexportHidden));
        bool keepPlacement = (pykeepPlacement == Py_None ? settings.getExportKeepPlacement()
                                                         : Base::asBoolean(pykeepPlacement));
        // clang-format on

        // new way
        if (pyoptions) {
            Py::Dict options(pyoptions);
            if (options.hasKey("legacy")) {
                legacyExport = static_cast<bool>(Py::Boolean(options.getItem("legacy")));
            }
            if (options.hasKey("exportHidden")) {
                exportHidden = static_cast<bool>(Py::Boolean(options.getItem("exportHidden")));
            }
            if (options.hasKey("keepPlacement")) {
                keepPlacement = static_cast<bool>(Py::Boolean(options.getItem("keepPlacement")));
            }
        }

        try {
            Py::Sequence list(object);
            std::vector<App::DocumentObject*> objs;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    auto pydoc = static_cast<App::DocumentObjectPy*>(item);
                    objs.push_back(pydoc->getDocumentObjectPtr());
                }
            }

            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

            Import::ExportOCAF2 ocaf(hDoc, &getShapeColors);
            if (!legacyExport || !ocaf.canFallback(objs)) {
                ocaf.setExportOptions(Import::ExportOCAF2::customExportOptions());
                ocaf.setExportHiddenObject(exportHidden);
                ocaf.setKeepPlacement(keepPlacement);

                ocaf.exportObjects(objs);
            }
            else {
                bool keepExplicitPlacement = true;
                ExportOCAFGui ocaf(hDoc, keepExplicitPlacement);
                ocaf.exportObjects(objs);
            }

            Base::FileInfo file(Utf8Name.c_str());
            if (file.hasExtension({"stp", "step"})) {
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Part/STEP");
                std::string scheme = hGrp->GetASCII("Scheme", Part::Interface::writeStepScheme());
                std::list<std::string> supported = Part::supportedSTEPSchemes();
                if (std::find(supported.begin(), supported.end(), scheme) != supported.end()) {
                    Part::Interface::writeStepScheme(scheme.c_str());
                }

                Import::WriterStep writer(file);
                writer.write(hDoc);
            }
            else if (file.hasExtension({"igs", "iges"})) {
                Import::WriterIges writer(file);
                writer.write(hDoc);
            }
            else if (file.hasExtension({"glb", "gltf"})) {
                Import::WriterGltf writer(file);
                writer.write(hDoc);
            }

            hApp->Close(hDoc);
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(Base::PyExc_FC_GeneralError, e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }
    Py::Object ocaf(const Py::Tuple& args)
    {
        const char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "s", &Name)) {
            throw Py::Exception();
        }

        try {
            Base::FileInfo file(Name);

            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

            if (file.hasExtension({"stp", "step"})) {
                Import::ReaderStep reader(file);
                reader.read(hDoc);
            }
            else if (file.hasExtension({"igs", "iges"})) {
                Import::ReaderIges reader(file);
                reader.read(hDoc);
            }
            else {
                throw Py::Exception(PyExc_IOError, "no supported file format");
            }

            static QPointer<QDialog> dlg = nullptr;
            if (!dlg) {
                dlg = new QDialog(Gui::getMainWindow());
                QTreeWidget* tree = new QTreeWidget();
                tree->setHeaderLabel(QString::fromLatin1("OCAF Browser"));

                QVBoxLayout* layout = new QVBoxLayout;
                layout->addWidget(tree);
                dlg->setLayout(layout);

                QDialogButtonBox* btn = new QDialogButtonBox(dlg);
                btn->setStandardButtons(QDialogButtonBox::Close);
                QObject::connect(btn, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
                QHBoxLayout* boxlayout = new QHBoxLayout;
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
            throw Py::Exception(Base::PyExc_FC_GeneralError, e.GetMessageString());
        }
        catch (const Base::Exception& e) {
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

}  // namespace ImportGui
