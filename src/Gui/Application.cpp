/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <boost/signals.hpp>
# include <boost/bind.hpp>
# include <sstream>
# include <stdexcept>
# include <QCloseEvent>
# include <QDir>
# include <QFileInfo>
# include <QLocale>
# include <QMessageBox>
# include <QPointer>
# include <QGLFormat>
# include <QGLPixelBuffer>
#if QT_VERSION >= 0x040200
# include <QGLFramebufferObject>
#endif
# include <QSessionManager>
# include <QTextStream>
#endif

#include <boost/interprocess/sync/file_lock.hpp>


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
#include "GuiApplicationNativeEventAware.h"
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

#include "SplitView3DInventor.h"
#include "View3DInventor.h"
#include "ViewProvider.h"
#include "ViewProviderExtern.h"
#include "ViewProviderFeature.h"
#include "ViewProviderPythonFeature.h"
#include "ViewProviderDocumentObjectGroup.h"
#include "ViewProviderGeometryObject.h"
#include "ViewProviderInventorObject.h"
#include "ViewProviderVRMLObject.h"
#include "ViewProviderAnnotation.h"
#include "ViewProviderMeasureDistance.h"
#include "ViewProviderPlacement.h"
#include "ViewProviderPlane.h"
#include "ViewProviderMaterialObject.h"

#include "Language/Translator.h"
#include "TaskView/TaskDialogPython.h"
#include "GuiInitScript.h"


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
    MacroManager*  macroMngr;
    /// List of all registered views
    std::list<Gui::BaseView*> passive;
    bool isClosing;
    bool startingUp;
    /// Handles all commands 
    CommandManager commandManager;
};

/** Observer that watches relabeled objects and make sure that the labels inside
 * a document are unique.
 * @note In the FreeCAD design it is explicitly allowed to have duplicate labels
 * (i.e. the user visible text e.g. in the tree view) while the internal names
 * are always guaranteed to be unique.
 */
class ObjectLabelObserver
{
public:
    /// The one and only instance.
    static ObjectLabelObserver* instance();
    /// Destructs the sole instance.
    static void destruct ();

    /** Checks the new label of the object and relabel it if needed
     * to make it unique document-wide
     */
    void slotRelabelObject(const App::DocumentObject&, const App::Property&);

private:
    static ObjectLabelObserver* _singleton;

    ObjectLabelObserver();
    ~ObjectLabelObserver();
    const App::DocumentObject* current;
    ParameterGrp::handle _hPGrp;
};

ObjectLabelObserver* ObjectLabelObserver::_singleton = 0;

ObjectLabelObserver* ObjectLabelObserver::instance()
{
    if (!_singleton)
        _singleton = new ObjectLabelObserver;
    return _singleton;
}

void ObjectLabelObserver::destruct ()
{
    delete _singleton;
    _singleton = 0;
}

void ObjectLabelObserver::slotRelabelObject(const App::DocumentObject& obj, const App::Property& prop)
{
    // observe only the Label property
    if (&prop == &obj.Label) {
        // have we processed this (or another?) object right now?
        if (current) {
            return;
        }

        std::string label = obj.Label.getValue();
        App::Document* doc = obj.getDocument();
        if (doc && !_hPGrp->GetBool("DuplicateLabels")) {
            std::vector<std::string> objectLabels;
            std::vector<App::DocumentObject*>::const_iterator it;
            std::vector<App::DocumentObject*> objs = doc->getObjects();
            bool match = false;

            for (it = objs.begin();it != objs.end();++it) {
                if (*it == &obj)
                    continue; // don't compare object with itself
                std::string objLabel = (*it)->Label.getValue();
                if (!match && objLabel == label)
                    match = true;
                objectLabels.push_back(objLabel);
            }

            // make sure that there is a name conflict otherwise we don't have to do anything
            if (match) {
                // remove number from end to avoid lengthy names
                size_t lastpos = label.length()-1;
                while (label[lastpos] >= 48 && label[lastpos] <= 57)
                    lastpos--;
                label = label.substr(0, lastpos+1);
                label = Base::Tools::getUniqueName(label, objectLabels, 3);
                this->current = &obj;
                const_cast<App::DocumentObject&>(obj).Label.setValue(label);
                this->current = 0;
            }
        }
    }
}

ObjectLabelObserver::ObjectLabelObserver() : current(0)
{
    App::GetApplication().signalChangedObject.connect(boost::bind
        (&ObjectLabelObserver::slotRelabelObject, this, _1, _2));
    _hPGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp");
    _hPGrp = _hPGrp->GetGroup("Preferences")->GetGroup("Document");
}

ObjectLabelObserver::~ObjectLabelObserver()
{
}

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
            std::auto_ptr<Gui::ViewProviderDocumentObject> vp(static_cast<Gui::ViewProviderDocumentObject*>(base));
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
    return PyString_FromString(SoDB::getVersion());
}

static PyObject *
FreeCADGui_getSoQtVersion(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    return PyString_FromString(SoQt::getVersionString());
}

struct PyMethodDef FreeCADGui_methods[] = {
    {"subgraphFromObject",FreeCADGui_subgraphFromObject,METH_VARARGS,
     "subgraphFromObject(object) -> Node\n\n"
     "Return the Inventor subgraph to an object"},
    {"getSoDBVersion",FreeCADGui_getSoDBVersion,METH_VARARGS,
     "getSoDBVersion() -> String\n\n"
     "Return a text string containing the name\n"
     "of the Coin library and version information"},
    {"getSoQtVersion",FreeCADGui_getSoQtVersion,METH_VARARGS,
     "getSoQtVersion() -> String\n\n"
     "Return a text string containing the name\n"
     "of the SoQt library and version information"},
    {NULL, NULL}  /* sentinel */
};

} // namespace Gui

Application::Application(bool GUIenabled)
{
    //App::GetApplication().Attach(this);
    if (GUIenabled) {
        App::GetApplication().signalNewDocument.connect(boost::bind(&Gui::Application::slotNewDocument, this, _1));
        App::GetApplication().signalDeleteDocument.connect(boost::bind(&Gui::Application::slotDeleteDocument, this, _1));
        App::GetApplication().signalRenameDocument.connect(boost::bind(&Gui::Application::slotRenameDocument, this, _1));
        App::GetApplication().signalActiveDocument.connect(boost::bind(&Gui::Application::slotActiveDocument, this, _1));
        App::GetApplication().signalRelabelDocument.connect(boost::bind(&Gui::Application::slotRelabelDocument, this, _1));


        // install the last active language
        ParameterGrp::handle hPGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp");
        hPGrp = hPGrp->GetGroup("Preferences")->GetGroup("General");
        QString lang = QLocale::languageToString(QLocale::system().language());
        Translator::instance()->activateLanguage(hPGrp->GetASCII("Language", (const char*)lang.toAscii()).c_str());
        GetWidgetFactorySupplier();

        ParameterGrp::handle hUnits = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Units");
        Base::UnitsApi::setDecimals(hUnits->GetInt("Decimals", Base::UnitsApi::getDecimals()));

        // Check for the symbols for group separator and deciaml point. They must be different otherwise
        // Qt doesn't work properly.
#if defined(Q_OS_WIN32)
        if (QLocale::system().groupSeparator() == QLocale::system().decimalPoint()) {
            QMessageBox::critical(0, QLatin1String("Invalid system settings"),
                QLatin1String("Your system uses the same symbol for decimal point and group separator.\n\n"
                              "This causes serious problems and makes the application fail to work properly.\n"
                              "Go to the system configuration panel of the OS and fix this issue, please."));
            throw Base::Exception("Invalid system settings");
        }
#endif

        // setting up Python binding
        Base::PyGILStateLocker lock;
        PyObject* module = Py_InitModule3("FreeCADGui", Application::Methods,
            "The functions in the FreeCADGui module allow working with GUI documents,\n"
            "view providers, views, workbenches and much more.\n\n"
            "The FreeCADGui instance provides a list of references of GUI documents which\n"
            "can be addressed by a string. These documents contain the view providers for\n"
            "objects in the associated App document. An App and GUI document can be\n"
            "accessed with the same name.\n\n"
            "The FreeCADGui module also provides a set of functions to work with so called\n"
            "workbenches.");
        Py::Module(module).setAttr(std::string("ActiveDocument"),Py::None());

        UiLoaderPy::init_type();
        Base::Interpreter().addType(UiLoaderPy::type_object(),
            module,"UiLoader");

        //insert Selection module
        PyObject* pSelectionModule = Py_InitModule3("Selection", SelectionSingleton::Methods,
            "Selection module");
        Py_INCREF(pSelectionModule);
        PyModule_AddObject(module, "Selection", pSelectionModule);

        SelectionFilterPy::init_type();
        Base::Interpreter().addType(SelectionFilterPy::type_object(),
            pSelectionModule,"Filter");

        Gui::TaskView::ControlPy::init_type();
        Py::Module(module).setAttr(std::string("Control"),
            Py::Object(Gui::TaskView::ControlPy::getInstance(), true));
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
    PythonDebugModule   ::init_module();
    PythonStdout        ::init_type();
    PythonStderr        ::init_type();
    OutputStdout        ::init_type();
    OutputStderr        ::init_type();
    PythonStdin         ::init_type();
    View3DInventorPy    ::init_type();

    d = new ApplicationP;

    // global access 
    Instance = this;

    // instanciate the workbench dictionary
    _pcWorkbenchDictionary = PyDict_New();

    createStandardOperations();
    MacroCommand::load();
    ObjectLabelObserver::instance();
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
    // we must run the garbage collector before shutting down the SoDB or SoQt 
    // subsystem because we may reference some class objects of them in Python
    Base::Interpreter().cleanupSWIG("SoBase *");
    // finish also Inventor subsystem
    SoFCDB::finish();
    SoQt::done();

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
    MacroCommand::save();
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

    // if the active document is empty and not modified, close it
    // in case of an automatically created empty document at startup
    App::Document* act = App::GetApplication().getActiveDocument();
    Gui::Document* gui = this->getDocument(act);
    if (act && act->countObjects() == 0 && gui && gui->isModified() == false){
        Command::doCommand(Command::App, "App.closeDocument('%s')", act->getName());
        qApp->processEvents(); // an update is needed otherwise the new view isn't shown
    }

    if (Module != 0) {
        // issue module loading
        Command::doCommand(Command::App, "import %s", Module);
        try {
            // load the file with the module
            Command::doCommand(Command::App, "%s.open(\"%s\")", Module, File.filePath().c_str());
            // ViewFit
            if (!File.hasExtension("FCStd") && sendHasMsgToActiveView("ViewFit"))
                //Command::doCommand(Command::Gui, "Gui.activeDocument().activeView().fitAll()");
                Command::doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
            // the original file name is required
            getMainWindow()->appendRecentFile(QString::fromUtf8(File.filePath().c_str()));
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

    if (Module != 0) {
        // issue module loading
        Command::doCommand(Command::App, "import %s", Module);

        try {
            // load the file with the module
            if (File.hasExtension("FCStd")) {
                Command::doCommand(Command::App, "%s.open(\"%s\")"
                                               , Module, File.filePath().c_str());
                if (activeDocument())
                    activeDocument()->setModified(false);
            }
            else {
                Command::doCommand(Command::App, "%s.insert(\"%s\",\"%s\")"
                                               , Module, File.filePath().c_str(), DocName);
                Command::doCommand(Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
                if (getDocument(DocName))
                    getDocument(DocName)->setModified(true);
            }

            // the original file name is required
            getMainWindow()->appendRecentFile(QString::fromUtf8(File.filePath().c_str()));
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
    Base::FileInfo File(FileName);
    std::string te = File.extension();

    if (Module != 0) {
        try {
            std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
                (App::DocumentObject::getClassTypeId(),DocName);
            if (sel.empty()) {
                App::Document* doc = App::GetApplication().getDocument(DocName);
                sel = doc->getObjectsOfType(App::DocumentObject::getClassTypeId());
            }

            std::stringstream str;
            str << "__objs__=[]" << std::endl;
            for (std::vector<App::DocumentObject*>::iterator it = sel.begin(); it != sel.end(); ++it) {
                str << "__objs__.append(FreeCAD.getDocument(\"" << DocName << "\").getObject(\""
                    << (*it)->getNameInDocument() << "\"))" << std::endl;
            }

            str << "import " << Module << std::endl;
            str << Module << ".export(__objs__,\"" << File.filePath() << "\")" << std::endl;
            str << "del __objs__" << std::endl;

            std::string code = str.str();
            // the original file name is required
            if (runPythonCode(code.c_str(), false))
                getMainWindow()->appendRecentFile(QString::fromUtf8(File.filePath().c_str()));
        }
        catch (const Base::PyException& e){
            // Usually thrown if the file is invalid somehow
            e.ReportException();
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
    Gui::CreateTestCommands();
}

void Application::slotNewDocument(const App::Document& Doc)
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
    pDoc->signalRenamedObject.connect(boost::bind(&Gui::Application::slotRenamedObject, this, _1));
    pDoc->signalActivatedObject.connect(boost::bind(&Gui::Application::slotActivatedObject, this, _1));


    signalNewDocument(*pDoc);
    pDoc->createView("View3DIv");
    qApp->processEvents(); // make sure to show the window stuff on the right place
}

void Application::slotDeleteDocument(const App::Document& Doc)
{
    std::map<const App::Document*, Gui::Document*>::iterator doc = d->documents.find(&Doc);
    if (doc == d->documents.end()) {
        Base::Console().Log("GUI document '%s' already deleted\n", Doc.getName());
        return;
    }

    // We must clear the selection here to notify all observers
    Gui::Selection().clearSelection(doc->second->getDocument()->getName());
    signalDeleteDocument(*doc->second);

    // If the active document gets destructed we must set it to 0. If there are further existing documents then the 
    // view that becomes active sets the active document again. So, we needn't worry about this.
    if (d->activeDocument == doc->second)
        setActiveDocument(0);

    // For exception-safety use a smart pointer
    auto_ptr<Document> delDoc (doc->second);
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

void Application::slotActiveDocument(const App::Document& Doc)
{
    std::map<const App::Document*, Gui::Document*>::iterator doc = d->documents.find(&Doc);
    // this can happen when closing a document with two views opened
    if (doc != d->documents.end())
        signalActiveDocument(*doc->second);
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
}

void Application::slotRenamedObject(const ViewProvider& vp)
{
    this->signalRenamedObject(vp);
}

void Application::slotActivatedObject(const ViewProvider& vp)
{
    this->signalActivatedObject(vp);
}

void Application::onLastWindowClosed(Gui::Document* pcDoc)
{
    if (!d->isClosing && pcDoc) {
        try {
            // Call the closing mechanism from Python. This also checks whether pcDoc is the last open document.
            Command::doCommand(Command::Doc, "App.closeDocument(\"%s\")", pcDoc->getDocument()->getName());
        }
        catch (const Base::PyException& e) {
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

/// Getter for the active view
Gui::Document* Application::activeDocument(void) const
{
    return d->activeDocument;
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

    // May be useful for error detection
    if (d->activeDocument) {
        App::Document* doc = d->activeDocument->getDocument();
        Base::Console().Log("Active document is %s (at %p)\n",doc->getName(), doc);
    }
    else {
        Base::Console().Log("No active document\n");
    }

    // notify all views attached to the application (not views belong to a special document)
    for(list<Gui::BaseView*>::iterator It=d->passive.begin();It!=d->passive.end();It++)
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
    App::Document* doc = obj->getDocument();
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
    for (It = d->documents.begin();It != d->documents.end();It++)
        It->second->onUpdate();
    // update all the independed views
    for (std::list<Gui::BaseView*>::iterator It2 = d->passive.begin();It2 != d->passive.end();It2++)
        (*It2)->onUpdate();
}

/// Gets called if a view gets activated, this manages the whole activation scheme
void Application::viewActivated(MDIView* pcView)
{
    // May be useful for error detection
    Base::Console().Log("Active view is %s (at %p)\n",
                 (const char*)pcView->windowTitle().toUtf8(),pcView);

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
    if (d->documents.size() == 0) {
        e->accept();
    }
    else {
        // ask all documents if closable
        std::map<const App::Document*, Gui::Document*>::iterator It;
        for (It = d->documents.begin();It!=d->documents.end();It++) {
            // a document may have several views attached, so ask it directly
#if 0
            MDIView* active = It->second->getActiveView();
            e->setAccepted(active->canClose());
#else
            e->setAccepted(It->second->canClose());
#endif
            if (!e->isAccepted())
                return;
        }
    }

    // ask all passive views if closable
    for (std::list<Gui::BaseView*>::iterator It = d->passive.begin();It!=d->passive.end();It++) {
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

    // we check for the currently active workbench and call its 'Deactivated'
    // method, if available
    PyObject* pcOldWorkbench = 0;
    if (oldWb) {
        pcOldWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, oldWb->name().c_str());
    }

    // get the python workbench object from the dictionary
    Base::PyGILStateLocker lock;
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
            type = result.as_std_string();
            if (Base::Type::fromName(type.c_str()).isDerivedFrom(Gui::PythonBaseWorkbench::getClassTypeId())) {
                Workbench* wb = WorkbenchManager::instance()->createWorkbench(name, type);
                handler.setAttr(std::string("__Workbench__"), Py::Object(wb->getPyObject(), true));
            }

            // import the matching module first
            Py::Callable activate(handler.getAttr(std::string("Initialize")));
            activate.apply(args);

            // Dependent on the implementation of a workbench handler the type
            // can be defined after the call of Initialize()
            if (type.empty()) {
                Py::String result(method.apply(args));
                type = result.as_std_string();
            }
        }

        // does the Python workbench handler have changed the workbench?
        Workbench* curWb = WorkbenchManager::instance()->active();
        if (curWb && curWb->name() == name)
            ok = true; // already active
        // now try to create and activate the matching workbench object
        else if (WorkbenchManager::instance()->activate(name, type)) {
            getMainWindow()->activateWorkbench(QString::fromAscii(name));
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
        QString msg = QString::fromAscii(e.what());
        QRegExp rx;
        // ignore '<type 'exceptions.ImportError'>' prefixes
        rx.setPattern(QLatin1String("^\\s*<type 'exceptions.ImportError'>:\\s*"));
        int pos = rx.indexIn(msg);
        while ( pos != -1 ) {
            msg = msg.mid(rx.matchedLength());
            pos = rx.indexIn(msg);
        }

        Base::Console().Error("%s\n", (const char*)msg.toAscii());
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
    PyObject* pcWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, wb.toAscii());
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
            Py::Object member = handler.getAttr(std::string("Icon"));
            Py::String data(member);
            std::string content = data.as_std_string();

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
        catch (Py::Exception& e) {
            e.clear();
        }
    }

    return QPixmap();
}

QString Application::workbenchToolTip(const QString& wb) const
{
    // get the python workbench object from the dictionary
    Base::PyGILStateLocker lock;
    PyObject* pcWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, wb.toAscii());
    // test if the workbench exists
    if (pcWorkbench) {
        // get its ToolTip member if possible
        try {
            Py::Object handler(pcWorkbench);
            Py::Object member = handler.getAttr(std::string("ToolTip"));
            if (member.isString()) {
                Py::String tip(member);
                return QString::fromUtf8(tip.as_std_string().c_str());
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
    PyObject* pcWorkbench = PyDict_GetItemString(_pcWorkbenchDictionary, wb.toAscii());
    // test if the workbench exists
    if (pcWorkbench) {
        // get its ToolTip member if possible
        Base::PyGILStateLocker locker;
        try {
            Py::Object handler(pcWorkbench);
            Py::Object member = handler.getAttr(std::string("MenuText"));
            if (member.isString()) {
                Py::String tip(member);
                return QString::fromUtf8(tip.as_std_string().c_str());
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
        QString items = QString::fromAscii(ht->second.c_str());
        hidden = items.split(QLatin1Char(';'), QString::SkipEmptyParts);
        if (hidden.isEmpty())
            hidden.push_back(QLatin1String(""));
    }
    if (et != config.end()) { 
        QString items = QString::fromAscii(et->second.c_str());
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
        const char* wbName = PyString_AsString(key);
        // add only allowed workbenches
        bool ok = true;
        if (!extra.isEmpty()&&ok) {
            ok = (extra.indexOf(QString::fromAscii(wbName)) != -1);
        }
        if (!hidden.isEmpty()&&ok) {
            ok = (hidden.indexOf(QString::fromAscii(wbName)) == -1);
        }
    
        // okay the item is visible
        if (ok)
            wb.push_back(QString::fromAscii(wbName));
        // also allow start workbench in case it is hidden
        else if (strcmp(wbName, start) == 0)
            wb.push_back(QString::fromAscii(wbName));
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
                    std::clog << "Application::setupContextMenu: " << s.as_std_string() << std::endl;
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

void Application::runCommand(bool bForce, const char* sCmd,...)
{
    // temp buffer
    size_t format_len = std::strlen(sCmd)+4024;
    char* format = (char*) malloc(format_len);
    va_list namelessVars;
    va_start(namelessVars, sCmd);  // Get the "..." vars
    vsnprintf(format, format_len, sCmd, namelessVars);
    va_end(namelessVars);

    if (bForce)
        d->macroMngr->addLine(MacroManager::App,format);
    else
        d->macroMngr->addLine(MacroManager::Gui,format);

    try { 
        Base::Interpreter().runString(format);
    }
    catch (...) {
        // free memory to avoid a leak if an exception occurred
        free (format);
        throw;
    }

    free (format);
}

bool Application::runPythonCode(const char* cmd, bool gui, bool pyexc)
{
    if (gui)
        d->macroMngr->addLine(MacroManager::Gui,cmd);
    else
        d->macroMngr->addLine(MacroManager::App,cmd);

    try {
        Base::Interpreter().runString(cmd);
        return true;
    }
    catch (Base::PyException &e) {
        if (pyexc) {
            e.ReportException();
            Base::Console().Error("Stack Trace: %s\n",e.getStackTrace().c_str());
        }
        else {
            throw; // re-throw to handle in calling instance
        }
    }
    catch (Base::AbortException&) {
    }
    catch (Base::Exception &e) {
        e.ReportException();
    }
    catch (std::exception &e) {
        std::string str;
        str += "C++ exception thrown (";
        str += e.what();
        str += ")";
        Base::Console().Error(str.c_str());
    }
    catch (const char* e) {
        Base::Console().Error("%s\n", e);
    }
#ifndef FC_DEBUG
    catch (...) {
        Base::Console().Error("Unknown C++ exception in command thrown\n");
    }
#endif
    return false;
}

//**************************************************************************
// Init, Destruct and ingleton

typedef void (*_qt_msg_handler_old)(QtMsgType type, const char *msg);
_qt_msg_handler_old old_qtmsg_handler = 0;

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
    Base::Console().Log("%s\n", msg);
#endif
}

#ifdef FC_DEBUG // redirect Coin messages to FreeCAD
void messageHandlerCoin(const SoError * error, void * userdata)
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
        (*old_qtmsg_handler)(QtDebugMsg, msg);
#endif
    }
    else if (error) {
        const char* msg = error->getDebugString().getString();
        Base::Console().Log( msg );
    }
}

void messageHandlerSoQt(const SbString errmsg, SoQt::FatalErrors errcode, void *userdata)
{
    Base::Console().Error( errmsg.getString() );
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
    try {
        initTypes();
        new Base::ScriptProducer( "FreeCADGuiInit", FreeCADGuiInit );
        init_resources();
        old_qtmsg_handler = qInstallMsgHandler(messageHandler);
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
    Gui::ViewProviderExtern                     ::init();
    Gui::ViewProviderDocumentObject             ::init();
    Gui::ViewProviderFeature                    ::init();
    Gui::ViewProviderDocumentObjectGroup        ::init();
    Gui::ViewProviderDocumentObjectGroupPython  ::init();
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
    Gui::ViewProviderPlane                      ::init();
    Gui::ViewProviderMaterialObject             ::init();
    Gui::ViewProviderMaterialObjectPython       ::init();

    // Workbench
    Gui::Workbench                              ::init();
    Gui::StdWorkbench                           ::init();
    Gui::BlankWorkbench                         ::init();
    Gui::NoneWorkbench                          ::init();
    Gui::TestWorkbench                          ::init();
    Gui::PythonBaseWorkbench                    ::init();
    Gui::PythonBlankWorkbench                   ::init();
    Gui::PythonWorkbench                        ::init();
}

namespace Gui {
/** Override QCoreApplication::notify() to fetch exceptions in Qt widgets
 * properly that are not handled in the event handler or slot.
 */
class GUIApplication : public GUIApplicationNativeEventAware
{
    int systemExit;
public:
    GUIApplication(int & argc, char ** argv, int exitcode)
        : GUIApplicationNativeEventAware(argc, argv), systemExit(exitcode)
    {
    }

    /**
     * Make forwarding events exception-safe and get more detailed information
     * where an unhandled exception comes from.
     */
    bool notify (QObject * receiver, QEvent * event)
    {
        if (!receiver && event) {
            Base::Console().Log("GUIApplication::notify: Unexpected null receiver, event type: %d\n",
                (int)event->type());
        }
        try {
            if (event->type() == Spaceball::ButtonEvent::ButtonEventType || 
                event->type() == Spaceball::MotionEvent::MotionEventType)
                return processSpaceballEvent(receiver, event);
            else
                return QApplication::notify(receiver, event);
        }
        catch (const Base::SystemExitException&) {
            qApp->exit(systemExit);
            return true;
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Unhandled Base::Exception caught in GUIApplication::notify.\n"
                                  "The error message is: %s\n", e.what());
        }
        catch (const std::exception& e) {
            Base::Console().Error("Unhandled std::exception caught in GUIApplication::notify.\n"
                                  "The error message is: %s\n", e.what());
        }
        catch (...) {
            Base::Console().Error("Unhandled unknown exception caught in GUIApplication::notify.\n");
        }

        // Print some more information to the log file (if active) to ease bug fixing
        if (receiver && event) {
            std::stringstream dump;
            dump << "The event type " << (int)event->type() << " was sent to "
                 << receiver->metaObject()->className() << "\n";
            dump << "Object tree:\n";
            if (receiver->isWidgetType()) {
                QWidget* w = qobject_cast<QWidget*>(receiver);
                while (w) {
                    dump << "\t";
                    dump << w->metaObject()->className();
                    QString name = w->objectName();
                    if (!name.isEmpty())
                        dump << " (" << (const char*)name.toUtf8() << ")";
                    w = w->parentWidget();
                    if (w)
                        dump << " is child of\n";
                }
                std::string str = dump.str();
                Base::Console().Log("%s",str.c_str());
            }
        }

        return true;
    }
    void commitData(QSessionManager &manager)
    {
        if (manager.allowsInteraction()) {
            if (!Gui::getMainWindow()->close()) {
                // cancel the shutdown
                manager.release();
                manager.cancel();
            }
        }
        else {
            // no user interaction allowed, thus close all documents and
            // the main window
            App::GetApplication().closeAllDocuments();
            Gui::getMainWindow()->close();
        }

    }
};
}

void Application::runApplication(void)
{
    // A new QApplication
    Base::Console().Log("Init: Creating Gui::Application and QApplication\n");
    // if application not yet created by the splasher
    int argc = App::Application::GetARGC();
    int systemExit = 1000;
    GUIApplication mainApp(argc, App::Application::GetARGV(), systemExit);
    // set application icon and window title
    const std::map<std::string,std::string>& cfg = App::Application::Config();
    std::map<std::string,std::string>::const_iterator it;
    it = cfg.find("Application");
    if (it != cfg.end()) {
        mainApp.setApplicationName(QString::fromUtf8(it->second.c_str()));
    }
    else {
        mainApp.setApplicationName(QString::fromUtf8(App::GetApplication().getExecutableName()));
    }
    mainApp.setWindowIcon(Gui::BitmapFactory().pixmap(App::Application::Config()["AppIcon"].c_str()));
    QString plugin;
    plugin = QString::fromUtf8(App::GetApplication().GetHomePath());
    plugin += QLatin1String("/plugins");
    QCoreApplication::addLibraryPath(plugin);

    // check for OpenGL
    if (!QGLFormat::hasOpenGL()) {
        QMessageBox::critical(0, QObject::tr("No OpenGL"), QObject::tr("This system does not support OpenGL"));
        throw Base::Exception("This system does not support OpenGL");
    }
#if QT_VERSION >= 0x040200
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        Base::Console().Log("This system does not support framebuffer objects");
    }
#endif
    if (!QGLPixelBuffer::hasOpenGLPbuffers()) {
        Base::Console().Log("This system does not support pbuffers");
    }

    QGLFormat::OpenGLVersionFlags version = QGLFormat::openGLVersionFlags ();
#if QT_VERSION >= 0x040500
    if (version & QGLFormat::OpenGL_Version_3_0)
        Base::Console().Log("OpenGL version 3.0 or higher is present\n");
    else
#endif
    if (version & QGLFormat::OpenGL_Version_2_1)
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

    Application app(true);
    MainWindow mw;
    mw.setWindowTitle(mainApp.applicationName());

    // set toolbar icon size
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    int size = hGrp->GetInt("ToolbarIconSize", 0);
    if (size >= 16) // must not be lower than this
        mw.setIconSize(QSize(size,size));

    // init the Inventor subsystem
    SoDB::init();
    SoQt::init(&mw);
    SoFCDB::init();

    QString home = QString::fromUtf8(App::GetApplication().GetHomePath());

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
        Base::Interpreter().runString(Base::ScriptFactory().ProduceScript("FreeCADGuiInit"));
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
    if (!wb.contains(QString::fromAscii(start.c_str()))) {
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
        }
    }

    //initialize spaceball.
    mainApp.initSpaceball(&mw);

#ifdef FC_DEBUG // redirect Coin messages to FreeCAD
    SoDebugError::setHandlerCallback( messageHandlerCoin, 0 );
    SoQt::setFatalErrorHandler( messageHandlerSoQt, 0 );
#endif


    Instance->d->startingUp = false;

#if 0
    // processing all command line files
    App::Application::processCmdLineFiles();

    // Create new document?
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Document");
    if (hGrp->GetBool("CreateNewDoc", false)) {
        App::GetApplication().newDocument();
    }
#else
    // gets called once we start the event loop
    QTimer::singleShot(0, &mw, SLOT(delayedStartup()));
#endif

    // run the Application event loop
    Base::Console().Log("Init: Entering event loop\n");

    try {
        std::stringstream s;
        s << Base::FileInfo::getTempPath() << App::GetApplication().getExecutableName()
          << "_" << QCoreApplication::applicationPid() << ".lock";
        // open a lock file with the PID
        Base::FileInfo fi(s.str());
        Base::ofstream lock(fi);
        boost::interprocess::file_lock flock(s.str().c_str());
        flock.lock();

        int ret = mainApp.exec();
        if (ret == systemExit)
            throw Base::SystemExitException();

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
    QDir tmp = QDir::temp();
    tmp.setNameFilters(QStringList() << QString::fromAscii("*.lock"));
    tmp.setFilter(QDir::Files);

    QList<QFileInfo> restoreDocFiles;
    QString exeName = QString::fromAscii(App::GetApplication().getExecutableName());
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
                    // delete the lock file immediately if not transient directories are related
                    tmp.remove(fn);
                }
                else {
                    int countDeletedDocs = 0;
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
                        else {
                            // store the transient directory in case it's not empty
                            restoreDocFiles << *it;
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
        //TODO:
    }
}
