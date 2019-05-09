/***************************************************************************
 *   Copyright (c) 2004 Juergen Riegel <juergen.riegel@web.de>             *
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

#ifndef _PreComp_
# include "InventorAll.h"
# include <boost/signals2.hpp>
# include <boost/bind.hpp>
# include <sstream>
# include <stdexcept>
# include <iostream>
# include <QCloseEvent>
# include <QDir>
# include <QFileInfo>
# include <QLocale>
# include <QMessageBox>
#if QT_VERSION >= 0x050000
# include <QMessageLogContext>
#endif
# include <QPointer>
# include <QSessionManager>
# include <QStatusBar>
# include <QTextStream>
# include <QTimer>
#endif

#include <boost/interprocess/sync/file_lock.hpp>
#include <QtOpenGL.h>
#if defined(HAVE_QT5_OPENGL)
#include <QWindow>
#endif

// FreeCAD Base header
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Factory.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>

#include "Application.h"
#include "AutoSaver.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Document.h"
#include "View.h"
#include "View3DPy.h"
#include "WidgetFactory.h"
#include "Command.h"
#include "Macro.h"
#include "ProgressBar.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "ToolBoxManager.h"
#include "WaitCursor.h"
#include "MenuManager.h"
#include "Window.h"
#include "Selection.h"
#include "BitmapFactory.h"
#include "SoFCDB.h"
#include "PythonConsolePy.h"
#include "PythonDebugger.h"
#include "View3DPy.h"
#include "DlgOnlineHelpImp.h"
#include "SpaceballEvent.h"
#include "Control.h"
#include "DocumentRecovery.h"
#include "TransactionObject.h"
#include "FileDialog.h"

#include "SplitView3DInventor.h"
#include "View3DInventor.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderExtension.h"
#include "ViewProviderExtern.h"
#include "ViewProviderFeature.h"
#include "ViewProviderPythonFeature.h"
#include "ViewProviderDocumentObjectGroup.h"
#include "ViewProviderDragger.h"
#include "ViewProviderGeometryObject.h"
#include "ViewProviderInventorObject.h"
#include "ViewProviderVRMLObject.h"
#include "ViewProviderAnnotation.h"
#include "ViewProviderMeasureDistance.h"
#include "ViewProviderPlacement.h"
#include "ViewProviderOriginFeature.h"
#include "ViewProviderPlane.h"
#include "ViewProviderLine.h"
#include "ViewProviderGeoFeatureGroup.h"
#include "ViewProviderOriginGroup.h"
#include "ViewProviderPart.h"
#include "ViewProviderOrigin.h"
#include "ViewProviderMaterialObject.h"
#include "ViewProviderTextDocument.h"
#include "ViewProviderGroupExtension.h"
#include "ViewProviderLink.h"
#include "LinkViewPy.h"
#include "AxisOriginPy.h"

#include "Language/Translator.h"
#include "TaskView/TaskView.h"
#include "TaskView/TaskDialogPython.h"
#include <Gui/Quarter/Quarter.h>
#include "View3DViewerPy.h"
#include <Gui/GuiInitScript.h>


using namespace Gui;
using namespace Gui::DockWnd;
using namespace std;


Application* Application::Instance = 0L;

namespace Gui {

// Pimpl class
struct ApplicationP
{
    ApplicationP() :
    activeDocument(0L),
    editDocument(0L),
    isClosing(false),
    startingUp(true)
    {
        // create the macro manager
        macroMngr = new MacroManager();
    }

    ~ApplicationP()
    {
        delete macroMngr;
    }

    /// list of all handled documents
    std::map<const App::Document*, Gui::Document*> documents;
    /// Active document
    Gui::Document*   activeDocument;
    Gui::Document*  editDocument;
    MacroManager*  macroMngr;
    /// List of all registered views
    std::list<Gui::BaseView*> passive;
    bool isClosing;
    bool startingUp;
    /// Handles all commands
    CommandManager commandManager;
};

static PyObject *
FreeCADGui_subgraphFromObject(PyObject * /*self*/, PyObject *args)
{
    PyObject *o;
    if (!PyArg_ParseTuple(args, "O!",&(App::DocumentObjectPy::Type), &o))
        return NULL;
    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(o)->getDocumentObjectPtr();
    std::string vp = obj->getViewProviderName();
    SoNode* node = 0;
    try {
        Base::BaseClass* base = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName(vp.c_str(), true));
        if (base && base->getTypeId().isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId())) {
            std::unique_ptr<Gui::ViewProviderDocumentObject> vp(static_cast<Gui::ViewProviderDocumentObject*>(base));
            std::map<std::string, App::Property*> Map;
            obj->getPropertyMap(Map);
            vp->attach(obj);
            for (std::map<std::string, App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it) {
                vp->updateData(it->second);
            }

            std::vector<std::string> modes = vp->getDisplayModes();
            if (!modes.empty())
                vp->setDisplayMode(modes.front().c_str());
            node = vp->getRoot()->copy();
            node->ref();
            std::string type = "So";
            type += node->getTypeId().getName().getString();
            type += " *";
            PyObject* proxy = 0;
            proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", type.c_str(), (void*)node, 1);
            return Py::new_reference_to(Py::Object(proxy, true));
        }
    }
    catch (const Base::Exception& e) {
        if (node) node->unref();
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FreeCADGui_getSoDBVersion(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromString(SoDB::getVersion());
#else
    return PyString_FromString(SoDB::getVersion());
#endif
}

// Copied from https://github.com/python/cpython/blob/master/Objects/moduleobject.c
#if PY_MAJOR_VERSION >= 3
#if PY_MINOR_VERSION <= 4
static int
_add_methods_to_object(PyObject *module, PyObject *name, PyMethodDef *functions)
{
    PyObject *func;
    PyMethodDef *fdef;

    for (fdef = functions; fdef->ml_name != NULL; fdef++) {
        if ((fdef->ml_flags & METH_CLASS) ||
            (fdef->ml_flags & METH_STATIC)) {
            PyErr_SetString(PyExc_ValueError,
                            "module functions cannot set"
                            " METH_CLASS or METH_STATIC");
            return -1;
        }
        func = PyCFunction_NewEx(fdef, (PyObject*)module, name);
        if (func == NULL) {
            return -1;
        }
        if (PyObject_SetAttrString(module, fdef->ml_name, func) != 0) {
            Py_DECREF(func);
            return -1;
        }
        Py_DECREF(func);
    }

    return 0;
}

int
PyModule_AddFunctions(PyObject *m, PyMethodDef *functions)
{
    int res;
    PyObject *name = PyModule_GetNameObject(m);
    if (name == NULL) {
        return -1;
    }

    res = _add_methods_to_object(m, name, functions);
    Py_DECREF(name);
    return res;
}
#endif
#endif

struct PyMethodDef FreeCADGui_methods[] = {
    {"subgraphFromObject",FreeCADGui_subgraphFromObject,METH_VARARGS,
     "subgraphFromObject(object) -> Node\n\n"
     "Return the Inventor subgraph to an object"},
    {"getSoDBVersion",FreeCADGui_getSoDBVersion,METH_VARARGS,
     "getSoDBVersion() -> String\n\n"
     "Return a text string containing the name\n"
     "of the Coin library and version information"},
    {NULL, NULL, 0, NULL}  /* sentinel */
};

} // namespace Gui

Application::Application(bool GUIenabled)
{
    //App::GetApplication().Attach(this);
    if (GUIenabled) {
        App::GetApplication().signalNewDocument.connect(boost::bind(&Gui::Application::slotNewDocument, this, _1, _2));
        App::GetApplication().signalDeleteDocument.connect(boost::bind(&Gui::Application::slotDeleteDocument, this, _1));
        App::GetApplication().signalRenameDocument.connect(boost::bind(&Gui::Application::slotRenameDocument, this, _1));
        App::GetApplication().signalActiveDocument.connect(boost::bind(&Gui::Application::slotActiveDocument, this, _1));
        App::GetApplication().signalRelabelDocument.connect(boost::bind(&Gui::Application::slotRelabelDocument, this, _1));
        App::GetApplication().signalShowHidden.connect(boost::bind(&Gui::Application::slotShowHidden, this, _1));


        // install the last active language
        ParameterGrp::handle hPGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp");
        hPGrp = hPGrp->GetGroup("Preferences")->GetGroup("General");
        QString lang = QLocale::languageToString(QLocale::system().language());
        Translator::instance()->activateLanguage(hPGrp->GetASCII("Language", (const char*)lang.toLatin1()).c_str());
        GetWidgetFactorySupplier();

        // Coin3d disabled VBO support for all Intel drivers but in the meantime they have improved
        // so we can try to override the workaround by setting COIN_VBO
        ParameterGrp::handle hViewGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
        if (hViewGrp->GetBool("UseVBO",false)) {
            (void)coin_setenv("COIN_VBO", "0", true);
        }

        // Check for the symbols for group separator and decimal point. They must be different otherwise
        // Qt doesn't work properly.
#if defined(Q_OS_WIN32)
        if (QLocale::system().groupSeparator() == QLocale::system().decimalPoint()) {
            QMessageBox::critical(0, QLatin1String("Invalid system settings"),
                QLatin1String("Your system uses the same symbol for decimal point and group separator.\n\n"
                              "This causes serious problems and makes the application fail to work properly.\n"
                              "Go to the system configuration panel of the OS and fix this issue, please."));
            throw Base::RuntimeError("Invalid system settings");
        }
#endif
#if 0 // QuantitySpinBox and InputField try to handle the group separator now
        // http://forum.freecadweb.org/viewtopic.php?f=10&t=6910
        // A workaround is to disable the group separator for double-to-string conversion, i.e.
        // setting the flag 'OmitGroupSeparator'.
        QLocale loc = QLocale::system();
        loc.setNumberOptions(QLocale::OmitGroupSeparator);
        QLocale::setDefault(loc);
#endif

        // setting up Python binding
        Base::PyGILStateLocker lock;

        PyDoc_STRVAR(FreeCADGui_doc,
            "The functions in the FreeCADGui module allow working with GUI documents,\n"
            "view providers, views, workbenches and much more.\n\n"
            "The FreeCADGui instance provides a list of references of GUI documents which\n"
            "can be addressed by a string. These documents contain the view providers for\n"
            "objects in the associated App document. An App and GUI document can be\n"
            "accessed with the same name.\n\n"
            "The FreeCADGui module also provides a set of functions to work with so called\n"
            "workbenches."
            );

#if PY_MAJOR_VERSION >= 3
        // if this returns a valid pointer then the 'FreeCADGui' Python module was loaded,
        // otherwise the executable was launched
        PyObject* modules = PyImport_GetModuleDict();
        PyObject* module = PyDict_GetItemString(modules, "FreeCADGui");
        if (!module) {
            static struct PyModuleDef FreeCADGuiModuleDef = {
                PyModuleDef_HEAD_INIT,
                "FreeCADGui", FreeCADGui_doc, -1,
                Application::Methods,
                NULL, NULL, NULL, NULL
            };
            module = PyModule_Create(&FreeCADGuiModuleDef);

            PyDict_SetItemString(modules, "FreeCADGui", module);
        }
        else {
            // extend the method list
            PyModule_AddFunctions(module, Application::Methods);
        }
#else
        PyObject* module = Py_InitModule3("FreeCADGui", Application::Methods, FreeCADGui_doc);
#endif
        Py::Module(module).setAttr(std::string("ActiveDocument"),Py::None());

        UiLoaderPy::init_type();
        Base::Interpreter().addType(UiLoaderPy::type_object(),
            module,"UiLoader");
        PyResource::init_type();

        // PySide additions
        PySideUicModule* pySide = new PySideUicModule();
        Py_INCREF(pySide->module().ptr());
        PyModule_AddObject(module, "PySideUic", pySide->module().ptr());

        //insert Selection module
#if PY_MAJOR_VERSION >= 3
        static struct PyModuleDef SelectionModuleDef = {
            PyModuleDef_HEAD_INIT,
            "Selection", "Selection module", -1,
            SelectionSingleton::Methods,
            NULL, NULL, NULL, NULL
        };
        PyObject* pSelectionModule = PyModule_Create(&SelectionModuleDef);
#else
        PyObject* pSelectionModule = Py_InitModule3("Selection", SelectionSingleton::Methods,"Selection module");
#endif
        Py_INCREF(pSelectionModule);
        PyModule_AddObject(module, "Selection", pSelectionModule);

        SelectionFilterPy::init_type();
        Base::Interpreter().addType(SelectionFilterPy::type_object(),
            pSelectionModule,"Filter");

        Gui::TaskView::ControlPy::init_type();
        Py::Module(module).setAttr(std::string("Control"),
            Py::Object(Gui::TaskView::ControlPy::getInstance(), true));

        Base::Interpreter().addType(&LinkViewPy::Type,module,"LinkView");
        Base::Interpreter().addType(&AxisOriginPy::Type,module,"AxisOrigin");
    }

    Base::PyGILStateLocker lock;
    PyObject *module = PyImport_AddModule("FreeCADGui");
    PyMethodDef *meth = FreeCADGui_methods;
    PyObject *dict = PyModule_GetDict(module);
    for (; meth->ml_name != NULL; meth++) {
        PyObject *descr;
        descr = PyCFunction_NewEx(meth,0,0);
        if (descr == NULL)
            break;
        if (PyDict_SetItemString(dict, meth->ml_name, descr) != 0)
            break;
        Py_DECREF(descr);
    }

    // Python console binding
    PythonDebugModule           ::init_module();
    PythonStdout                ::init_type();
    PythonStderr                ::init_type();
    OutputStdout                ::init_type();
    OutputStderr                ::init_type();
    PythonStdin                 ::init_type();
    View3DInventorPy            ::init_type();
    View3DInventorViewerPy      ::init_type();
    AbstractSplitViewPy         ::init_type();

    d = new ApplicationP;

    // global access
    Instance = this;

    // instantiate the workbench dictionary
    _pcWorkbenchDictionary = PyDict_New();

    if (GUIenabled) {
        createStandardOperations();
        MacroCommand::load();
    }
}

Application::~Application()
{
    Base::Console().Log("Destruct Gui::Application\n");
    WorkbenchManager::destruct();
    SelectionSingleton::destruct();
    Translator::destruct();
    WidgetFactorySupplier::destruct();
    BitmapFactoryInst::destruct();

#if 0
    // we must run the garbage collector before shutting down the SoDB
    // subsystem because we may reference some class objects of them in Python
    Base::Interpreter().cleanupSWIG("SoBase *");
    // finish also Inventor subsystem
    SoFCDB::finish();

#if (COIN_MAJOR_VERSION >= 2) && (COIN_MINOR_VERSION >= 4)
    SoDB::finish();
#elif (COIN_MAJOR_VERSION >= 3)
    SoDB::finish();
#else
    SoDB::cleanup();
#endif
#endif
    {
    Base::PyGILStateLocker lock;
    Py_DECREF(_pcWorkbenchDictionary);
    }

    // save macros
    try {
        MacroCommand::save();
    }
    catch (const Base::Exception& e) {
        std::cerr << "Saving macros failed: " << e.what() << std::endl;
    }
    //App::GetApplication().Detach(this);

    delete d;
    Instance = 0;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// creating std commands
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Application::open(const char* FileName, const char* Module)
{
    WaitCursor wc;
    wc.setIgnoreEvents(WaitCursor::NoEvents);
    Base::FileInfo File(FileName);
    string te = File.extension();
    string unicodepath = Base::Tools::escapedUnicodeFromUtf8(File.filePath().c_str());
    // if the active document is empty and not modified, close it
    // in case of an automatically created empty document at startup
    App::Document* act = App::GetApplication().getActiveDocument();
    Gui::Document* gui = this->getDocument(act);
    if (act && act->countObjects() == 0 && gui && gui->isModified() == false){
        Command::doCommand(Command::App, "App.closeDocument('%s')", act->getName());
        qApp->processEvents(); // an update is needed otherwise the new view isn't shown
    }

    if (Module != 0) {
        try {
            // issue module loading
            Command::doCommand(Command::App, "import %s", Module);

            // load the file with the module
            Command::doCommand(Command::App, "%s.open(u\"%s\")", Module, unicodepath.c_str());

            // ViewFit
            if (!File.hasExtension("FCStd") && sendHasMsgToActiveView("ViewFit")) {
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
                    ("User parameter:BaseApp/Preferences/View");
                if (hGrp->GetBool("AutoFitToView", true))
                    Command::doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
            }

            // the original file name is required
            QString filename = QString::fromUtf8(File.filePath().c_str());
            getMainWindow()->appendRecentFile(filename);
            FileDialog::setWorkingDirectory(filename);
        }
        catch (const Base::PyException& e){
            // Usually thrown if the file is invalid somehow
            e.ReportException();
        }
    }
    else {
        wc.restoreCursor();
        QMessageBox::warning(getMainWindow(), QObject::tr("Unknown filetype"),
            QObject::tr("Cannot open unknown filetype: %1").arg(QLatin1String(te.c_str())));
        wc.setWaitCursor();
        return;
    }
}

void Application::importFrom(const char* FileName, const char* DocName, const char* Module)
{
    WaitCursor wc;
    wc.setIgnoreEvents(WaitCursor::NoEvents);
    Base::FileInfo File(FileName);
    std::string te = File.extension();
    string unicodepath = Base::Tools::escapedUnicodeFromUtf8(File.filePath().c_str());

    if (Module != 0) {
        try {
            // issue module loading
            Command::doCommand(Command::App, "import %s", Module);

            // load the file with the module
            if (File.hasExtension("FCStd")) {
                Command::doCommand(Command::App, "%s.open(u\"%s\")"
                                               , Module, unicodepath.c_str());
                if (activeDocument())
                    activeDocument()->setModified(false);
            }
            else {
                if (DocName) {
                    Command::doCommand(Command::App, "%s.insert(u\"%s\",\"%s\")"
                                                   , Module, unicodepath.c_str(), DocName);
                }
                else {
                    Command::doCommand(Command::App, "%s.insert(u\"%s\")"
                                                   , Module, unicodepath.c_str());
                }
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
                    ("User parameter:BaseApp/Preferences/View");
                if (hGrp->GetBool("AutoFitToView", true))
                    Command::doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
                Gui::Document* doc = activeDocument();
                if (DocName) doc = getDocument(DocName);
                if (doc)
                    doc->setModified(true);
            }

            // the original file name is required
            QString filename = QString::fromUtf8(File.filePath().c_str());
            getMainWindow()->appendRecentFile(filename);
            FileDialog::setWorkingDirectory(filename);
        }
        catch (const Base::PyException& e){
            // Usually thrown if the file is invalid somehow
            e.ReportException();
        }
    }
    else {
        wc.restoreCursor();
        QMessageBox::warning(getMainWindow(), QObject::tr("Unknown filetype"),
            QObject::tr("Cannot open unknown filetype: %1").arg(QLatin1String(te.c_str())));
        wc.setWaitCursor();
    }
}

void Application::exportTo(const char* FileName, const char* DocName, const char* Module)
{
    WaitCursor wc;
    wc.setIgnoreEvents(WaitCursor::NoEvents);
    Base::FileInfo File(FileName);
    std::string te = File.extension();
    string unicodepath = Base::Tools::escapedUnicodeFromUtf8(File.filePath().c_str());

    if (Module != 0) {
        try {
            std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
                (App::DocumentObject::getClassTypeId(),DocName);
            if (sel.empty()) {
                App::Document* doc = App::GetApplication().getDocument(DocName);
                sel = doc->getObjectsOfType(App::DocumentObject::getClassTypeId());
            }

            std::stringstream str;
            std::set<App::DocumentObject*> unique_objs;
            str << "__objs__=[]" << std::endl;
            for (std::vector<App::DocumentObject*>::iterator it = sel.begin(); it != sel.end(); ++it) {
                if (unique_objs.insert(*it).second) {
                    str << "__objs__.append(FreeCAD.getDocument(\"" << DocName << "\").getObject(\""
                        << (*it)->getNameInDocument() << "\"))" << std::endl;
                }
            }

            str << "import " << Module << std::endl;
            str << Module << ".export(__objs__,u\"" << unicodepath << "\")" << std::endl;
            //str << "del __objs__" << std::endl;

            std::string code = str.str();
            // the original file name is required
            Gui::Command::runCommand(Gui::Command::App, code.c_str());
            // search for a module that is able to open the exported file because otherwise
            // it doesn't need to be added to the recent files list (#0002047)
            std::map<std::string, std::string> importMap = App::GetApplication().getImportFilters(te.c_str());
            if (!importMap.empty())
                getMainWindow()->appendRecentFile(QString::fromUtf8(File.filePath().c_str()));

            // allow exporters to pass _objs__ to submodules before deleting it
            Gui::Command::runCommand(Gui::Command::App, "del __objs__");
        }
        catch (const Base::PyException& e){
            // Usually thrown if the file is invalid somehow
            e.ReportException();
            wc.restoreCursor();
            QMessageBox::critical(getMainWindow(), QObject::tr("Export failed"),
                QString::fromUtf8(e.what()));
            wc.setWaitCursor();
        }
    }
    else {
        wc.restoreCursor();
        QMessageBox::warning(getMainWindow(), QObject::tr("Unknown filetype"),
            QObject::tr("Cannot save to unknown filetype: %1").arg(QLatin1String(te.c_str())));
        wc.setWaitCursor();
    }
}

void Application::createStandardOperations()
{
    // register the application Standard commands from CommandStd.cpp
    Gui::CreateStdCommands();
    Gui::CreateDocCommands();
    Gui::CreateFeatCommands();
    Gui::CreateMacroCommands();
    Gui::CreateViewStdCommands();
    Gui::CreateWindowStdCommands();
    Gui::CreateStructureCommands();
    Gui::CreateTestCommands();
    Gui::CreateLinkCommands();
}

void Application::slotNewDocument(const App::Document& Doc, bool isMainDoc)
{
#ifdef FC_DEBUG
    std::map<const App::Document*, Gui::Document*>::const_iterator it = d->documents.find(&Doc);
    assert(it==d->documents.end());
#endif
    Gui::Document* pDoc = new Gui::Document(const_cast<App::Document*>(&Doc),this);
    d->documents[&Doc] = pDoc;

    // connect the signals to the application for the new document
    pDoc->signalNewObject.connect(boost::bind(&Gui::Application::slotNewObject, this, _1));
    pDoc->signalDeletedObject.connect(boost::bind(&Gui::Application::slotDeletedObject, this, _1));
    pDoc->signalChangedObject.connect(boost::bind(&Gui::Application::slotChangedObject, this, _1, _2));
    pDoc->signalRelabelObject.connect(boost::bind(&Gui::Application::slotRelabelObject, this, _1));
    pDoc->signalActivatedObject.connect(boost::bind(&Gui::Application::slotActivatedObject, this, _1));
    pDoc->signalInEdit.connect(boost::bind(&Gui::Application::slotInEdit, this, _1));
    pDoc->signalResetEdit.connect(boost::bind(&Gui::Application::slotResetEdit, this, _1));

    signalNewDocument(*pDoc, isMainDoc);
    if(isMainDoc)
        pDoc->createView(View3DInventor::getClassTypeId());
    // FIXME: Do we really need this further? Calling processEvents() mixes up order of execution in an
    // unpredicatable way. At least it seems that with Qt5 we don't need this any more.
#if QT_VERSION < 0x050000
    // qApp->processEvents(); // make sure to show the window stuff on the right place
#endif
}

void Application::slotDeleteDocument(const App::Document& Doc)
{
    std::map<const App::Document*, Gui::Document*>::iterator doc = d->documents.find(&Doc);
    if (doc == d->documents.end()) {
        Base::Console().Log("GUI document '%s' already deleted\n", Doc.getName());
        return;
    }

    // We must clear the selection here to notify all observers.
    // And because of possible cross document link, better clear all selection
    // to be safe
    Gui::Selection().clearCompleteSelection();
    doc->second->signalDeleteDocument(*doc->second);
    signalDeleteDocument(*doc->second);

    doc->second->beforeDelete();

    // If the active document gets destructed we must set it to 0. If there are further existing documents then the
    // view that becomes active sets the active document again. So, we needn't worry about this.
    if (d->activeDocument == doc->second)
        setActiveDocument(0);

    // For exception-safety use a smart pointer
    unique_ptr<Document> delDoc (doc->second);
    d->documents.erase(doc);
}

void Application::slotRelabelDocument(const App::Document& Doc)
{
    std::map<const App::Document*, Gui::Document*>::iterator doc = d->documents.find(&Doc);
#ifdef FC_DEBUG
    assert(doc!=d->documents.end());
#endif

    signalRelabelDocument(*doc->second);
    doc->second->onRelabel();
}

void Application::slotRenameDocument(const App::Document& Doc)
{
    std::map<const App::Document*, Gui::Document*>::iterator doc = d->documents.find(&Doc);
#ifdef FC_DEBUG
    assert(doc!=d->documents.end());
#endif

    signalRenameDocument(*doc->second);
}

void Application::slotShowHidden(const App::Document& Doc)
{
    std::map<const App::Document*, Gui::Document*>::iterator doc = d->documents.find(&Doc);
#ifdef FC_DEBUG
    assert(doc!=d->documents.end());
#endif

    signalShowHidden(*doc->second);
}

void Application::slotActiveDocument(const App::Document& Doc)
{
    std::map<const App::Document*, Gui::Document*>::iterator doc = d->documents.find(&Doc);
    // this can happen when closing a document with two views opened
    if (doc != d->documents.end()) {
        // this can happen when calling App.setActiveDocument directly from Python
        // because no MDI view will be activated
        if (d->activeDocument != doc->second) {
            d->activeDocument = doc->second;
            if (d->activeDocument) {
                Base::PyGILStateLocker lock;
                Py::Object active(d->activeDocument->getPyObject(), true);
                Py::Module("FreeCADGui").setAttr(std::string("ActiveDocument"),active);

                auto view = getMainWindow()->activeWindow();
                if(!view || view->getAppDocument()!=&Doc) {
                    Gui::MDIView* view = d->activeDocument->getActiveView();
                    getMainWindow()->setActiveWindow(view);
                }
            }
            else {
                Base::PyGILStateLocker lock;
                Py::Module("FreeCADGui").setAttr(std::string("ActiveDocument"),Py::None());
            }
        }
        signalActiveDocument(*doc->second);
        getMainWindow()->updateActions();
    }
}

void Application::slotNewObject(const ViewProvider& vp)
{
    this->signalNewObject(vp);
}

void Application::slotDeletedObject(const ViewProvider& vp)
{
    this->signalDeletedObject(vp);
}

void Application::slotChangedObject(const ViewProvider& vp, const App::Property& prop)
{
    this->signalChangedObject(vp,prop);
    getMainWindow()->updateActions(true);
}

void Application::slotRelabelObject(const ViewProvider& vp)
{
    this->signalRelabelObject(vp);
}

void Application::slotActivatedObject(const ViewProvider& vp)
{
    this->signalActivatedObject(vp);
    getMainWindow()->updateActions();
}

void Application::slotInEdit(const Gui::ViewProviderDocumentObject& vp)
{
    this->signalInEdit(vp);
}

void Application::slotResetEdit(const Gui::ViewProviderDocumentObject& vp)
{
    this->signalResetEdit(vp);
}

void Application::onLastWindowClosed(Gui::Document* pcDoc)
{
    if (!d->isClosing && pcDoc) {
        try {
            // Call the closing mechanism from Python. This also checks whether pcDoc is the last open document.
            Command::doCommand(Command::Doc, "App.closeDocument(\"%s\")", pcDoc->getDocument()->getName());
        }
        catch (const Base::Exception& e) {
            e.ReportException();
        }
        catch (const Py::Exception&) {
            Base::PyException e;
            e.ReportException();
        }
    }
}

/// send Messages to the active view
bool Application::sendMsgToActiveView(const char* pMsg, const char** ppReturn)
{
    MDIView* pView = getMainWindow()->activeWindow();
    return pView ? pView->onMsg(pMsg,ppReturn) : false;
}

bool Application::sendHasMsgToActiveView(const char* pMsg)
{
    MDIView* pView = getMainWindow()->activeWindow();
    return pView ? pView->onHasMsg(pMsg) : false;
}

/// send Messages to the active view
bool Application::sendMsgToFocusView(const char* pMsg, const char** ppReturn)
{
    MDIView* pView = getMainWindow()->activeWindow();
    if(!pView)
        return false;
    for(auto focus=qApp->focusWidget();focus;focus=focus->parentWidget()) {
        if(focus == pView)
            return pView->onMsg(pMsg,ppReturn);
    }
    return false;
}

bool Application::sendHasMsgToFocusView(const char* pMsg)
{
    MDIView* pView = getMainWindow()->activeWindow();
    if(!pView)
        return false;
    for(auto focus=qApp->focusWidget();focus;focus=focus->parentWidget()) {
        if(focus == pView)
            return pView->onHasMsg(pMsg);
    }
    return false;
}

Gui::MDIView* Application::activeView(void) const
{
    if (activeDocument())
        return activeDocument()->getActiveView();
    else
        return NULL;
}

/**
 * @brief Application::activateView
 * Activates a view of the given type of the active document.
 * If a view of this type doesn't exist and \a create is true
 * a new view of this type will be created.
 * @param type
 * @param create
 */
void Application::activateView(const Base::Type& type, bool create)
{
    Document* doc = activeDocument();
    if (doc) {
        MDIView* mdiView = doc->getActiveView();
        if (mdiView && mdiView->isDerivedFrom(type))
            return;
        std::list<MDIView*> mdiViews = doc->getMDIViewsOfType(type);
        if (!mdiViews.empty())
            doc->setActiveWindow(mdiViews.back());
        else if (create)
            doc->createView(type);
    }
}

/// Getter for the active view
Gui::Document* Application::activeDocument(void) const
{
    return d->activeDocument;
}

Gui::Document* Application::editDocument(void) const
{
    return d->editDocument;
}

Gui::MDIView* Application::editViewOfNode(SoNode *node) const
{
    return d->editDocument?d->editDocument->getViewOfNode(node):0;
}

void Application::setEditDocument(Gui::Document *doc) {
    if(!doc) 
        d->editDocument = 0;
    for(auto &v : d->documents)
        v.second->_resetEdit();
    d->editDocument = doc;
    getMainWindow()->updateActions();
}

void Application::setActiveDocument(Gui::Document* pcDocument)
{
    if (d->activeDocument == pcDocument)
        return; // nothing needs to be done
    if (pcDocument) {
        // This happens if a document with more than one view is about being
        // closed and a second view is activated. The document is still not
        // removed from the map.
        App::Document* doc = pcDocument->getDocument();
        if (d->documents.find(doc) == d->documents.end())
            return;
    }
    d->activeDocument = pcDocument;
    std::string nameApp, nameGui;

    // This adds just a line to the macro file but does not set the active document
    // Macro recording of this is problematic, thus it's written out as comment.
    if (pcDocument){
        nameApp += "App.setActiveDocument(\"";
        nameApp += pcDocument->getDocument()->getName();
        nameApp +=  "\")\n";
        nameApp += "App.ActiveDocument=App.getDocument(\"";
        nameApp += pcDocument->getDocument()->getName();
        nameApp +=  "\")";
        macroManager()->addLine(MacroManager::Cmt,nameApp.c_str());
        nameGui += "Gui.ActiveDocument=Gui.getDocument(\"";
        nameGui += pcDocument->getDocument()->getName();
        nameGui +=  "\")";
        macroManager()->addLine(MacroManager::Cmt,nameGui.c_str());
    }
    else {
        nameApp += "App.setActiveDocument(\"\")\n";
        nameApp += "App.ActiveDocument=None";
        macroManager()->addLine(MacroManager::Cmt,nameApp.c_str());
        nameGui += "Gui.ActiveDocument=None";
        macroManager()->addLine(MacroManager::Cmt,nameGui.c_str());
    }

    // Sets the currently active document
    try {
        Base::Interpreter().runString(nameApp.c_str());
        Base::Interpreter().runString(nameGui.c_str());
    }
    catch (const Base::Exception& e) {
        Base::Console().Warning(e.what());
        return;
    }

#ifdef FC_DEBUG
    // May be useful for error detection
    if (d->activeDocument) {
        App::Document* doc = d->activeDocument->getDocument();
        Base::Console().Log("Active document is %s (at %p)\n",doc->getName(), doc);
    }
    else {
        Base::Console().Log("No active document\n");
    }
#endif

    // notify all views attached to the application (not views belong to a special document)
    for(list<Gui::BaseView*>::iterator It=d->passive.begin();It!=d->passive.end();++It)
        (*It)->setDocument(pcDocument);
}

Gui::Document* Application::getDocument(const char* name) const
{
    App::Document* pDoc = App::GetApplication().getDocument( name );
    std::map<const App::Document*, Gui::Document*>::const_iterator it = d->documents.find(pDoc);
    if ( it!=d->documents.end() )
        return it->second;
    else
        return 0;
}

Gui::Document* Application::getDocument(const App::Document* pDoc) const
{
    std::map<const App::Document*, Gui::Document*>::const_iterator it = d->documents.find(pDoc);
    if ( it!=d->documents.end() )
        return it->second;
    else
        return 0;
}

void Application::showViewProvider(const App::DocumentObject* obj)
{
    ViewProvider* vp = getViewProvider(obj);
    if (vp) vp->show();
}

void Application::hideViewProvider(const App::DocumentObject* obj)
{
    ViewProvider* vp = getViewProvider(obj);
    if (vp) vp->hide();
}

Gui::ViewProvider* Application::getViewProvider(const App::DocumentObject* obj) const
{
    App::Document* doc = obj ? obj->getDocument() : 0;
    if (doc) {
        Gui::Document* gui = getDocument(doc);
        if (gui) {
            ViewProvider* vp = gui->getViewProvider(obj);
            return vp;
        }
    }

    return 0;
}

void Application::attachView(Gui::BaseView* pcView)
{
    d->passive.push_back(pcView);
}

void Application::detachView(Gui::BaseView* pcView)
{
    d->passive.remove(pcView);
}

void Application::onUpdate(void)
{
    // update all documents
    std::map<const App::Document*, Gui::Document*>::iterator It;
    for (It = d->documents.begin();It != d->documents.end();++It)
        It->second->onUpdate();
    // update all the independent views
    for (std::list<Gui::BaseView*>::iterator It2 = d->passive.begin();It2 != d->passive.end();++It2)
        (*It2)->onUpdate();
}

/// Gets called if a view gets activated, this manages the whole activation scheme
void Application::viewActivated(MDIView* pcView)
{
#ifdef FC_DEBUG
    // May be useful for error detection
    Base::Console().Log("Active view is %s (at %p)\n",
                 (const char*)pcView->windowTitle().toUtf8(),pcView);
#endif

    signalActivateView(pcView);

    // Set the new active document which is taken of the activated view. If, however,
    // this view is passive we let the currently active document unchanged as we would
    // have no document active which is causing a lot of trouble.
    if (!pcView->isPassive())
        setActiveDocument(pcView->getGuiDocument());
}


void Application::updateActive(void)
{
    activeDocument()->onUpdate();
}

void Application::tryClose(QCloseEvent * e)
{
    e->setAccepted(getMainWindow()->closeAllDocuments(false));
    if(!e->isAccepted())
        return;

    // ask all passive views if closable
    for (std::list<Gui::BaseView*>::iterator It = d->passive.begin();It!=d->passive.end();++It) {
        e->setAccepted((*It)->canClose());
        if (!e->isAccepted())
            return;
    }

    if (e->isAccepted()) {
        d->isClosing = true;

        std::map<const App::Document*, Gui::Document*>::iterator It;

        //detach the passive views
        //SetActiveDocument(0);
        std::list<Gui::BaseView*>::iterator itp = d->passive.begin();
        while (itp != d->passive.end()) {
            (*itp)->onClose();
            itp = d->passive.begin();
        }

        // remove all documents
        size_t cnt = d->documents.size();
        while (d->documents.size() > 0 && cnt > 0) {
            // destroys also the Gui document
            It = d->documents.begin();
            App::GetApplication().closeDocument(It->second->getDocument()->getName());
            --cnt; // avoid infinite loop
        }
    }
}

/**
 * Activate the matching workbench to the registered workbench handler with name \a name.
 * The handler must be an instance of a class written in Python.
 * Normally, if a handler gets activated a workbench with the same name gets created unless it
 * already exists.
 *
 * The old workbench gets deactivated before. If the workbench to the handler is already
 * active or if the switch fails false is returned.
 */
bool Application::activateWorkbench(const char* name)
{
    bool ok = false;
    WaitCursor wc;
    Workbench* oldWb = WorkbenchManager::instance()->active();
    if (oldWb && oldWb->name() == name)
        return false; // already active

    Base::PyGILStateLocker lock;
    // we check for the currently active workbench and call its 'Deactivated'
    // method, if available
    PyObject* pcOldWorkbench = 0;
    if (oldWb) {
        pcOldWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, oldWb->name().c_str());
    }

    // get the python workbench object from the dictionary
    PyObject* pcWorkbench = 0;
    pcWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, name);
    // test if the workbench exists
    if (!pcWorkbench)
        return false;

    try {
        std::string type;
        Py::Object handler(pcWorkbench);
        if (!handler.hasAttr(std::string("__Workbench__"))) {
            // call its GetClassName method if possible
            Py::Callable method(handler.getAttr(std::string("GetClassName")));
            Py::Tuple args;
            Py::String result(method.apply(args));
            type = result.as_std_string("ascii");
            if (Base::Type::fromName(type.c_str()).isDerivedFrom(Gui::PythonBaseWorkbench::getClassTypeId())) {
                Workbench* wb = WorkbenchManager::instance()->createWorkbench(name, type);
                if (!wb)
                    throw Py::RuntimeError("Failed to instantiate workbench of type " + type);
                handler.setAttr(std::string("__Workbench__"), Py::Object(wb->getPyObject(), true));
            }

            // import the matching module first
            Py::Callable activate(handler.getAttr(std::string("Initialize")));
            activate.apply(args);

            // Dependent on the implementation of a workbench handler the type
            // can be defined after the call of Initialize()
            if (type.empty()) {
                Py::String result(method.apply(args));
                type = result.as_std_string("ascii");
            }
        }

        // does the Python workbench handler have changed the workbench?
        Workbench* curWb = WorkbenchManager::instance()->active();
        if (curWb && curWb->name() == name)
            ok = true; // already active
        // now try to create and activate the matching workbench object
        else if (WorkbenchManager::instance()->activate(name, type)) {
            getMainWindow()->activateWorkbench(QString::fromLatin1(name));
            this->signalActivateWorkbench(name);
            ok = true;
        }

        // if we still not have this member then it must be built-in C++ workbench
        // which could be created after loading the appropriate module
        if (!handler.hasAttr(std::string("__Workbench__"))) {
            Workbench* wb = WorkbenchManager::instance()->getWorkbench(name);
            if (wb) handler.setAttr(std::string("__Workbench__"), Py::Object(wb->getPyObject(), true));
        }

        // If the method Deactivate is available we call it
        if (pcOldWorkbench) {
            Py::Object handler(pcOldWorkbench);
            if (handler.hasAttr(std::string("Deactivated"))) {
                Py::Object method(handler.getAttr(std::string("Deactivated")));
                if (method.isCallable()) {
                    Py::Tuple args;
                    Py::Callable activate(method);
                    activate.apply(args);
                }
            }
        }

        if (oldWb)
            oldWb->deactivated();

        // If the method Activate is available we call it
        if (handler.hasAttr(std::string("Activated"))) {
            Py::Object method(handler.getAttr(std::string("Activated")));
            if (method.isCallable()) {
                Py::Tuple args;
                Py::Callable activate(method);
                activate.apply(args);
            }
        }

        // now get the newly activated workbench
        Workbench* newWb = WorkbenchManager::instance()->active();
        if (newWb)
            newWb->activated();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        QString msg = QString::fromLatin1(e.what());
        QRegExp rx;
        // ignore '<type 'exceptions.ImportError'>' prefixes
        rx.setPattern(QLatin1String("^\\s*<type 'exceptions.ImportError'>:\\s*"));
        int pos = rx.indexIn(msg);
        while ( pos != -1 ) {
            msg = msg.mid(rx.matchedLength());
            pos = rx.indexIn(msg);
        }

        Base::Console().Error("%s\n", (const char*)msg.toLatin1());
        if (!d->startingUp)
            Base::Console().Error("%s\n", e.getStackTrace().c_str());
        else
            Base::Console().Log("%s\n", e.getStackTrace().c_str());

        if (!d->startingUp) {
            wc.restoreCursor();
            QMessageBox::critical(getMainWindow(), QObject::tr("Workbench failure"),
                QObject::tr("%1").arg(msg));
            wc.setWaitCursor();
        }
    }

    return ok;
}

QPixmap Application::workbenchIcon(const QString& wb) const
{
    Base::PyGILStateLocker lock;
    // get the python workbench object from the dictionary
    PyObject* pcWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, wb.toLatin1());
    // test if the workbench exists
    if (pcWorkbench) {
        // make a unique icon name
        std::stringstream str;
        str << static_cast<const void *>(pcWorkbench) << std::ends;
        std::string iconName = str.str();
        QPixmap icon;
        if (BitmapFactory().findPixmapInCache(iconName.c_str(), icon))
            return icon;

        // get its Icon member if possible
        try {
            Py::Object handler(pcWorkbench);
            if (handler.hasAttr(std::string("Icon"))) {
                Py::Object member = handler.getAttr(std::string("Icon"));
                Py::String data(member);
                std::string content = data.as_std_string("utf-8");

                // test if in XPM format
                QByteArray ary;
                int strlen = (int)content.size();
                ary.resize(strlen);
                for (int j=0; j<strlen; j++)
                    ary[j]=content[j];
                if (ary.indexOf("/* XPM */") > 0) {
                    // Make sure to remove crap around the XPM data
                    QList<QByteArray> lines = ary.split('\n');
                    QByteArray buffer;
                    buffer.reserve(ary.size()+lines.size());
                    for (QList<QByteArray>::iterator it = lines.begin(); it != lines.end(); ++it) {
                        QByteArray trim = it->trimmed();
                        if (!trim.isEmpty()) {
                            buffer.append(trim);
                            buffer.append('\n');
                        }
                    }
                    icon.loadFromData(buffer, "XPM");
                }
                else {
                    // is it a file name...
                    QString file = QString::fromUtf8(content.c_str());
                    icon.load(file);
                    if (icon.isNull()) {
                        // ... or the name of another icon?
                        icon = BitmapFactory().pixmap(file.toUtf8());
                    }
                }

                if (!icon.isNull()) {
                    BitmapFactory().addPixmapToCache(iconName.c_str(), icon);
                }

                return icon;
            }
        }
        catch (Py::Exception& e) {
            e.clear();
        }
    }

    QIcon icon = QApplication::windowIcon();
    if (!icon.isNull()) {
        QList<QSize> s = icon.availableSizes();
        if (!s.isEmpty())
            return icon.pixmap(s[0]);
    }
    return QPixmap();
}

QString Application::workbenchToolTip(const QString& wb) const
{
    // get the python workbench object from the dictionary
    Base::PyGILStateLocker lock;
    PyObject* pcWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, wb.toLatin1());
    // test if the workbench exists
    if (pcWorkbench) {
        // get its ToolTip member if possible
        try {
            Py::Object handler(pcWorkbench);
            Py::Object member = handler.getAttr(std::string("ToolTip"));
            if (member.isString()) {
                Py::String tip(member);
                return QString::fromUtf8(tip.as_std_string("utf-8").c_str());
            }
        }
        catch (Py::Exception& e) {
            e.clear();
        }
    }

    return QString();
}

QString Application::workbenchMenuText(const QString& wb) const
{
    // get the python workbench object from the dictionary
    Base::PyGILStateLocker lock;
    PyObject* pcWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, wb.toLatin1());
    // test if the workbench exists
    if (pcWorkbench) {
        // get its ToolTip member if possible
        Base::PyGILStateLocker locker;
        try {
            Py::Object handler(pcWorkbench);
            Py::Object member = handler.getAttr(std::string("MenuText"));
            if (member.isString()) {
                Py::String tip(member);
                return QString::fromUtf8(tip.as_std_string("utf-8").c_str());
            }
        }
        catch (Py::Exception& e) {
            e.clear();
        }
    }

    return QString();
}

QStringList Application::workbenches(void) const
{
    // If neither 'HiddenWorkbench' nor 'ExtraWorkbench' is set then all workbenches are returned.
    const std::map<std::string,std::string>& config = App::Application::Config();
    std::map<std::string, std::string>::const_iterator ht = config.find("HiddenWorkbench");
    std::map<std::string, std::string>::const_iterator et = config.find("ExtraWorkbench");
    std::map<std::string, std::string>::const_iterator st = config.find("StartWorkbench");
    const char* start = (st != config.end() ? st->second.c_str() : "<none>");
    QStringList hidden, extra;
    if (ht != config.end()) {
        QString items = QString::fromLatin1(ht->second.c_str());
        hidden = items.split(QLatin1Char(';'), QString::SkipEmptyParts);
        if (hidden.isEmpty())
            hidden.push_back(QLatin1String(""));
    }
    if (et != config.end()) {
        QString items = QString::fromLatin1(et->second.c_str());
        extra = items.split(QLatin1Char(';'), QString::SkipEmptyParts);
        if (extra.isEmpty())
            extra.push_back(QLatin1String(""));
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    QStringList wb;
    // insert all items
    while (PyDict_Next(_pcWorkbenchDictionary, &pos, &key, &value)) {
        /* do something interesting with the values... */
#if PY_MAJOR_VERSION >= 3
        const char* wbName = PyUnicode_AsUTF8(key);
#else
        const char* wbName = PyString_AsString(key);
#endif
        // add only allowed workbenches
        bool ok = true;
        if (!extra.isEmpty()&&ok) {
            ok = (extra.indexOf(QString::fromLatin1(wbName)) != -1);
        }
        if (!hidden.isEmpty()&&ok) {
            ok = (hidden.indexOf(QString::fromLatin1(wbName)) == -1);
        }

        // okay the item is visible
        if (ok)
            wb.push_back(QString::fromLatin1(wbName));
        // also allow start workbench in case it is hidden
        else if (strcmp(wbName, start) == 0)
            wb.push_back(QString::fromLatin1(wbName));
    }

    return wb;
}

void Application::setupContextMenu(const char* recipient, MenuItem* items) const
{
    Workbench* actWb = WorkbenchManager::instance()->active();
    if (actWb) {
        // when populating the context-menu of a Python workbench invoke the method
        // 'ContextMenu' of the handler object
        if (actWb->getTypeId().isDerivedFrom(PythonWorkbench::getClassTypeId())) {
            static_cast<PythonWorkbench*>(actWb)->clearContextMenu();
            Base::PyGILStateLocker lock;
            PyObject* pWorkbench = 0;
            pWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, actWb->name().c_str());

            try {
                // call its GetClassName method if possible
                Py::Object handler(pWorkbench);
                Py::Callable method(handler.getAttr(std::string("ContextMenu")));
                Py::Tuple args(1);
                args.setItem(0, Py::String(recipient));
                method.apply(args);
            }
            catch (Py::Exception& e) {
                Py::Object o = Py::type(e);
                e.clear();
                if (o.isString()) {
                    Py::String s(o);
                    std::clog << "Application::setupContextMenu: " << s.as_std_string("utf-8") << std::endl;
                }
            }
        }
        actWb->setupContextMenu(recipient, items);
    }
}

bool Application::isClosing(void)
{
    return d->isClosing;
}

MacroManager *Application::macroManager(void)
{
    return d->macroMngr;
}

CommandManager &Application::commandManager(void)
{
    return d->commandManager;
}

//**************************************************************************
// Init, Destruct and singleton

#if QT_VERSION >= 0x050000
typedef void (*_qt_msg_handler_old)(QtMsgType, const QMessageLogContext &, const QString &);
#else
typedef void (*_qt_msg_handler_old)(QtMsgType type, const char *msg);
#endif
_qt_msg_handler_old old_qtmsg_handler = 0;

#if QT_VERSION >= 0x050000
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
#ifdef FC_DEBUG
    switch (type)
    {
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
#endif
    case QtDebugMsg:
        Base::Console().Message("%s\n", msg.toUtf8().constData());
        break;
    case QtWarningMsg:
        Base::Console().Warning("%s\n", msg.toUtf8().constData());
        break;
    case QtCriticalMsg:
        Base::Console().Error("%s\n", msg.toUtf8().constData());
        break;
    case QtFatalMsg:
        Base::Console().Error("%s\n", msg.toUtf8().constData());
        abort();                    // deliberately core dump
    }
#ifdef FC_OS_WIN32
    if (old_qtmsg_handler)
        (*old_qtmsg_handler)(type, context, msg);
#endif
#else
    // do not stress user with Qt internals but write to log file if enabled
    Q_UNUSED(type);
    Base::Console().Log("%s\n", msg.toUtf8().constData());
#endif
}
#else
void messageHandler(QtMsgType type, const char *msg)
{
#ifdef FC_DEBUG
    switch (type)
    {
    case QtDebugMsg:
        Base::Console().Message("%s\n", msg);
        break;
    case QtWarningMsg:
        Base::Console().Warning("%s\n", msg);
        break;
    case QtCriticalMsg:
        Base::Console().Error("%s\n", msg);
        break;
    case QtFatalMsg:
        Base::Console().Error("%s\n", msg);
        abort();                    // deliberately core dump
    }
#ifdef FC_OS_WIN32
    if (old_qtmsg_handler)
        (*old_qtmsg_handler)(type, msg);
#endif
#else
    // do not stress user with Qt internals but write to log file if enabled
    Q_UNUSED(type);
    Base::Console().Log("%s\n", msg);
#endif
}
#endif

#ifdef FC_DEBUG // redirect Coin messages to FreeCAD
void messageHandlerCoin(const SoError * error, void * /*userdata*/)
{
    if (error && error->getTypeId() == SoDebugError::getClassTypeId()) {
        const SoDebugError* dbg = static_cast<const SoDebugError*>(error);
        const char* msg = error->getDebugString().getString();
        switch (dbg->getSeverity())
        {
        case SoDebugError::INFO:
            Base::Console().Message("%s\n", msg);
            break;
        case SoDebugError::WARNING:
            Base::Console().Warning("%s\n", msg);
            break;
        default: // error
            Base::Console().Error("%s\n", msg);
            break;
        }
#ifdef FC_OS_WIN32
    if (old_qtmsg_handler)
#if QT_VERSION >=0x050000
        (*old_qtmsg_handler)(QtDebugMsg, QMessageLogContext(), QString::fromLatin1(msg));
#else
        (*old_qtmsg_handler)(QtDebugMsg, msg);
#endif
#endif
    }
    else if (error) {
        const char* msg = error->getDebugString().getString();
        Base::Console().Log( msg );
    }
}

#endif

// To fix bug #0000345 move Q_INIT_RESOURCE() outside initApplication()
static void init_resources()
{
    // init resources
    Q_INIT_RESOURCE(resource);
    Q_INIT_RESOURCE(translation);
}

void Application::initApplication(void)
{
    static bool init = false;
    if (init) {
        Base::Console().Error("Tried to run Gui::Application::initApplication() twice!\n");
        return;
    }

    try {
        initTypes();
        new Base::ScriptProducer( "FreeCADGuiInit", FreeCADGuiInit );
        init_resources();
#if QT_VERSION >=0x050000
        old_qtmsg_handler = qInstallMessageHandler(messageHandler);
#else
        old_qtmsg_handler = qInstallMsgHandler(messageHandler);
#endif
        init = true;
    }
    catch (...) {
        // force to flush the log
        App::Application::destructObserver();
        throw;
    }
}

void Application::initTypes(void)
{
    // views
    Gui::BaseView                               ::init();
    Gui::MDIView                                ::init();
    Gui::View3DInventor                         ::init();
    Gui::AbstractSplitView                      ::init();
    Gui::SplitView3DInventor                    ::init();
    // View Provider
    Gui::ViewProvider                           ::init();
    Gui::ViewProviderExtension                  ::init();
    Gui::ViewProviderExtensionPython            ::init();
    Gui::ViewProviderGroupExtension             ::init();
    Gui::ViewProviderGroupExtensionPython       ::init();
    Gui::ViewProviderGeoFeatureGroupExtension   ::init();
    Gui::ViewProviderGeoFeatureGroupExtensionPython::init();
    Gui::ViewProviderOriginGroupExtension       ::init();
    Gui::ViewProviderOriginGroupExtensionPython ::init();
    Gui::ViewProviderExtern                     ::init();
    Gui::ViewProviderDocumentObject             ::init();
    Gui::ViewProviderFeature                    ::init();
    Gui::ViewProviderDocumentObjectGroup        ::init();
    Gui::ViewProviderDocumentObjectGroupPython  ::init();
    Gui::ViewProviderDragger                    ::init();
    Gui::ViewProviderGeometryObject             ::init();
    Gui::ViewProviderInventorObject             ::init();
    Gui::ViewProviderVRMLObject                 ::init();
    Gui::ViewProviderAnnotation                 ::init();
    Gui::ViewProviderAnnotationLabel            ::init();
    Gui::ViewProviderPointMarker                ::init();
    Gui::ViewProviderMeasureDistance            ::init();
    Gui::ViewProviderPythonFeature              ::init();
    Gui::ViewProviderPythonGeometry             ::init();
    Gui::ViewProviderPlacement                  ::init();
    Gui::ViewProviderPlacementPython            ::init();
    Gui::ViewProviderOriginFeature              ::init();
    Gui::ViewProviderPlane                      ::init();
    Gui::ViewProviderLine                       ::init();
    Gui::ViewProviderGeoFeatureGroup            ::init();
    Gui::ViewProviderGeoFeatureGroupPython      ::init();
    Gui::ViewProviderOriginGroup                ::init();
    Gui::ViewProviderPart                       ::init();
    Gui::ViewProviderOrigin                     ::init();
    Gui::ViewProviderMaterialObject             ::init();
    Gui::ViewProviderMaterialObjectPython       ::init();
    Gui::ViewProviderTextDocument               ::init();
    Gui::ViewProviderLinkObserver               ::init();
    Gui::LinkView                               ::init();
    Gui::ViewProviderLink                       ::init();
    Gui::ViewProviderLinkPython                 ::init();
    Gui::AxisOrigin                             ::init();

    // Workbench
    Gui::Workbench                              ::init();
    Gui::StdWorkbench                           ::init();
    Gui::BlankWorkbench                         ::init();
    Gui::NoneWorkbench                          ::init();
    Gui::TestWorkbench                          ::init();
    Gui::PythonBaseWorkbench                    ::init();
    Gui::PythonBlankWorkbench                   ::init();
    Gui::PythonWorkbench                        ::init();

    // register transaction type
    new App::TransactionProducer<TransactionViewProvider>
            (ViewProviderDocumentObject::getClassTypeId());
}

void Application::initOpenInventor(void)
{
    // init the Inventor subsystem
    SoDB::init();
    SIM::Coin3D::Quarter::Quarter::init();
    SoFCDB::init();
}

void Application::runInitGuiScript(void)
{
    Base::Interpreter().runString(Base::ScriptFactory().ProduceScript("FreeCADGuiInit"));
}

void Application::runApplication(void)
{
    const std::map<std::string,std::string>& cfg = App::Application::Config();
    std::map<std::string,std::string>::const_iterator it;

    // A new QApplication
    Base::Console().Log("Init: Creating Gui::Application and QApplication\n");
    // if application not yet created by the splasher
    int argc = App::Application::GetARGC();
    GUISingleApplication mainApp(argc, App::Application::GetARGV());
    // http://forum.freecadweb.org/viewtopic.php?f=3&t=15540
    mainApp.setAttribute(Qt::AA_DontShowIconsInMenus, false);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    mainApp.setAttribute(Qt::AA_UseDesktopOpenGL);
#endif

#ifdef Q_OS_UNIX
    // Make sure that we use '.' as decimal point. See also
    // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=559846
    // and issue #0002891
    // http://doc.qt.io/qt-5/qcoreapplication.html#locale-settings
    setlocale(LC_NUMERIC, "C");
#endif

    // check if a single or multiple instances can run
    it = cfg.find("SingleInstance");
    if (it != cfg.end() && mainApp.isRunning()) {
        // send the file names to be opened to the server application so that this
        // opens them
        QDir cwd = QDir::current();
        std::list<std::string> files = App::Application::getCmdLineFiles();
        for (std::list<std::string>::iterator jt = files.begin(); jt != files.end(); ++jt) {
            QString fn = QString::fromUtf8(jt->c_str(), static_cast<int>(jt->size()));
            QFileInfo fi(fn);
            // if path name is relative make it absolute because the running instance
            // cannot determine the full path when trying to load the file
            if (fi.isRelative()) {
                fn = cwd.absoluteFilePath(fn);
                fn = QDir::cleanPath(fn);
            }

            QByteArray msg = fn.toUtf8();
            msg.prepend("OpenFile:");
            if (!mainApp.sendMessage(msg)) {
                qWarning("Failed to send message to server");
                break;
            }
        }
        return;
    }

#if QT_VERSION >= 0x050600
    //Enable automatic scaling based on pixel density of display (added in Qt 5.6)
    mainApp.setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= 0x050100
    //Enable support for highres images (added in Qt 5.1, but off by default)
    mainApp.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    // set application icon and window title
    it = cfg.find("Application");
    if (it != cfg.end()) {
        mainApp.setApplicationName(QString::fromUtf8(it->second.c_str()));
    }
    else {
        mainApp.setApplicationName(QString::fromUtf8(App::GetApplication().getExecutableName()));
    }
#ifndef Q_OS_MACX
    mainApp.setWindowIcon(Gui::BitmapFactory().pixmap(App::Application::Config()["AppIcon"].c_str()));
#endif
    QString plugin;
    plugin = QString::fromUtf8(App::GetApplication().getHomePath());
    plugin += QLatin1String("/plugins");
    QCoreApplication::addLibraryPath(plugin);

    // setup the search paths for Qt style sheets
    QStringList qssPaths;
    qssPaths << QString::fromUtf8((App::Application::getUserAppDataDir() + "Gui/Stylesheets/").c_str())
             << QString::fromUtf8((App::Application::getResourceDir() + "Gui/Stylesheets/").c_str())
             << QLatin1String(":/stylesheets");
    QDir::setSearchPaths(QString::fromLatin1("qss"), qssPaths);

    // set search paths for images
    QStringList imagePaths;
    imagePaths << QString::fromUtf8((App::Application::getUserAppDataDir() + "Gui/images").c_str())
               << QString::fromUtf8((App::Application::getUserAppDataDir() + "pixmaps").c_str())
               << QLatin1String(":/icons");
    QDir::setSearchPaths(QString::fromLatin1("images"), imagePaths);

    // register action style event type
    ActionStyleEvent::EventType = QEvent::registerEventType(QEvent::User + 1);

    // check for OpenGL
#if !defined(HAVE_QT5_OPENGL)
    if (!QGLFormat::hasOpenGL()) {
        QMessageBox::critical(0, QObject::tr("No OpenGL"), QObject::tr("This system does not support OpenGL"));
        throw Base::RuntimeError("This system does not support OpenGL");
    }
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        Base::Console().Log("This system does not support framebuffer objects\n");
    }
    if (!QGLPixelBuffer::hasOpenGLPbuffers()) {
        Base::Console().Log("This system does not support pbuffers\n");
    }

    QGLFormat::OpenGLVersionFlags version = QGLFormat::openGLVersionFlags ();
    if (version & QGLFormat::OpenGL_Version_3_0)
        Base::Console().Log("OpenGL version 3.0 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_2_1)
        Base::Console().Log("OpenGL version 2.1 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_2_0)
        Base::Console().Log("OpenGL version 2.0 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_1_5)
        Base::Console().Log("OpenGL version 1.5 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_1_4)
        Base::Console().Log("OpenGL version 1.4 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_1_3)
        Base::Console().Log("OpenGL version 1.3 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_1_2)
        Base::Console().Log("OpenGL version 1.2 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_1_1)
        Base::Console().Log("OpenGL version 1.1 or higher is present\n");
    else if (version & QGLFormat::OpenGL_Version_None)
        Base::Console().Log("No OpenGL is present or no OpenGL context is current\n");
#endif

#if !defined(Q_OS_LINUX)
    QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << QString::fromLatin1(":/icons/FreeCAD-default"));
    QIcon::setThemeName(QLatin1String("FreeCAD-default"));
#endif

    ParameterGrp::handle hTheme = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Bitmaps/Theme");
    std::string searchpath = hTheme->GetASCII("SearchPath");
    if (!searchpath.empty()) {
        QStringList searchPaths = QIcon::themeSearchPaths();
        searchPaths.prepend(QString::fromUtf8(searchpath.c_str()));
        QIcon::setThemeSearchPaths(searchPaths);
    }

    std::string name = hTheme->GetASCII("Name");
    if (!name.empty()) {
        QIcon::setThemeName(QString::fromLatin1(name.c_str()));
    }

#if defined(FC_OS_LINUX)
    // See #0001588
    QString path = FileDialog::restoreLocation();
    FileDialog::setWorkingDirectory(QDir::currentPath());
    FileDialog::saveLocation(path);
#else
    FileDialog::setWorkingDirectory(FileDialog::restoreLocation());
#endif

    Application app(true);
    MainWindow mw;
    mw.setProperty("QuitOnClosed", true);

    // allow to disable version number
    ParameterGrp::handle hGen = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General");
    bool showVersion = hGen->GetBool("ShowVersionInTitle",true);

    if (showVersion) {
        // set main window title with FreeCAD Version
        std::map<std::string, std::string>& config = App::Application::Config();
        QString major  = QString::fromLatin1(config["BuildVersionMajor"].c_str());
        QString minor  = QString::fromLatin1(config["BuildVersionMinor"].c_str());
        QString title = QString::fromLatin1("%1 %2.%3").arg(mainApp.applicationName(), major, minor);
        mw.setWindowTitle(title);
    } else {
        mw.setWindowTitle(mainApp.applicationName());
    }

    QObject::connect(&mainApp, SIGNAL(messageReceived(const QList<QByteArray> &)),
                     &mw, SLOT(processMessages(const QList<QByteArray> &)));

    ParameterGrp::handle hDocGrp = WindowParameter::getDefaultParameter()->GetGroup("Document");
    int timeout = hDocGrp->GetInt("AutoSaveTimeout", 15); // 15 min
    if (!hDocGrp->GetBool("AutoSaveEnabled", true))
        timeout = 0;
    AutoSaver::instance()->setTimeout(timeout * 60000);
    AutoSaver::instance()->setCompressed(hDocGrp->GetBool("AutoSaveCompressed", true));

    // set toolbar icon size
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    int size = hGrp->GetInt("ToolbarIconSize", 0);
    if (size >= 16) // must not be lower than this
        mw.setIconSize(QSize(size,size));

#if defined(HAVE_QT5_OPENGL)
    {
        QWindow window;
        window.setSurfaceType(QWindow::OpenGLSurface);
        window.create();

        QOpenGLContext context;
        if (context.create()) {
            context.makeCurrent(&window);
            if (!context.functions()->hasOpenGLFeature(QOpenGLFunctions::Framebuffers)) {
                Base::Console().Log("This system does not support framebuffer objects\n");
            }
            if (!context.functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures)) {
                Base::Console().Log("This system does not support NPOT textures\n");
            }

            int major = context.format().majorVersion();
            int minor = context.format().minorVersion();
            const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
            Base::Console().Log("OpenGL version is: %d.%d (%s)\n", major, minor, glVersion);
        }
    }
#endif

    // init the Inventor subsystem
    initOpenInventor();

    QString home = QString::fromUtf8(App::GetApplication().getHomePath());

    it = cfg.find("WindowTitle");
    if (it != cfg.end()) {
        QString title = QString::fromUtf8(it->second.c_str());
        mw.setWindowTitle(title);
    }
    it = cfg.find("WindowIcon");
    if (it != cfg.end()) {
        QString path = QString::fromUtf8(it->second.c_str());
        if (QDir(path).isRelative()) {
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }
        QApplication::setWindowIcon(QIcon(path));
    }
    it = cfg.find("ProgramLogo");
    if (it != cfg.end()) {
        QString path = QString::fromUtf8(it->second.c_str());
        if (QDir(path).isRelative()) {
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }
        QPixmap px(path);
        if (!px.isNull()) {
            QLabel* logo = new QLabel();
            logo->setPixmap(px.scaledToHeight(32));
            mw.statusBar()->addPermanentWidget(logo, 0);
            logo->setFrameShape(QFrame::NoFrame);
        }
    }
    bool hidden = false;
    it = cfg.find("StartHidden");
    if (it != cfg.end()) {
        hidden = true;
    }

    // show splasher while initializing the GUI
    if (!hidden)
        mw.startSplasher();

    // running the GUI init script
    try {
        Base::Console().Log("Run Gui init script\n");
        runInitGuiScript();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Error in FreeCADGuiInit.py: %s\n", e.what());
        mw.stopSplasher();
        throw;
    }

    // stop splash screen and set immediately the active window that may be of interest
    // for scripts using Python binding for Qt
    mw.stopSplasher();
    mainApp.setActiveWindow(&mw);

    // Activate the correct workbench
    std::string start = App::Application::Config()["StartWorkbench"];
    Base::Console().Log("Init: Activating default workbench %s\n", start.c_str());
    start = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                           GetASCII("AutoloadModule", start.c_str());
    // if the auto workbench is not visible then force to use the default workbech
    // and replace the wrong entry in the parameters
    QStringList wb = app.workbenches();
    if (!wb.contains(QString::fromLatin1(start.c_str()))) {
        start = App::Application::Config()["StartWorkbench"];
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                              SetASCII("AutoloadModule", start.c_str());
    }

    // Call this before showing the main window because otherwise:
    // 1. it shows a white window for a few seconds which doesn't look nice
    // 2. the layout of the toolbars is completely broken
    app.activateWorkbench(start.c_str());

    // show the main window
    if (!hidden) {
        Base::Console().Log("Init: Showing main window\n");
        mw.loadWindowSettings();
    }

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    QMdiArea* mdi = mw.findChild<QMdiArea*>();
    mdi->setProperty("showImage", hGrp->GetBool("TiledBackground", false));

    std::string style = hGrp->GetASCII("StyleSheet");
    if (!style.empty()) {
        QFile f(QLatin1String(style.c_str()));
        if (f.open(QFile::ReadOnly)) {
            mdi->setBackground(QBrush(Qt::NoBrush));
            QTextStream str(&f);
            qApp->setStyleSheet(str.readAll());

            ActionStyleEvent e(ActionStyleEvent::Clear);
            qApp->sendEvent(&mw, &e);
        }
    }
    else {
        if (hGrp->GetBool("TiledBackground", false)) {
            mdi->setBackground(QPixmap(QLatin1String("images:background.png")));
        }
#if QT_VERSION == 0x050600 && defined(Q_OS_WIN32)
        // Under Windows the tree indicator branch gets corrupted after a while.
        // For more details see also https://bugreports.qt.io/browse/QTBUG-52230
        // and https://codereview.qt-project.org/#/c/154357/2//ALL,unified
        // A workaround for Qt 5.6.0 is to set a minimal style sheet.
        QString qss = QString::fromLatin1(
               "QTreeView::branch:closed:has-children  {\n"
               "    image: url(:/icons/style/windows_branch_closed.png);\n"
               "}\n"
               "\n"
               "QTreeView::branch:open:has-children  {\n"
               "    image: url(:/icons/style/windows_branch_open.png);\n"
               "}\n");
        qApp->setStyleSheet(qss);
#endif
    }

    //initialize spaceball.
    mainApp.initSpaceball(&mw);

#ifdef FC_DEBUG // redirect Coin messages to FreeCAD
    SoDebugError::setHandlerCallback( messageHandlerCoin, 0 );
#endif


    Instance->d->startingUp = false;

    // gets called once we start the event loop
    QTimer::singleShot(0, &mw, SLOT(delayedStartup()));

    // run the Application event loop
    Base::Console().Log("Init: Entering event loop\n");

    // boot phase reference point
    // https://forum.freecadweb.org/viewtopic.php?f=10&t=21665
    Gui::getMainWindow()->setProperty("eventLoop", true);

    try {
        std::stringstream s;
        s << App::Application::getTempPath() << App::GetApplication().getExecutableName()
          << "_" << QCoreApplication::applicationPid() << ".lock";
        // open a lock file with the PID
        Base::FileInfo fi(s.str());
        Base::ofstream lock(fi);
        boost::interprocess::file_lock flock(s.str().c_str());
        flock.lock();

        mainApp.exec();
        // Qt can't handle exceptions thrown from event handlers, so we need
        // to manually rethrow SystemExitExceptions.
        if(mainApp.caughtException.get())
            throw Base::SystemExitException(*mainApp.caughtException.get());

        // close the lock file, in case of a crash we can see the existing lock file
        // on the next restart and try to repair the documents, if needed.
        flock.unlock();
        lock.close();
        fi.deleteFile();
    }
    catch (const Base::SystemExitException&) {
        Base::Console().Message("System exit\n");
        throw;
    }
    catch (const std::exception& e) {
        // catching nasty stuff coming out of the event loop
        App::Application::destructObserver();
        Base::Console().Error("Event loop left through unhandled exception: %s\n", e.what());
        throw;
    }
    catch (...) {
        // catching nasty stuff coming out of the event loop
        App::Application::destructObserver();
        Base::Console().Error("Event loop left through unhandled exception\n");
        throw;
    }

    Base::Console().Log("Finish: Event loop left\n");
}

void Application::checkForPreviousCrashes()
{
    QDir tmp = QString::fromUtf8(App::Application::getTempPath().c_str());
    tmp.setNameFilters(QStringList() << QString::fromLatin1("*.lock"));
    tmp.setFilter(QDir::Files);

    QList<QFileInfo> restoreDocFiles;
    QString exeName = QString::fromLatin1(App::GetApplication().getExecutableName());
    QList<QFileInfo> locks = tmp.entryInfoList();
    for (QList<QFileInfo>::iterator it = locks.begin(); it != locks.end(); ++it) {
        QString bn = it->baseName();
        // ignore the lock file for this instance
        QString pid = QString::number(QCoreApplication::applicationPid());
        if (bn.startsWith(exeName) && bn.indexOf(pid) < 0) {
            QString fn = it->absoluteFilePath();
            boost::interprocess::file_lock flock((const char*)fn.toLocal8Bit());
            if (flock.try_lock()) {
                // OK, this file is a leftover from a previous crash
                QString crashed_pid = bn.mid(exeName.length()+1);
                // search for transient directories with this PID
                QString filter;
                QTextStream str(&filter);
                str << exeName << "_Doc_*_" << crashed_pid;
                tmp.setNameFilters(QStringList() << filter);
                tmp.setFilter(QDir::Dirs);
                QList<QFileInfo> dirs = tmp.entryInfoList();
                if (dirs.isEmpty()) {
                    // delete the lock file immediately if no transient directories are related
                    tmp.remove(fn);
                }
                else {
                    int countDeletedDocs = 0;
                    QString recovery_files = QString::fromLatin1("fc_recovery_files");
                    for (QList<QFileInfo>::iterator it = dirs.begin(); it != dirs.end(); ++it) {
                        QDir doc_dir(it->absoluteFilePath());
                        doc_dir.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);
                        uint entries = doc_dir.entryList().count();
                        if (entries == 0) {
                            // in this case we can delete the transient directory because
                            // we cannot do anything
                            if (tmp.rmdir(it->filePath()))
                                countDeletedDocs++;
                        }
                        // search for the existence of a recovery file
                        else if (doc_dir.exists(QLatin1String("fc_recovery_file.xml"))) {
                            // store the transient directory in case it's not empty
                            restoreDocFiles << *it;
                        }
                        // search for the 'fc_recovery_files' sub-directory and check that it's the only entry
                        else if (entries == 1 && doc_dir.exists(recovery_files)) {
                            // if the sub-directory is empty delete the transient directory
                            QDir rec_dir(doc_dir.absoluteFilePath(recovery_files));
                            rec_dir.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);
                            if (rec_dir.entryList().isEmpty()) {
                                doc_dir.rmdir(recovery_files);
                                if (tmp.rmdir(it->filePath()))
                                    countDeletedDocs++;
                            }
                        }
                    }

                    // all directories corresponding to the lock file have been deleted
                    // so delete the lock file, too
                    if (countDeletedDocs == dirs.size()) {
                        tmp.remove(fn);
                    }
                }
            }
        }
    }

    if (!restoreDocFiles.isEmpty()) {
        Gui::Dialog::DocumentRecovery dlg(restoreDocFiles, Gui::getMainWindow());
        if (dlg.foundDocuments())
            dlg.exec();
    }
}

App::Document *Application::reopen(App::Document *doc) {
    if(!doc) return 0;
    std::vector<std::string> docs;
    std::string name = doc->FileName.getValue();
    if(doc->testStatus(App::Document::PartialDoc))
        docs.push_back(doc->FileName.getValue());
    else {
        for(auto d : doc->getDependentDocuments(true))
            if(d->testStatus(App::Document::PartialDoc))
                docs.push_back(d->FileName.getValue());
    }
    std::set<const Gui::Document*> untouchedDocs;
    for(auto &v : d->documents) {
        if(!v.second->isModified() && !v.second->getDocument()->isTouched())
            untouchedDocs.insert(v.second);
    }
    for(auto &file : docs) 
        open(file.c_str(),"FreeCAD");

    doc = 0;
    for(auto &v : d->documents) {
        if(name == v.first->FileName.getValue())
            doc = const_cast<App::Document*>(v.first);
        if(untouchedDocs.count(v.second)) {
            if(!v.second->isModified()) continue;
            bool reset = true;
            for(auto obj : v.second->getDocument()->getObjects()) {
                if(!obj->isTouched())
                    continue;
                std::vector<App::Property*> props;
                obj->getPropertyList(props);
                for(auto prop : props){
                    auto link = dynamic_cast<App::PropertyLinkBase*>(prop);
                    if(link && link->checkRestore()) {
                        reset = false;
                        break;
                    }
                }
                if(!reset)
                    break;
            }
            if(reset) {
                v.second->getDocument()->purgeTouched();
                v.second->setModified(false);
            }
        }
    }
    return doc;
}
