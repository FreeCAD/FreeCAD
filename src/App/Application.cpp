/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/



#include "PreCompiled.h"

#ifndef _PreComp_
# include <iostream>
# include <sstream>
# include <exception>
# include <ios>
# include <functional>
# if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
# include <unistd.h>
# include <pwd.h>
# include <sys/types.h>
# elif defined(__MINGW32__)
# define WINVER 0x502 // needed for SetDllDirectory
# include <Windows.h>
# endif
# include <ctime>
# include <csignal>
# include <boost/program_options.hpp>
#endif

#ifdef FC_OS_WIN32
# include <Shlobj.h>
#endif

#if defined(FC_OS_BSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#include "Application.h"
#include "Document.h"

// FreeCAD Base header
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <Base/Factory.h>
#include <Base/ExceptionFactory.h>
#include <Base/FileInfo.h>
#include <Base/Type.h>
#include <Base/BaseClass.h>
#include <Base/Persistence.h>
#include <Base/Reader.h>
#include <Base/MatrixPy.h>
#include <Base/VectorPy.h>
#include <Base/AxisPy.h>
#include <Base/CoordinateSystemPy.h>
#include <Base/BoundBoxPy.h>
#include <Base/PlacementPy.h>
#include <Base/RotationPy.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <Base/Translate.h>
#include <Base/UnitsApi.h>
#include <Base/QuantityPy.h>
#include <Base/UnitPy.h>
#include <Base/TypePy.h>

#include "GeoFeature.h"
#include "FeatureTest.h"
#include "FeaturePython.h"
#include "ComplexGeoData.h"
#include "Property.h"
#include "PropertyContainer.h"
#include "PropertyUnits.h"
#include "PropertyFile.h"
#include "PropertyLinks.h"
#include "PropertyPythonObject.h"
#include "PropertyExpressionEngine.h"
#include "Document.h"
#include "DocumentObjectGroup.h"
#include "DocumentObjectFileIncluded.h"
#include "InventorObject.h"
#include "VRMLObject.h"
#include "Annotation.h"
#include "MeasureDistance.h"
#include "Placement.h"
#include "GeoFeatureGroupExtension.h"
#include "OriginGroupExtension.h"
#include "Part.h"
#include "OriginFeature.h"
#include "Origin.h"
#include "MaterialObject.h"
#include "TextDocument.h"
#include "ExpressionParser.h"
#include "Transactions.h"
#include <App/MaterialPy.h>
#include <Base/GeometryPyCXX.h>
#include "Link.h"

#include "DocumentPy.h"
#include "DocumentObjectGroupPy.h"
#include "LinkBaseExtensionPy.h"
#include "OriginGroupExtensionPy.h"
#include "PartPy.h"

// If you stumble here, run the target "BuildExtractRevision" on Windows systems
// or the Python script "SubWCRev.py" on Linux based systems which builds
// src/Build/Version.h. Or create your own from src/Build/Version.h.in!
#include <Build/Version.h>
#include "Branding.h"

#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

using namespace App;
using namespace std;
using namespace boost;
using namespace boost::program_options;
namespace sp = std::placeholders;


// scriptings (scripts are built-in but can be overridden by command line option)
#include <App/InitScript.h>
#include <App/TestScript.h>
#include <App/CMakeScript.h>

#ifdef _MSC_VER // New handler for Microsoft Visual C++ compiler
# pragma warning( disable : 4535 )
# if !defined(_DEBUG) && defined(HAVE_SEH)
# define FC_SE_TRANSLATOR
# endif

# include <new.h>
# include <eh.h> // VC exception handling
#else // Ansi C/C++ new handler
# include <new>
#endif

FC_LOG_LEVEL_INIT("App",true,true)

//using Base::GetConsole;
using namespace Base;
using namespace App;
using namespace std;

//==========================================================================
// Application
//==========================================================================

ParameterManager *App::Application::_pcSysParamMngr;
ParameterManager *App::Application::_pcUserParamMngr;
Base::ConsoleObserverStd  *Application::_pConsoleObserverStd =0;
Base::ConsoleObserverFile *Application::_pConsoleObserverFile =0;

AppExport std::map<std::string,std::string> Application::mConfig;
BaseExport extern PyObject* Base::BaseExceptionFreeCADError;
BaseExport extern PyObject* Base::BaseExceptionFreeCADAbort;

//**************************************************************************
// Construction and destruction

PyDoc_STRVAR(FreeCAD_doc,
     "The functions in the FreeCAD module allow working with documents.\n"
     "The FreeCAD instance provides a list of references of documents which\n"
     "can be addressed by a string. Hence the document name must be unique.\n"
     "\n"
     "The document has the read-only attribute FileName which points to the\n"
     "file the document should be stored to.\n"
    );

PyDoc_STRVAR(Console_doc,
     "FreeCAD Console\n"
    );

PyDoc_STRVAR(Base_doc,
    "The Base module contains the classes for the geometric basics\n"
    "like vector, matrix, bounding box, placement, rotation, axis, ...\n"
    );

// This is called via the PyImport_AppendInittab mechanism called
// during initialization, to make the built-in __FreeCADBase__
// module known to Python.
PyMODINIT_FUNC
init_freecad_base_module(void)
{
    static struct PyModuleDef BaseModuleDef = {
        PyModuleDef_HEAD_INIT,
        "__FreeCADBase__", Base_doc, -1,
        NULL, NULL, NULL, NULL, NULL
    };
    return PyModule_Create(&BaseModuleDef);
}

// Set in inside Application
static PyMethodDef* __AppMethods = nullptr;

PyMODINIT_FUNC
init_freecad_module(void)
{
    static struct PyModuleDef FreeCADModuleDef = {
        PyModuleDef_HEAD_INIT,
        "FreeCAD", FreeCAD_doc, -1,
        __AppMethods,
        NULL, NULL, NULL, NULL
    };
    return PyModule_Create(&FreeCADModuleDef);
}

Application::Application(std::map<std::string,std::string> &mConfig)
  : _mConfig(mConfig), _pActiveDoc(0), _isRestoring(false),_allowPartial(false)
  , _isClosingAll(false), _objCount(-1), _activeTransactionID(0)
  , _activeTransactionGuard(0), _activeTransactionTmpName(false)
{
    //_hApp = new ApplicationOCC;
    mpcPramManager["System parameter"] = _pcSysParamMngr;
    mpcPramManager["User parameter"] = _pcUserParamMngr;


    // setting up Python binding
    Base::PyGILStateLocker lock;
    PyObject* modules = PyImport_GetModuleDict();

    __AppMethods = Application::Methods;
    PyObject* pAppModule = PyImport_ImportModule ("FreeCAD");
    if (!pAppModule) {
        PyErr_Clear();
        pAppModule = init_freecad_module();
        PyDict_SetItemString(modules, "FreeCAD", pAppModule);
    }
    Py::Module(pAppModule).setAttr(std::string("ActiveDocument"),Py::None());

    static struct PyModuleDef ConsoleModuleDef = {
        PyModuleDef_HEAD_INIT,
        "__FreeCADConsole__", Console_doc, -1,
        ConsoleSingleton::Methods,
        NULL, NULL, NULL, NULL
    };
    PyObject* pConsoleModule = PyModule_Create(&ConsoleModuleDef);

    // introducing additional classes

    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    Base::Interpreter().addType(&Base::VectorPy::Type, pAppModule, "Vector");
    Base::Interpreter().addType(&Base::MatrixPy::Type, pAppModule, "Matrix");
    Base::Interpreter().addType(&Base::BoundBoxPy::Type, pAppModule, "BoundBox");
    Base::Interpreter().addType(&Base::PlacementPy::Type, pAppModule, "Placement");
    Base::Interpreter().addType(&Base::RotationPy::Type, pAppModule, "Rotation");
    Base::Interpreter().addType(&Base::AxisPy::Type, pAppModule, "Axis");

    // Note: Create an own module 'Base' which should provide the python
    // binding classes from the base module. At a later stage we should
    // remove these types from the FreeCAD module.

    PyObject* pBaseModule = PyImport_ImportModule ("__FreeCADBase__");
    if (!pBaseModule) {
        PyErr_Clear();
        pBaseModule = init_freecad_base_module();
        PyDict_SetItemString(modules, "__FreeCADBase__", pBaseModule);
    }

    Base::BaseExceptionFreeCADError = PyErr_NewException("Base.FreeCADError", PyExc_RuntimeError, NULL);
    Py_INCREF(Base::BaseExceptionFreeCADError);
    PyModule_AddObject(pBaseModule, "FreeCADError", Base::BaseExceptionFreeCADError);

    Base::BaseExceptionFreeCADAbort = PyErr_NewException("Base.FreeCADAbort", PyExc_BaseException, NULL);
    Py_INCREF(Base::BaseExceptionFreeCADAbort);
    PyModule_AddObject(pBaseModule, "FreeCADAbort", Base::BaseExceptionFreeCADAbort);

    // Python types
    Base::Interpreter().addType(&Base::VectorPy          ::Type,pBaseModule,"Vector");
    Base::Interpreter().addType(&Base::MatrixPy          ::Type,pBaseModule,"Matrix");
    Base::Interpreter().addType(&Base::BoundBoxPy        ::Type,pBaseModule,"BoundBox");
    Base::Interpreter().addType(&Base::PlacementPy       ::Type,pBaseModule,"Placement");
    Base::Interpreter().addType(&Base::RotationPy        ::Type,pBaseModule,"Rotation");
    Base::Interpreter().addType(&Base::AxisPy            ::Type,pBaseModule,"Axis");
    Base::Interpreter().addType(&Base::CoordinateSystemPy::Type,pBaseModule,"CoordinateSystem");
    Base::Interpreter().addType(&Base::TypePy            ::Type,pBaseModule,"TypeId");

    Base::Interpreter().addType(&App::MaterialPy::Type, pAppModule, "Material");

    // Add document types
    Base::Interpreter().addType(&App::PropertyContainerPy::Type, pAppModule, "PropertyContainer");
    Base::Interpreter().addType(&App::ExtensionContainerPy::Type, pAppModule, "ExtensionContainer");
    Base::Interpreter().addType(&App::DocumentPy::Type, pAppModule, "Document");
    Base::Interpreter().addType(&App::DocumentObjectPy::Type, pAppModule, "DocumentObject");
    Base::Interpreter().addType(&App::DocumentObjectGroupPy::Type, pAppModule, "DocumentObjectGroup");
    Base::Interpreter().addType(&App::GeoFeaturePy::Type, pAppModule, "GeoFeature");
    Base::Interpreter().addType(&App::PartPy::Type, pAppModule, "Part");

    // Add extension types
    Base::Interpreter().addType(&App::ExtensionPy::Type, pAppModule, "Extension");
    Base::Interpreter().addType(&App::DocumentObjectExtensionPy::Type, pAppModule, "DocumentObjectExtension");
    Base::Interpreter().addType(&App::GroupExtensionPy::Type, pAppModule, "GroupExtension");
    Base::Interpreter().addType(&App::GeoFeatureGroupExtensionPy::Type, pAppModule, "GeoFeatureGroupExtension");
    Base::Interpreter().addType(&App::OriginGroupExtensionPy::Type, pAppModule, "OriginGroupExtension");
    Base::Interpreter().addType(&App::LinkBaseExtensionPy::Type, pAppModule, "LinkBaseExtension");

    //insert Base and Console
    Py_INCREF(pBaseModule);
    PyModule_AddObject(pAppModule, "Base", pBaseModule);
    Py_INCREF(pConsoleModule);
    PyModule_AddObject(pAppModule, "Console", pConsoleModule);

    // Translate module
    PyObject* pTranslateModule = (new Base::Translate)->module().ptr();
    Py_INCREF(pTranslateModule);
    PyModule_AddObject(pAppModule, "Qt", pTranslateModule);

    //insert Units module
    static struct PyModuleDef UnitsModuleDef = {
        PyModuleDef_HEAD_INIT,
        "Units", "The Unit API", -1,
        Base::UnitsApi::Methods,
        NULL, NULL, NULL, NULL
    };
    PyObject* pUnitsModule = PyModule_Create(&UnitsModuleDef);
    Base::Interpreter().addType(&Base::QuantityPy  ::Type,pUnitsModule,"Quantity");
    // make sure to set the 'nb_true_divide' slot
    Base::Interpreter().addType(&Base::UnitPy      ::Type,pUnitsModule,"Unit");

    Py_INCREF(pUnitsModule);
    PyModule_AddObject(pAppModule, "Units", pUnitsModule);

    Base::ProgressIndicatorPy::init_type();
    Base::Interpreter().addType(Base::ProgressIndicatorPy::type_object(),
        pBaseModule,"ProgressIndicator");

    Base::Vector2dPy::init_type();
    Base::Interpreter().addType(Base::Vector2dPy::type_object(),
        pBaseModule,"Vector2d");
}

Application::~Application()
{
}

//**************************************************************************
// Interface

/// get called by the document when the name is changing
void Application::renameDocument(const char *OldName, const char *NewName)
{
#if 1
    (void)OldName;
    (void)NewName;
    throw Base::RuntimeError("Renaming document internal name is no longer allowed!");
#else
    std::map<std::string,Document*>::iterator pos;
    pos = DocMap.find(OldName);

    if (pos != DocMap.end()) {
        Document* temp;
        temp = pos->second;
        DocMap.erase(pos);
        DocMap[NewName] = temp;
        signalRenameDocument(*temp);
    }
    else {
        throw Base::RuntimeError("Application::renameDocument(): no document with this name to rename!");
    }
#endif
}

Document* Application::newDocument(const char * Name, const char * UserName, bool createView, bool tempDoc)
{
    // get a valid name anyway!
    if (!Name || Name[0] == '\0')
        Name = "Unnamed";
    string name = getUniqueDocumentName(Name, tempDoc);

    // return the temporary document if it exists
    if (tempDoc) {
        auto it = DocMap.find(name);
        if (it != DocMap.end() && it->second->testStatus(Document::TempDoc))
            return it->second;
    }

    std::string userName;
    if (UserName && UserName[0] != '\0') {
        userName = UserName;
    }
    else {
        userName = Name;
        std::vector<std::string> names;
        names.reserve(DocMap.size());
        std::map<string,Document*>::const_iterator pos;
        for (pos = DocMap.begin();pos != DocMap.end();++pos) {
            names.push_back(pos->second->Label.getValue());
        }

        if (!names.empty())
            userName = Base::Tools::getUniqueName(userName, names);
    }

    // create the FreeCAD document
    std::unique_ptr<Document> newDoc(new Document(name.c_str()));
    if (tempDoc)
        newDoc->setStatus(Document::TempDoc, true);

    auto oldActiveDoc = _pActiveDoc;
    auto doc = newDoc.get();

    // add the document to the internal list
    DocMap[name] = newDoc.release(); // now owned by the Application
    _pActiveDoc = DocMap[name];

    // connect the signals to the application for the new document
    _pActiveDoc->signalBeforeChange.connect(std::bind(&App::Application::slotBeforeChangeDocument, this, sp::_1, sp::_2));
    _pActiveDoc->signalChanged.connect(std::bind(&App::Application::slotChangedDocument, this, sp::_1, sp::_2));
    _pActiveDoc->signalNewObject.connect(std::bind(&App::Application::slotNewObject, this, sp::_1));
    _pActiveDoc->signalDeletedObject.connect(std::bind(&App::Application::slotDeletedObject, this, sp::_1));
    _pActiveDoc->signalBeforeChangeObject.connect(std::bind(&App::Application::slotBeforeChangeObject, this, sp::_1, sp::_2));
    _pActiveDoc->signalChangedObject.connect(std::bind(&App::Application::slotChangedObject, this, sp::_1, sp::_2));
    _pActiveDoc->signalRelabelObject.connect(std::bind(&App::Application::slotRelabelObject, this, sp::_1));
    _pActiveDoc->signalActivatedObject.connect(std::bind(&App::Application::slotActivatedObject, this, sp::_1));
    _pActiveDoc->signalUndo.connect(std::bind(&App::Application::slotUndoDocument, this, sp::_1));
    _pActiveDoc->signalRedo.connect(std::bind(&App::Application::slotRedoDocument, this, sp::_1));
    _pActiveDoc->signalRecomputedObject.connect(std::bind(&App::Application::slotRecomputedObject, this, sp::_1));
    _pActiveDoc->signalRecomputed.connect(std::bind(&App::Application::slotRecomputed, this, sp::_1));
    _pActiveDoc->signalBeforeRecompute.connect(std::bind(&App::Application::slotBeforeRecompute, this, sp::_1));
    _pActiveDoc->signalOpenTransaction.connect(std::bind(&App::Application::slotOpenTransaction, this, sp::_1, sp::_2));
    _pActiveDoc->signalCommitTransaction.connect(std::bind(&App::Application::slotCommitTransaction, this, sp::_1));
    _pActiveDoc->signalAbortTransaction.connect(std::bind(&App::Application::slotAbortTransaction, this, sp::_1));
    _pActiveDoc->signalStartSave.connect(std::bind(&App::Application::slotStartSaveDocument, this, sp::_1, sp::_2));
    _pActiveDoc->signalFinishSave.connect(std::bind(&App::Application::slotFinishSaveDocument, this, sp::_1, sp::_2));
    _pActiveDoc->signalChangePropertyEditor.connect(
            std::bind(&App::Application::slotChangePropertyEditor, this, sp::_1, sp::_2));

    // make sure that the active document is set in case no GUI is up
    {
        Base::PyGILStateLocker lock;
        Py::Object active(_pActiveDoc->getPyObject(), true);
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"),active);
    }

    signalNewDocument(*_pActiveDoc, createView);

    // set the UserName after notifying all observers
    _pActiveDoc->Label.setValue(userName);

    // set the old document active again if the new is temporary
    if (tempDoc && oldActiveDoc)
        setActiveDocument(oldActiveDoc);

    return doc;
}

bool Application::closeDocument(const char* name)
{
    map<string,Document*>::iterator pos = DocMap.find( name );
    if (pos == DocMap.end()) // no such document
        return false;

    Base::ConsoleRefreshDisabler disabler;

    // Trigger observers before removing the document from the internal map.
    // Some observers might rely on this document still being there.
    signalDeleteDocument(*pos->second);

    // For exception-safety use a smart pointer
    if (_pActiveDoc == pos->second)
        setActiveDocument((Document*)0);
    std::unique_ptr<Document> delDoc (pos->second);
    DocMap.erase( pos );

    _objCount = -1;

    // Trigger observers after removing the document from the internal map.
    signalDeletedDocument();

    return true;
}

void Application::closeAllDocuments(void)
{
    Base::FlagToggler<bool> flag(_isClosingAll);
    std::map<std::string,Document*>::iterator pos;
    while((pos = DocMap.begin()) != DocMap.end())
        closeDocument(pos->first.c_str());
}

App::Document* Application::getDocument(const char *Name) const
{
    std::map<std::string,Document*>::const_iterator pos;

    pos = DocMap.find(Name);

    if (pos == DocMap.end())
        return 0;

    return pos->second;
}

const char * Application::getDocumentName(const App::Document* doc) const
{
    for (std::map<std::string,Document*>::const_iterator it = DocMap.begin(); it != DocMap.end(); ++it)
        if (it->second == doc)
            return it->first.c_str();

    return 0;
}

std::vector<App::Document*> Application::getDocuments() const
{
    std::vector<App::Document*> docs;
    for (std::map<std::string,Document*>::const_iterator it = DocMap.begin(); it != DocMap.end(); ++it)
        docs.push_back(it->second);
    return docs;
}

std::string Application::getUniqueDocumentName(const char *Name, bool tempDoc) const
{
    if (!Name || *Name == '\0')
        return std::string();
    std::string CleanName = Base::Tools::getIdentifier(Name);

    // name in use?
    std::map<string,Document*>::const_iterator pos;
    pos = DocMap.find(CleanName);

    if (pos == DocMap.end() || (tempDoc && pos->second->testStatus(Document::TempDoc))) {
        // if not, name is OK
        return CleanName;
    }
    else {
        std::vector<std::string> names;
        names.reserve(DocMap.size());
        for (pos = DocMap.begin();pos != DocMap.end();++pos) {
            if (!tempDoc || !pos->second->testStatus(Document::TempDoc))
                names.push_back(pos->first);
        }
        return Base::Tools::getUniqueName(CleanName, names);
    }
}

int Application::addPendingDocument(const char *FileName, const char *objName, bool allowPartial)
{
    if(!_isRestoring)
        return 0;
    if(allowPartial && _allowPartial)
        return -1;
    assert(FileName && FileName[0]);
    assert(objName && objName[0]);
    auto ret =  _pendingDocMap.emplace(FileName,std::set<std::string>());
    ret.first->second.emplace(objName);
    if(ret.second) {
        _pendingDocs.push_back(ret.first->first.c_str());
        return 1;
    }
    return -1;
}

bool Application::isRestoring() const {
    return _isRestoring || Document::isAnyRestoring();
}

bool Application::isClosingAll() const {
    return _isClosingAll;
}

struct DocTiming {
    FC_DURATION_DECLARE(d1);
    FC_DURATION_DECLARE(d2);
    DocTiming() {
        FC_DURATION_INIT(d1);
        FC_DURATION_INIT(d2);
    }
};

class DocOpenGuard {
public:
    bool &flag;
    boost::signals2::signal<void ()> &signal;
    DocOpenGuard(bool &f, boost::signals2::signal<void ()> &s)
        :flag(f),signal(s)
    {
        flag = true;
    }
    ~DocOpenGuard() {
        if(flag) {
            flag = false;
            try {
                signal();
            }
            catch (const boost::exception&) {
                // reported by code analyzers
                Base::Console().Warning("~DocOpenGuard: Unexpected boost exception\n");
            }
        }
    }
};

Document* Application::openDocument(const char * FileName, bool createView) {
    std::vector<std::string> filenames(1,FileName);
    auto docs = openDocuments(filenames,0,0,0,createView);
    if(docs.size())
        return docs.front();
    return 0;
}

std::vector<Document*> Application::openDocuments(const std::vector<std::string> &filenames,
                                                  const std::vector<std::string> *paths,
                                                  const std::vector<std::string> *labels,
                                                  std::vector<std::string> *errs,
                                                  bool createView)
{
    std::vector<Document*> res(filenames.size(), nullptr);
    if (filenames.empty())
        return res;

    if (errs)
        errs->resize(filenames.size());

    DocOpenGuard guard(_isRestoring, signalFinishOpenDocument);
    _pendingDocs.clear();
    _pendingDocsReopen.clear();
    _pendingDocMap.clear();

    signalStartOpenDocument();

    ParameterGrp::handle hGrp = GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    _allowPartial = !hGrp->GetBool("NoPartialLoading",false);

    for (auto &name : filenames)
        _pendingDocs.push_back(name.c_str());

    std::map<Document *, DocTiming> newDocs;

    FC_TIME_INIT(t);

    for (std::size_t count=0;; ++count) {
        const char *name = _pendingDocs.front();
        _pendingDocs.pop_front();
        bool isMainDoc = count < filenames.size();

        try {
            _objCount = -1;
            std::set<std::string> objNames;
            if (_allowPartial) {
                auto it = _pendingDocMap.find(name);
                if (it != _pendingDocMap.end())
                    objNames.swap(it->second);
            }

            FC_TIME_INIT(t1);
            DocTiming timing;

            const char *path = name;
            const char *label = 0;
            if (isMainDoc) {
                if (paths && paths->size()>count)
                    path = (*paths)[count].c_str();

                if (labels && labels->size()>count)
                    label = (*labels)[count].c_str();
            }

            auto doc = openDocumentPrivate(path, name, label, isMainDoc, createView, objNames);
            FC_DURATION_PLUS(timing.d1,t1);
            if (doc)
                newDocs.emplace(doc,timing);

            if (isMainDoc)
                res[count] = doc;
            _objCount = -1;
        }
        catch (const Base::Exception &e) {
            if (!errs && isMainDoc)
                throw;
            if (errs && isMainDoc)
                (*errs)[count] = e.what();
            else
                Console().Error("Exception opening file: %s [%s]\n", name, e.what());
        }
        catch (const std::exception &e) {
            if (!errs && isMainDoc)
                throw;
            if (errs && isMainDoc)
                (*errs)[count] = e.what();
            else
                Console().Error("Exception opening file: %s [%s]\n", name, e.what());
        }
        catch (...) {
            if (errs) {
                if (isMainDoc)
                    (*errs)[count] = "unknown error";
            }
            else {
                _pendingDocs.clear();
                _pendingDocsReopen.clear();
                _pendingDocMap.clear();
                throw;
            }
        }

        if (_pendingDocs.empty()) {
            if (_pendingDocsReopen.empty())
                break;
            _allowPartial = false;
            _pendingDocs.swap(_pendingDocsReopen);
        }
    }

    _pendingDocs.clear();
    _pendingDocsReopen.clear();
    _pendingDocMap.clear();

    Base::SequencerLauncher seq("Postprocessing...", newDocs.size());

    std::vector<Document*> docs;
    docs.reserve(newDocs.size());
    for (auto &v : newDocs) {
        // Notify PropertyXLink to attach newly opened documents and restore
        // relevant external links
        PropertyXLink::restoreDocument(*v.first);
        docs.push_back(v.first);
    }

    // After external links has been restored, we can now sort the document
    // according to their dependency order.
    docs = Document::getDependentDocuments(docs, true);
    for (auto it=docs.begin(); it!=docs.end();) {
        Document *doc = *it;
        // It is possible that the newly opened document depends on an existing
        // document, which will be included with the above call to
        // Document::getDependentDocuments(). Make sure to exclude that.
        auto dit = newDocs.find(doc);
        if (dit == newDocs.end()) {
            it = docs.erase(it);
            continue;
        }
        ++it;
        FC_TIME_INIT(t1);
        // Finalize document restoring with the correct order
        doc->afterRestore(true);
        FC_DURATION_PLUS(dit->second.d2,t1);
        seq.next();
    }

    // Set the active document using the first successfully restored main
    // document (i.e. documents explicitly asked for by caller).
    for (auto doc : res) {
        if (doc) {
            setActiveDocument(doc);
            break;
        }
    }

    for (auto doc : docs) {
        auto &timing = newDocs[doc];
        FC_DURATION_LOG(timing.d1, doc->getName() << " restore");
        FC_DURATION_LOG(timing.d2, doc->getName() << " postprocess");
    }
    FC_TIME_LOG(t,"total");

    _isRestoring = false;
    signalFinishOpenDocument();
    return res;
}

Document* Application::openDocumentPrivate(const char * FileName,
        const char *propFileName, const char *label,
        bool isMainDoc, bool createView,
        const std::set<std::string> &objNames)
{
    FileInfo File(FileName);

    if (!File.exists()) {
        std::stringstream str;
        str << "File '" << FileName << "' does not exist!";
        throw Base::FileSystemError(str.str().c_str());
    }

    // Before creating a new document we check whether the document is already open
    std::string filepath = File.filePath();
    QString canonicalPath = QFileInfo(QString::fromUtf8(FileName)).canonicalFilePath();
    for (std::map<std::string,Document*>::iterator it = DocMap.begin(); it != DocMap.end(); ++it) {
        // get unique path separators
        std::string fi = FileInfo(it->second->FileName.getValue()).filePath();
        if (filepath != fi) {
            if (canonicalPath == QFileInfo(QString::fromUtf8(fi.c_str())).canonicalFilePath()) {
                bool samePath = (canonicalPath == QString::fromUtf8(FileName));
                FC_WARN("Identical physical path '" << canonicalPath.toUtf8().constData() << "'\n"
                        << (samePath?"":"  for file '") << (samePath?"":FileName) << (samePath?"":"'\n")
                        << "  with existing document '" << it->second->Label.getValue()
                        << "' in path: '" << it->second->FileName.getValue() << "'");
            }
            continue;
        }
        if(it->second->testStatus(App::Document::PartialDoc)
                || it->second->testStatus(App::Document::PartialRestore)) {
            // Here means a document is already partially loaded, but the document
            // is requested again, either partial or not. We must check if the
            // document contains the required object

            if(isMainDoc) {
                // Main document must be open fully, so close and reopen
                closeDocument(it->first.c_str());
                break;
            }

            if(_allowPartial) {
                bool reopen = false;
                for(auto &name : objNames) {
                    auto obj = it->second->getObject(name.c_str());
                    if(!obj || obj->testStatus(App::PartialObject)) {
                        reopen = true;
                        break;
                    }
                }
                if(!reopen)
                    return 0;
            }
            auto &names = _pendingDocMap[FileName];
            names.clear();
            _pendingDocsReopen.push_back(FileName);
            return 0;
        }

        if(!isMainDoc)
            return 0;

        return it->second;
    }

    std::string name;
    if(propFileName != FileName) {
        FileInfo fi(propFileName);
        name = fi.fileNamePure();
    }else
        name = File.fileNamePure();

    // Use the same name for the internal and user name.
    // The file name is UTF-8 encoded which means that the internal name will be modified
    // to only contain valid ASCII characters but the user name will be kept.
    if(!label)
        label = name.c_str();
    Document* newDoc = newDocument(name.c_str(),label,isMainDoc && createView);

    newDoc->FileName.setValue(propFileName==FileName?File.filePath():propFileName);

    try {
        // read the document
        newDoc->restore(File.filePath().c_str(),true,objNames);
        return newDoc;
    }
    // if the project file itself is corrupt then
    // close the document
    catch (const Base::FileException&) {
        closeDocument(newDoc->getName());
        throw;
    }
    catch (const std::ios_base::failure&) {
        closeDocument(newDoc->getName());
        throw;
    }
    // but for any other exceptions leave it open to give the
    // user a chance to fix it
    catch (...) {
        throw;
    }
}

Document* Application::getActiveDocument(void) const
{
    return _pActiveDoc;
}

void Application::setActiveDocument(Document* pDoc)
{
    _pActiveDoc = pDoc;

    // make sure that the active document is set in case no GUI is up
    if (pDoc) {
        Base::PyGILStateLocker lock;
        Py::Object active(pDoc->getPyObject(), true);
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"),active);
    }
    else {
        Base::PyGILStateLocker lock;
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"),Py::None());
    }

    if (pDoc)
        signalActiveDocument(*pDoc);
}

void Application::setActiveDocument(const char *Name)
{
    // If no active document is set, resort to a default.
    if (*Name == '\0') {
        _pActiveDoc = 0;
        return;
    }

    std::map<std::string,Document*>::iterator pos;
    pos = DocMap.find(Name);

    if (pos != DocMap.end()) {
        setActiveDocument(pos->second);
    }
    else {
        std::stringstream s;
        s << "Try to activate unknown document '" << Name << "'";
        throw Base::RuntimeError(s.str());
    }
}

static int _TransSignalCount;
static bool _TransSignalled;
Application::TransactionSignaller::TransactionSignaller(bool abort, bool signal)
    :abort(abort)
{
    ++_TransSignalCount;
    if(signal && !_TransSignalled) {
        _TransSignalled = true;
        GetApplication().signalBeforeCloseTransaction(abort);
    }
}

Application::TransactionSignaller::~TransactionSignaller() {
    if(--_TransSignalCount == 0 && _TransSignalled) {
        _TransSignalled = false;
        try {
            GetApplication().signalCloseTransaction(abort);
        }
        catch (const boost::exception&) {
            // reported by code analyzers
            Base::Console().Warning("~TransactionSignaller: Unexpected boost exception\n");
        }
    }
}

const char* Application::getHomePath(void) const
{
    return _mConfig["AppHomePath"].c_str();
}

const char* Application::getExecutableName(void) const
{
    return _mConfig["ExeName"].c_str();
}

std::string Application::getTempPath()
{
    return mConfig["AppTempPath"];
}

std::string Application::getTempFileName(const char* FileName)
{
    return Base::FileInfo::getTempFileName(FileName, getTempPath().c_str());
}

std::string Application::getUserAppDataDir()
{
    return mConfig["UserAppData"];
}

std::string Application::getUserMacroDir()
{
    std::string path("Macro/");
    return mConfig["UserAppData"] + path;
}

std::string Application::getResourceDir()
{
#ifdef RESOURCEDIR
    std::string path(RESOURCEDIR);
    path.append("/");
    QDir dir(QString::fromUtf8(RESOURCEDIR));
    if (dir.isAbsolute())
        return path;
    else
        return mConfig["AppHomePath"] + path;
#else
    return mConfig["AppHomePath"];
#endif
}

std::string Application::getHelpDir()
{
#ifdef DOCDIR
    std::string path(DOCDIR);
    path.append("/");
    QDir dir(QString::fromUtf8(DOCDIR));
    if (dir.isAbsolute())
        return path;
    else
        return mConfig["AppHomePath"] + path;
#else
    return mConfig["DocPath"];
#endif
}

int Application::checkLinkDepth(int depth, bool no_throw) {
    if(_objCount<0) {
        _objCount = 0;
        for(auto &v : DocMap)
            _objCount += v.second->countObjects();
    }
    if(depth > _objCount+2) {
        const char *msg = "Link recursion limit reached. "
                "Please check for cyclic reference.";
        if(no_throw) {
            FC_ERR(msg);
            return 0;
        }else
            throw Base::RuntimeError(msg);
    }
    return _objCount+2;
}

std::set<DocumentObject *> Application::getLinksTo(
        const DocumentObject *obj, int options, int maxCount) const
{
    std::set<DocumentObject *> links;
    if(!obj) {
        for(auto &v : DocMap) {
            v.second->getLinksTo(links,obj,options,maxCount);
            if(maxCount && (int)links.size()>=maxCount)
                break;
        }
    } else {
        std::set<Document*> docs;
        for(auto o : obj->getInList()) {
            if(o && o->getNameInDocument() && docs.insert(o->getDocument()).second) {
                o->getDocument()->getLinksTo(links,obj,options,maxCount);
                if(maxCount && (int)links.size()>=maxCount)
                    break;
            }
        }
    }
    return links;
}

bool Application::hasLinksTo(const DocumentObject *obj) const {
    return !getLinksTo(obj,0,1).empty();
}

ParameterManager & Application::GetSystemParameter(void)
{
    return *_pcSysParamMngr;
}

ParameterManager & Application::GetUserParameter(void)
{
    return *_pcUserParamMngr;
}

ParameterManager * Application::GetParameterSet(const char* sName) const
{
    std::map<std::string,ParameterManager *>::const_iterator it = mpcPramManager.find(sName);
    if ( it != mpcPramManager.end() )
        return it->second;
    else
        return 0;
}

const std::map<std::string,ParameterManager *> & Application::GetParameterSetList(void) const
{
    return mpcPramManager;
}

void Application::AddParameterSet(const char* sName)
{
    std::map<std::string,ParameterManager *>::const_iterator it = mpcPramManager.find(sName);
    if ( it != mpcPramManager.end() )
        return;
    mpcPramManager[sName] = new ParameterManager();
}

void Application::RemoveParameterSet(const char* sName)
{
    std::map<std::string,ParameterManager *>::iterator it = mpcPramManager.find(sName);
    // Must not delete user or system parameter
    if ( it == mpcPramManager.end() || it->second == _pcUserParamMngr || it->second == _pcSysParamMngr )
        return;
    delete it->second;
    mpcPramManager.erase(it);
}

Base::Reference<ParameterGrp>  Application::GetParameterGroupByPath(const char* sName)
{
    std::string cName = sName,cTemp;

    std::string::size_type pos = cName.find(':');

    // is there a path separator ?
    if (pos == std::string::npos) {
        throw Base::ValueError("Application::GetParameterGroupByPath() no parameter set name specified");
    }
    // assigning the parameter set name
    cTemp.assign(cName,0,pos);
    cName.erase(0,pos+1);

    // test if name is valid
    std::map<std::string,ParameterManager *>::iterator It = mpcPramManager.find(cTemp.c_str());
    if (It == mpcPramManager.end())
        throw Base::ValueError("Application::GetParameterGroupByPath() unknown parameter set name specified");

    return It->second->GetGroup(cName.c_str());
}

void Application::addImportType(const char* Type, const char* ModuleName)
{
    FileTypeItem item;
    item.filter = Type;
    item.module = ModuleName;

    // Extract each filetype from 'Type' literal
    std::string::size_type pos = item.filter.find("*.");
    while ( pos != std::string::npos ) {
        std::string::size_type next = item.filter.find_first_of(" )", pos+1);
        std::string::size_type len = next-pos-2;
        std::string type = item.filter.substr(pos+2,len);
        item.types.push_back(type);
        pos = item.filter.find("*.", next);
    }

    // Due to branding stuff replace "FreeCAD" with the branded application name
    if (strncmp(Type, "FreeCAD", 7) == 0) {
        std::string AppName = Config()["ExeName"];
        AppName += item.filter.substr(7);
        item.filter = AppName;
        // put to the front of the array
        _mImportTypes.insert(_mImportTypes.begin(),item);
    }
    else {
        _mImportTypes.push_back(item);
    }
}

void Application::changeImportModule(const char* Type, const char* OldModuleName, const char* NewModuleName)
{
    for (auto& it : _mImportTypes) {
        if (it.filter == Type && it.module == OldModuleName) {
            it.module = NewModuleName;
            break;
        }
    }
}

std::vector<std::string> Application::getImportModules(const char* Type) const
{
    std::vector<std::string> modules;
    for (std::vector<FileTypeItem>::const_iterator it = _mImportTypes.begin(); it != _mImportTypes.end(); ++it) {
        const std::vector<std::string>& types = it->types;
        for (std::vector<std::string>::const_iterator jt = types.begin(); jt != types.end(); ++jt) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt->c_str()) == 0)
#else
            if (_stricmp(Type,jt->c_str()) == 0)
#endif
                modules.push_back(it->module);
        }
    }

    return modules;
}

std::vector<std::string> Application::getImportModules() const
{
    std::vector<std::string> modules;
    for (std::vector<FileTypeItem>::const_iterator it = _mImportTypes.begin(); it != _mImportTypes.end(); ++it)
        modules.push_back(it->module);
    std::sort(modules.begin(), modules.end());
    modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
    return modules;
}

std::vector<std::string> Application::getImportTypes(const char* Module) const
{
    std::vector<std::string> types;
    for (std::vector<FileTypeItem>::const_iterator it = _mImportTypes.begin(); it != _mImportTypes.end(); ++it) {
#ifdef __GNUC__
        if (strcasecmp(Module,it->module.c_str()) == 0)
#else
        if (_stricmp(Module,it->module.c_str()) == 0)
#endif
            types.insert(types.end(), it->types.begin(), it->types.end());
    }

    return types;
}

std::vector<std::string> Application::getImportTypes(void) const
{
    std::vector<std::string> types;
    for (std::vector<FileTypeItem>::const_iterator it = _mImportTypes.begin(); it != _mImportTypes.end(); ++it) {
        types.insert(types.end(), it->types.begin(), it->types.end());
    }

    std::sort(types.begin(), types.end());
    types.erase(std::unique(types.begin(), types.end()), types.end());

    return types;
}

std::map<std::string, std::string> Application::getImportFilters(const char* Type) const
{
    std::map<std::string, std::string> moduleFilter;
    for (std::vector<FileTypeItem>::const_iterator it = _mImportTypes.begin(); it != _mImportTypes.end(); ++it) {
        const std::vector<std::string>& types = it->types;
        for (std::vector<std::string>::const_iterator jt = types.begin(); jt != types.end(); ++jt) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt->c_str()) == 0)
#else
            if (_stricmp(Type,jt->c_str()) == 0)
#endif
                moduleFilter[it->filter] = it->module;
        }
    }

    return moduleFilter;
}

std::map<std::string, std::string> Application::getImportFilters(void) const
{
    std::map<std::string, std::string> filter;
    for (std::vector<FileTypeItem>::const_iterator it = _mImportTypes.begin(); it != _mImportTypes.end(); ++it) {
        filter[it->filter] = it->module;
    }

    return filter;
}

void Application::addExportType(const char* Type, const char* ModuleName)
{
    FileTypeItem item;
    item.filter = Type;
    item.module = ModuleName;

    // Extract each filetype from 'Type' literal
    std::string::size_type pos = item.filter.find("*.");
    while ( pos != std::string::npos ) {
        std::string::size_type next = item.filter.find_first_of(" )", pos+1);
        std::string::size_type len = next-pos-2;
        std::string type = item.filter.substr(pos+2,len);
        item.types.push_back(type);
        pos = item.filter.find("*.", next);
    }

    // Due to branding stuff replace "FreeCAD" with the branded application name
    if (strncmp(Type, "FreeCAD", 7) == 0) {
        std::string AppName = Config()["ExeName"];
        AppName += item.filter.substr(7);
        item.filter = AppName;
        // put to the front of the array
        _mExportTypes.insert(_mExportTypes.begin(),item);
    }
    else {
        _mExportTypes.push_back(item);
    }
}

void Application::changeExportModule(const char* Type, const char* OldModuleName, const char* NewModuleName)
{
    for (auto& it : _mExportTypes) {
        if (it.filter == Type && it.module == OldModuleName) {
            it.module = NewModuleName;
            break;
        }
    }
}

std::vector<std::string> Application::getExportModules(const char* Type) const
{
    std::vector<std::string> modules;
    for (std::vector<FileTypeItem>::const_iterator it = _mExportTypes.begin(); it != _mExportTypes.end(); ++it) {
        const std::vector<std::string>& types = it->types;
        for (std::vector<std::string>::const_iterator jt = types.begin(); jt != types.end(); ++jt) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt->c_str()) == 0)
#else
            if (_stricmp(Type,jt->c_str()) == 0)
#endif
                modules.push_back(it->module);
        }
    }

    return modules;
}

std::vector<std::string> Application::getExportModules() const
{
    std::vector<std::string> modules;
    for (std::vector<FileTypeItem>::const_iterator it = _mExportTypes.begin(); it != _mExportTypes.end(); ++it)
        modules.push_back(it->module);
    std::sort(modules.begin(), modules.end());
    modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
    return modules;
}

std::vector<std::string> Application::getExportTypes(const char* Module) const
{
    std::vector<std::string> types;
    for (std::vector<FileTypeItem>::const_iterator it = _mExportTypes.begin(); it != _mExportTypes.end(); ++it) {
#ifdef __GNUC__
        if (strcasecmp(Module,it->module.c_str()) == 0)
#else
        if (_stricmp(Module,it->module.c_str()) == 0)
#endif
            types.insert(types.end(), it->types.begin(), it->types.end());
    }

    return types;
}

std::vector<std::string> Application::getExportTypes(void) const
{
    std::vector<std::string> types;
    for (std::vector<FileTypeItem>::const_iterator it = _mExportTypes.begin(); it != _mExportTypes.end(); ++it) {
        types.insert(types.end(), it->types.begin(), it->types.end());
    }

    std::sort(types.begin(), types.end());
    types.erase(std::unique(types.begin(), types.end()), types.end());

    return types;
}

std::map<std::string, std::string> Application::getExportFilters(const char* Type) const
{
    std::map<std::string, std::string> moduleFilter;
    for (std::vector<FileTypeItem>::const_iterator it = _mExportTypes.begin(); it != _mExportTypes.end(); ++it) {
        const std::vector<std::string>& types = it->types;
        for (std::vector<std::string>::const_iterator jt = types.begin(); jt != types.end(); ++jt) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt->c_str()) == 0)
#else
            if (_stricmp(Type,jt->c_str()) == 0)
#endif
                moduleFilter[it->filter] = it->module;
        }
    }

    return moduleFilter;
}

std::map<std::string, std::string> Application::getExportFilters(void) const
{
    std::map<std::string, std::string> filter;
    for (std::vector<FileTypeItem>::const_iterator it = _mExportTypes.begin(); it != _mExportTypes.end(); ++it) {
        filter[it->filter] = it->module;
    }

    return filter;
}

//**************************************************************************
// signaling
void Application::slotBeforeChangeDocument(const App::Document& doc, const Property& prop)
{
    this->signalBeforeChangeDocument(doc, prop);
}

void Application::slotChangedDocument(const App::Document& doc, const Property& prop)
{
    this->signalChangedDocument(doc, prop);
}

void Application::slotNewObject(const App::DocumentObject&O)
{
    this->signalNewObject(O);
    _objCount = -1;
}

void Application::slotDeletedObject(const App::DocumentObject&O)
{
    this->signalDeletedObject(O);
    _objCount = -1;
}

void Application::slotBeforeChangeObject(const DocumentObject& O, const Property& Prop)
{
    this->signalBeforeChangeObject(O, Prop);
}

void Application::slotChangedObject(const App::DocumentObject&O, const App::Property& P)
{
    this->signalChangedObject(O,P);
}

void Application::slotRelabelObject(const App::DocumentObject&O)
{
    this->signalRelabelObject(O);
}

void Application::slotActivatedObject(const App::DocumentObject&O)
{
    this->signalActivatedObject(O);
}

void Application::slotUndoDocument(const App::Document& d)
{
    this->signalUndoDocument(d);
}

void Application::slotRedoDocument(const App::Document& d)
{
    this->signalRedoDocument(d);
}

void Application::slotRecomputedObject(const DocumentObject& obj)
{
    this->signalObjectRecomputed(obj);
}

void Application::slotRecomputed(const Document& doc)
{
    this->signalRecomputed(doc);
}

void Application::slotBeforeRecompute(const Document& doc)
{
    this->signalBeforeRecomputeDocument(doc);
}

void Application::slotOpenTransaction(const Document& d, string s)
{
    this->signalOpenTransaction(d, s);
}

void Application::slotCommitTransaction(const Document& d)
{
    this->signalCommitTransaction(d);
}

void Application::slotAbortTransaction(const Document& d)
{
    this->signalAbortTransaction(d);
}

void Application::slotStartSaveDocument(const App::Document& doc, const std::string& filename)
{
    this->signalStartSaveDocument(doc, filename);
}

void Application::slotFinishSaveDocument(const App::Document& doc, const std::string& filename)
{
    this->signalFinishSaveDocument(doc, filename);
}

void Application::slotChangePropertyEditor(const App::Document &doc, const App::Property &prop)
{
    this->signalChangePropertyEditor(doc,prop);
}

//**************************************************************************
// Init, Destruct and singleton

Application * Application::_pcSingleton = 0;

int Application::_argc;
char ** Application::_argv;


void Application::cleanupUnits()
{
    try {
        Base::PyGILStateLocker lock;
        Py::Module mod (Py::Module("FreeCAD").getAttr("Units").ptr());

        Py::List attr(mod.dir());
        for (Py::List::iterator it = attr.begin(); it != attr.end(); ++it) {
            mod.delAttr(Py::String(*it));
        }
    }
    catch (Py::Exception& e) {
        Base::PyGILStateLocker lock;
        e.clear();
    }
}

void Application::destruct(void)
{
    // saving system parameter
    Console().Log("Saving system parameter...\n");
    _pcSysParamMngr->SaveDocument();
    // saving the User parameter
    Console().Log("Saving system parameter...done\n");
    Console().Log("Saving user parameter...\n");
    _pcUserParamMngr->SaveDocument();
    Console().Log("Saving user parameter...done\n");

    // now save all other parameter files
    std::map<std::string,ParameterManager *>& paramMgr = _pcSingleton->mpcPramManager;
    for (std::map<std::string,ParameterManager *>::iterator it = paramMgr.begin(); it != paramMgr.end(); ++it) {
        if ((it->second != _pcSysParamMngr) && (it->second != _pcUserParamMngr)) {
            if (it->second->HasSerializer()) {
                Console().Log("Saving %s...\n", it->first.c_str());
                it->second->SaveDocument();
                Console().Log("Saving %s...done\n", it->first.c_str());
            }
        }

        // clean up
        delete it->second;
    }

    paramMgr.clear();
    _pcSysParamMngr = 0;
    _pcUserParamMngr = 0;

#ifdef FC_DEBUG
    // Do this only in debug mode for memory leak checkers
    cleanupUnits();
#endif

    // not initialized or double destruct!
    assert(_pcSingleton);
    delete _pcSingleton;

    // We must detach from console and delete the observer to save our file
    destructObserver();

    Base::Interpreter().finalize();

    ScriptFactorySingleton::Destruct();
    InterpreterSingleton::Destruct();
    Base::Type::destruct();
    ParameterManager::Terminate();
}

void Application::destructObserver(void)
{
    if ( _pConsoleObserverFile ) {
        Console().DetachObserver(_pConsoleObserverFile);
        delete _pConsoleObserverFile;
        _pConsoleObserverFile = 0;
    }
    if ( _pConsoleObserverStd ) {
        Console().DetachObserver(_pConsoleObserverStd);
        delete _pConsoleObserverStd;
        _pConsoleObserverStd = 0;
    }
}

/** freecadNewHandler()
 * prints an error message and throws an exception
 */
#ifdef _MSC_VER // New handler for Microsoft Visual C++ compiler
int __cdecl freecadNewHandler(size_t size )
{
    // throw an exception
    throw Base::MemoryException();
    return 0;
}
#else // Ansi C/C++ new handler
static void freecadNewHandler ()
{
    // throw an exception
    throw Base::MemoryException();
}
#endif

#if defined(FC_OS_LINUX)
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>

// This function produces a stack backtrace with demangled function & method names.
void printBacktrace(size_t skip=0)
{
    void *callstack[128];
    size_t nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
    size_t nFrames = backtrace(callstack, nMaxFrames);
    char **symbols = backtrace_symbols(callstack, nFrames);

    for (size_t i = skip; i < nFrames; i++) {
        char *demangled = NULL;
        int status = -1;
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname && info.dli_fname) {
            if (info.dli_sname[0] == '_') {
                demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
            }
        }

        std::stringstream str;
        if (status == 0) {
            void* offset = (void*)((char*)callstack[i] - (char*)info.dli_saddr);
            str << "#" << (i-skip) << "  " << callstack[i] << " in " << demangled << " from " << info.dli_fname << "+" << offset << std::endl;
            free(demangled);
        }
        else {
            str << "#" << (i-skip) << "  " << symbols[i] << std::endl;
        }

        // cannot directly print to cerr when using --write-log
        std::cerr << str.str();
    }

    free(symbols);
}
#endif

void segmentation_fault_handler(int sig)
{
#if defined(FC_OS_LINUX)
    (void)sig;
    std::cerr << "Program received signal SIGSEGV, Segmentation fault.\n";
    printBacktrace(2);
    exit(1);
#else
    switch (sig) {
        case SIGSEGV:
            std::cerr << "Illegal storage access..." << std::endl;
#if !defined(_DEBUG)
            throw Base::AccessViolation("Illegal storage access! Please save your work under a new file name and restart the application!");
#endif
            break;
        case SIGABRT:
            std::cerr << "Abnormal program termination..." << std::endl;
#if !defined(_DEBUG)
            throw Base::AbnormalProgramTermination("Break signal occurred");
#endif
            break;
        default:
            std::cerr << "Unknown error occurred..." << std::endl;
            break;
    }
#endif // FC_OS_LINUX
}

void unhandled_exception_handler()
{
    std::cerr << "Terminating..." << std::endl;
}

void unexpection_error_handler()
{
    std::cerr << "Unexpected error occurred..." << std::endl;
    // try to throw an exception and give the user chance to save their work
#if !defined(_DEBUG)
    throw Base::AbnormalProgramTermination("Unexpected error occurred! Please save your work under a new file name and restart the application!");
#else
    terminate();
#endif
}

#if defined(FC_SE_TRANSLATOR) // Microsoft compiler
void my_se_translator_filter(unsigned int code, EXCEPTION_POINTERS* pExp)
{
    Q_UNUSED(pExp)
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        throw Base::AccessViolation();
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        //throw Base::DivisionByZeroError("Division by zero!");
        Base::Console().Error("SEH exception (%u): Division by zero\n", code);
        return;
    }

    std::stringstream str;
    str << "SEH exception of type: " << code;
    // general C++ SEH exception for things we don't need to handle separately....
    throw Base::RuntimeError(str.str());
}
#endif

void Application::init(int argc, char ** argv)
{
    try {
        // install our own new handler
#ifdef _MSC_VER // Microsoft compiler
        _set_new_handler ( freecadNewHandler ); // Setup new handler
        _set_new_mode( 1 ); // Re-route malloc failures to new handler !
#else   // Ansi compiler
        std::set_new_handler (freecadNewHandler); // ANSI new handler
#endif
        // if an unexpected crash occurs we can install a handler function to
        // write some additional information
#if defined (_MSC_VER) // Microsoft compiler
        std::signal(SIGSEGV,segmentation_fault_handler);
        std::signal(SIGABRT,segmentation_fault_handler);
        std::set_terminate(unhandled_exception_handler);
           ::set_unexpected(unexpection_error_handler);
#elif defined(FC_OS_LINUX)
        std::signal(SIGSEGV,segmentation_fault_handler);
#endif
#if defined(FC_SE_TRANSLATOR)
        _set_se_translator(my_se_translator_filter);
#endif
        initTypes();

        initConfig(argc,argv);
        initApplication();
    }
    catch (...) {
        // force the log to flush
        destructObserver();
        throw;
    }
}

void Application::initTypes(void)
{
    // Base types
    Base::Type                      ::init();
    Base::BaseClass                 ::init();
    Base::Exception                 ::init();
    Base::AbortException            ::init();
    Base::Persistence               ::init();

    // Complex data classes
    Data::ComplexGeoData            ::init();
    Data::Segment                   ::init();

    // Properties
    App ::Property                  ::init();
    App ::PropertyContainer         ::init();
    App ::PropertyLists             ::init();
    App ::PropertyBool              ::init();
    App ::PropertyBoolList          ::init();
    App ::PropertyFloat             ::init();
    App ::PropertyFloatList         ::init();
    App ::PropertyFloatConstraint   ::init();
    App ::PropertyPrecision         ::init();
    App ::PropertyQuantity          ::init();
    App ::PropertyQuantityConstraint::init();
    App ::PropertyAngle             ::init();
    App ::PropertyDistance          ::init();
    App ::PropertyLength            ::init();
    App ::PropertyArea              ::init();
    App ::PropertyVolume            ::init();
    App ::PropertyFrequency         ::init();
    App ::PropertySpeed             ::init();
    App ::PropertyAcceleration      ::init();
    App ::PropertyForce             ::init();
    App ::PropertyPressure          ::init();
    App ::PropertyVacuumPermittivity::init();
    App ::PropertyInteger           ::init();
    App ::PropertyIntegerConstraint ::init();
    App ::PropertyPercent           ::init();
    App ::PropertyEnumeration       ::init();
    App ::PropertyIntegerList       ::init();
    App ::PropertyIntegerSet        ::init();
    App ::PropertyMap               ::init();
    App ::PropertyString            ::init();
    App ::PropertyPersistentObject  ::init();
    App ::PropertyUUID              ::init();
    App ::PropertyFont              ::init();
    App ::PropertyStringList        ::init();
    App ::PropertyLinkBase          ::init();
    App ::PropertyLinkListBase      ::init();
    App ::PropertyLink              ::init();
    App ::PropertyLinkChild         ::init();
    App ::PropertyLinkGlobal        ::init();
    App ::PropertyLinkHidden        ::init();
    App ::PropertyLinkSub           ::init();
    App ::PropertyLinkSubChild      ::init();
    App ::PropertyLinkSubGlobal     ::init();
    App ::PropertyLinkSubHidden     ::init();
    App ::PropertyLinkList          ::init();
    App ::PropertyLinkListChild     ::init();
    App ::PropertyLinkListGlobal    ::init();
    App ::PropertyLinkListHidden    ::init();
    App ::PropertyLinkSubList       ::init();
    App ::PropertyLinkSubListChild  ::init();
    App ::PropertyLinkSubListGlobal ::init();
    App ::PropertyLinkSubListHidden ::init();
    App ::PropertyXLink             ::init();
    App ::PropertyXLinkSub          ::init();
    App ::PropertyXLinkSubList      ::init();
    App ::PropertyXLinkList         ::init();
    App ::PropertyXLinkContainer    ::init();
    App ::PropertyMatrix            ::init();
    App ::PropertyVector            ::init();
    App ::PropertyVectorDistance    ::init();
    App ::PropertyPosition          ::init();
    App ::PropertyDirection         ::init();
    App ::PropertyVectorList        ::init();
    App ::PropertyPlacement         ::init();
    App ::PropertyPlacementList     ::init();
    App ::PropertyPlacementLink     ::init();
    App ::PropertyGeometry          ::init();
    App ::PropertyComplexGeoData    ::init();
    App ::PropertyColor             ::init();
    App ::PropertyColorList         ::init();
    App ::PropertyMaterial          ::init();
    App ::PropertyMaterialList      ::init();
    App ::PropertyPath              ::init();
    App ::PropertyFile              ::init();
    App ::PropertyFileIncluded      ::init();
    App ::PropertyPythonObject      ::init();
    App ::PropertyExpressionContainer  ::init();
    App ::PropertyExpressionEngine  ::init();

    // Extension classes
    App ::Extension                     ::init();
    App ::ExtensionContainer            ::init();
    App ::DocumentObjectExtension       ::init();
    App ::GroupExtension                ::init();
    App ::GroupExtensionPython          ::init();
    App ::GeoFeatureGroupExtension      ::init();
    App ::GeoFeatureGroupExtensionPython::init();
    App ::OriginGroupExtension          ::init();
    App ::OriginGroupExtensionPython    ::init();
    App ::LinkBaseExtension             ::init();
    App ::LinkBaseExtensionPython       ::init();
    App ::LinkExtension                 ::init();
    App ::LinkExtensionPython           ::init();

    // Document classes
    App ::TransactionalObject       ::init();
    App ::DocumentObject            ::init();
    App ::GeoFeature                ::init();
    App ::FeatureTest               ::init();
    App ::FeatureTestException      ::init();
    App ::FeaturePython             ::init();
    App ::GeometryPython            ::init();
    App ::Document                  ::init();
    App ::DocumentObjectGroup       ::init();
    App ::DocumentObjectGroupPython ::init();
    App ::DocumentObjectFileIncluded::init();
    App ::InventorObject            ::init();
    App ::VRMLObject                ::init();
    App ::Annotation                ::init();
    App ::AnnotationLabel           ::init();
    App ::MeasureDistance           ::init();
    App ::MaterialObject            ::init();
    App ::MaterialObjectPython      ::init();
    App ::TextDocument              ::init();
    App ::Placement                 ::init();
    App ::PlacementPython           ::init();
    App ::OriginFeature             ::init();
    App ::Plane                     ::init();
    App ::Line                      ::init();
    App ::Part                      ::init();
    App ::Origin                    ::init();
    App ::Link                      ::init();
    App ::LinkPython                ::init();
    App ::LinkElement               ::init();
    App ::LinkElementPython         ::init();
    App ::LinkGroup                 ::init();
    App ::LinkGroupPython           ::init();

    // Expression classes
    App ::Expression                ::init();
    App ::UnitExpression            ::init();
    App ::NumberExpression          ::init();
    App ::ConstantExpression        ::init();
    App ::OperatorExpression        ::init();
    App ::VariableExpression        ::init();
    App ::ConditionalExpression     ::init();
    App ::StringExpression          ::init();
    App ::FunctionExpression        ::init();
    App ::RangeExpression           ::init();
    App ::PyObjectExpression        ::init();

    // register transaction type
    new App::TransactionProducer<TransactionDocumentObject>
            (DocumentObject::getClassTypeId());

    // register exception producer types
    new ExceptionProducer<Base::AbortException>;
    new ExceptionProducer<Base::XMLBaseException>;
    new ExceptionProducer<Base::XMLParseException>;
    new ExceptionProducer<Base::XMLAttributeError>;
    new ExceptionProducer<Base::FileException>;
    new ExceptionProducer<Base::FileSystemError>;
    new ExceptionProducer<Base::BadFormatError>;
    new ExceptionProducer<Base::MemoryException>;
    new ExceptionProducer<Base::AccessViolation>;
    new ExceptionProducer<Base::AbnormalProgramTermination>;
    new ExceptionProducer<Base::UnknownProgramOption>;
    new ExceptionProducer<Base::ProgramInformation>;
    new ExceptionProducer<Base::TypeError>;
    new ExceptionProducer<Base::ValueError>;
    new ExceptionProducer<Base::IndexError>;
    new ExceptionProducer<Base::NameError>;
    new ExceptionProducer<Base::ImportError>;
    new ExceptionProducer<Base::AttributeError>;
    new ExceptionProducer<Base::RuntimeError>;
    new ExceptionProducer<Base::BadGraphError>;
    new ExceptionProducer<Base::NotImplementedError>;
    new ExceptionProducer<Base::DivisionByZeroError>;
    new ExceptionProducer<Base::ReferencesError>;
    new ExceptionProducer<Base::ExpressionError>;
    new ExceptionProducer<Base::ParserError>;
    new ExceptionProducer<Base::UnicodeError>;
    new ExceptionProducer<Base::OverflowError>;
    new ExceptionProducer<Base::UnderflowError>;
    new ExceptionProducer<Base::UnitsMismatchError>;
    new ExceptionProducer<Base::CADKernelError>;
    new ExceptionProducer<Base::RestoreError>;
}

void Application::initConfig(int argc, char ** argv)
{
    // find the home path....
    mConfig["AppHomePath"] = FindHomePath(argv[0]);

    // Version of the application extracted from SubWCRef into src/Build/Version.h
    // We only set these keys if not yet defined. Therefore it suffices to search
    // only for 'BuildVersionMajor'.
    if (App::Application::Config().find("BuildVersionMajor") == App::Application::Config().end()) {
        std::stringstream str; str << FCVersionMajor << "." << FCVersionMinor;
        App::Application::Config()["ExeVersion"         ] = str.str();
        App::Application::Config()["BuildVersionMajor"  ] = FCVersionMajor;
        App::Application::Config()["BuildVersionMinor"  ] = FCVersionMinor;
        App::Application::Config()["BuildRevision"      ] = FCRevision;
        App::Application::Config()["BuildRepositoryURL" ] = FCRepositoryURL;
        App::Application::Config()["BuildRevisionDate"  ] = FCRevisionDate;
#if defined(FCRepositoryHash)
        App::Application::Config()["BuildRevisionHash"  ] = FCRepositoryHash;
#endif
#if defined(FCRepositoryBranch)
        App::Application::Config()["BuildRevisionBranch"] = FCRepositoryBranch;
#endif
    }

    _argc = argc;
    _argv = argv;

    // Now it's time to read-in the file branding.xml if it exists
    Branding brand;
    QString binDir = QString::fromUtf8((mConfig["AppHomePath"] + "bin").c_str());
    QFileInfo fi(binDir, QString::fromLatin1("branding.xml"));
    if (fi.exists() && brand.readFile(fi.absoluteFilePath())) {
        Branding::XmlConfig cfg = brand.getUserDefines();
        for (Branding::XmlConfig::iterator it = cfg.begin(); it != cfg.end(); ++it) {
            App::Application::Config()[it.key()] = it.value();
        }
    }

    // extract home paths
    ExtractUserPath();

#   ifdef FC_DEBUG
    mConfig["Debug"] = "1";
#   else
    mConfig["Debug"] = "0";
#   endif

    // init python
    PyImport_AppendInittab ("FreeCAD", init_freecad_module);
    PyImport_AppendInittab ("__FreeCADBase__", init_freecad_base_module);
    const char* pythonpath = Interpreter().init(argc,argv);
    if (pythonpath)
        mConfig["PythonSearchPath"] = pythonpath;
    else
        Base::Console().Warning("Encoding of Python paths failed\n");

    // Parse the options that have impact on the init process
    ParseOptions(argc,argv);

    // Init console ===========================================================
    Base::PyGILStateLocker lock;
    if (mConfig["LoggingConsole"] == "1") {
        _pConsoleObserverStd = new ConsoleObserverStd();
        Console().AttachObserver(_pConsoleObserverStd);
    }
    if (mConfig["Verbose"] == "Strict")
        Console().UnsetConsoleMode(ConsoleSingleton::Verbose);

    // file logging Init ===========================================================
    if (mConfig["LoggingFile"] == "1") {
        _pConsoleObserverFile = new ConsoleObserverFile(mConfig["LoggingFileName"].c_str());
        Console().AttachObserver(_pConsoleObserverFile);
    }
    else
        _pConsoleObserverFile = 0;

    // Banner ===========================================================
    if (!(mConfig["RunMode"] == "Cmd")) {
        // Remove banner if FreeCAD is invoked via the -c command as regular
        // Python interpreter
        if (!(mConfig["Verbose"] == "Strict"))
            Console().Message("%s %s, Libs: %s.%sR%s\n%s",mConfig["ExeName"].c_str(),
                              mConfig["ExeVersion"].c_str(),
                              mConfig["BuildVersionMajor"].c_str(),
                              mConfig["BuildVersionMinor"].c_str(),
                              mConfig["BuildRevision"].c_str(),
                              mConfig["CopyrightInfo"].c_str());
        else
            Console().Message("%s %s, Libs: %s.%sB%s\n",mConfig["ExeName"].c_str(),
                              mConfig["ExeVersion"].c_str(),
                              mConfig["BuildVersionMajor"].c_str(),
                              mConfig["BuildVersionMinor"].c_str(),
                              mConfig["BuildRevision"].c_str());
    }
    LoadParameters();

    auto loglevelParam = _pcUserParamMngr->GetGroup("BaseApp/LogLevels");
    const auto &loglevels = loglevelParam->GetIntMap();
    bool hasDefault = false;
    for (const auto &v : loglevels) {
        if (v.first == "Default") {
#ifndef FC_DEBUG
            if (v.second>=0) {
                hasDefault = true;
                Base::Console().SetDefaultLogLevel(v.second);
            }
#endif
        }
        else if (v.first == "DebugDefault") {
#ifdef FC_DEBUG
            if (v.second>=0) {
                hasDefault = true;
                Base::Console().SetDefaultLogLevel(v.second);
            }
#endif
        }
        else {
            *Base::Console().GetLogLevel(v.first.c_str()) = v.second;
        }
    }

    if (!hasDefault) {
#ifdef FC_DEBUG
        loglevelParam->SetInt("DebugDefault", Base::Console().LogLevel(-1));
#else
        loglevelParam->SetInt("Default", Base::Console().LogLevel(-1));
#endif
    }

    // Change application tmp. directory
    std::string tmpPath = _pcUserParamMngr->GetGroup("BaseApp/Preferences/General")->GetASCII("TempPath");
    Base::FileInfo di(tmpPath);
    if (di.exists() && di.isDir()) {
        mConfig["AppTempPath"] = tmpPath + "/";
    }


    // capture python variables
    SaveEnv("PYTHONPATH");
    SaveEnv("PYTHONHOME");
    SaveEnv("TCL_LIBRARY");
    SaveEnv("TCLLIBPATH");

    // capture CasCade variables
    SaveEnv("CSF_MDTVFontDirectory");
    SaveEnv("CSF_MDTVTexturesDirectory");
    SaveEnv("CSF_UnitsDefinition");
    SaveEnv("CSF_UnitsLexicon");
    SaveEnv("CSF_StandardDefaults");
    SaveEnv("CSF_PluginDefaults");
    SaveEnv("CSF_LANGUAGE");
    SaveEnv("CSF_SHMessage");
    SaveEnv("CSF_XCAFDefaults");
    SaveEnv("CSF_GraphicShr");
    SaveEnv("CSF_IGESDefaults");
    SaveEnv("CSF_STEPDefaults");

    // capture path
    SaveEnv("PATH");
    logStatus();
}

void Application::SaveEnv(const char* s)
{
    char *c = getenv(s);
    if (c)
        mConfig[s] = c;
}

void Application::initApplication(void)
{
    // interpreter and Init script ==========================================================
    // register scripts
    new ScriptProducer( "CMakeVariables", CMakeVariables );
    new ScriptProducer( "FreeCADInit",    FreeCADInit    );
    new ScriptProducer( "FreeCADTest",    FreeCADTest    );

    // creating the application
    if (!(mConfig["Verbose"] == "Strict")) Console().Log("Create Application\n");
    Application::_pcSingleton = new Application(mConfig);

    // set up Unit system default
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
       ("User parameter:BaseApp/Preferences/Units");
    UnitsApi::setSchema((UnitSystem)hGrp->GetInt("UserSchema",0));
    UnitsApi::setDecimals(hGrp->GetInt("Decimals", Base::UnitsApi::getDecimals()));

    // In case we are using fractional inches, get user setting for min unit
    int denom = hGrp->GetInt("FracInch", Base::QuantityFormat::getDefaultDenominator());
    Base::QuantityFormat::setDefaultDenominator(denom);


#if defined (_DEBUG)
    Console().Log("Application is built with debug information\n");
#endif

    // starting the init script
    Console().Log("Run App init script\n");
    try {
        Interpreter().runString(Base::ScriptFactory().ProduceScript("CMakeVariables"));
        Interpreter().runString(Base::ScriptFactory().ProduceScript("FreeCADInit"));
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }

    // seed randomizer
    srand(time(0));
}

std::list<std::string> Application::getCmdLineFiles()
{
    std::list<std::string> files;

    // cycling through all the open files
    unsigned short count = 0;
    count = atoi(mConfig["OpenFileCount"].c_str());
    std::string File;

    for (unsigned short i=0; i<count; i++) {
        // getting file name
        std::ostringstream temp;
        temp << "OpenFile" << i;

        std::string file(mConfig[temp.str()]);
        files.push_back(file);
    }

    return files;
}

std::list<std::string> Application::processFiles(const std::list<std::string>& files)
{
    std::list<std::string> processed;
    Base::Console().Log("Init: Processing command line files\n");
    for (std::list<std::string>::const_iterator it = files.begin(); it != files.end(); ++it) {
        Base::FileInfo file(*it);

        Base::Console().Log("Init:     Processing file: %s\n",file.filePath().c_str());

        try {
            if (file.hasExtension("fcstd") || file.hasExtension("std")) {
                // try to open
                Application::_pcSingleton->openDocument(file.filePath().c_str());
                processed.push_back(*it);
            }
            else if (file.hasExtension("fcscript") || file.hasExtension("fcmacro")) {
                Base::Interpreter().runFile(file.filePath().c_str(), true);
                processed.push_back(*it);
            }
            else if (file.hasExtension("py")) {
                try{
                    Base::Interpreter().loadModule(file.fileNamePure().c_str());
                    processed.push_back(*it);
                }
                catch(const PyException&) {
                    // if loading the module does not work, try just running the script (run in __main__)
                    Base::Interpreter().runFile(file.filePath().c_str(),true);
                    processed.push_back(*it);
                }
            }
            else {
                std::string ext = file.extension();
                std::vector<std::string> mods = App::GetApplication().getImportModules(ext.c_str());
                if (!mods.empty()) {
                    std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(file.filePath().c_str());
                    escapedstr = Base::Tools::escapeEncodeFilename(escapedstr);

                    Base::Interpreter().loadModule(mods.front().c_str());
                    Base::Interpreter().runStringArg("import %s",mods.front().c_str());
                    Base::Interpreter().runStringArg("%s.open(u\"%s\")",mods.front().c_str(),
                            escapedstr.c_str());
                    processed.push_back(*it);
                    Base::Console().Log("Command line open: %s.open(u\"%s\")\n",mods.front().c_str(),escapedstr.c_str());
                }
                else if (file.exists()) {
                    Console().Warning("File format not supported: %s \n", file.filePath().c_str());
                }
            }
        }
        catch (const Base::SystemExitException&) {
            throw; // re-throw to main() function
        }
        catch (const Base::Exception& e) {
            Console().Error("Exception while processing file: %s [%s]\n", file.filePath().c_str(), e.what());
        }
        catch (...) {
            Console().Error("Unknown exception while processing file: %s \n", file.filePath().c_str());
        }
    }

    return processed; // successfully processed files
}

void Application::processCmdLineFiles(void)
{
    // process files passed to command line
    std::list<std::string> files = getCmdLineFiles();
    std::list<std::string> processed = processFiles(files);

    if (files.empty()) {
        if (mConfig["RunMode"] == "Exit")
            mConfig["RunMode"] = "Cmd";
    }
    else if (processed.empty() && files.size() == 1 && mConfig["RunMode"] == "Cmd") {
        // In case we are in console mode and the argument is not a file but Python code
        // then execute it. This is to behave like the standard Python executable.
        Base::FileInfo file(files.front());
        if (!file.exists()) {
            Interpreter().runString(files.front().c_str());
            mConfig["RunMode"] = "Exit";
        }
    }

    const std::map<std::string,std::string>& cfg = Application::Config();
    std::map<std::string,std::string>::const_iterator it = cfg.find("SaveFile");
    if (it != cfg.end()) {
        std::string output = it->second;
        output = Base::Tools::escapeEncodeFilename(output);

        Base::FileInfo fi(output);
        std::string ext = fi.extension();
        try {
            std::vector<std::string> mods = App::GetApplication().getExportModules(ext.c_str());
            if (!mods.empty()) {
                Base::Interpreter().loadModule(mods.front().c_str());
                Base::Interpreter().runStringArg("import %s",mods.front().c_str());
                Base::Interpreter().runStringArg("%s.export(App.ActiveDocument.Objects, '%s')"
                    ,mods.front().c_str(),output.c_str());
            }
            else {
                Console().Warning("File format not supported: %s \n", output.c_str());
            }
        }
        catch (const Base::Exception& e) {
            Console().Error("Exception while saving to file: %s [%s]\n", output.c_str(), e.what());
        }
        catch (...) {
            Console().Error("Unknown exception while saving to file: %s \n", output.c_str());
        }
    }
}

void Application::runApplication()
{
    // process all files given through command line interface
    processCmdLineFiles();

    if (mConfig["RunMode"] == "Cmd") {
        // Run the commandline interface
        Interpreter().runCommandLine("FreeCAD Console mode");
    }
    else if (mConfig["RunMode"] == "Internal") {
        // run internal script
        Console().Log("Running internal script:\n");
        Interpreter().runString(Base::ScriptFactory().ProduceScript(mConfig["ScriptFileName"].c_str()));
    }
    else if (mConfig["RunMode"] == "Exit") {
        // getting out
        Console().Log("Exiting on purpose\n");
    }
    else {
        Console().Log("Unknown Run mode (%d) in main()?!?\n\n",mConfig["RunMode"].c_str());
    }
}

void Application::logStatus()
{
    time_t now;
    time(&now);
    Console().Log("Time = %s", ctime(&now));

    for (std::map<std::string,std::string>::iterator It = mConfig.begin();It!= mConfig.end();++It) {
        Console().Log("%s = %s\n",It->first.c_str(),It->second.c_str());
    }
}

void Application::LoadParameters(void)
{
    // Init parameter sets ===========================================================
    //
    if (mConfig.find("UserParameter") == mConfig.end())
        mConfig["UserParameter"]   = mConfig["UserAppData"] + "user.cfg";
    if (mConfig.find("SystemParameter") == mConfig.end())
        mConfig["SystemParameter"] = mConfig["UserAppData"] + "system.cfg";

    // create standard parameter sets
    _pcSysParamMngr = new ParameterManager();
    _pcSysParamMngr->SetSerializer(new ParameterSerializer(mConfig["SystemParameter"]));

    _pcUserParamMngr = new ParameterManager();
    _pcUserParamMngr->SetSerializer(new ParameterSerializer(mConfig["UserParameter"]));

    try {
        if (_pcSysParamMngr->LoadOrCreateDocument() && !(mConfig["Verbose"] == "Strict")) {
            // Configuration file optional when using as Python module
            if (!Py_IsInitialized()) {
                Console().Warning("   Parameter does not exist, writing initial one\n");
                Console().Message("   This warning normally means that FreeCAD is running for the first time\n"
                                  "   or the configuration was deleted or moved. FreeCAD is generating the standard\n"
                                  "   configuration.\n");
            }
        }
    }
    catch (const Base::Exception& e) {
        // try to proceed with an empty XML document
        Base::Console().Error("%s in file %s.\n"
                              "Continue with an empty configuration.\n",
                              e.what(), mConfig["SystemParameter"].c_str());
        _pcSysParamMngr->CreateDocument();
    }

    try {
        if (_pcUserParamMngr->LoadOrCreateDocument() && !(mConfig["Verbose"] == "Strict")) {
            // The user parameter file doesn't exist. When an alternative parameter file is offered
            // this will be used.
            std::map<std::string, std::string>::iterator it = mConfig.find("UserParameterTemplate");
            if (it != mConfig.end()) {
                QString path = QString::fromUtf8(it->second.c_str());
                if (QDir(path).isRelative()) {
                    QString home = QString::fromUtf8(mConfig["AppHomePath"].c_str());
                    path = QFileInfo(QDir(home), path).absoluteFilePath();
                }
                QFileInfo fi(path);
                if (fi.exists()) {
                    _pcUserParamMngr->LoadDocument(path.toUtf8().constData());
                }
            }

            // Configuration file optional when using as Python module
            if (!Py_IsInitialized()) {
                Console().Warning("   User settings do not exist, writing initial one\n");
                Console().Message("   This warning normally means that FreeCAD is running for the first time\n"
                                  "   or your configuration was deleted or moved. The system defaults\n"
                                  "   will be automatically generated for you.\n");
            }
        }
    }
    catch (const Base::Exception& e) {
        // try to proceed with an empty XML document
        Base::Console().Error("%s in file %s.\n"
                              "Continue with an empty configuration.\n",
                              e.what(), mConfig["UserParameter"].c_str());
        _pcUserParamMngr->CreateDocument();
    }
}


#if defined(_MSC_VER)
// fix weird error while linking boost (all versions of VC)
// VS2010: https://forum.freecadweb.org/viewtopic.php?f=4&t=1886&p=12553&hilit=boost%3A%3Afilesystem%3A%3Aget#p12553
namespace boost { namespace program_options { std::string arg="arg"; } }
namespace boost { namespace program_options {
    const unsigned options_description::m_default_line_length = 80;
} }
#endif

pair<string, string> customSyntax(const string& s)
{
#if defined(FC_OS_MACOSX)
    if (s.find("-psn_") == 0)
        return make_pair(string("psn"), s.substr(5));
#endif
    if (s.find("-display") == 0)
        return make_pair(string("display"), string("null"));
    else if (s.find("-style") == 0)
        return make_pair(string("style"), string("null"));
    else if (s.find("-graphicssystem") == 0)
        return make_pair(string("graphicssystem"), string("null"));
    else if (s.find("-widgetcount") == 0)
        return make_pair(string("widgetcount"), string(""));
    else if (s.find("-geometry") == 0)
        return make_pair(string("geometry"), string("null"));
    else if (s.find("-font") == 0)
        return make_pair(string("font"), string("null"));
    else if (s.find("-fn") == 0)
        return make_pair(string("fn"), string("null"));
    else if (s.find("-background") == 0)
        return make_pair(string("background"), string("null"));
    else if (s.find("-bg") == 0)
        return make_pair(string("bg"), string("null"));
    else if (s.find("-foreground") == 0)
        return make_pair(string("foreground"), string("null"));
    else if (s.find("-fg") == 0)
        return make_pair(string("fg"), string("null"));
    else if (s.find("-button") == 0)
        return make_pair(string("button"), string("null"));
    else if (s.find("-btn") == 0)
        return make_pair(string("btn"), string("null"));
    else if (s.find("-name") == 0)
        return make_pair(string("name"), string("null"));
    else if (s.find("-title") == 0)
        return make_pair(string("title"), string("null"));
    else if (s.find("-visual") == 0)
        return make_pair(string("visual"), string("null"));
//  else if (s.find("-ncols") == 0)
//    return make_pair(string("ncols"), boost::program_options::value<int>(1));
//  else if (s.find("-cmap") == 0)
//    return make_pair(string("cmap"), string("null"));
    else if ('@' == s[0])
        return std::make_pair(string("response-file"), s.substr(1));
    else
        return make_pair(string(), string());

}

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, " "));
    return os;
}

void Application::ParseOptions(int ac, char ** av)
{
    // Declare a group of options that will be
    // allowed only on the command line
    options_description generic("Generic options");
    generic.add_options()
    ("version,v", "Prints version string")
    ("help,h", "Prints help message")
    ("console,c", "Starts in console mode")
    ("response-file", value<string>(),"Can be specified with '@name', too")
    ("dump-config", "Dumps configuration")
    ("get-config", value<string>(), "Prints the value of the requested configuration key")
    ;

    // Declare a group of options that will be
    // allowed both on the command line and in
    // the config file
    std::string descr("Writes a log file to:\n");
    descr += mConfig["UserAppData"];
    descr += mConfig["ExeName"];
    descr += ".log";
    boost::program_options::options_description config("Configuration");
    config.add_options()
    //("write-log,l", value<string>(), "write a log file")
    ("write-log,l", descr.c_str())
    ("log-file", value<string>(), "Unlike --write-log this allows logging to an arbitrary file")
    ("user-cfg,u", value<string>(),"User config file to load/save user settings")
    ("system-cfg,s", value<string>(),"System config file to load/save system settings")
    ("run-test,t",   value<string>()   ,"Test case - or 0 for all")
    ("module-path,M", value< vector<string> >()->composing(),"Additional module paths")
    ("python-path,P", value< vector<string> >()->composing(),"Additional python paths")
    ("single-instance", "Allow to run a single instance of the application")
    ;


    // Hidden options, will be allowed both on the command line and
    // in the config file, but will not be shown to the user.
    boost::program_options::options_description hidden("Hidden options");
    hidden.add_options()
    ("input-file", boost::program_options::value< vector<string> >(), "input file")
    ("output",     boost::program_options::value<string>(),"output file")
    ("hidden",                                             "don't show the main window")
    // this are to ignore for the window system (QApplication)
    ("style",      boost::program_options::value< string >(), "set the application GUI style")
    ("stylesheet", boost::program_options::value< string >(), "set the application stylesheet")
    ("session",    boost::program_options::value< string >(), "restore the application from an earlier session")
    ("reverse",                                               "set the application's layout direction from right to left")
    ("widgetcount",                                           "print debug messages about widgets")
    ("graphicssystem", boost::program_options::value< string >(), "backend to be used for on-screen widgets and pixmaps")
    ("display",    boost::program_options::value< string >(), "set the X-Server")
    ("geometry ",  boost::program_options::value< string >(), "set the X-Window geometry")
    ("font",       boost::program_options::value< string >(), "set the X-Window font")
    ("fn",         boost::program_options::value< string >(), "set the X-Window font")
    ("background", boost::program_options::value< string >(), "set the X-Window background color")
    ("bg",         boost::program_options::value< string >(), "set the X-Window background color")
    ("foreground", boost::program_options::value< string >(), "set the X-Window foreground color")
    ("fg",         boost::program_options::value< string >(), "set the X-Window foreground color")
    ("button",     boost::program_options::value< string >(), "set the X-Window button color")
    ("btn",        boost::program_options::value< string >(), "set the X-Window button color")
    ("name",       boost::program_options::value< string >(), "set the X-Window name")
    ("title",      boost::program_options::value< string >(), "set the X-Window title")
    ("visual",     boost::program_options::value< string >(), "set the X-Window to color scheme")
    ("ncols",      boost::program_options::value< int    >(), "set the X-Window to color scheme")
    ("cmap",                                                  "set the X-Window to color scheme")
#if defined(FC_OS_MACOSX)
    ("psn",        boost::program_options::value< string >(), "process serial number")
#endif
    ;

    // Ignored options, will be safely ignored. Mostly used by underlying libs.
    //boost::program_options::options_description x11("X11 options");
    //x11.add_options()
    //    ("display",  boost::program_options::value< string >(), "set the X-Server")
    //    ;
    //0000723: improper handling of qt specific command line arguments
    std::vector<std::string> args;
    bool merge=false;
    for (int i=1; i<ac; i++) {
        if (merge) {
            merge = false;
            args.back() += "=";
            args.back() += av[i];
        }
        else {
            args.push_back(av[i]);
        }
        if (strcmp(av[i],"-style") == 0) {
            merge = true;
        }
        else if (strcmp(av[i],"-stylesheet") == 0) {
            merge = true;
        }
        else if (strcmp(av[i],"-session") == 0) {
            merge = true;
        }
        else if (strcmp(av[i],"-graphicssystem") == 0) {
            merge = true;
        }
    }

    // 0000659: SIGABRT on startup in boost::program_options (Boost 1.49)
    // Add some text to the constructor
    options_description cmdline_options("Command-line options");
    cmdline_options.add(generic).add(config).add(hidden);

    boost::program_options::options_description config_file_options("Config");
    config_file_options.add(config).add(hidden);

    boost::program_options::options_description visible("Allowed options");
    visible.add(generic).add(config);

    boost::program_options::positional_options_description p;
    p.add("input-file", -1);

    variables_map vm;
    try {
        store( boost::program_options::command_line_parser(args).
               options(cmdline_options).positional(p).extra_parser(customSyntax).run(), vm);

        std::ifstream ifs("FreeCAD.cfg");
        if (ifs)
            store(parse_config_file(ifs, config_file_options), vm);
        notify(vm);
    }
    catch (const std::exception& e) {
        std::stringstream str;
        str << e.what() << endl << endl << visible << endl;
        throw UnknownProgramOption(str.str());
    }
    catch (...) {
        std::stringstream str;
        str << "Wrong or unknown option, bailing out!" << endl << endl << visible << endl;
        throw UnknownProgramOption(str.str());
    }

    if (vm.count("help")) {
        std::stringstream str;
        str << mConfig["ExeName"] << endl << endl;
        str << "For a detailed description see https://www.freecadweb.org/wiki/Start_up_and_Configuration" << endl<<endl;
        str << "Usage: " << mConfig["ExeName"] << " [options] File1 File2 ..." << endl << endl;
        str << visible << endl;
        throw Base::ProgramInformation(str.str());
    }

    if (vm.count("response-file")) {
        // Load the file and tokenize it
        std::ifstream ifs(vm["response-file"].as<string>().c_str());
        if (!ifs) {
            Base::Console().Error("Could no open the response file\n");
            std::stringstream str;
            str << "Could no open the response file: '"
                << vm["response-file"].as<string>() << "'" << endl;
            throw Base::UnknownProgramOption(str.str());
        }
        // Read the whole file into a string
        stringstream ss;
        ss << ifs.rdbuf();
        // Split the file content
        char_separator<char> sep(" \n\r");
        tokenizer<char_separator<char> > tok(ss.str(), sep);
        vector<string> args;
        copy(tok.begin(), tok.end(), back_inserter(args));
        // Parse the file and store the options
        store( boost::program_options::command_line_parser(args).
               options(cmdline_options).positional(p).extra_parser(customSyntax).run(), vm);
    }

    if (vm.count("version")) {
        std::stringstream str;
        str << mConfig["ExeName"] << " " << mConfig["ExeVersion"]
            << " Revision: " << mConfig["BuildRevision"] << std::endl;
        throw Base::ProgramInformation(str.str());
    }

    if (vm.count("console")) {
        mConfig["Console"] = "1";
        mConfig["RunMode"] = "Cmd";
    }

    if (vm.count("module-path")) {
        vector<string> Mods = vm["module-path"].as< vector<string> >();
        string temp;
        for (vector<string>::const_iterator It= Mods.begin();It != Mods.end();++It)
            temp += *It + ";";
        temp.erase(temp.end()-1);
        mConfig["AdditionalModulePaths"] = temp;
    }

    if (vm.count("python-path")) {
        vector<string> Paths = vm["python-path"].as< vector<string> >();
        for (vector<string>::const_iterator It= Paths.begin();It != Paths.end();++It)
            Base::Interpreter().addPythonPath(It->c_str());
    }

    if (vm.count("input-file")) {
        vector<string> files(vm["input-file"].as< vector<string> >());
        int OpenFileCount=0;
        for (vector<string>::const_iterator It = files.begin();It != files.end();++It) {

            //cout << "Input files are: "
            //     << vm["input-file"].as< vector<string> >() << "\n";

            std::ostringstream temp;
            temp << "OpenFile" << OpenFileCount;
            mConfig[temp.str()] = *It;
            OpenFileCount++;
        }
        std::ostringstream buffer;
        buffer << OpenFileCount;
        mConfig["OpenFileCount"] = buffer.str();
    }

    if (vm.count("output")) {
        string file = vm["output"].as<string>();
        mConfig["SaveFile"] = file;
    }

    if (vm.count("hidden")) {
        mConfig["StartHidden"] = "1";
    }

    if (vm.count("write-log")) {
        mConfig["LoggingFile"] = "1";
        //mConfig["LoggingFileName"] = vm["write-log"].as<string>();
        mConfig["LoggingFileName"] = mConfig["UserAppData"] + mConfig["ExeName"] + ".log";
    }

    if (vm.count("log-file")) {
        mConfig["LoggingFile"] = "1";
        mConfig["LoggingFileName"] = vm["log-file"].as<string>();
    }

    if (vm.count("user-cfg")) {
        mConfig["UserParameter"] = vm["user-cfg"].as<string>();
    }

    if (vm.count("system-cfg")) {
        mConfig["SystemParameter"] = vm["system-cfg"].as<string>();
    }

    if (vm.count("run-test")) {
        string testCase = vm["run-test"].as<string>();
        if ( "0" == testCase) {
            testCase = "TestApp.All";
        }
        mConfig["TestCase"] = testCase;
        mConfig["RunMode"] = "Internal";
        mConfig["ScriptFileName"] = "FreeCADTest";
        //sScriptName = FreeCADTest;
    }

    if (vm.count("single-instance")) {
        mConfig["SingleInstance"] = "1";
    }

    if (vm.count("dump-config")) {
        std::stringstream str;
        for (std::map<std::string,std::string>::iterator it=mConfig.begin(); it != mConfig.end(); ++it) {
            str << it->first << "=" << it->second << std::endl;
        }
        throw Base::ProgramInformation(str.str());
    }

    if (vm.count("get-config")) {
        std::string configKey = vm["get-config"].as<string>();
        std::stringstream str;
        std::map<std::string,std::string>::iterator pos;
        pos = mConfig.find(configKey);
        if (pos != mConfig.end()) {
            str << pos->second;
        }
        str << std::endl;
        throw Base::ProgramInformation(str.str());
    }
}

void Application::ExtractUserPath()
{
    // std paths
    mConfig["BinPath"] = mConfig["AppHomePath"] + "bin" + PATHSEP;
    mConfig["DocPath"] = mConfig["AppHomePath"] + "doc" + PATHSEP;

    // Set application tmp. directory
    mConfig["AppTempPath"] = Base::FileInfo::getTempPath();

    // this is to support a portable version of FreeCAD
    QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
    QString userHome = env.value(QString::fromLatin1("FREECAD_USER_HOME"));
    QString userData = env.value(QString::fromLatin1("FREECAD_USER_DATA"));
    QString userTemp = env.value(QString::fromLatin1("FREECAD_USER_TEMP"));

    // verify env. variables
    if (!userHome.isEmpty()) {
        QDir dir(userHome);
        if (dir.exists())
            userHome = QDir::toNativeSeparators(dir.canonicalPath());
        else
            userHome.clear();
    }

    if (!userData.isEmpty()) {
        QDir dir(userData);
        if (dir.exists())
            userData = QDir::toNativeSeparators(dir.canonicalPath());
        else
            userData.clear();
    }
    else if (!userHome.isEmpty()) {
        // if FREECAD_USER_HOME is set but not FREECAD_USER_DATA
        userData = userHome;
    }

    // override temp directory if set by env. variable
    if (!userTemp.isEmpty()) {
        QDir dir(userTemp);
        if (dir.exists()) {
            userTemp = dir.canonicalPath();
            userTemp += QDir::separator();
            userTemp = QDir::toNativeSeparators(userTemp);
            mConfig["AppTempPath"] = userTemp.toUtf8().data();
        }
    }
    else if (!userHome.isEmpty()) {
        // if FREECAD_USER_HOME is set but not FREECAD_USER_TEMP
        QDir dir(userHome);
        dir.mkdir(QString::fromLatin1("temp"));
        QFileInfo fi(dir, QString::fromLatin1("temp"));
        QString tmp(fi.absoluteFilePath());
        tmp += QDir::separator();
        tmp = QDir::toNativeSeparators(tmp);
        mConfig["AppTempPath"] = tmp.toUtf8().data();
    }

#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD)
    // Default paths for the user specific stuff
    struct passwd *pwd = getpwuid(getuid());
    if (pwd == NULL)
        throw Base::RuntimeError("Getting HOME path from system failed!");
    mConfig["UserHomePath"] = pwd->pw_dir;
    if (!userHome.isEmpty()) {
        mConfig["UserHomePath"] = userHome.toUtf8().data();
    }

    std::string path = pwd->pw_dir;
    if (!userData.isEmpty()) {
        path = userData.toUtf8().data();
    }

    std::string appData(path);
    Base::FileInfo fi(appData.c_str());
    if (!fi.exists()) {
        // This should never ever happen
        std::stringstream str;
        str << "Application data directory " << appData << " does not exist!";
        throw Base::FileSystemError(str.str());
    }

    // In order to write into our data path, we must create some directories, first.
    // If 'AppDataSkipVendor' is defined, the value of 'ExeVendor' must not be part of
    // the path.
    appData += PATHSEP;
    appData += ".";
    if (mConfig.find("AppDataSkipVendor") == mConfig.end()) {
        appData += mConfig["ExeVendor"];
        fi.setFile(appData.c_str());
        if (!fi.exists() && !Py_IsInitialized()) {
            if (!fi.createDirectory()) {
                std::string error = "Cannot create directory ";
                error += appData;
                // Want more details on console
                std::cerr << error << std::endl;
                throw Base::FileSystemError(error);
            }
        }
        appData += PATHSEP;
    }

    appData += mConfig["ExeName"];
    fi.setFile(appData.c_str());
    if (!fi.exists() && !Py_IsInitialized()) {
        if (!fi.createDirectory()) {
            std::string error = "Cannot create directory ";
            error += appData;
            // Want more details on console
            std::cerr << error << std::endl;
            throw Base::FileSystemError(error);
        }
    }

    // Actually the name of the directory where the parameters are stored should be the name of
    // the application due to branding reasons.
    appData += PATHSEP;
    mConfig["UserAppData"] = appData;

#elif defined(FC_OS_MACOSX)
    // Default paths for the user specific stuff on the platform
    struct passwd *pwd = getpwuid(getuid());
    if (pwd == NULL)
        throw Base::RuntimeError("Getting HOME path from system failed!");
    mConfig["UserHomePath"] = pwd->pw_dir;
    if (!userHome.isEmpty()) {
        mConfig["UserHomePath"] = userHome.toUtf8().data();
    }

    std::string appData = pwd->pw_dir;
    if (!userData.isEmpty()) {
        appData = userData.toUtf8().data();
    }
    appData += PATHSEP;
    appData += "Library";
    appData += PATHSEP;
    appData += "Preferences";
    Base::FileInfo fi(appData.c_str());
    if (!fi.exists()) {
        // This should never ever happen
        std::stringstream str;
        str << "Application data directory " << appData << " does not exist!";
        throw Base::FileSystemError(str.str());
    }

    // In order to write to our data path, we must create some directories, first.
    // If 'AppDataSkipVendor' is defined the value of 'ExeVendor' must not be part of
    // the path.
    appData += PATHSEP;
    if (mConfig.find("AppDataSkipVendor") == mConfig.end()) {
        appData += mConfig["ExeVendor"];
        fi.setFile(appData.c_str());
        if (!fi.exists() && !Py_IsInitialized()) {
            if (!fi.createDirectory()) {
                std::string error = "Cannot create directory ";
                error += appData;
                // Want more details on console
                std::cerr << error << std::endl;
                throw Base::FileSystemError(error);
            }
        }
        appData += PATHSEP;
    }

    appData += mConfig["ExeName"];
    fi.setFile(appData.c_str());
    if (!fi.exists() && !Py_IsInitialized()) {
        if (!fi.createDirectory()) {
            std::string error = "Cannot create directory ";
            error += appData;
            // Want more details on console
            std::cerr << error << std::endl;
            throw Base::FileSystemError(error);
        }
    }

    // Actually the name of the directory where the parameters are stored should be the name of
    // the application due to branding reasons.
    appData += PATHSEP;
    mConfig["UserAppData"] = appData;

#elif defined(FC_OS_WIN32)
    WCHAR szPath[MAX_PATH];
    char dest[MAX_PATH*3];
    // Get the default path where we can save our documents. It seems that
    // 'CSIDL_MYDOCUMENTS' doesn't work on all machines, so we use 'CSIDL_PERSONAL'
    // which does the same.
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, szPath))) {
        WideCharToMultiByte(CP_UTF8, 0, szPath, -1,dest, 256, NULL, NULL);
        mConfig["UserHomePath"] = dest;
    }
    else {
        mConfig["UserHomePath"] = mConfig["AppHomePath"];
    }

    if (!userHome.isEmpty()) {
        mConfig["UserHomePath"] = userHome.toUtf8().data();
    }

    // In the second step we want the directory where user settings of the application can be
    // kept. There we create a directory with name of the vendor and a sub-directory with name
    // of the application.
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        // convert to UTF8
        WideCharToMultiByte(CP_UTF8, 0, szPath, -1,dest, 256, NULL, NULL);

        std::string appData = dest;
        if (!userData.isEmpty()) {
            appData = userData.toUtf8().data();
        }
        Base::FileInfo fi(appData.c_str());
        if (!fi.exists()) {
            // This should never ever happen
            std::stringstream str;
            str << "Application data directory " << appData << " does not exist!";
            throw Base::FileSystemError(str.str());
        }

        // In order to write to our data path we must create some directories first.
        // If 'AppDataSkipVendor' is defined the value of 'ExeVendor' must not be part of
        // the path.
        if (mConfig.find("AppDataSkipVendor") == mConfig.end()) {
            appData += PATHSEP;
            appData += mConfig["ExeVendor"];
            fi.setFile(appData.c_str());
            if (!fi.exists() && !Py_IsInitialized()) {
                if (!fi.createDirectory()) {
                    std::string error = "Cannot create directory ";
                    error += appData;
                    // Want more details on console
                    std::cerr << error << std::endl;
                    throw Base::FileSystemError(error);
                }
            }
        }

        appData += PATHSEP;
        appData += mConfig["ExeName"];
        fi.setFile(appData.c_str());
        if (!fi.exists() && !Py_IsInitialized()) {
            if (!fi.createDirectory()) {
                std::string error = "Cannot create directory ";
                error += appData;
                // Want more details on console
                std::cerr << error << std::endl;
                throw Base::FileSystemError(error);
            }
        }

        // Actually the name of the directory where the parameters are stored should be the name of
        // the application due to branding reasons.
        appData += PATHSEP;
        mConfig["UserAppData"] = appData;

        // Create the default macro directory
        fi.setFile(getUserMacroDir());
        if (!fi.exists() && !Py_IsInitialized()) {
            if (!fi.createDirectory()) {
                // If the creation fails only write an error but do not raise an
                // exception because it doesn't prevent FreeCAD from working
                std::string error = "Cannot create directory ";
                error += fi.fileName();
                // Want more details on console
                std::cerr << error << std::endl;
            }
        }
    }
#else
# error "Implement ExtractUserPath() for your platform."
#endif
}

#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD)
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

std::string Application::FindHomePath(const char* sCall)
{
    // We have three ways to start this application either use one of the two executables or
    // import the FreeCAD.so module from a running Python session. In the latter case the
    // Python interpreter is already initialized.
    std::string absPath;
    std::string homePath;
    if (Py_IsInitialized()) {
        // Note: realpath is known to cause a buffer overflow because it
        // expands the given path to an absolute path of unknown length.
        // Even setting PATH_MAX does not necessarily solve the problem
        // for sure but the risk of overflow is rather small.
        char resolved[PATH_MAX];
        char* path = realpath(sCall, resolved);
        if (path)
            absPath = path;
    }
    else {
        // Find the path of the executable. Theoretically, there could occur a
        // race condition when using readlink, but we only use this method to
        // get the absolute path of the executable to compute the actual home
        // path. In the worst case we simply get q wrong path and FreeCAD is not
        // able to load its modules.
        char resolved[PATH_MAX];
#if defined(FC_OS_BSD)
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;
        size_t cb = sizeof(resolved);
        sysctl(mib, 4, resolved, &cb, NULL, 0);
        int nchars = strlen(resolved);
#else
        int nchars = readlink("/proc/self/exe", resolved, PATH_MAX);
#endif
        if (nchars < 0 || nchars >= PATH_MAX)
            throw Base::FileSystemError("Cannot determine the absolute path of the executable");
        resolved[nchars] = '\0'; // enforce null termination
        absPath = resolved;
    }

    // should be an absolute path now
    std::string::size_type pos = absPath.find_last_of("/");
    homePath.assign(absPath,0,pos);
    pos = homePath.find_last_of("/");
    homePath.assign(homePath,0,pos+1);

    return homePath;
}

#elif defined(FC_OS_MACOSX)
#include <mach-o/dyld.h>
#include <string>
#include <stdlib.h>
#include <sys/param.h>

std::string Application::FindHomePath(const char* call)
{
    // If Python is initialized at this point, then we're being run from
    // MainPy.cpp, which hopefully rewrote argv[0] to point at the
    // FreeCAD shared library.
    if (!Py_IsInitialized()) {
        uint32_t sz = 0;
        char *buf;

        _NSGetExecutablePath(NULL, &sz); //function only returns "sz" if first arg is to small to hold value
        buf = new char[++sz];

        if (_NSGetExecutablePath(buf, &sz) == 0) {
            char resolved[PATH_MAX];
            char* path = realpath(buf, resolved);
            delete [] buf;

            if (path) {
                std::string Call(resolved), TempHomePath;
                std::string::size_type pos = Call.find_last_of(PATHSEP);
                TempHomePath.assign(Call,0,pos);
                pos = TempHomePath.find_last_of(PATHSEP);
                TempHomePath.assign(TempHomePath,0,pos+1);
                return TempHomePath;
            }
        } else {
            delete [] buf;
        }
    }

    return call;
}

#elif defined (FC_OS_WIN32)
std::string Application::FindHomePath(const char* sCall)
{
    // We have several ways to start this application:
    // * use one of the two executables
    // * import the FreeCAD.pyd module from a running Python session. In this case the
    //   Python interpreter is already initialized.
    // * use a custom dll that links FreeCAD core dlls and that is loaded by its host application
    //   In this case the calling name should be set to FreeCADBase.dll or FreeCADApp.dll in order
    //   to locate the correct home directory
    wchar_t szFileName [MAX_PATH];
    QString dll(QString::fromUtf8(sCall));
    if (Py_IsInitialized() || dll.endsWith(QLatin1String(".dll"))) {
        GetModuleFileNameW(GetModuleHandleA(sCall),szFileName, MAX_PATH-1);
    }
    else {
        GetModuleFileNameW(0, szFileName, MAX_PATH-1);
    }

    std::wstring Call(szFileName), homePath;
    std::wstring::size_type pos = Call.find_last_of(PATHSEP);
    homePath.assign(Call,0,pos);
    pos = homePath.find_last_of(PATHSEP);
    homePath.assign(homePath,0,pos+1);

    // switch to posix style
    for (std::wstring::iterator it = homePath.begin(); it != homePath.end(); ++it) {
        if (*it == '\\')
            *it = '/';
    }

    // fixes #0001638 to avoid to load DLLs from Windows' system directories before FreeCAD's bin folder
    std::wstring binPath = homePath;
    binPath += L"bin";
    SetDllDirectoryW(binPath.c_str());

    // https://stackoverflow.com/questions/5625884/conversion-of-stdwstring-to-qstring-throws-linker-error
#ifdef _MSC_VER
    QString str = QString::fromUtf16(reinterpret_cast<const ushort *>(homePath.c_str()));
#else
    QString str = QString::fromStdWString(homePath);
#endif
    // convert to utf-8
    return str.toUtf8().data();
}

#else
# error "std::string Application::FindHomePath(const char*) not implemented"
#endif
