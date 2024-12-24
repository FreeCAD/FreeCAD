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
# if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
#  include <unistd.h>
#  include <pwd.h>
#  include <sys/types.h>
# elif defined(__MINGW32__)
#  undef WINVER
#  define WINVER 0x502 // needed for SetDllDirectory
#  include <Windows.h>
# endif
# include <boost/program_options.hpp>
# include <boost/date_time/posix_time/posix_time.hpp>
# include <boost/scope_exit.hpp>
# include <chrono>
# include <random>
# include <fmt/format.h>
#endif

#ifdef FC_OS_WIN32
# include <Shlobj.h>
# include <codecvt>
#endif

#if defined(FC_OS_BSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <LibraryVersions.h>

#include <App/MaterialPy.h>
#include <App/MetadataPy.h>
// FreeCAD Base header
#include <Base/AxisPy.h>
#include <Base/BaseClass.h>
#include <Base/BoundBoxPy.h>
#include <Base/ConsoleObserver.h>
#include <Base/ServiceProvider.h>
#include <Base/CoordinateSystemPy.h>
#include <Base/Exception.h>
#include <Base/ExceptionFactory.h>
#include <Base/FileInfo.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Interpreter.h>
#include <Base/MatrixPy.h>
#include <Base/QuantityPy.h>
#include <Base/Parameter.h>
#include <Base/Persistence.h>
#include <Base/PlacementPy.h>
#include <Base/PrecisionPy.h>
#include <Base/ProgressIndicatorPy.h>
#include <Base/RotationPy.h>
#include <Base/Tools.h>
#include <Base/Translate.h>
#include <Base/Type.h>
#include <Base/TypePy.h>
#include <Base/UnitPy.h>
#include <Base/UnitsApi.h>
#include <Base/VectorPy.h>

#include "Annotation.h"
#include "Application.h"
#include "CleanupProcess.h"
#include "ComplexGeoData.h"
#include "Services.h"
#include "DocumentObjectFileIncluded.h"
#include "DocumentObjectGroup.h"
#include "DocumentObjectGroupPy.h"
#include "DocumentObserver.h"
#include "DocumentPy.h"
#include "ExpressionParser.h"
#include "FeatureTest.h"
#include "FeaturePython.h"
#include "GeoFeature.h"
#include "GeoFeatureGroupExtension.h"
#include "ImagePlane.h"
#include "InventorObject.h"
#include "Link.h"
#include "LinkBaseExtensionPy.h"
#include "VarSet.h"
#include "MaterialObject.h"
#include "MeasureManagerPy.h"
#include "Origin.h"
#include "Datums.h"
#include "OriginGroupExtension.h"
#include "OriginGroupExtensionPy.h"
#include "SuppressibleExtension.h"
#include "SuppressibleExtensionPy.h"
#include "Part.h"
#include "GeoFeaturePy.h"
#include "Placement.h"
#include "ProgramOptionsUtilities.h"
#include "Property.h"
#include "PropertyContainer.h"
#include "PropertyExpressionEngine.h"
#include "PropertyFile.h"
#include "PropertyLinks.h"
#include "PropertyPythonObject.h"
#include "StringHasherPy.h"
#include "StringIDPy.h"
#include "TextDocument.h"
#include "Transactions.h"
#include "VRMLObject.h"

// If you stumble here, run the target "BuildExtractRevision" on Windows systems
// or the Python script "SubWCRev.py" on Linux based systems which builds
// src/Build/Version.h. Or create your own from src/Build/Version.h.in!
#include <Build/Version.h>
#include "Branding.h"


// scriptings (scripts are built-in but can be overridden by command line option)
#include <App/InitScript.h>
#include <App/TestScript.h>
#include <App/CMakeScript.h>

#include "SafeMode.h"

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

FC_LOG_LEVEL_INIT("App", true, true)

using namespace App;
using namespace Base;
using namespace std;
using namespace boost;
using namespace boost::program_options;
using Base::FileInfo;
namespace sp = std::placeholders;

//==========================================================================
// Application
//==========================================================================

Reference<ParameterManager> Application::_pcSysParamMngr;
Reference<ParameterManager> Application::_pcUserParamMngr;
ConsoleObserverStd  *Application::_pConsoleObserverStd = nullptr;
ConsoleObserverFile *Application::_pConsoleObserverFile = nullptr;

AppExport std::map<std::string, std::string> Application::mConfig;


//**************************************************************************
// Construction and destruction

// clang-format off
PyDoc_STRVAR(FreeCAD_doc,
     "The functions in the FreeCAD module allow working with documents.\n"
     "The FreeCAD instance provides a list of references of documents which\n"
     "can be addressed by a string. Hence the document name must be unique.\n"
     "\n"
     "The document has the read-only attribute FileName which points to the\n"
     "file the document should be stored to.\n"
    );

PyDoc_STRVAR(Console_doc,
    "FreeCAD Console module.\n\n"
    "The Console module contains functions to manage log entries, messages,\n"
    "warnings and errors.\n"
    "There are also functions to get/set the status of the observers used as\n"
    "logging interfaces."
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
        nullptr, nullptr, nullptr, nullptr, nullptr
    };
    return PyModule_Create(&BaseModuleDef);
}

// Set in inside Application
static PyMethodDef* ApplicationMethods = nullptr;

PyMODINIT_FUNC
init_freecad_module(void)
{
    static struct PyModuleDef FreeCADModuleDef = {
        PyModuleDef_HEAD_INIT,
        "FreeCAD", FreeCAD_doc, -1,
        ApplicationMethods,
        nullptr, nullptr, nullptr, nullptr
    };
    return PyModule_Create(&FreeCADModuleDef);
}

PyMODINIT_FUNC
init_image_module()
{
    static struct PyModuleDef ImageModuleDef = {
        PyModuleDef_HEAD_INIT,
        "Image", "", -1,
        nullptr,
        nullptr, nullptr, nullptr, nullptr
    };
    return PyModule_Create(&ImageModuleDef);
}
// clang-format on

Application::Application(std::map<std::string,std::string> &mConfig)
  : _mConfig(mConfig)
{
    mpcPramManager["System parameter"] = _pcSysParamMngr;
    mpcPramManager["User parameter"] = _pcUserParamMngr;

    setupPythonTypes();
}

Application::~Application() = default;

void Application::setupPythonTypes()
{
    // setting up Python binding
    PyGILStateLocker lock;
    PyObject* modules = PyImport_GetModuleDict();

    ApplicationMethods = Methods;
    PyObject* pAppModule = PyImport_ImportModule ("FreeCAD");
    if (!pAppModule) {
        PyErr_Clear();
        pAppModule = init_freecad_module();
        PyDict_SetItemString(modules, "FreeCAD", pAppModule);
    }
    Py::Module(pAppModule).setAttr(std::string("ActiveDocument"),Py::None());

    // clang-format off
    static struct PyModuleDef ConsoleModuleDef = {
        PyModuleDef_HEAD_INIT,
        "__FreeCADConsole__", Console_doc, -1,
        ConsoleSingleton::Methods,
        nullptr, nullptr, nullptr, nullptr
    };
    PyObject* pConsoleModule = PyModule_Create(&ConsoleModuleDef);

    // fake Image module
    PyObject* imageModule = init_image_module();
    PyDict_SetItemString(modules, "Image", imageModule);

    // introducing additional classes

    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    Interpreter().addType(&VectorPy::Type, pAppModule, "Vector");
    Interpreter().addType(&MatrixPy::Type, pAppModule, "Matrix");
    Interpreter().addType(&BoundBoxPy::Type, pAppModule, "BoundBox");
    Interpreter().addType(&PlacementPy::Type, pAppModule, "Placement");
    Interpreter().addType(&RotationPy::Type, pAppModule, "Rotation");
    Interpreter().addType(&AxisPy::Type, pAppModule, "Axis");

    // Note: Create an own module 'Base' which should provide the python
    // binding classes from the base module. At a later stage we should
    // remove these types from the FreeCAD module.

    PyObject* pBaseModule = PyImport_ImportModule ("__FreeCADBase__");
    if (!pBaseModule) {
        PyErr_Clear();
        pBaseModule = init_freecad_base_module();
        PyDict_SetItemString(modules, "__FreeCADBase__", pBaseModule);
    }

    setupPythonException(pBaseModule);


    // Python types
    Interpreter().addType(&VectorPy          ::Type,pBaseModule,"Vector");
    Interpreter().addType(&MatrixPy          ::Type,pBaseModule,"Matrix");
    Interpreter().addType(&BoundBoxPy        ::Type,pBaseModule,"BoundBox");
    Interpreter().addType(&PlacementPy       ::Type,pBaseModule,"Placement");
    Interpreter().addType(&RotationPy        ::Type,pBaseModule,"Rotation");
    Interpreter().addType(&AxisPy            ::Type,pBaseModule,"Axis");
    Interpreter().addType(&CoordinateSystemPy::Type,pBaseModule,"CoordinateSystem");
    Interpreter().addType(&TypePy            ::Type,pBaseModule,"TypeId");
    Interpreter().addType(&PrecisionPy       ::Type,pBaseModule,"Precision");

    Interpreter().addType(&MaterialPy::Type, pAppModule, "Material");
    Interpreter().addType(&MetadataPy::Type, pAppModule, "Metadata");

    Interpreter().addType(&MeasureManagerPy::Type, pAppModule, "MeasureManager");

    Interpreter().addType(&StringHasherPy::Type, pAppModule, "StringHasher");
    Interpreter().addType(&StringIDPy::Type, pAppModule, "StringID");

    // Add document types
    Interpreter().addType(&PropertyContainerPy::Type, pAppModule, "PropertyContainer");
    Interpreter().addType(&ExtensionContainerPy::Type, pAppModule, "ExtensionContainer");
    Interpreter().addType(&DocumentPy::Type, pAppModule, "Document");
    Interpreter().addType(&DocumentObjectPy::Type, pAppModule, "DocumentObject");
    Interpreter().addType(&DocumentObjectGroupPy::Type, pAppModule, "DocumentObjectGroup");
    Interpreter().addType(&GeoFeaturePy::Type, pAppModule, "GeoFeature");

    // Add extension types
    Interpreter().addType(&ExtensionPy::Type, pAppModule, "Extension");
    Interpreter().addType(&DocumentObjectExtensionPy::Type, pAppModule, "DocumentObjectExtension");
    Interpreter().addType(&GroupExtensionPy::Type, pAppModule, "GroupExtension");
    Interpreter().addType(&GeoFeatureGroupExtensionPy::Type, pAppModule, "GeoFeatureGroupExtension");
    Interpreter().addType(&OriginGroupExtensionPy::Type, pAppModule, "OriginGroupExtension");
    Interpreter().addType(&LinkBaseExtensionPy::Type, pAppModule, "LinkBaseExtension");

    //insert Base and Console
    Py_INCREF(pBaseModule);
    PyModule_AddObject(pAppModule, "Base", pBaseModule);
    Py_INCREF(pConsoleModule);
    PyModule_AddObject(pAppModule, "Console", pConsoleModule);

    // Translate module
    PyObject* pTranslateModule = Interpreter().addModule(new Translate);
    Py_INCREF(pTranslateModule);
    PyModule_AddObject(pAppModule, "Qt", pTranslateModule);

    //insert Units module
    static struct PyModuleDef UnitsModuleDef = {
        PyModuleDef_HEAD_INIT,
        "Units", "The Unit API", -1,
        UnitsApi::Methods,
        nullptr, nullptr, nullptr, nullptr
    };
    PyObject* pUnitsModule = PyModule_Create(&UnitsModuleDef);
    Interpreter().addType(&QuantityPy  ::Type,pUnitsModule,"Quantity");
    // make sure to set the 'nb_true_divide' slot
    Interpreter().addType(&UnitPy      ::Type,pUnitsModule,"Unit");

    Py_INCREF(pUnitsModule);
    PyModule_AddObject(pAppModule, "Units", pUnitsModule);

    ProgressIndicatorPy::init_type();
    Interpreter().addType(ProgressIndicatorPy::type_object(),
        pBaseModule,"ProgressIndicator");

    Vector2dPy::init_type();
    Interpreter().addType(Vector2dPy::type_object(),
        pBaseModule,"Vector2d");
    // clang-format on
}

/*
 * Define custom Python exception types
 */
void Application::setupPythonException(PyObject* module)
{
    auto setup = [&module, str {"Base."}](const std::string& ename, auto excpt) {
        auto exception = PyErr_NewException((str + ename).c_str(), excpt, nullptr);
        Py_INCREF(exception);
        PyModule_AddObject(module, ename.c_str(), exception);
        return exception;
    };

    PyExc_FC_GeneralError = setup("FreeCADError", PyExc_RuntimeError);
    PyExc_FC_FreeCADAbort = setup("FreeCADAbort", PyExc_BaseException);
    PyExc_FC_XMLBaseException = setup("XMLBaseException", PyExc_Exception);
    PyExc_FC_XMLParseException = setup("XMLParseException", PyExc_FC_XMLBaseException);
    PyExc_FC_XMLAttributeError = setup("XMLAttributeError", PyExc_FC_XMLBaseException);
    PyExc_FC_UnknownProgramOption = setup("UnknownProgramOption", PyExc_BaseException);
    PyExc_FC_BadFormatError = setup("BadFormatError", PyExc_FC_GeneralError);
    PyExc_FC_BadGraphError = setup("BadGraphError", PyExc_FC_GeneralError);
    PyExc_FC_ExpressionError = setup("ExpressionError", PyExc_FC_GeneralError);
    PyExc_FC_ParserError = setup("ParserError", PyExc_FC_GeneralError);
    PyExc_FC_CADKernelError = setup("CADKernelError", PyExc_FC_GeneralError);
    PyExc_FC_PropertyError = setup("PropertyError", PyExc_AttributeError);
    PyExc_FC_AbortIOException = setup("AbortIOException", PyExc_BaseException);
}

//**************************************************************************
// Interface

/// get called by the document when the name is changing
void Application::renameDocument(const char *OldName, const char *NewName)
{
    (void)OldName;
    (void)NewName;
    throw RuntimeError("Renaming document internal name is no longer allowed!");
}

Document* Application::newDocument(const char * Name, const char * UserName, DocumentCreateFlags CreateFlags)
{
    auto getNameAndLabel = [this](const char* Name,
                                  const char* UserName) -> std::tuple<std::string, std::string> {
        const bool defaultName = (!Name || Name[0] == '\0');

        // get a valid name anyway!
        if (defaultName) {
            Name = "Unnamed";
        }

        std::string userName;
        if (UserName && UserName[0] != '\0') {
            userName = UserName;
        }
        else {
            userName = defaultName ? QObject::tr("Unnamed").toStdString() : Name;

            std::vector<std::string> names;
            names.reserve(DocMap.size());
            for (const auto& pos : DocMap) {
                names.emplace_back(pos.second->Label.getValue());
            }

            if (!names.empty()) {
                userName = Tools::getUniqueName(userName, names);
            }
        }

        return std::make_tuple(std::string(Name), userName);
    };

    const auto tuple = getNameAndLabel(Name, UserName);
    std::string name = std::get<0>(tuple);
    std::string userName = std::get<1>(tuple);
    name = getUniqueDocumentName(name.c_str(), CreateFlags.temporary);

    // return the temporary document if it exists
    if (CreateFlags.temporary) {
        auto it = DocMap.find(name);
        if (it != DocMap.end() && it->second->testStatus(Document::TempDoc))
            return it->second;
    }

    // create the FreeCAD document
    std::unique_ptr<Document> newDoc(new Document(name.c_str()));
    newDoc->setStatus(Document::TempDoc, CreateFlags.temporary);

    const auto oldActiveDoc = _pActiveDoc;
    const auto doc = newDoc.release(); // now owned by the Application

    // add the document to the internal list
    DocMap[name] = doc;
    _pActiveDoc = doc;

    //NOLINTBEGIN
    // clang-format off
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
    _pActiveDoc->signalChangePropertyEditor.connect(std::bind(&App::Application::slotChangePropertyEditor, this, sp::_1, sp::_2));
    // clang-format on
    //NOLINTEND

    // make sure that the active document is set in case no GUI is up
    {
        PyGILStateLocker lock;
        const Py::Object active(_pActiveDoc->getPyObject(), true);
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"), active);
    }

    signalNewDocument(*_pActiveDoc, CreateFlags.createView);

    // set the UserName after notifying all observers
    _pActiveDoc->Label.setValue(userName);

    // set the old document active again if the new is temporary
    if (CreateFlags.temporary && oldActiveDoc)
        setActiveDocument(oldActiveDoc);

    return doc;
}

bool Application::closeDocument(const char* name)
{
    const auto pos = DocMap.find( name );
    if (pos == DocMap.end()) // no such document
        return false;

    ConsoleRefreshDisabler disabler;

    // Trigger observers before removing the document from the internal map.
    // Some observers might rely on this document still being there.
    signalDeleteDocument(*pos->second);

    // For exception-safety use a smart pointer
    if (_pActiveDoc == pos->second) {
        setActiveDocument(static_cast<Document*>(nullptr));
    }
    const std::unique_ptr<Document> delDoc (pos->second);
    DocMap.erase( pos );
    DocFileMap.erase(FileInfo(delDoc->FileName.getValue()).filePath());

    _objCount = -1;

    // Trigger observers after removing the document from the internal map.
    signalDeletedDocument();

    return true;
}

void Application::closeAllDocuments()
{
    FlagToggler<bool> flag(_isClosingAll);
    std::map<std::string,Document*>::iterator pos;
    while((pos = DocMap.begin()) != DocMap.end())
        closeDocument(pos->first.c_str());
}

Document* Application::getDocument(const char *Name) const
{

    const auto pos = DocMap.find(Name);

    if (pos == DocMap.end())
        return nullptr;

    return pos->second;
}

const char * Application::getDocumentName(const Document* doc) const
{
    for (const auto& [str, docp] : DocMap) {
        if (docp == doc) {
            return str.c_str();
        }
    }

    return nullptr;
}

std::vector<Document*> Application::getDocuments() const
{
    std::vector<Document*> docs;
    for (const auto & it : DocMap)
        docs.push_back(it.second);
    return docs;
}

std::string Application::getUniqueDocumentName(const char *Name, bool tempDoc) const
{
    if (!Name || *Name == '\0')
        return {};
    std::string CleanName = Tools::getIdentifier(Name);

    // name in use?
    auto pos = DocMap.find(CleanName);

    if (pos == DocMap.end() || (tempDoc && pos->second->testStatus(Document::TempDoc))) {
        // if not, name is OK
        return CleanName;
    }
    else {
        std::vector<std::string> names;
        names.reserve(DocMap.size());
        for (pos = DocMap.begin(); pos != DocMap.end(); ++pos) {
            if (!tempDoc || !pos->second->testStatus(Document::TempDoc))
                names.push_back(pos->first);
        }
        return Tools::getUniqueName(CleanName, names);
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
    if (!_docReloadAttempts[FileName].emplace(objName).second) {
        return -1;
    }
    const auto ret =  _pendingDocMap.emplace(FileName,std::vector<std::string>());
    ret.first->second.emplace_back(objName);
    if(ret.second) {
        _pendingDocs.emplace_back(ret.first->first.c_str());
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
    signals2::signal<void ()> &signal;
    DocOpenGuard(bool &f, signals2::signal<void ()> &s)
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
                Console().Warning("~DocOpenGuard: Unexpected boost exception\n");
            }
        }
    }
};

Document* Application::openDocument(const char * FileName, DocumentCreateFlags createFlags) {
    std::vector<std::string> filenames(1,FileName);
    auto docs = openDocuments(filenames, nullptr, nullptr, nullptr, createFlags);
    if(!docs.empty())
        return docs.front();
    return nullptr;
}

Document *Application::getDocumentByPath(const char *path, PathMatchMode checkCanonical) const {
    if(!path || !path[0])
        return nullptr;
    if (DocFileMap.empty()) {
        for (const auto& v : DocMap) {
            const auto& file = v.second->FileName.getStrValue();
            if (!file.empty()) {
                DocFileMap[FileInfo(file.c_str()).filePath()] = v.second;
            }
        }
    }
    const auto it = DocFileMap.find(FileInfo(path).filePath());
    if(it != DocFileMap.end())
        return it->second;

    if (checkCanonical == PathMatchMode::MatchAbsolute) {
        return nullptr;
    }

    const std::string filepath = FileInfo(path).filePath();
    const QString canonicalPath = QFileInfo(QString::fromUtf8(path)).canonicalFilePath();
    for (const auto &v : DocMap) {
        QFileInfo fi(QString::fromUtf8(v.second->FileName.getValue()));
        if (canonicalPath == fi.canonicalFilePath()) {
            if (checkCanonical == PathMatchMode::MatchCanonical) {
                return v.second;
            }
            const bool samePath = (canonicalPath == QString::fromUtf8(filepath.c_str()));
            FC_WARN("Identical physical path '" << canonicalPath.toUtf8().constData() << "'\n"
                    << (samePath?"":"  for file '") << (samePath?"":filepath.c_str()) << (samePath?"":"'\n")
                    << "  with existing document '" << v.second->Label.getValue()
                    << "' in path: '" << v.second->FileName.getValue() << "'");
            break;
        }
    }
    return nullptr;
}

std::vector<Document*> Application::openDocuments(const std::vector<std::string> &filenames,
                                                  const std::vector<std::string> *paths,
                                                  const std::vector<std::string> *labels,
                                                  std::vector<std::string> *errs,
                                                  DocumentCreateFlags createFlags)
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
    _docReloadAttempts.clear();

    signalStartOpenDocument();

    ParameterGrp::handle hGrp = GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    _allowPartial = !hGrp->GetBool("NoPartialLoading",false);

    for (auto &name : filenames)
        _pendingDocs.emplace_back(name.c_str());

    std::map<DocumentT, DocTiming> timings;

    FC_TIME_INIT(t);

    std::vector<DocumentT> openedDocs;

    int pass = 0;
    do {
        std::set<DocumentT> newDocs;
        for (std::size_t count=0;; ++count) {
            std::string name = std::move(_pendingDocs.front());
            _pendingDocs.pop_front();
            bool isMainDoc = (pass == 0 && count < filenames.size());

            try {
                _objCount = -1;
                std::vector<std::string> objNames;
                if (_allowPartial) {
                    auto it = _pendingDocMap.find(name);
                    if (it != _pendingDocMap.end()) {
                        if(isMainDoc)
                            it->second.clear();
                        else
                            objNames.swap(it->second);
                        _pendingDocMap.erase(it);
                    }
                }

                FC_TIME_INIT(t1);
                DocTiming timing;

                const char *path = name.c_str();
                const char *label = nullptr;
                if (isMainDoc) {
                    if (paths && paths->size()>count)
                        path = (*paths)[count].c_str();

                    if (labels && labels->size()>count)
                        label = (*labels)[count].c_str();
                }

                auto doc = openDocumentPrivate(path, name.c_str(), label, isMainDoc, createFlags, std::move(objNames));
                FC_DURATION_PLUS(timing.d1,t1);
                if (doc) {
                    timings[doc].d1 += timing.d1;
                    newDocs.emplace(doc);
                }

                if (isMainDoc)
                    res[count] = doc;
                _objCount = -1;
            }
            catch (const Exception &e) {
                e.ReportException();
                if (!errs && isMainDoc)
                    throw;
                if (errs && isMainDoc)
                    (*errs)[count] = e.what();
                else
                    Console().Error("Exception opening file: %s [%s]\n", name.c_str(), e.what());
            }
            catch (const std::exception &e) {
                if (!errs && isMainDoc)
                    throw;
                if (errs && isMainDoc)
                    (*errs)[count] = e.what();
                else
                    Console().Error("Exception opening file: %s [%s]\n", name.c_str(), e.what());
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
                if(_pendingDocsReopen.empty())
                    break;
                _pendingDocs = std::move(_pendingDocsReopen);
                _pendingDocsReopen.clear();
                for(const auto &file : _pendingDocs) {
                    auto doc = getDocumentByPath(file.c_str());
                    if(doc)
                        closeDocument(doc->getName());
                }
            }
        }

        ++pass;
        _pendingDocMap.clear();

        std::vector<Document*> docs;
        docs.reserve(newDocs.size());
        for(const auto &d : newDocs) {
            auto doc = d.getDocument();
            if(!doc)
                continue;
            // Notify PropertyXLink to attach newly opened documents and restore
            // relevant external links
            PropertyXLink::restoreDocument(*doc);
            docs.push_back(doc);
        }

        SequencerLauncher seq("Postprocessing...", docs.size());

        // After external links has been restored, we can now sort the document
        // according to their dependency order.
        try {
            docs = Document::getDependentDocuments(docs, true);
        } catch (Exception &e) {
            e.ReportException();
        }
        for(auto it=docs.begin(); it!=docs.end();) {
            auto doc = *it;

            // It is possible that the newly opened document depends on an existing
            // document, which will be included with the above call to
            // Document::getDependentDocuments(). Make sure to exclude that.
            if(!newDocs.count(doc)) {
                it = docs.erase(it);
                continue;
            }

            auto &timing = timings[doc];
            FC_TIME_INIT(t1);
            // Finalize document restoring with the correct order
            if(doc->afterRestore(true)) {
                openedDocs.emplace_back(doc);
                it = docs.erase(it);
            } else {
                ++it;
                // Here means this is a partial loaded document, and we need to
                // reload it fully because of touched objects. The reason of
                // reloading a partial document with touched object is because
                // partial document is supposed to be readonly, while a
                // 'touched' object requires recomputation. And an object may
                // become touched during restoring if externally linked
                // document time stamp mismatches with the stamp saved.
                _pendingDocs.emplace_back(doc->FileName.getValue());
                _pendingDocMap.erase(doc->FileName.getValue());
            }
            FC_DURATION_PLUS(timing.d2,t1);
            seq.next();
        }
        // Close the document for reloading
        for(const auto doc : docs)
            closeDocument(doc->getName());

    }while(!_pendingDocs.empty());

    // Set the active document using the first successfully restored main
    // document (i.e. documents explicitly asked for by caller).
    for (auto doc : res) {
        if (doc) {
            setActiveDocument(doc);
            break;
        }
    }

    for (auto &doc : openedDocs) {
        auto &timing = timings[doc];
        FC_DURATION_LOG(timing.d1, doc.getDocumentName() << " restore");
        FC_DURATION_LOG(timing.d2, doc.getDocumentName() << " postprocess");
    }
    FC_TIME_LOG(t,"total");
    _isRestoring = false;

    signalFinishOpenDocument();
    return res;
}

Document* Application::openDocumentPrivate(const char * FileName,
        const char *propFileName, const char *label,
        bool isMainDoc, DocumentCreateFlags createFlags,
        std::vector<std::string> &&objNames)
{
    FileInfo File(FileName);

    if (!File.exists()) {
        std::stringstream str;
        str << "File '" << FileName << "' does not exist!";
        throw FileSystemError(str.str().c_str());
    }

    // Before creating a new document we check whether the document is already open
    auto doc = getDocumentByPath(File.filePath().c_str(), PathMatchMode::MatchCanonicalWarning);
    if(doc) {
        if(doc->testStatus(Document::PartialDoc)
                || doc->testStatus(Document::PartialRestore)) {
            // Here means a document is already partially loaded, but the document
            // is requested again, either partial or not. We must check if the
            // document contains the required object

            if(isMainDoc) {
                // Main document must be open fully, so close and reopen
                closeDocument(doc->getName());
                doc = nullptr;
            } else if(_allowPartial) {
                bool reopen = false;
                for(const auto &name : objNames) {
                    auto obj = doc->getObject(name.c_str());
                    if(!obj || obj->testStatus(PartialObject)) {
                        reopen = true;
                        // NOTE: We are about to reload this document with
                        // extra objects. However, it is possible to repeat
                        // this process several times, if it is linked by
                        // multiple documents and each with a different set of
                        // objects. To partially solve this problem, we do not
                        // close and reopen the document immediately here, but
                        // add it to _pendingDocsReopen to delay reloading.
                        for(auto obj2 : doc->getObjects())
                            objNames.emplace_back(obj2->getNameInDocument());
                        _pendingDocMap[doc->FileName.getValue()] = std::move(objNames);
                        break;
                    }
                }
                if(!reopen)
                    return nullptr;
            }

            if(doc) {
                _pendingDocsReopen.emplace_back(FileName);
                return nullptr;
            }
        }

        if(!isMainDoc)
            return nullptr;
        else if(doc)
            return doc;
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

    Document* newDoc = newDocument(name.c_str(), label, createFlags);
    newDoc->FileName.setValue(propFileName==FileName?File.filePath():propFileName);

    try {
        // read the document
        newDoc->restore(File.filePath().c_str(),true,objNames);
        if(!DocFileMap.empty())
            DocFileMap[FileInfo(newDoc->FileName.getValue()).filePath()] = newDoc;
        return newDoc;
    }
    // if the project file itself is corrupt then
    // close the document
    catch (const FileException&) {
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

Document* Application::getActiveDocument() const
{
    return _pActiveDoc;
}

void Application::setActiveDocument(Document* pDoc)
{
    _pActiveDoc = pDoc;

    // make sure that the active document is set in case no GUI is up
    if (pDoc) {
        PyGILStateLocker lock;
        const Py::Object active(pDoc->getPyObject(), true);
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"),active);
    }
    else {
        PyGILStateLocker lock;
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"),Py::None());
    }

    if (pDoc)
        signalActiveDocument(*pDoc);
}

void Application::setActiveDocument(const char *Name)
{
    // If no active document is set, resort to a default.
    if (*Name == '\0') {
        _pActiveDoc = nullptr;
        return;
    }

    if (const auto pos = DocMap.find(Name); pos != DocMap.end()) {
        setActiveDocument(pos->second);
    }
    else {
        std::stringstream s;
        s << "Try to activate unknown document '" << Name << "'";
        throw RuntimeError(s.str());
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
            Console().Warning("~TransactionSignaller: Unexpected boost exception\n");
        }
    }
}

int64_t Application::applicationPid()
{
    static int64_t randomNumber = []() {
        const auto tp = std::chrono::high_resolution_clock::now();
        const auto dur = tp.time_since_epoch();
        const auto seed = dur.count();
        std::mt19937 generator(static_cast<unsigned>(seed));
        constexpr int64_t minValue {1};
        constexpr int64_t maxValue {1000000};
        std::uniform_int_distribution<int64_t> distribution(minValue, maxValue);
        return distribution(generator);
    }();
    return randomNumber;
}

std::string Application::getHomePath()
{
    return mConfig["AppHomePath"];
}

std::string Application::getExecutableName()
{
    return mConfig["ExeName"];
}

std::string Application::getNameWithVersion()
{
    auto appname = QCoreApplication::applicationName().toStdString();
    auto config = App::Application::Config();
    auto major = config["BuildVersionMajor"];
    auto minor = config["BuildVersionMinor"];
    auto point = config["BuildVersionPoint"];
    auto suffix = config["BuildVersionSuffix"];
    return fmt::format("{} {}.{}.{}{}", appname, major, minor, point, suffix);
}

std::string Application::getTempPath()
{
    return mConfig["AppTempPath"];
}

std::string Application::getTempFileName(const char* FileName)
{
    return FileInfo::getTempFileName(FileName, getTempPath().c_str());
}

std::string Application::getUserCachePath()
{
    return mConfig["UserCachePath"];
}

std::string Application::getUserConfigPath()
{
    return mConfig["UserConfigPath"];
}

std::string Application::getUserAppDataDir()
{
    return mConfig["UserAppData"];
}

std::string Application::getUserMacroDir()
{
    return mConfig["UserMacroPath"];
}

std::string Application::getResourceDir()
{
#ifdef RESOURCEDIR
    // #6892: Conda may inject null characters => remove them
    std::string path = std::string(RESOURCEDIR).c_str();
    path += PATHSEP;
    const QDir dir(QString::fromStdString(path));
    if (dir.isAbsolute())
        return path;
    return mConfig["AppHomePath"] + path;
#else
    return mConfig["AppHomePath"];
#endif
}

std::string Application::getLibraryDir()
{
#ifdef LIBRARYDIR
    // #6892: Conda may inject null characters => remove them
    std::string path = std::string(LIBRARYDIR).c_str();
    const QDir dir(QString::fromStdString(path));
    if (dir.isAbsolute())
        return path;
    return mConfig["AppHomePath"] + path;
#else
    return mConfig["AppHomePath"] + "lib";
#endif
}

std::string Application::getHelpDir()
{
#ifdef DOCDIR
    // #6892: Conda may inject null characters => remove them
    std::string path = std::string(DOCDIR).c_str();
    path += PATHSEP;
    const QDir dir(QString::fromStdString(path));
    if (dir.isAbsolute())
        return path;
    return mConfig["AppHomePath"] + path;
#else
    return mConfig["DocPath"];
#endif
}

int Application::checkLinkDepth(int depth, MessageOption option)
{
    if (_objCount < 0) {
        _objCount = 0;
        for (const auto &v : DocMap) {
            _objCount += v.second->countObjects();
        }
    }

    if (depth > _objCount + 2) {
        const auto msg = "Link recursion limit reached. "
                "Please check for cyclic reference.";
        switch (option) {
        case MessageOption::Quiet:
            return 0;
        case MessageOption::Error:
            FC_ERR(msg);
            return 0;
        case MessageOption::Throw:
            throw RuntimeError(msg);
        }
    }

    return _objCount + 2;
}

std::set<DocumentObject *> Application::getLinksTo(
        const DocumentObject *obj, int options, int maxCount) const
{
    std::set<DocumentObject *> links;
    if(!obj) {
        for(auto &v : DocMap) {
            v.second->getLinksTo(links,obj,options,maxCount);
            if(maxCount && static_cast<int>(links.size())>=maxCount)
                break;
        }
    } else {
        std::set<Document*> docs;
        for (const auto o : obj->getInList()) {
            if(o && o->isAttachedToDocument() && docs.insert(o->getDocument()).second) {
                o->getDocument()->getLinksTo(links,obj,options,maxCount);
                if(maxCount && static_cast<int>(links.size())>=maxCount)
                    break;
            }
        }
    }
    return links;
}

bool Application::hasLinksTo(const DocumentObject *obj) const {
    return !getLinksTo(obj,0,1).empty();
}

ParameterManager & Application::GetSystemParameter()
{
    return *_pcSysParamMngr;
}

ParameterManager & Application::GetUserParameter()
{
    return *_pcUserParamMngr;
}

ParameterManager * Application::GetParameterSet(const char* sName) const
{
    const auto it = mpcPramManager.find(sName);
    if ( it != mpcPramManager.end() )
        return it->second;
    else
        return nullptr;
}

const std::map<std::string,Reference<ParameterManager>> &
Application::GetParameterSetList() const
{
    return mpcPramManager;
}

void Application::AddParameterSet(const char* sName)
{
    const auto it = mpcPramManager.find(sName);
    if ( it != mpcPramManager.end() )
        return;
    mpcPramManager[sName] = ParameterManager::Create();
}

void Application::RemoveParameterSet(const char* sName)
{
    const auto it = mpcPramManager.find(sName);
    // Must not delete user or system parameter
    if ( it == mpcPramManager.end() || it->second == _pcUserParamMngr || it->second == _pcSysParamMngr )
        return;
    mpcPramManager.erase(it);
}

Reference<ParameterGrp>  Application::GetParameterGroupByPath(const char* sName)
{
    std::string cName = sName, cTemp;

    const std::string::size_type pos = cName.find(':');

    // is there a path separator ?
    if (pos == std::string::npos) {
        throw ValueError("Application::GetParameterGroupByPath() no parameter set name specified");
    }
    // assigning the parameter set name
    cTemp.assign(cName,0,pos);
    cName.erase(0,pos+1);

    // test if name is valid
    const auto It = mpcPramManager.find(cTemp.c_str());
    if (It == mpcPramManager.end())
        throw ValueError("Application::GetParameterGroupByPath() unknown parameter set name specified");

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
        const std::string::size_type next = item.filter.find_first_of(" )", pos + 1);
        const std::string::size_type len = next-pos-2;
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
    for (const auto & it : _mImportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt.c_str()) == 0)
#else
            if (_stricmp(Type,jt.c_str()) == 0)
#endif
                modules.push_back(it.module);
        }
    }

    return modules;
}

std::vector<std::string> Application::getImportModules() const
{
    std::vector<std::string> modules;
    for (const auto & it : _mImportTypes)
        modules.push_back(it.module);
    std::sort(modules.begin(), modules.end());
    modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
    return modules;
}

std::vector<std::string> Application::getImportTypes(const char* Module) const
{
    std::vector<std::string> types;
    for (const auto & it : _mImportTypes) {
#ifdef __GNUC__
        if (strcasecmp(Module,it.module.c_str()) == 0)
#else
        if (_stricmp(Module,it.module.c_str()) == 0)
#endif
            types.insert(types.end(), it.types.begin(), it.types.end());
    }

    return types;
}

std::vector<std::string> Application::getImportTypes() const
{
    std::vector<std::string> types;
    for (const auto & it : _mImportTypes) {
        types.insert(types.end(), it.types.begin(), it.types.end());
    }

    std::sort(types.begin(), types.end());
    types.erase(std::unique(types.begin(), types.end()), types.end());

    return types;
}

std::map<std::string, std::string> Application::getImportFilters(const char* Type) const
{
    std::map<std::string, std::string> moduleFilter;
    for (const auto & it : _mImportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt.c_str()) == 0)
#else
            if (_stricmp(Type,jt.c_str()) == 0)
#endif
                moduleFilter[it.filter] = it.module;
        }
    }

    return moduleFilter;
}

std::map<std::string, std::string> Application::getImportFilters() const
{
    std::map<std::string, std::string> filter;
    for (const auto & it : _mImportTypes) {
        filter[it.filter] = it.module;
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
        const std::string::size_type next = item.filter.find_first_of(" )", pos + 1);
        const std::string::size_type len = next-pos-2;
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
    for (const auto & it : _mExportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt.c_str()) == 0)
#else
            if (_stricmp(Type,jt.c_str()) == 0)
#endif
                modules.push_back(it.module);
        }
    }

    return modules;
}

std::vector<std::string> Application::getExportModules() const
{
    std::vector<std::string> modules;
    for (const auto & it : _mExportTypes)
        modules.push_back(it.module);
    std::sort(modules.begin(), modules.end());
    modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
    return modules;
}

std::vector<std::string> Application::getExportTypes(const char* Module) const
{
    std::vector<std::string> types;
    for (const auto & it : _mExportTypes) {
#ifdef __GNUC__
        if (strcasecmp(Module,it.module.c_str()) == 0)
#else
        if (_stricmp(Module,it.module.c_str()) == 0)
#endif
            types.insert(types.end(), it.types.begin(), it.types.end());
    }

    return types;
}

std::vector<std::string> Application::getExportTypes() const
{
    std::vector<std::string> types;
    for (const FileTypeItem& it : _mExportTypes) {
        types.insert(types.end(), it.types.begin(), it.types.end());
    }

    std::sort(types.begin(), types.end());
    types.erase(std::unique(types.begin(), types.end()), types.end());

    return types;
}

std::map<std::string, std::string> Application::getExportFilters(const char* Type) const
{
    std::map<std::string, std::string> moduleFilter;
    for (const auto & it : _mExportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
#ifdef __GNUC__
            if (strcasecmp(Type,jt.c_str()) == 0)
#else
            if (_stricmp(Type,jt.c_str()) == 0)
#endif
                moduleFilter[it.filter] = it.module;
        }
    }

    return moduleFilter;
}

std::map<std::string, std::string> Application::getExportFilters() const
{
    std::map<std::string, std::string> filter;
    for (const FileTypeItem& it : _mExportTypes) {
        filter[it.filter] = it.module;
    }

    return filter;
}

//**************************************************************************
// signaling
void Application::slotBeforeChangeDocument(const Document& doc, const Property& prop)
{
    this->signalBeforeChangeDocument(doc, prop);
}

void Application::slotChangedDocument(const Document& doc, const Property& prop)
{
    this->signalChangedDocument(doc, prop);
}

void Application::slotNewObject(const DocumentObject&O)
{
    this->signalNewObject(O);
    _objCount = -1;
}

void Application::slotDeletedObject(const DocumentObject&O)
{
    this->signalDeletedObject(O);
    _objCount = -1;
}

void Application::slotBeforeChangeObject(const DocumentObject& O, const Property& Prop)
{
    this->signalBeforeChangeObject(O, Prop);
}

void Application::slotChangedObject(const DocumentObject&O, const Property& P)
{
    this->signalChangedObject(O,P);
}

void Application::slotRelabelObject(const DocumentObject&O)
{
    this->signalRelabelObject(O);
}

void Application::slotActivatedObject(const DocumentObject&O)
{
    this->signalActivatedObject(O);
}

void Application::slotUndoDocument(const Document& d)
{
    this->signalUndoDocument(d);
}

void Application::slotRedoDocument(const Document& d)
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

void Application::slotStartSaveDocument(const Document& doc, const std::string& filename)
{
    this->signalStartSaveDocument(doc, filename);
}

void Application::slotFinishSaveDocument(const Document& doc, const std::string& filename)
{
    DocFileMap.clear();
    this->signalFinishSaveDocument(doc, filename);
}

void Application::slotChangePropertyEditor(const Document &doc, const Property &prop)
{
    this->signalChangePropertyEditor(doc,prop);
}

//**************************************************************************
// Init, Destruct and singleton

Application * Application::_pcSingleton = nullptr;

int Application::_argc;
char ** Application::_argv;


void Application::cleanupUnits()
{
    try {
        PyGILStateLocker lock;
        Py::Module mod (Py::Module("FreeCAD").getAttr("Units").ptr());

        Py::List attr(mod.dir());
        for (Py::List::iterator it = attr.begin(); it != attr.end(); ++it) {
            mod.delAttr(Py::String(*it));
        }
    }
    catch (Py::Exception& e) {
        PyGILStateLocker lock;
        e.clear();
    }
}

void Application::destruct()
{
    // saving system parameter
    if (_pcSysParamMngr->IgnoreSave()) {
        Console().Warning("Discard system parameter\n");
    }
    else {
        Console().Log("Saving system parameter...\n");
        _pcSysParamMngr->SaveDocument();
        Console().Log("Saving system parameter...done\n");
    }
    // saving the User parameter
    if (_pcUserParamMngr->IgnoreSave()) {
        Console().Warning("Discard user parameter\n");
    }
    else {
        Console().Log("Saving user parameter...\n");
        _pcUserParamMngr->SaveDocument();
        Console().Log("Saving user parameter...done\n");
    }

    // now save all other parameter files
    auto& paramMgr = _pcSingleton->mpcPramManager;
    for (auto it : paramMgr) {
        if ((it.second != _pcSysParamMngr) && (it.second != _pcUserParamMngr)) {
            if (it.second->HasSerializer() && !it.second->IgnoreSave()) {
                Console().Log("Saving %s...\n", it.first.c_str());
                it.second->SaveDocument();
                Console().Log("Saving %s...done\n", it.first.c_str());
            }
        }
    }

    paramMgr.clear();
    _pcSysParamMngr = nullptr;
    _pcUserParamMngr = nullptr;

#ifdef FC_DEBUG
    // Do this only in debug mode for memory leak checkers
    cleanupUnits();
#endif

    CleanupProcess::callCleanup();

    // not initialized or double destruct!
    assert(_pcSingleton);
    delete _pcSingleton;

    // We must detach from console and delete the observer to save our file
    destructObserver();

    Interpreter().finalize();

    ScriptFactorySingleton::Destruct();
    InterpreterSingleton::Destruct();
    Type::destruct();
    ParameterManager::Terminate();
    SafeMode::Destruct();
}

void Application::destructObserver()
{
    if ( _pConsoleObserverFile ) {
        Console().DetachObserver(_pConsoleObserverFile);
        delete _pConsoleObserverFile;
        _pConsoleObserverFile = nullptr;
    }
    if ( _pConsoleObserverStd ) {
        Console().DetachObserver(_pConsoleObserverStd);
        delete _pConsoleObserverStd;
        _pConsoleObserverStd = nullptr;
    }
}

/** freecadNewHandler()
 * prints an error message and throws an exception
 */
#ifdef _MSC_VER // New handler for Microsoft Visual C++ compiler
int __cdecl freecadNewHandler(size_t size )
{
    // throw an exception
    throw MemoryException();
    return 0;
}
#else // Ansi C/C++ new handler
static void freecadNewHandler ()
{
    // throw an exception
    throw MemoryException();
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

#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

// This function produces a stack backtrace with demangled function & method names.
void printBacktrace(size_t skip=0)
{
#if defined HAVE_BACKTRACE_SYMBOLS
    void *callstack[128];
    size_t nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
    size_t nFrames = backtrace(callstack, nMaxFrames);
    char **symbols = backtrace_symbols(callstack, nFrames);

    for (size_t i = skip; i < nFrames; i++) {
        char *demangled = nullptr;
        int status = -1;
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname && info.dli_fname) {
            if (info.dli_sname[0] == '_') {
                demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
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
#else //HAVE_BACKTRACE_SYMBOLS
    (void)skip;
    std::cerr << "Cannot print the stacktrace because the C runtime library doesn't provide backtrace or backtrace_symbols\n";
#endif
}
#endif

void segmentation_fault_handler(int sig)
{
#if defined(FC_OS_LINUX)
    (void)sig;
    std::cerr << "Program received signal SIGSEGV, Segmentation fault.\n";
    printBacktrace(2);
#if defined(FC_DEBUG)
    abort();
#else
    _exit(1);
#endif
#else
    switch (sig) {
        case SIGSEGV:
            std::cerr << "Illegal storage access..." << std::endl;
#if !defined(_DEBUG)
            throw AccessViolation("Illegal storage access! Please save your work under a new file name and restart the application!");
#endif
            break;
        case SIGABRT:
            std::cerr << "Abnormal program termination..." << std::endl;
#if !defined(_DEBUG)
            throw AbnormalProgramTermination("Break signal occurred");
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
    throw AbnormalProgramTermination("Unexpected error occurred! Please save your work under a new file name and restart the application!");
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
        throw AccessViolation();
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        Console().Error("SEH exception (%u): Division by zero\n", code);
        return;
    }

    std::stringstream str;
    str << "SEH exception of type: " << code;
    // general C++ SEH exception for things we don't need to handle separately....
    throw RuntimeError(str.str());
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

// clang-format off
void Application::initTypes()
{
    // Base types
    Type                      ::init();
    BaseClass                 ::init();
    Exception                 ::init();
    AbortException            ::init();
    Persistence               ::init();

    // Complex data classes
    Data::ComplexGeoData            ::init();
    Data::Segment                   ::init();

    // Properties
    // Note: the order matters
    Property                   ::init();
    PropertyContainer          ::init();
    PropertyLists              ::init();
    PropertyBool               ::init();
    PropertyBoolList           ::init();
    PropertyFloat              ::init();
    PropertyFloatList          ::init();
    PropertyFloatConstraint    ::init();
    PropertyPrecision          ::init();
    PropertyQuantity           ::init();
    PropertyQuantityConstraint ::init();
    PropertyInteger            ::init();
    PropertyIntegerConstraint  ::init();
    PropertyPercent            ::init();
    PropertyEnumeration        ::init();
    PropertyIntegerList        ::init();
    PropertyIntegerSet         ::init();
    PropertyMap                ::init();
    PropertyString             ::init();
    PropertyPersistentObject   ::init();
    PropertyUUID               ::init();
    PropertyFont               ::init();
    PropertyStringList         ::init();
    PropertyLinkBase           ::init();
    PropertyLinkListBase       ::init();
    PropertyLink               ::init();
    PropertyLinkChild          ::init();
    PropertyLinkGlobal         ::init();
    PropertyLinkHidden         ::init();
    PropertyLinkSub            ::init();
    PropertyLinkSubChild       ::init();
    PropertyLinkSubGlobal      ::init();
    PropertyLinkSubHidden      ::init();
    PropertyLinkList           ::init();
    PropertyLinkListChild      ::init();
    PropertyLinkListGlobal     ::init();
    PropertyLinkListHidden     ::init();
    PropertyLinkSubList        ::init();
    PropertyLinkSubListChild   ::init();
    PropertyLinkSubListGlobal  ::init();
    PropertyLinkSubListHidden  ::init();
    PropertyXLink              ::init();
    PropertyXLinkSub           ::init();
    PropertyXLinkSubHidden     ::init();
    PropertyXLinkSubList       ::init();
    PropertyXLinkList          ::init();
    PropertyXLinkContainer     ::init();
    PropertyMatrix             ::init();
    PropertyVector             ::init();
    PropertyVectorDistance     ::init();
    PropertyPosition           ::init();
    PropertyDirection          ::init();
    PropertyVectorList         ::init();
    PropertyPlacement          ::init();
    PropertyPlacementList      ::init();
    PropertyPlacementLink      ::init();
    PropertyRotation           ::init();
    PropertyGeometry           ::init();
    PropertyComplexGeoData     ::init();
    PropertyColor              ::init();
    PropertyColorList          ::init();
    PropertyMaterial           ::init();
    PropertyMaterialList       ::init();
    PropertyPath               ::init();
    PropertyFile               ::init();
    PropertyFileIncluded       ::init();
    PropertyPythonObject       ::init();
    PropertyExpressionContainer::init();
    PropertyExpressionEngine   ::init();
    // all know unit properties
    PropertyAcceleration               ::init();
    PropertyAmountOfSubstance          ::init();
    PropertyAngle                      ::init();
    PropertyArea                       ::init();
    PropertyCompressiveStrength        ::init();
    PropertyCurrentDensity             ::init();
    PropertyDensity                    ::init();
    PropertyDissipationRate            ::init();
    PropertyDistance                   ::init();
    PropertyDynamicViscosity           ::init();
    PropertyElectricalCapacitance      ::init();
    PropertyElectricalConductance      ::init();
    PropertyElectricalConductivity     ::init();
    PropertyElectricalInductance       ::init();
    PropertyElectricalResistance       ::init();
    PropertyElectricCharge             ::init();
    PropertySurfaceChargeDensity       ::init();
    PropertyElectricCurrent            ::init();
    PropertyElectricPotential          ::init();
    PropertyElectromagneticPotential   ::init();
    PropertyFrequency                  ::init();
    PropertyForce                      ::init();
    PropertyHeatFlux                   ::init();
    PropertyInverseArea                ::init();
    PropertyInverseLength              ::init();
    PropertyInverseVolume              ::init();
    PropertyKinematicViscosity         ::init();
    PropertyLength                     ::init();
    PropertyLuminousIntensity          ::init();
    PropertyMagneticFieldStrength      ::init();
    PropertyMagneticFlux               ::init();
    PropertyMagneticFluxDensity        ::init();
    PropertyMagnetization              ::init();
    PropertyMass                       ::init();
    PropertyMoment                     ::init();
    PropertyPressure                   ::init();
    PropertyPower                      ::init();
    PropertyShearModulus               ::init();
    PropertySpecificEnergy             ::init();
    PropertySpecificHeat               ::init();
    PropertySpeed                      ::init();
    PropertyStiffness                  ::init();
    PropertyStiffnessDensity           ::init();
    PropertyStress                     ::init();
    PropertyTemperature                ::init();
    PropertyThermalConductivity        ::init();
    PropertyThermalExpansionCoefficient::init();
    PropertyThermalTransferCoefficient ::init();
    PropertyTime                       ::init();
    PropertyUltimateTensileStrength    ::init();
    PropertyVacuumPermittivity         ::init();
    PropertyVelocity                   ::init();
    PropertyVolume                     ::init();
    PropertyVolumeFlowRate             ::init();
    PropertyVolumetricThermalExpansionCoefficient::init();
    PropertyWork                       ::init();
    PropertyYieldStrength              ::init();
    PropertyYoungsModulus              ::init();

    // Extension classes
    Extension                     ::init();
    ExtensionContainer            ::init();
    DocumentObjectExtension       ::init();
    GroupExtension                ::init();
    GroupExtensionPython          ::init();
    GeoFeatureGroupExtension      ::init();
    GeoFeatureGroupExtensionPython::init();
    OriginGroupExtension          ::init();
    OriginGroupExtensionPython    ::init();
    LinkBaseExtension             ::init();
    LinkBaseExtensionPython       ::init();
    LinkExtension                 ::init();
    LinkExtensionPython           ::init();
    SuppressibleExtension         ::init();
    SuppressibleExtensionPython   ::init();

    // Document classes
    TransactionalObject       ::init();
    DocumentObject            ::init();
    GeoFeature                ::init();

    // Test features
    FeatureTest               ::init();
    FeatureTestException      ::init();
    FeatureTestColumn         ::init();
    FeatureTestRow            ::init();
    FeatureTestAbsAddress     ::init();
    FeatureTestPlacement      ::init();
    FeatureTestAttribute      ::init();

    // Feature class
    FeaturePython             ::init();
    GeometryPython            ::init();
    Document                  ::init();
    DocumentObjectGroup       ::init();
    DocumentObjectGroupPython ::init();
    DocumentObjectFileIncluded::init();
    Image::ImagePlane              ::init();
    InventorObject            ::init();
    VRMLObject                ::init();
    Annotation                ::init();
    AnnotationLabel           ::init();
    MaterialObject            ::init();
    MaterialObjectPython      ::init();
    TextDocument              ::init();
    Placement                 ::init();
    PlacementPython           ::init();
    DatumElement              ::init();
    Plane                     ::init();
    Line                      ::init();
    Point                     ::init();
    LocalCoordinateSystem     ::init();
    Part                      ::init();
    Origin                    ::init();
    Link                      ::init();
    LinkPython                ::init();
    LinkElement               ::init();
    LinkElementPython         ::init();
    LinkGroup                 ::init();
    LinkGroupPython           ::init();
    VarSet                    ::init();

    // Expression classes
    Expression                ::init();
    UnitExpression            ::init();
    NumberExpression          ::init();
    ConstantExpression        ::init();
    OperatorExpression        ::init();
    VariableExpression        ::init();
    ConditionalExpression     ::init();
    StringExpression          ::init();
    FunctionExpression        ::init();
    RangeExpression           ::init();
    PyObjectExpression        ::init();

    // Topological naming classes
    StringHasher              ::init();
    StringID                  ::init();

    // register transaction type
    new TransactionProducer<TransactionDocumentObject>
            (DocumentObject::getClassTypeId());

    // register exception producer types
    new ExceptionProducer<AbortException>;
    new ExceptionProducer<XMLBaseException>;
    new ExceptionProducer<XMLParseException>;
    new ExceptionProducer<XMLAttributeError>;
    new ExceptionProducer<FileException>;
    new ExceptionProducer<FileSystemError>;
    new ExceptionProducer<BadFormatError>;
    new ExceptionProducer<MemoryException>;
    new ExceptionProducer<AccessViolation>;
    new ExceptionProducer<AbnormalProgramTermination>;
    new ExceptionProducer<UnknownProgramOption>;
    new ExceptionProducer<ProgramInformation>;
    new ExceptionProducer<TypeError>;
    new ExceptionProducer<ValueError>;
    new ExceptionProducer<IndexError>;
    new ExceptionProducer<NameError>;
    new ExceptionProducer<ImportError>;
    new ExceptionProducer<AttributeError>;
    new ExceptionProducer<RuntimeError>;
    new ExceptionProducer<BadGraphError>;
    new ExceptionProducer<NotImplementedError>;
    new ExceptionProducer<ZeroDivisionError>;
    new ExceptionProducer<ReferenceError>;
    new ExceptionProducer<ExpressionError>;
    new ExceptionProducer<ParserError>;
    new ExceptionProducer<UnicodeError>;
    new ExceptionProducer<OverflowError>;
    new ExceptionProducer<UnderflowError>;
    new ExceptionProducer<UnitsMismatchError>;
    new ExceptionProducer<CADKernelError>;
    new ExceptionProducer<RestoreError>;
    new ExceptionProducer<PropertyError>;

    Base::registerServiceImplementation<CenterOfMassProvider>(new NullCenterOfMass);
}

namespace {

void parseProgramOptions(int ac, char ** av, const string& exe, variables_map& vm)
{
    // Declare a group of options that will be
    // allowed only on the command line
    options_description generic("Generic options");
    generic.add_options()
    ("version,v", "Prints version string")
    ("verbose", "Prints verbose version string")
    ("help,h", "Prints help message")
    ("console,c", "Starts in console mode")
    ("response-file", value<string>(),"Can be specified with '@name', too")
    ("dump-config", "Dumps configuration")
    ("get-config", value<string>(), "Prints the value of the requested configuration key")
    ("set-config", value< vector<string> >()->multitoken(), "Sets the value of a configuration key")
    ("keep-deprecated-paths", "If set then config files are kept on the old location")
    ;

    // Declare a group of options that will be
    // allowed both on the command line and in
    // the config file
    std::stringstream descr;
    descr << "Writes " << exe << ".log to the user directory.";
    options_description config("Configuration");
    config.add_options()
    ("write-log,l", descr.str().c_str())
    ("log-file", value<string>(), "Unlike --write-log this allows logging to an arbitrary file")
    ("user-cfg,u", value<string>(),"User config file to load/save user settings")
    ("system-cfg,s", value<string>(),"System config file to load/save system settings")
    ("run-test,t", value<string>()->implicit_value(""),"Run a given test case (use 0 (zero) to run all tests). If no argument is provided then return list of all available tests.")
    ("run-open,r", value<string>()->implicit_value(""),"Run a given test case (use 0 (zero) to run all tests). If no argument is provided then return list of all available tests.  Keeps UI open after test(s) complete.")
    ("module-path,M", value< vector<string> >()->composing(),"Additional module paths")
    ("macro-path,E", value< vector<string> >()->composing(),"Additional macro paths")
    ("python-path,P", value< vector<string> >()->composing(),"Additional python paths")
    ("disable-addon", value< vector<string> >()->composing(),"Disable a given addon.")
    ("single-instance", "Allow to run a single instance of the application")
    ("safe-mode", "Force enable safe mode")
    ("pass", value< vector<string> >()->multitoken(), "Ignores the following arguments and pass them through to be used by a script")
    ;


    // Hidden options, will be allowed both on the command line and
    // in the config file, but will not be shown to the user.
    options_description hidden("Hidden options");
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
            args.emplace_back(av[i]);
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

    options_description config_file_options("Config");
    config_file_options.add(config).add(hidden);

    options_description visible("Allowed options");
    visible.add(generic).add(config);

    positional_options_description p;
    p.add("input-file", -1);

    try {
        store( command_line_parser(args).
               options(cmdline_options).positional(p).extra_parser(Util::customSyntax).run(), vm);

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
        str << exe << endl << endl;
        str << "For a detailed description see https://www.freecad.org/wiki/Start_up_and_Configuration" << endl<<endl;
        str << "Usage: " << exe << " [options] File1 File2 ..." << endl << endl;
        str << visible << endl;
        throw ProgramInformation(str.str());
    }

    if (vm.count("response-file")) {
        // Load the file and tokenize it
        std::ifstream ifs(vm["response-file"].as<string>().c_str());
        if (!ifs) {
            Console().Error("Could no open the response file\n");
            std::stringstream str;
            str << "Could no open the response file: '"
                << vm["response-file"].as<string>() << "'" << endl;
            throw UnknownProgramOption(str.str());
        }
        // Read the whole file into a string
        stringstream ss;
        ss << ifs.rdbuf();
        // Split the file content
        char_separator<char> sep(" \n\r");
        tokenizer<char_separator<char> > tok(ss.str(), sep);
        vector<string> args2;
        copy(tok.begin(), tok.end(), back_inserter(args2));
        // Parse the file and store the options
        store( command_line_parser(args2).
               options(cmdline_options).positional(p).extra_parser(Util::customSyntax).run(), vm);
    }
}

void processProgramOptions(const variables_map& vm, std::map<std::string,std::string>& mConfig)
{
    if (vm.count("version")) {
        std::stringstream str;
        str << mConfig["ExeName"] << " " << mConfig["ExeVersion"]
            << " Revision: " << mConfig["BuildRevision"] << std::endl;
        if (vm.count("verbose")) {
            str << "\nLibrary versions:\n";
            str << "boost    " << BOOST_LIB_VERSION << '\n';
            str << "Coin3D   " << fcCoin3dVersion << '\n';
            str << "Eigen3   " << fcEigen3Version << '\n';
#ifdef OCC_VERSION_STRING_EXT
            str << "OCC      " << OCC_VERSION_STRING_EXT << '\n';
#endif
            str << "Qt       " << QT_VERSION_STR << '\n';
            str << "Python   " << PY_VERSION << '\n';
            str << "PySide   " << fcPysideVersion << '\n';
            str << "shiboken " << fcShibokenVersion << '\n';
#ifdef SMESH_VERSION_STR
            str << "SMESH    " << SMESH_VERSION_STR << '\n';
#endif
            str << "VTK      " << fcVtkVersion << '\n';
            str << "xerces-c " << fcXercescVersion << '\n';
        }
        throw ProgramInformation(str.str());
    }

    if (vm.count("module-path")) {
        auto  Mods = vm["module-path"].as< vector<string> >();
        string temp;
        for (const auto & It : Mods)
            temp += It + ";";
        temp.erase(temp.end()-1);
        mConfig["AdditionalModulePaths"] = temp;
    }

    if (vm.count("macro-path")) {
        vector<string> Macros = vm["macro-path"].as< vector<string> >();
        string temp;
        for (const auto & It : Macros)
            temp += It + ";";
        temp.erase(temp.end()-1);
        mConfig["AdditionalMacroPaths"] = temp;
    }

    if (vm.count("python-path")) {
        auto  Paths = vm["python-path"].as< vector<string> >();
        for (const auto & It : Paths)
            Interpreter().addPythonPath(It.c_str());
    }

    if (vm.count("disable-addon")) {
        auto Addons = vm["disable-addon"].as< vector<string> >();
        string temp;
        for (const auto & It : Addons) {
            temp += It + ";";
        }
        temp.erase(temp.end()-1);
        mConfig["DisabledAddons"] = temp;
    }

    if (vm.count("input-file")) {
        auto  files(vm["input-file"].as< vector<string> >());
        int OpenFileCount=0;
        for (const auto & It : files) {

            std::ostringstream temp;
            temp << "OpenFile" << OpenFileCount;
            mConfig[temp.str()] = It;
            OpenFileCount++;
        }
        std::ostringstream buffer;
        buffer << OpenFileCount;
        mConfig["OpenFileCount"] = buffer.str();
    }

    if (vm.count("output")) {
        auto  file = vm["output"].as<string>();
        mConfig["SaveFile"] = file;
    }

    if (vm.count("hidden")) {
        mConfig["StartHidden"] = "1";
    }

    if (vm.count("write-log")) {
        mConfig["LoggingFile"] = "1";
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

    if (vm.count("run-test") || vm.count("run-open")) {
        string testCase = vm.count("run-open") ? vm["run-open"].as<string>() : vm["run-test"].as<string>();

        if ( "0" == testCase) {
            testCase = "TestApp.All";
        }
        else if (testCase.empty()) {
            testCase = "TestApp.PrintAll";
        }
        mConfig["TestCase"] = testCase;
        mConfig["RunMode"] = "Internal";
        mConfig["ScriptFileName"] = "FreeCADTest";
        mConfig["ExitTests"] = vm.count("run-open") == 0  ? "yes" : "no";
    }

    if (vm.count("single-instance")) {
        mConfig["SingleInstance"] = "1";
    }

    if (vm.count("dump-config")) {
        std::stringstream str;
        for (const auto & it : mConfig) {
            str << it.first << "=" << it.second << std::endl;
        }
        throw ProgramInformation(str.str());
    }

    if (vm.count("get-config")) {
        auto  configKey = vm["get-config"].as<string>();
        std::stringstream str;
        std::map<std::string,std::string>::iterator pos;
        pos = mConfig.find(configKey);
        if (pos != mConfig.end()) {
            str << pos->second;
        }
        str << std::endl;
        throw ProgramInformation(str.str());
    }

    if (vm.count("set-config")) {
        auto  configKeyValue = vm["set-config"].as< std::vector<std::string> >();
        for (const auto& it : configKeyValue) {
            auto pos = it.find('=');
            if (pos != std::string::npos) {
                std::string key = it.substr(0, pos);
                std::string val = it.substr(pos + 1);
                mConfig[key] = val;
            }
        }
    }
}
}
// clang-format on

void Application::initConfig(int argc, char ** argv)
{
    // find the home path....
    mConfig["AppHomePath"] = FindHomePath(argv[0]);

    // Version of the application extracted from SubWCRef into src/Build/Version.h
    // We only set these keys if not yet defined. Therefore it suffices to search
    // only for 'BuildVersionMajor'.
    if (Config().find("BuildVersionMajor") == Config().end()) {
        std::stringstream str;
        str << FCVersionMajor
            << "." << FCVersionMinor
            << "." << FCVersionPoint;
        Config()["ExeVersion"         ] = str.str();
        Config()["BuildVersionMajor"  ] = FCVersionMajor;
        Config()["BuildVersionMinor"  ] = FCVersionMinor;
        Config()["BuildVersionPoint"  ] = FCVersionPoint;
        Config()["BuildVersionSuffix" ] = FCVersionSuffix;
        Config()["BuildRevision"      ] = FCRevision;
        Config()["BuildRepositoryURL" ] = FCRepositoryURL;
        Config()["BuildRevisionDate"  ] = FCRevisionDate;
#if defined(FCRepositoryHash)
        Config()["BuildRevisionHash"  ] = FCRepositoryHash;
#endif
#if defined(FCRepositoryBranch)
        Config()["BuildRevisionBranch"] = FCRepositoryBranch;
#endif
    }

    _argc = argc;
    _argv = argv;

    // Now it's time to read-in the file branding.xml if it exists
    Branding brand;
    QString binDir = QString::fromUtf8((mConfig["AppHomePath"] + "bin").c_str());
    QFileInfo fi(binDir, QStringLiteral("branding.xml"));
    if (fi.exists() && brand.readFile(fi.absoluteFilePath())) {
        Branding::XmlConfig cfg = brand.getUserDefines();
        for (Branding::XmlConfig::iterator it = cfg.begin(); it != cfg.end(); ++it) {
            Config()[it.key()] = it.value();
        }
    }

    variables_map vm;
    {
        BOOST_SCOPE_EXIT_ALL(&) {
            // console-mode needs to be set (if possible) also in case parseProgramOptions
            // throws, as it's needed when reporting such exceptions
            if (vm.count("console")) {
                mConfig["Console"] = "1";
                mConfig["RunMode"] = "Cmd";
            }
        };
        parseProgramOptions(argc, argv, mConfig["ExeName"], vm);
    }

    if (vm.count("keep-deprecated-paths")) {
        mConfig["KeepDeprecatedPaths"] = "1";
    }

    // extract home paths
    ExtractUserPath();

    if (vm.count("safe-mode")) {
        SafeMode::StartSafeMode();
    }

#   ifdef FC_DEBUG
    mConfig["Debug"] = "1";
#   else
    mConfig["Debug"] = "0";
#   endif

    if (!Py_IsInitialized()) {
        // init python
        PyImport_AppendInittab ("FreeCAD", init_freecad_module);
        PyImport_AppendInittab ("__FreeCADBase__", init_freecad_base_module);
    }
    else {
        // "import FreeCAD" in a normal Python 3.12 interpreter would raise
        //     Fatal Python error: PyImport_AppendInittab:
        //         PyImport_AppendInittab() may not be called after Py_Initialize()
        //  because the (external) interpreter is already initialized.
        //  Therefore we use a workaround as described in https://stackoverflow.com/a/57019607

        PyObject* sysModules = PyImport_GetModuleDict();

        auto moduleName = "FreeCAD";
        PyImport_AddModule(moduleName);
        ApplicationMethods = Methods;
        PyObject *pyModule = init_freecad_module();
        PyDict_SetItemString(sysModules, moduleName, pyModule);
        Py_DECREF(pyModule);

        moduleName = "__FreeCADBase__";
        PyImport_AddModule(moduleName);
        pyModule = init_freecad_base_module();
        PyDict_SetItemString(sysModules, moduleName, pyModule);
        Py_DECREF(pyModule);
    }

    const char* pythonpath = Interpreter().init(argc,argv);
    if (pythonpath)
        mConfig["PythonSearchPath"] = pythonpath;
    else
        Console().Warning("Encoding of Python paths failed\n");

    // Handle the options that have impact on the init process
    processProgramOptions(vm, mConfig);

    // Init console ===========================================================
    PyGILStateLocker lock;
    _pConsoleObserverStd = new ConsoleObserverStd();
    Console().AttachObserver(_pConsoleObserverStd);
    if (mConfig["LoggingConsole"] != "1") {
        _pConsoleObserverStd->bMsg = false;
        _pConsoleObserverStd->bLog = false;
        _pConsoleObserverStd->bWrn = false;
        _pConsoleObserverStd->bErr = false;
    }
    if (mConfig["Verbose"] == "Strict")
        Console().UnsetConsoleMode(ConsoleSingleton::Verbose);

    // file logging Init ===========================================================
    if (mConfig["LoggingFile"] == "1") {
        _pConsoleObserverFile = new ConsoleObserverFile(mConfig["LoggingFileName"].c_str());
        Console().AttachObserver(_pConsoleObserverFile);
    }
    else
        _pConsoleObserverFile = nullptr;

    // Banner ===========================================================
    if (mConfig["RunMode"] != "Cmd") {
        // Remove banner if FreeCAD is invoked via the -c command as regular
        // Python interpreter
        if (mConfig["Verbose"] != "Strict")
            Console().Message("%s %s, Libs: %s.%s.%s%sR%s\n%s",
                              mConfig["ExeName"].c_str(),
                              mConfig["ExeVersion"].c_str(),
                              mConfig["BuildVersionMajor"].c_str(),
                              mConfig["BuildVersionMinor"].c_str(),
                              mConfig["BuildVersionPoint"].c_str(),
                              mConfig["BuildVersionSuffix"].c_str(),
                              mConfig["BuildRevision"].c_str(),
                              mConfig["CopyrightInfo"].c_str());
        else
            Console().Message("%s %s, Libs: %s.%s.%s%sR%s\n",
                              mConfig["ExeName"].c_str(),
                              mConfig["ExeVersion"].c_str(),
                              mConfig["BuildVersionMajor"].c_str(),
                              mConfig["BuildVersionMinor"].c_str(),
                              mConfig["BuildVersionPoint"].c_str(),
                              mConfig["BuildVersionSuffix"].c_str(),
                              mConfig["BuildRevision"].c_str());

        if (SafeMode::SafeModeEnabled()) {
            Console().Message("FreeCAD is running in _SAFE_MODE_.\n"
                                    "Safe mode temporarily disables your configurations and "
                                    "addons. Restart the application to exit safe mode.\n\n");
        }
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
                Console().SetDefaultLogLevel(v.second);
            }
#endif
        }
        else if (v.first == "DebugDefault") {
#ifdef FC_DEBUG
            if (v.second>=0) {
                hasDefault = true;
                Console().SetDefaultLogLevel(v.second);
            }
#endif
        }
        else {
            *Console().GetLogLevel(v.first.c_str()) = v.second;
        }
    }

    if (!hasDefault) {
#ifdef FC_DEBUG
        loglevelParam->SetInt("DebugDefault", Console().LogLevel(-1));
#else
        loglevelParam->SetInt("Default", Console().LogLevel(-1));
#endif
    }

    // Change application tmp. directory
    std::string tmpPath = _pcUserParamMngr->GetGroup("BaseApp/Preferences/General")->GetASCII("TempPath");
    FileInfo di(tmpPath);
    if (di.exists() && di.isDir()) {
        mConfig["AppTempPath"] = tmpPath + PATHSEP;
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

    // Save version numbers of the libraries
#ifdef OCC_VERSION_STRING_EXT
    mConfig["OCC_VERSION"] = OCC_VERSION_STRING_EXT;
#endif
    mConfig["BOOST_VERSION"] = BOOST_LIB_VERSION;
    mConfig["PYTHON_VERSION"] = PY_VERSION;
    mConfig["QT_VERSION"] = QT_VERSION_STR;
    mConfig["EIGEN_VERSION"] = fcEigen3Version;
    mConfig["PYSIDE_VERSION"] = fcPysideVersion;
#ifdef SMESH_VERSION_STR
    mConfig["SMESH_VERSION"] = SMESH_VERSION_STR;
#endif
    mConfig["XERCESC_VERSION"] = fcXercescVersion;


    logStatus();
}

void Application::SaveEnv(const char* s)
{
    const char *c = getenv(s);
    if (c)
        mConfig[s] = c;
}

void Application::initApplication()
{
    // interpreter and Init script ==========================================================
    // register scripts
    new ScriptProducer( "CMakeVariables", CMakeVariables );
    new ScriptProducer( "FreeCADInit",    FreeCADInit    );
    new ScriptProducer( "FreeCADTest",    FreeCADTest    );

    // creating the application
    if (mConfig["Verbose"] != "Strict")
        Console().Log("Create Application\n");
    _pcSingleton = new Application(mConfig);

    // set up Unit system default
    const ParameterGrp::handle hGrp = GetApplication().GetParameterGroupByPath
       ("User parameter:BaseApp/Preferences/Units");
    UnitsApi::setSchema(static_cast<UnitSystem>(hGrp->GetInt("UserSchema", 0)));
    UnitsApi::setDecimals(hGrp->GetInt("Decimals", UnitsApi::getDecimals()));

    // In case we are using fractional inches, get user setting for min unit
    const int denom = hGrp->GetInt("FracInch", QuantityFormat::getDefaultDenominator());
    QuantityFormat::setDefaultDenominator(denom);


#if defined (_DEBUG)
    Console().Log("Application is built with debug information\n");
#endif

    // starting the init script
    Console().Log("Run App init script\n");
    try {
        Interpreter().runString(ScriptFactory().ProduceScript("CMakeVariables"));
        Interpreter().runString(ScriptFactory().ProduceScript("FreeCADInit"));
    }
    catch (const Exception& e) {
        e.ReportException();
    }

    // seed randomizer
    srand(time(nullptr));
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
    Console().Log("Init: Processing command line files\n");
    for (const auto & it : files) {
        FileInfo file(it);

        Console().Log("Init:     Processing file: %s\n",file.filePath().c_str());

        try {
            if (file.hasExtension("fcstd") || file.hasExtension("std")) {
                // try to open
                _pcSingleton->openDocument(file.filePath().c_str());
                processed.push_back(it);
            }
            else if (file.hasExtension("fcscript") || file.hasExtension("fcmacro")) {
                Interpreter().runFile(file.filePath().c_str(), true);
                processed.push_back(it);
            }
            else if (file.hasExtension("py")) {
                try {
                    Interpreter().addPythonPath(file.dirPath().c_str());
                    Interpreter().loadModule(file.fileNamePure().c_str());
                    processed.push_back(it);
                }
                catch (const PyException&) {
                    // if loading the module does not work, try just running the script (run in __main__)
                    Interpreter().runFile(file.filePath().c_str(),true);
                    processed.push_back(it);
                }
            }
            else {
                std::string ext = file.extension();
                std::vector<std::string> mods = GetApplication().getImportModules(ext.c_str());
                if (!mods.empty()) {
                    std::string escapedstr = Tools::escapedUnicodeFromUtf8(file.filePath().c_str());
                    escapedstr = Tools::escapeEncodeFilename(escapedstr);

                    Interpreter().loadModule(mods.front().c_str());
                    Interpreter().runStringArg("import %s",mods.front().c_str());
                    Interpreter().runStringArg("%s.open(u\"%s\")",mods.front().c_str(),
                            escapedstr.c_str());
                    processed.push_back(it);
                    Console().Log("Command line open: %s.open(u\"%s\")\n",mods.front().c_str(),escapedstr.c_str());
                }
                else if (file.exists()) {
                    Console().Warning("File format not supported: %s \n", file.filePath().c_str());
                }
            }
        }
        catch (const SystemExitException&) {
            throw; // re-throw to main() function
        }
        catch (const Exception& e) {
            Console().Error("Exception while processing file: %s [%s]\n", file.filePath().c_str(), e.what());
        }
        catch (...) {
            Console().Error("Unknown exception while processing file: %s \n", file.filePath().c_str());
        }
    }

    return processed; // successfully processed files
}

void Application::processCmdLineFiles()
{
    // process files passed to command line
    const std::list<std::string> files = getCmdLineFiles();
    const std::list<std::string> processed = processFiles(files);

    if (files.empty()) {
        if (mConfig["RunMode"] == "Exit")
            mConfig["RunMode"] = "Cmd";
    }
    else if (processed.empty() && files.size() == 1 && mConfig["RunMode"] == "Cmd") {
        // In case we are in console mode and the argument is not a file but Python code
        // then execute it. This is to behave like the standard Python executable.
        const FileInfo file(files.front());
        if (!file.exists()) {
            Interpreter().runString(files.front().c_str());
            mConfig["RunMode"] = "Exit";
        }
    }

    const std::map<std::string, std::string>& cfg = Config();
    const auto it = cfg.find("SaveFile");
    if (it != cfg.end()) {
        std::string output = it->second;
        output = Tools::escapeEncodeFilename(output);

        const FileInfo fi(output);
        const std::string ext = fi.extension();
        try {
            const std::vector<std::string> mods = GetApplication().getExportModules(ext.c_str());
            if (!mods.empty()) {
                Interpreter().loadModule(mods.front().c_str());
                Interpreter().runStringArg("import %s",mods.front().c_str());
                Interpreter().runStringArg("%s.export(App.ActiveDocument.Objects, '%s')"
                    ,mods.front().c_str(),output.c_str());
            }
            else {
                Console().Warning("File format not supported: %s \n", output.c_str());
            }
        }
        catch (const Exception& e) {
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
        Interpreter().runString(ScriptFactory().ProduceScript(mConfig["ScriptFileName"].c_str()));
    }
    else if (mConfig["RunMode"] == "Exit") {
        // getting out
        Console().Log("Exiting on purpose\n");
    }
    else {
        Console().Log("Unknown Run mode (%d) in main()?!?\n\n", mConfig["RunMode"].c_str());
    }
}

void Application::logStatus()
{
    const std::string time_str = to_simple_string(
        posix_time::second_clock::local_time());
    Console().Log("Time = %s\n", time_str.c_str());

    for (const auto & It : mConfig) {
        Console().Log("%s = %s\n", It.first.c_str(), It.second.c_str());
    }
}

void Application::LoadParameters()
{
    // Init parameter sets ===========================================================
    //
    if (mConfig.find("UserParameter") == mConfig.end())
        mConfig["UserParameter"]   = mConfig["UserConfigPath"] + "user.cfg";
    if (mConfig.find("SystemParameter") == mConfig.end())
        mConfig["SystemParameter"] = mConfig["UserConfigPath"] + "system.cfg";

    // create standard parameter sets
    _pcSysParamMngr = ParameterManager::Create();
    _pcSysParamMngr->SetSerializer(new ParameterSerializer(mConfig["SystemParameter"]));

    _pcUserParamMngr = ParameterManager::Create();
    _pcUserParamMngr->SetSerializer(new ParameterSerializer(mConfig["UserParameter"]));

    try {
        if (_pcSysParamMngr->LoadOrCreateDocument() && mConfig["Verbose"] != "Strict") {
            // Configuration file optional when using as Python module
            if (!Py_IsInitialized()) {
                Console().Warning("   Parameter does not exist, writing initial one\n");
                Console().Message("   This warning normally means that FreeCAD is running for the first time\n"
                                        "   or the configuration was deleted or moved. FreeCAD is generating the standard\n"
                                        "   configuration.\n");
            }
        }
    }
    catch (const Exception& e) {
        // try to proceed with an empty XML document
        Console().Error("%s in file %s.\n"
                              "Continue with an empty configuration.\n",
                              e.what(), mConfig["SystemParameter"].c_str());
        _pcSysParamMngr->CreateDocument();
    }

    try {
        if (_pcUserParamMngr->LoadOrCreateDocument() && mConfig["Verbose"] != "Strict") {
            // The user parameter file doesn't exist. When an alternative parameter file is offered
            // this will be used.
            const auto it = mConfig.find("UserParameterTemplate");
            if (it != mConfig.end()) {
                QString path = QString::fromUtf8(it->second.c_str());
                if (QDir(path).isRelative()) {
                    const QString home = QString::fromUtf8(mConfig["AppHomePath"].c_str());
                    path = QFileInfo(QDir(home), path).absoluteFilePath();
                }
                const QFileInfo fi(path);
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
    catch (const Exception& e) {
        // try to proceed with an empty XML document
        Console().Error("%s in file %s.\n"
                              "Continue with an empty configuration.\n",
                              e.what(), mConfig["UserParameter"].c_str());
        _pcUserParamMngr->CreateDocument();
    }
}

#if defined(_MSC_VER) && BOOST_VERSION < 108200
    // fix weird error while linking boost (all versions of VC)
    // VS2010: https://forum.freecad.org/viewtopic.php?f=4&t=1886&p=12553&hilit=boost%3A%3Afilesystem%3A%3Aget#p12553
    namespace boost { namespace program_options { std::string arg="arg"; } }
    namespace boost { namespace program_options {
    const unsigned options_description::m_default_line_length = 80;
    } }
#endif

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, " "));
    return os;
}

namespace {

/*!
 * \brief getUserHome
 * Returns the user's home directory.
 */
QString getUserHome()
{
    QString path;
#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD) || defined(FC_OS_MACOSX)
    // Default paths for the user specific stuff
    struct passwd pwd;
    struct passwd *result;
    const std::size_t buflen = 16384;
    std::vector<char> buffer(buflen);
    const int error = getpwuid_r(getuid(), &pwd, buffer.data(), buffer.size(), &result);
    Q_UNUSED(error)
    if (!result)
        throw RuntimeError("Getting HOME path from system failed!");
    path = QString::fromUtf8(result->pw_dir);
#else
    path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#endif

    return path;
}

/*!
 * \brief getOldGenericDataLocation
 * Returns a directory location where persistent data shared across applications can be stored.
 * This method returns the old non-XDG-compliant root path where to store config files and application data.
 */
#if defined(FC_OS_WIN32)
QString getOldGenericDataLocation(QString home)
{
#if defined(FC_OS_WIN32)
    WCHAR szPath[MAX_PATH];
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        return QString::fromStdString(converter.to_bytes(szPath));
    }
#elif defined(FC_OS_MACOSX)
    QFileInfo fi(home, QStringLiteral("Library/Preferences"));
    home = fi.absoluteFilePath();
#endif

    return home;
}
#endif

/*!
 * \brief getSubDirectories
 * To a given path it adds the sub-directories where to store application specific files.
 */
void getSubDirectories(std::map<std::string,std::string>& mConfig, std::vector<std::string>& appData)
{
    // If 'AppDataSkipVendor' is defined, the value of 'ExeVendor' must not be part of
    // the path.
    if (mConfig.find("AppDataSkipVendor") == mConfig.end()) {
        appData.push_back(mConfig["ExeVendor"]);
    }
    appData.push_back(mConfig["ExeName"]);
}

/*!
 * \brief getOldDataLocation
 * To a given path it adds the sub-directories where to store application specific files.
 * On Linux or BSD a hidden directory (i.e. starting with a dot) is added.
 */
void getOldDataLocation(std::map<std::string,std::string>& mConfig, std::vector<std::string>& appData)
{
    // Actually the name of the directory where the parameters are stored should be the name of
    // the application due to branding reasons.
#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD)
    // If 'AppDataSkipVendor' is defined, the value of 'ExeVendor' must not be part of
    // the path.
    if (mConfig.find("AppDataSkipVendor") == mConfig.end()) {
        appData.push_back(std::string(".") + mConfig["ExeVendor"]);
        appData.push_back(mConfig["ExeName"]);
    } else {
        appData.push_back(std::string(".") + mConfig["ExeName"]);
    }

#elif defined(FC_OS_MACOSX) || defined(FC_OS_WIN32)
    getSubDirectories(mConfig, appData);
#endif
}

/*!
 * \brief findUserHomePath
 * If the passed path name is not empty it will be returned, otherwise
 * the user home path of the system will be returned.
 */
QString findUserHomePath(const QString& userHome)
{
    if (userHome.isEmpty()) {
        return getUserHome();
    }
    else {
        return userHome;
    }
}

/*!
 * \brief findPath
 * Returns the path where to store application files to.
 * If \a customHome is not empty it will be used, otherwise a path starting from \a stdHome will be used.
 */
std::filesystem::path findPath(const QString& stdHome, const QString& customHome,
                                 const std::vector<std::string>& paths, bool create)
{
    QString dataPath = customHome;
    if (dataPath.isEmpty()) {
        dataPath = stdHome;
    }

    std::filesystem::path appData(FileInfo::stringToPath(dataPath.toStdString()));

    // If a custom user home path is given then don't modify it
    if (customHome.isEmpty()) {
        for (const auto& it : paths)
            appData = appData / it;
    }

    // In order to write to our data path, we must create some directories, first.
    if (create && !std::filesystem::exists(appData) && !Py_IsInitialized()) {
        try {
            std::filesystem::create_directories(appData);
        } catch (const std::filesystem::filesystem_error& e) {
            throw Base::FileSystemError("Could not create directories. Failed with: " + e.code().message());
        }
    }

    return appData;
}

/*!
 * \brief getCustomPaths
 * Returns a tuple of path names where to store config, data and temp. files.
 * The method therefore reads the environment variables:
 * \list
 * \li FREECAD_USER_HOME
 * \li FREECAD_USER_DATA
 * \li FREECAD_USER_TEMP
 * \endlist
 */
std::tuple<QString, QString, QString> getCustomPaths()
{
    const QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
    QString userHome = env.value(QStringLiteral("FREECAD_USER_HOME"));
    QString userData = env.value(QStringLiteral("FREECAD_USER_DATA"));
    QString userTemp = env.value(QStringLiteral("FREECAD_USER_TEMP"));

    auto toNativePath = [](QString& path) {
        if (!path.isEmpty()) {
            const QDir dir(path);
            if (dir.exists())
                path = QDir::toNativeSeparators(dir.canonicalPath());
            else
                path.clear();
        }
    };

    // verify env. variables
    toNativePath(userHome);
    toNativePath(userData);
    toNativePath(userTemp);

    // if FREECAD_USER_HOME is set but not FREECAD_USER_DATA
    if (!userHome.isEmpty() && userData.isEmpty()) {
        userData = userHome;
    }

    // if FREECAD_USER_HOME is set but not FREECAD_USER_TEMP
    if (!userHome.isEmpty() && userTemp.isEmpty()) {
        const QDir dir(userHome);
        dir.mkdir(QStringLiteral("temp"));
        const QFileInfo fi(dir, QStringLiteral("temp"));
        userTemp = fi.absoluteFilePath();
    }

    return {userHome, userData, userTemp};
}

/*!
 * \brief getStandardPaths
 * Returns a tuple of XDG-compliant standard paths names where to store config, data and cached files.
 * The method therefore reads the environment variables:
 * \list
 * \li XDG_CONFIG_HOME
 * \li XDG_DATA_HOME
 * \li XDG_CACHE_HOME
 * \endlist
 */
std::tuple<QString, QString, QString, QString> getStandardPaths()
{
    QString configHome = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString cacheHome = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    // Keep the old behaviour
#if defined(FC_OS_WIN32)
    configHome = getOldGenericDataLocation(QString());
    dataHome = configHome;

    // On systems with non-7-bit-ASCII application data directories
    // GetTempPathW will return a path in DOS format. This path will be
    // accepted by boost's file_lock class.
    // Since boost 1.76 there is now a version that accepts a wide string.
#if (BOOST_VERSION < 107600)
    tempPath = QString::fromStdString(FileInfo::getTempPath());
    cacheHome = tempPath;
#endif
#endif

    return std::make_tuple(configHome, dataHome, cacheHome, tempPath);
}
}

void Application::ExtractUserPath()
{
    bool keepDeprecatedPaths = mConfig.count("KeepDeprecatedPaths") > 0;

    // std paths
    mConfig["BinPath"] = mConfig["AppHomePath"] + "bin" + PATHSEP;
    mConfig["DocPath"] = mConfig["AppHomePath"] + "doc" + PATHSEP;

    // this is to support a portable version of FreeCAD
    auto paths = getCustomPaths();
    QString customHome = std::get<0>(paths);
    QString customData = std::get<1>(paths);
    QString customTemp = std::get<2>(paths);

    // get the system standard paths
    auto stdPaths = getStandardPaths();
    QString configHome = std::get<0>(stdPaths);
    QString dataHome = std::get<1>(stdPaths);
    QString cacheHome = std::get<2>(stdPaths);
    QString tempPath = std::get<3>(stdPaths);

    // User home path
    //
    QString homePath = findUserHomePath(customHome);
    mConfig["UserHomePath"] = homePath.toUtf8().data();

    // the old path name to save config and data files
    std::vector<std::string> subdirs;
    if (keepDeprecatedPaths) {
        configHome = homePath;
        dataHome = homePath;
        cacheHome = homePath;
        getOldDataLocation(mConfig, subdirs);
    }
    else {
        getSubDirectories(mConfig, subdirs);
    }

    // User data path
    //
    std::filesystem::path data = findPath(dataHome, customData, subdirs, true);
    mConfig["UserAppData"] = FileInfo::pathToString(data) + PATHSEP;


    // User config path
    //
    std::filesystem::path config = findPath(configHome, customHome, subdirs, true);
    mConfig["UserConfigPath"] = FileInfo::pathToString(config) + PATHSEP;


    // User cache path
    //
    std::vector<std::string> cachedirs = subdirs;
    cachedirs.emplace_back("Cache");
    std::filesystem::path cache = findPath(cacheHome, customTemp, cachedirs, true);
    mConfig["UserCachePath"] = FileInfo::pathToString(cache) + PATHSEP;


    // Set application tmp. directory
    //
    std::vector<std::string> empty;
    std::filesystem::path tmp = findPath(tempPath, customTemp, empty, true);
    mConfig["AppTempPath"] = FileInfo::pathToString(tmp) + PATHSEP;


    // Set the default macro directory
    //
    std::vector<std::string> macrodirs = subdirs;
    macrodirs.emplace_back("Macro");
    std::filesystem::path macro = findPath(dataHome, customData, macrodirs, true);
    mConfig["UserMacroPath"] = FileInfo::pathToString(macro) + PATHSEP;
}

// TODO: Consider using this for all UNIX-like OSes
#if defined(__OpenBSD__)
#include <cstdio>
#include <cstdlib>
#include <sys/param.h>
#include <QCoreApplication>

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
        int argc = 1;
        QCoreApplication app(argc, (char**)(&sCall));
        absPath = QCoreApplication::applicationFilePath().toStdString();
    }

    // should be an absolute path now
    std::string::size_type pos = absPath.find_last_of("/");
    homePath.assign(absPath,0,pos);
    pos = homePath.find_last_of("/");
    homePath.assign(homePath,0,pos+1);

    return homePath;
}

#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD)
#include <cstdio>
#include <cstdlib>
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
            throw FileSystemError("Cannot determine the absolute path of the executable");
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
#include <cstdlib>
#include <sys/param.h>

std::string Application::FindHomePath(const char* call)
{
    // If Python is initialized at this point, then we're being run from
    // MainPy.cpp, which hopefully rewrote argv[0] to point at the
    // FreeCAD shared library.
    if (!Py_IsInitialized()) {
        uint32_t sz = 0;

        _NSGetExecutablePath(
            nullptr, &sz);  // function only returns "sz" if first arg is to small to hold value

        if (const auto buf = new char[++sz]; _NSGetExecutablePath(buf, &sz) == 0) {
            char resolved[PATH_MAX];
            const char* path = realpath(buf, resolved);
            delete [] buf;

            if (path) {
                const std::string Call(resolved);
                std::string TempHomePath;
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
