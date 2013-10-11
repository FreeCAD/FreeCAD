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
# include <OSD_Exception.hxx>
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
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>
#include <Mod/Import/App/ImportOCAF.h>



class ImportOCAFExt : public Import::ImportOCAF
{
public:
    ImportOCAFExt(Handle_TDocStd_Document h, App::Document* d, const std::string& name)
        : ImportOCAF(h, d, name)
    {
    }

private:
    void applyColors(Part::Feature* part, const std::vector<App::Color>& colors)
    {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
            static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.setValues(colors);
        }
    }
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

        ImportOCAFExt ocaf(hDoc, pcDoc, file.fileNamePure());
        ocaf.loadShapes();
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
                    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
                    if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
                        colors = static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.getValues();
                        if (colors.empty())
                            colors.push_back(static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.getValue());
                    }
                    ocaf.saveShape(part, colors);
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
