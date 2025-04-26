// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <FCConfig.h>

# if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
#  include <unistd.h>
#  include <pwd.h>
#  include <sys/types.h>
# elif defined(__MINGW32__)
#  undef WINVER
#  define WINVER 0x502 // needed for SetDllDirectory
#  include <Windows.h>
# endif
# include <boost/algorithm/string.hpp>
# include <boost/program_options.hpp>
# include <boost/date_time/posix_time/posix_time.hpp>
# include <boost/scope_exit.hpp>
# include <chrono>
# include <random>
# include <memory>
# include <utility>
# include <set>
# include <list>
# include <algorithm>
# include <iostream>
# include <map>
# include <tuple>
# include <vector>
# include <fmt/format.h>

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
#include <QRegularExpression>
#include <QSettings>
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
#include <Base/UniqueNameManager.h>
#include <Base/Tools.h>
#include <Base/Translate.h>
#include <Base/Type.h>
#include <Base/TypePy.h>
#include <Base/UnitPy.h>
#include <Base/UnitsApi.h>
#include <Base/VectorPy.h>

#include "Annotation.h"
#include "Application.h"
#include "ApplicationDirectories.h"
#include "ApplicationDirectoriesPy.h"
#include "ApplicationPy.h"
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
namespace sp = std::placeholders;
namespace fs = std::filesystem;

//==========================================================================
// Application
//==========================================================================

Base::Reference<ParameterManager> Application::_pcSysParamMngr;
Base::Reference<ParameterManager> Application::_pcUserParamMngr;
Base::ConsoleObserverStd  *Application::_pConsoleObserverStd = nullptr;
Base::ConsoleObserverFile *Application::_pConsoleObserverFile = nullptr;

AppExport std::map<std::string, std::string> Application::mConfig;
std::unique_ptr<ApplicationDirectories> Application::_appDirs;


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
    Base::PyGILStateLocker lock;
    PyObject* modules = PyImport_GetModuleDict();

    ApplicationMethods = ApplicationPy::Methods;
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
        Base::ConsoleSingleton::Methods,
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
    Base::InterpreterSingleton::addType(&Base::VectorPy::Type, pAppModule, "Vector");
    Base::InterpreterSingleton::addType(&Base::MatrixPy::Type, pAppModule, "Matrix");
    Base::InterpreterSingleton::addType(&Base::BoundBoxPy::Type, pAppModule, "BoundBox");
    Base::InterpreterSingleton::addType(&Base::PlacementPy::Type, pAppModule, "Placement");
    Base::InterpreterSingleton::addType(&Base::RotationPy::Type, pAppModule, "Rotation");
    Base::InterpreterSingleton::addType(&Base::AxisPy::Type, pAppModule, "Axis");

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
    Base::InterpreterSingleton::addType(&Base::VectorPy          ::Type,pBaseModule,"Vector");
    Base::InterpreterSingleton::addType(&Base::MatrixPy          ::Type,pBaseModule,"Matrix");
    Base::InterpreterSingleton::addType(&Base::BoundBoxPy        ::Type,pBaseModule,"BoundBox");
    Base::InterpreterSingleton::addType(&Base::PlacementPy       ::Type,pBaseModule,"Placement");
    Base::InterpreterSingleton::addType(&Base::RotationPy        ::Type,pBaseModule,"Rotation");
    Base::InterpreterSingleton::addType(&Base::AxisPy            ::Type,pBaseModule,"Axis");
    Base::InterpreterSingleton::addType(&Base::CoordinateSystemPy::Type,pBaseModule,"CoordinateSystem");
    Base::InterpreterSingleton::addType(&Base::TypePy            ::Type,pBaseModule,"TypeId");
    Base::InterpreterSingleton::addType(&Base::PrecisionPy       ::Type,pBaseModule,"Precision");

    Base::InterpreterSingleton::addType(&ApplicationDirectoriesPy::Type, pAppModule, "ApplicationDirectories");
    Base::InterpreterSingleton::addType(&MaterialPy::Type, pAppModule, "Material");
    Base::InterpreterSingleton::addType(&MetadataPy::Type, pAppModule, "Metadata");

    Base::InterpreterSingleton::addType(&MeasureManagerPy::Type, pAppModule, "MeasureManager");

    Base::InterpreterSingleton::addType(&StringHasherPy::Type, pAppModule, "StringHasher");
    Base::InterpreterSingleton::addType(&StringIDPy::Type, pAppModule, "StringID");

    // Add document types
    Base::InterpreterSingleton::addType(&PropertyContainerPy::Type, pAppModule, "PropertyContainer");
    Base::InterpreterSingleton::addType(&ExtensionContainerPy::Type, pAppModule, "ExtensionContainer");
    Base::InterpreterSingleton::addType(&DocumentPy::Type, pAppModule, "Document");
    Base::InterpreterSingleton::addType(&DocumentObjectPy::Type, pAppModule, "DocumentObject");
    Base::InterpreterSingleton::addType(&DocumentObjectGroupPy::Type, pAppModule, "DocumentObjectGroup");
    Base::InterpreterSingleton::addType(&GeoFeaturePy::Type, pAppModule, "GeoFeature");

    // Add extension types
    Base::InterpreterSingleton::addType(&ExtensionPy::Type, pAppModule, "Extension");
    Base::InterpreterSingleton::addType(&DocumentObjectExtensionPy::Type, pAppModule, "DocumentObjectExtension");
    Base::InterpreterSingleton::addType(&GroupExtensionPy::Type, pAppModule, "GroupExtension");
    Base::InterpreterSingleton::addType(&GeoFeatureGroupExtensionPy::Type, pAppModule, "GeoFeatureGroupExtension");
    Base::InterpreterSingleton::addType(&OriginGroupExtensionPy::Type, pAppModule, "OriginGroupExtension");
    Base::InterpreterSingleton::addType(&LinkBaseExtensionPy::Type, pAppModule, "LinkBaseExtension");

    //insert Base and Console
    Py_INCREF(pBaseModule);
    PyModule_AddObject(pAppModule, "Base", pBaseModule);
    Py_INCREF(pConsoleModule);
    PyModule_AddObject(pAppModule, "Console", pConsoleModule);

    // Translate module
    PyObject* pTranslateModule = Base::Interpreter().addModule(new Base::Translate);
    Py_INCREF(pTranslateModule);
    PyModule_AddObject(pAppModule, "Qt", pTranslateModule);

    //insert Units module
    static struct PyModuleDef UnitsModuleDef = {
        PyModuleDef_HEAD_INIT,
        "Units", "The Unit API", -1,
        Base::UnitsApi::Methods,
        nullptr, nullptr, nullptr, nullptr
    };
    PyObject* pUnitsModule = PyModule_Create(&UnitsModuleDef);
    Base::InterpreterSingleton::addType(&Base::QuantityPy  ::Type,pUnitsModule,"Quantity");
    // make sure to set the 'nb_true_divide' slot
    Base::InterpreterSingleton::addType(&Base::UnitPy      ::Type,pUnitsModule,"Unit");

    Py_INCREF(pUnitsModule);
    PyModule_AddObject(pAppModule, "Units", pUnitsModule);

    Base::ProgressIndicatorPy::init_type();
    Base::InterpreterSingleton::addType(Base::ProgressIndicatorPy::type_object(),
        pBaseModule,"ProgressIndicator");

    Base::Vector2dPy::init_type();
    Base::InterpreterSingleton::addType(Base::Vector2dPy::type_object(),
        pBaseModule,"Vector2d");
    // clang-format on
}

/*
 * Define custom Python exception types
 */
void Application::setupPythonException(PyObject* module)
{
    auto setup = [&module, str {"Base."}](const std::string& ename, auto pyExcType) {
        auto exception = PyErr_NewException((str + ename).c_str(), pyExcType, nullptr);
        Py_INCREF(exception);
        PyModule_AddObject(module, ename.c_str(), exception);
        return exception;
    };

    Base::PyExc_FC_GeneralError = setup("FreeCADError", PyExc_RuntimeError);
    Base::PyExc_FC_FreeCADAbort = setup("FreeCADAbort", PyExc_BaseException);
    Base::PyExc_FC_XMLBaseException = setup("XMLBaseException", PyExc_Exception);
    Base::PyExc_FC_XMLParseException = setup("XMLParseException", Base::PyExc_FC_XMLBaseException);
    Base::PyExc_FC_XMLAttributeError = setup("XMLAttributeError", Base::PyExc_FC_XMLBaseException);
    Base::PyExc_FC_UnknownProgramOption = setup("UnknownProgramOption", PyExc_BaseException);
    Base::PyExc_FC_BadFormatError = setup("BadFormatError", Base::PyExc_FC_GeneralError);
    Base::PyExc_FC_BadGraphError = setup("BadGraphError", Base::PyExc_FC_GeneralError);
    Base::PyExc_FC_ExpressionError = setup("ExpressionError", Base::PyExc_FC_GeneralError);
    Base::PyExc_FC_ParserError = setup("ParserError", Base::PyExc_FC_GeneralError);
    Base::PyExc_FC_CADKernelError = setup("CADKernelError", Base::PyExc_FC_GeneralError);
    Base::PyExc_FC_PropertyError = setup("PropertyError", PyExc_AttributeError);
    Base::PyExc_FC_AbortIOException = setup("AbortIOException", PyExc_BaseException);
}

//**************************************************************************
// Interface

Document* Application::newDocument(const char * proposedName, const char * proposedLabel, DocumentInitFlags CreateFlags)
{
    bool isUsingDefaultName = Base::Tools::isNullOrEmpty(proposedName);
    // get a valid name anyway!
    if (isUsingDefaultName) {
        proposedName = "Unnamed";
    }
    std::string name(getUniqueDocumentName(proposedName, CreateFlags.temporary));

    // return the temporary document if it exists
    if (CreateFlags.temporary) {
        auto it = DocMap.find(name);
        if (it != DocMap.end() && it->second->testStatus(Document::TempDoc)) {
            return it->second;
        }
    }

    // Determine the document's Label
    std::string label;
    if (!Base::Tools::isNullOrEmpty(proposedLabel)) {
        // If a label is supplied it is used even if not unique
        label = proposedLabel;
    }
    else {
        label = isUsingDefaultName ? QObject::tr("Unnamed").toStdString() : proposedName;

        if (!DocMap.empty()) {
            // The assumption here is that there are not many documents and
            // documents are rarely created so the cost
            // of building this manager each time is inconsequential
            Base::UniqueNameManager names;
            for (const auto& pos : DocMap) {
                names.addExactName(pos.second->Label.getValue());
            }

            label = names.makeUniqueName(label);
        }
    }
    // create the FreeCAD document
    auto doc = new Document(name.c_str());
    doc->setStatus(Document::TempDoc, CreateFlags.temporary);

    // add the document to the internal list
    DocMap[name] = doc;

    //NOLINTBEGIN
    // clang-format off
    // connect the signals to the application for the new document
    doc->signalBeforeChange.connect(std::bind(&Application::slotBeforeChangeDocument, this, sp::_1, sp::_2));
    doc->signalChanged.connect(std::bind(&Application::slotChangedDocument, this, sp::_1, sp::_2));
    doc->signalNewObject.connect(std::bind(&Application::slotNewObject, this, sp::_1));
    doc->signalDeletedObject.connect(std::bind(&Application::slotDeletedObject, this, sp::_1));
    doc->signalBeforeChangeObject.connect(std::bind(&Application::slotBeforeChangeObject, this, sp::_1, sp::_2));
    doc->signalChangedObject.connect(std::bind(&Application::slotChangedObject, this, sp::_1, sp::_2));
    doc->signalRelabelObject.connect(std::bind(&Application::slotRelabelObject, this, sp::_1));
    doc->signalActivatedObject.connect(std::bind(&Application::slotActivatedObject, this, sp::_1));
    doc->signalUndo.connect(std::bind(&Application::slotUndoDocument, this, sp::_1));
    doc->signalRedo.connect(std::bind(&Application::slotRedoDocument, this, sp::_1));
    doc->signalRecomputedObject.connect(std::bind(&Application::slotRecomputedObject, this, sp::_1));
    doc->signalRecomputed.connect(std::bind(&Application::slotRecomputed, this, sp::_1));
    doc->signalBeforeRecompute.connect(std::bind(&Application::slotBeforeRecompute, this, sp::_1));
    doc->signalOpenTransaction.connect(std::bind(&Application::slotOpenTransaction, this, sp::_1, sp::_2));
    doc->signalCommitTransaction.connect(std::bind(&Application::slotCommitTransaction, this, sp::_1));
    doc->signalAbortTransaction.connect(std::bind(&Application::slotAbortTransaction, this, sp::_1));
    doc->signalStartSave.connect(std::bind(&Application::slotStartSaveDocument, this, sp::_1, sp::_2));
    doc->signalFinishSave.connect(std::bind(&Application::slotFinishSaveDocument, this, sp::_1, sp::_2));
    doc->signalChangePropertyEditor.connect(std::bind(&Application::slotChangePropertyEditor, this, sp::_1, sp::_2));
    // clang-format on
    //NOLINTEND

    // (temporarily) make this the active document for the upcoming notifications.
    // Signal NewDocument rather than ActiveDocument (which is what setActiveDocument would do)
    auto oldActiveDoc = _pActiveDoc;
    setActiveDocumentNoSignal(doc);
    signalNewDocument(*doc, CreateFlags.createView);

    doc->Label.setValue(label);

    // set the old document active again if the new is temporary
    if (CreateFlags.temporary && oldActiveDoc) {
        setActiveDocument(oldActiveDoc);
    }
    return doc;
}

bool Application::closeDocument(const Document* doc)
{
    return closeDocument(doc->getName());
}

bool Application::closeDocument(const char* name)
{
    const auto pos = DocMap.find( name );
    if (pos == DocMap.end()) // no such document
        return false;

    Base::ConsoleRefreshDisabler disabler;

    // Trigger observers before removing the document from the internal map.
    // Some observers might rely on this document still being there.
    signalDeleteDocument(*pos->second);

    // For exception-safety use a smart pointer
    if (_pActiveDoc == pos->second) {
        setActiveDocument(static_cast<Document*>(nullptr));
    }
    const std::unique_ptr<Document> delDoc (pos->second);
    DocMap.erase( pos );
    DocFileMap.erase(Base::FileInfo(delDoc->FileName.getValue()).filePath());

    _objCount = -1;

    // Trigger observers after removing the document from the internal map.
    signalDeletedDocument();

    return true;
}

void Application::closeAllDocuments()
{
    Base::FlagToggler<bool> flag(_isClosingAll);
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
    for (const auto & it : DocMap) {
        if (it.second == doc) {
            return it.first.c_str();
        }
    }

    return nullptr;
}

std::vector<Document*> Application::getDocuments() const
{
    std::vector<Document*> docs;
    docs.reserve(DocMap.size());
    for (const auto & it : DocMap)
        docs.push_back(it.second);
    return docs;
}

std::string Application::getUniqueDocumentName(const char* Name, bool tempDoc) const
{
    if (!Name || *Name == '\0') {
        return {};
    }
    std::string CleanName = Base::Tools::getIdentifier(Name);

    // name in use?
    auto pos = DocMap.find(CleanName);

    if (pos == DocMap.end() || (tempDoc && pos->second->testStatus(Document::TempDoc))) {
        // if not, name is OK
        return CleanName;
    }
    // The assumption here is that there are not many documents and
    // documents are rarely created so the cost
    // of building this manager each time is inconsequential
    Base::UniqueNameManager names;
    for (const auto& pos : DocMap) {
        if (!tempDoc || !pos.second->testStatus(Document::TempDoc)) {
            names.addExactName(pos.first);
        }
    }

    return names.makeUniqueName(CleanName);
}

int Application::addPendingDocument(const char *FileName, const char *objName, bool allowPartial)
{
    if(!_isRestoring)
        return 0;
    if(allowPartial && _allowPartial)
        return -1;
    assert(!Base::Tools::isNullOrEmpty(FileName));
    assert(!Base::Tools::isNullOrEmpty(objName));
    if(!_docReloadAttempts[FileName].emplace(objName).second)
        return -1;
    const auto ret = _pendingDocMap.emplace(FileName,std::vector<std::string>());
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
    fastsignals::signal<void ()> &signal;
    DocOpenGuard(bool &f, fastsignals::signal<void ()> &s)
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
                Base::Console().warning("~DocOpenGuard: Unexpected boost exception\n");
            }
        }
    }
};

Document* Application::openDocument(const char * FileName, DocumentInitFlags initFlags) {
    std::vector<std::string> filenames(1,FileName);
    auto docs = openDocuments(filenames, nullptr, nullptr, nullptr, initFlags);
    if(!docs.empty())
        return docs.front();
    return nullptr;
}

Document *Application::getDocumentByPath(const char *path, PathMatchMode checkCanonical) const {
    if(Base::Tools::isNullOrEmpty(path))
        return nullptr;
    if (DocFileMap.empty()) {
        for(const auto &v : DocMap) {
            const auto &file = v.second->FileName.getStrValue();
            if(!file.empty())
                DocFileMap[Base::FileInfo(file.c_str()).filePath()] = v.second;
        }
    }
    const auto it = DocFileMap.find(Base::FileInfo(path).filePath());
    if(it != DocFileMap.end())
        return it->second;

    if (checkCanonical == PathMatchMode::MatchAbsolute) {
        return nullptr;
    }

    const std::string filepath = Base::FileInfo(path).filePath();
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
                                                  DocumentInitFlags initFlags)
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

                auto doc = openDocumentPrivate(path, name.c_str(), label, isMainDoc, initFlags, std::move(objNames));
                FC_DURATION_PLUS(timing.d1,t1);
                if (doc) {
                    timings[doc].d1 += timing.d1;
                    newDocs.emplace(doc);
                }

                if (isMainDoc)
                    res[count] = doc;
                _objCount = -1;
            }
            catch (const Base::Exception &e) {
                e.reportException();
                if (!errs && isMainDoc)
                    throw;
                if (errs && isMainDoc)
                    (*errs)[count] = e.what();
                else
                    Base::Console().error("Exception opening file: %s [%s]\n", name.c_str(), e.what());
            }
            catch (const std::exception &e) {
                if (!errs && isMainDoc)
                    throw;
                if (errs && isMainDoc)
                    (*errs)[count] = e.what();
                else
                    Base::Console().error("Exception opening file: %s [%s]\n", name.c_str(), e.what());
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

        Base::SequencerLauncher seq("Postprocessing...", docs.size());

        // After external links has been restored, we can now sort the document
        // according to their dependency order.
        try {
            docs = Document::getDependentDocuments(docs, true);
        } catch (Base::Exception &e) {
            e.reportException();
        }
        for(auto it=docs.begin(); it!=docs.end();) {
            auto doc = *it;

            // It is possible that the newly opened document depends on an existing
            // document, which will be included with the above call to
            // Document::getDependentDocuments(). Make sure to exclude that.
            if(!newDocs.contains(doc)) {
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
    PropertyLinkBase::updateAllElementReferences();
    _isRestoring = false;

    signalFinishOpenDocument();
    return res;
}

Document* Application::openDocumentPrivate(const char * FileName,
        const char *propFileName, const char *label,
        bool isMainDoc, DocumentInitFlags initFlags,
        std::vector<std::string> &&objNames)
{
    Base::FileInfo File(FileName);

    if (!File.exists()) {
        std::stringstream str;
        str << "File '" << FileName << "' does not exist!";
        throw Base::FileSystemError(str.str());
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

        if (!isMainDoc) {
            return nullptr;
        }

        if (doc) {
            return doc;
        }
    }

    std::string name;
    if(propFileName != FileName) {
        Base::FileInfo fi(propFileName);
        name = fi.fileNamePure();
    }else
        name = File.fileNamePure();

    // Use the same name for the internal and user name.
    // The file name is UTF-8 encoded which means that the internal name will be modified
    // to only contain valid ASCII characters but the user name will be kept.
    if(!label)
        label = name.c_str();

    initFlags.createView &= isMainDoc;
    Document* newDoc = newDocument(name.c_str(), label, initFlags);
    newDoc->FileName.setValue(propFileName==FileName?File.filePath():propFileName);

    try {
        // read the document
        newDoc->restore(File.filePath().c_str(),true,objNames);
        if(!DocFileMap.empty())
            DocFileMap[Base::FileInfo(newDoc->FileName.getValue()).filePath()] = newDoc;
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

Document* Application::getActiveDocument() const
{
    return _pActiveDoc;
}

void Application::setActiveDocument(Document* pDoc)
{
    setActiveDocumentNoSignal(pDoc);

    if (pDoc) {
        signalActiveDocument(*pDoc);
    }
}

void Application::setActiveDocumentNoSignal(Document* pDoc)
{
    _pActiveDoc = pDoc;

    // make sure that the active document is set in case no GUI is up
    if (pDoc) {
        Base::PyGILStateLocker lock;
        const Py::Object active(pDoc->getPyObject(), true);
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"), active);
    }
    else {
        Base::PyGILStateLocker lock;
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"), Py::None());
    }
}

void Application::setActiveDocument(const char* Name)
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
            Base::Console().warning("~TransactionSignaller: Unexpected boost exception\n");
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
    return Base::FileInfo::pathToString(Application::directories()->getHomePath()) + PATHSEP;
}

std::string Application::getExecutableName()
{
    return mConfig["ExeName"];
}

std::string Application::getNameWithVersion()
{
    auto appname = QCoreApplication::applicationName().toStdString();
    auto config = Application::Config();
    auto major = config["BuildVersionMajor"];
    auto minor = config["BuildVersionMinor"];
    auto point = config["BuildVersionPoint"];
    auto suffix = config["BuildVersionSuffix"];
    return fmt::format("{} {}.{}.{}{}", appname, major, minor, point, suffix);
}

bool Application::isDevelopmentVersion()
{
    static std::string suffix = []() constexpr {
        return FCVersionSuffix;
    }();
    return suffix == "dev";
}

const std::unique_ptr<ApplicationDirectories>& Application::directories() {
    return _appDirs;
}

std::string Application::getTempPath()
{
    return Base::FileInfo::pathToString(_appDirs->getTempPath()) + PATHSEP;
}

std::string Application::getTempFileName(const char* FileName)
{
    return Base::FileInfo::pathToString(_appDirs->getTempFileName(FileName ? FileName : std::string()));
}

std::string Application::getUserCachePath()
{
    return Base::FileInfo::pathToString(_appDirs->getUserCachePath()) + PATHSEP;
}

std::string Application::getUserConfigPath()
{
    return Base::FileInfo::pathToString(_appDirs->getUserConfigPath()) + PATHSEP;
}

std::string Application::getUserAppDataDir()
{
    return Base::FileInfo::pathToString(_appDirs->getUserAppDataDir()) + PATHSEP;
}

std::string Application::getUserMacroDir()
{
    return Base::FileInfo::pathToString(_appDirs->getUserMacroDir()) + PATHSEP;
}

std::string Application::getResourceDir()
{
    return Base::FileInfo::pathToString(_appDirs->getResourceDir()) + PATHSEP;
}

std::string Application::getLibraryDir()
{
    return Base::FileInfo::pathToString(_appDirs->getLibraryDir()) + PATHSEP;
}

std::string Application::getHelpDir()
{
    return Base::FileInfo::pathToString(_appDirs->getHelpDir()) + PATHSEP;
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
            throw Base::RuntimeError(msg);
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

    return it != mpcPramManager.end() ? it->second : nullptr;
}

const std::map<std::string,Base::Reference<ParameterManager>> &
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

Base::Reference<ParameterGrp>  Application::GetParameterGroupByPath(const char* sName)
{
    std::string cName = sName, cTemp;

    const std::string::size_type pos = cName.find(':');

    // is there a path separator ?
    if (pos == std::string::npos) {
        throw Base::ValueError("Application::GetParameterGroupByPath() no parameter set name specified");
    }
    // assigning the parameter set name
    cTemp.assign(cName,0,pos);
    cName.erase(0,pos+1);

    // test if name is valid
    const auto It = mpcPramManager.find(cTemp);
    if (It == mpcPramManager.end())
        throw Base::ValueError("Application::GetParameterGroupByPath() unknown parameter set name specified");

    return It->second->GetGroup(cName.c_str());
}

void Application::addImportType(const char* filter, const char* moduleName)
{
    FileTypeItem item;
    item.filter = filter;
    item.module = moduleName;

    // Extract each filetype from 'Type' literal
    std::string::size_type pos = item.filter.find("*.");
    while ( pos != std::string::npos ) {
        const std::string::size_type next = item.filter.find_first_of(" )", pos + 1);
        const std::string::size_type len = next-pos-2;
        std::string type = item.filter.substr(pos+2,len);
        item.types.push_back(std::move(type));
        pos = item.filter.find("*.", next);
    }

    // Due to branding stuff replace "FreeCAD" with the branded application name
    if (strncmp(filter, "FreeCAD", 7) == 0) {
        std::string AppName = getExecutableName();
        AppName += item.filter.substr(7);
        item.filter = std::move(AppName);
        // put to the front of the array
        _mImportTypes.insert(_mImportTypes.begin(),std::move(item));
    }
    else {
        _mImportTypes.push_back(std::move(item));
    }
}

void Application::changeImportModule(const char* filter, const char* oldModuleName, const char* newModuleName)
{
    for (auto& it : _mImportTypes) {
        if (it.filter == filter && it.module == oldModuleName) {
            it.module = newModuleName;
            break;
        }
    }
}

std::vector<std::string> Application::getImportModules(const std::string& extension) const
{
    std::vector<std::string> modules;
    for (const auto & it : _mImportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
            if (boost::iequals(extension, jt)) {
                modules.push_back(it.module);
            }
        }
    }

    return modules;
}

std::vector<std::string> Application::getImportModules() const
{
    std::vector<std::string> modules;
    modules.reserve(_mImportTypes.size());
    for (const auto& it : _mImportTypes) {
        modules.push_back(it.module);
    }
    std::sort(modules.begin(), modules.end());
    modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
    return modules;
}

std::vector<std::string> Application::getImportTypes(const std::string& Module) const
{
    std::vector<std::string> types;
    for (const auto & it : _mImportTypes) {
        if (boost::iequals(Module, it.module)) {
            types.insert(types.end(), it.types.begin(), it.types.end());
        }
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

std::map<std::string, std::string> Application::getImportFilters(const std::string& extension) const
{
    std::map<std::string, std::string> moduleFilter;
    for (const auto & it : _mImportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
            if (boost::iequals(extension, jt)) {
                moduleFilter[it.filter] = it.module;
            }
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

void Application::addExportType(const char* filter, const char* moduleName)
{
    FileTypeItem item;
    item.filter = filter;
    item.module = moduleName;

    // Extract each filetype from 'Type' literal
    std::string::size_type pos = item.filter.find("*.");
    while ( pos != std::string::npos ) {
        const std::string::size_type next = item.filter.find_first_of(" )", pos + 1);
        const std::string::size_type len = next-pos-2;
        std::string type = item.filter.substr(pos+2,len);
        item.types.push_back(std::move(type));
        pos = item.filter.find("*.", next);
    }

    // Due to branding stuff replace "FreeCAD" with the branded application name
    if (strncmp(filter, "FreeCAD", 7) == 0) {
        std::string AppName = getExecutableName();
        AppName += item.filter.substr(7);
        item.filter = std::move(AppName);
        // put to the front of the array
        _mExportTypes.insert(_mExportTypes.begin(),std::move(item));
    }
    else {
        _mExportTypes.push_back(std::move(item));
    }
}

void Application::changeExportModule(const char* filter, const char* oldModuleName, const char* newModuleName)
{
    for (auto& it : _mExportTypes) {
        if (it.filter == filter && it.module == oldModuleName) {
            it.module = newModuleName;
            break;
        }
    }
}

std::vector<std::string> Application::getExportModules(const std::string& extension) const
{
    std::vector<std::string> modules;
    for (const auto & it : _mExportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
            if (boost::iequals(extension, jt)) {
                modules.push_back(it.module);
            }
        }
    }

    return modules;
}

std::vector<std::string> Application::getExportModules() const
{
    std::vector<std::string> modules;
    modules.reserve(_mExportTypes.size());
    for (const auto& it : _mExportTypes) {
        modules.push_back(it.module);
    }
    std::sort(modules.begin(), modules.end());
    modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
    return modules;
}

std::vector<std::string> Application::getExportTypes(const std::string& Module) const
{
    std::vector<std::string> types;
    for (const auto & it : _mExportTypes) {
        if (boost::iequals(Module, it.module)) {
            types.insert(types.end(), it.types.begin(), it.types.end());
        }
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

std::map<std::string, std::string> Application::getExportFilters(const std::string& extension) const
{
    std::map<std::string, std::string> moduleFilter;
    for (const auto & it : _mExportTypes) {
        const std::vector<std::string>& types = it.types;
        for (const auto & jt : types) {
            if (boost::iequals(extension, jt)) {
                moduleFilter[it.filter] = it.module;
            }
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

void Application::slotNewObject(const DocumentObject& obj)
{
    this->signalNewObject(obj);
    _objCount = -1;
}

void Application::slotDeletedObject(const DocumentObject& obj)
{
    this->signalDeletedObject(obj);
    _objCount = -1;
}

void Application::slotBeforeChangeObject(const DocumentObject& obj, const Property& prop)
{
    this->signalBeforeChangeObject(obj, prop);
}

void Application::slotChangedObject(const DocumentObject& obj, const Property& prop)
{
    this->signalChangedObject(obj, prop);
}

void Application::slotRelabelObject(const DocumentObject& obj)
{
    this->signalRelabelObject(obj);
}

void Application::slotActivatedObject(const DocumentObject& obj)
{
    this->signalActivatedObject(obj);
}

void Application::slotUndoDocument(const Document& doc)
{
    this->signalUndoDocument(doc);
}

void Application::slotRedoDocument(const Document& doc)
{
    this->signalRedoDocument(doc);
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

void Application::slotOpenTransaction(const Document &doc, std::string name)
{
    this->signalOpenTransaction(doc, std::move(name));
}

void Application::slotCommitTransaction(const Document& doc)
{
    this->signalCommitTransaction(doc);
}

void Application::slotAbortTransaction(const Document& doc)
{
    this->signalAbortTransaction(doc);
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

void Application::slotChangePropertyEditor(const Document& doc, const Property& prop)
{
    this->signalChangePropertyEditor(doc, prop);
}

//**************************************************************************
// Init, Destruct and singleton

Application * Application::_pcSingleton = nullptr;

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

void Application::destruct()
{
    // saving system parameter
    if (_pcSysParamMngr->IgnoreSave()) {
        Base::Console().warning("Discard system parameter\n");
    }
    else {
        Base::Console().log("Saving system parameter...\n");
        _pcSysParamMngr->SaveDocument();
        Base::Console().log("Saving system parameter...done\n");
    }
    // saving the User parameter
    if (_pcUserParamMngr->IgnoreSave()) {
        Base::Console().warning("Discard user parameter\n");
    }
    else {
        Base::Console().log("Saving user parameter...\n");
        _pcUserParamMngr->SaveDocument();
        Base::Console().log("Saving user parameter...done\n");
    }

    // now save all other parameter files
    auto& paramMgr = _pcSingleton->mpcPramManager;
    for (const auto &it : paramMgr) {
        if ((it.second != _pcSysParamMngr) && (it.second != _pcUserParamMngr)) {
            if (it.second->HasSerializer() && !it.second->IgnoreSave()) {
                Base::Console().log("Saving %s...\n", it.first.c_str());
                it.second->SaveDocument();
                Base::Console().log("Saving %s...done\n", it.first.c_str());
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

    Base::Interpreter().finalize();

    Base::ScriptFactorySingleton::Destruct();
    Base::InterpreterSingleton::Destruct();
    Base::Type::destruct();
    ParameterManager::Terminate();
    SafeMode::Destruct();
}

void Application::destructObserver()
{
    if ( _pConsoleObserverFile ) {
        Base::Console().detachObserver(_pConsoleObserverFile);
        delete _pConsoleObserverFile;
        _pConsoleObserverFile = nullptr;
    }
    if ( _pConsoleObserverStd ) {
        Base::Console().detachObserver(_pConsoleObserverStd);
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
            str << "#" << (i-skip) << "  " << callstack[i] << " in " << demangled << " from " << info.dli_fname << "+" << offset << '\n';
            free(demangled);
        }
        else {
            str << "#" << (i-skip) << "  " << symbols[i] << '\n';
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
            std::cerr << "Illegal storage access..." << '\n';
#if !defined(_DEBUG)
            throw Base::AccessViolation("Illegal storage access! Please save your work under a new file name and restart the application!");
#endif
            break;
        case SIGABRT:
            std::cerr << "Abnormal program termination..." << '\n';
#if !defined(_DEBUG)
            throw Base::AbnormalProgramTermination("Break signal occurred");
#endif
            break;
        default:
            std::cerr << "Unknown error occurred..." << '\n';
            break;
    }
#endif // FC_OS_LINUX
}

void unhandled_exception_handler()
{
    std::cerr << "Terminating..." << '\n';
}

void unexpection_error_handler()
{
    std::cerr << "Unexpected error occurred..." << '\n';
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
        Base::Console().error("SEH exception (%u): Division by zero\n", code);
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

// clang-format off
void Application::initTypes()
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
    // Note: the order matters
    App::Property                   ::init();
    App::PropertyContainer          ::init();
    App::PropertyLists              ::init();
    App::PropertyBool               ::init();
    App::PropertyBoolList           ::init();
    App::PropertyFloat              ::init();
    App::PropertyFloatList          ::init();
    App::PropertyFloatConstraint    ::init();
    App::PropertyPrecision          ::init();
    App::PropertyQuantity           ::init();
    App::PropertyQuantityConstraint ::init();
    App::PropertyInteger            ::init();
    App::PropertyIntegerConstraint  ::init();
    App::PropertyPercent            ::init();
    App::PropertyEnumeration        ::init();
    App::PropertyIntegerList        ::init();
    App::PropertyIntegerSet         ::init();
    App::PropertyMap                ::init();
    App::PropertyString             ::init();
    App::PropertyPersistentObject   ::init();
    App::PropertyUUID               ::init();
    App::PropertyFont               ::init();
    App::PropertyStringList         ::init();
    App::PropertyLinkBase           ::init();
    App::PropertyLinkListBase       ::init();
    App::PropertyLink               ::init();
    App::PropertyLinkChild          ::init();
    App::PropertyLinkGlobal         ::init();
    App::PropertyLinkHidden         ::init();
    App::PropertyLinkSub            ::init();
    App::PropertyLinkSubChild       ::init();
    App::PropertyLinkSubGlobal      ::init();
    App::PropertyLinkSubHidden      ::init();
    App::PropertyLinkList           ::init();
    App::PropertyLinkListChild      ::init();
    App::PropertyLinkListGlobal     ::init();
    App::PropertyLinkListHidden     ::init();
    App::PropertyLinkSubList        ::init();
    App::PropertyLinkSubListChild   ::init();
    App::PropertyLinkSubListGlobal  ::init();
    App::PropertyLinkSubListHidden  ::init();
    App::PropertyXLink              ::init();
    App::PropertyXLinkSub           ::init();
    App::PropertyXLinkSubHidden     ::init();
    App::PropertyXLinkSubList       ::init();
    App::PropertyXLinkList          ::init();
    App::PropertyXLinkContainer     ::init();
    App::PropertyMatrix             ::init();
    App::PropertyVector             ::init();
    App::PropertyVectorDistance     ::init();
    App::PropertyPosition           ::init();
    App::PropertyDirection          ::init();
    App::PropertyVectorList         ::init();
    App::PropertyPlacement          ::init();
    App::PropertyPlacementList      ::init();
    App::PropertyPlacementLink      ::init();
    App::PropertyRotation           ::init();
    App::PropertyGeometry           ::init();
    App::PropertyComplexGeoData     ::init();
    App::PropertyColor              ::init();
    App::PropertyColorList          ::init();
    App::PropertyMaterial           ::init();
    App::PropertyMaterialList       ::init();
    App::PropertyPath               ::init();
    App::PropertyFile               ::init();
    App::PropertyFileIncluded       ::init();
    App::PropertyPythonObject       ::init();
    App::PropertyExpressionContainer::init();
    App::PropertyExpressionEngine   ::init();
    // all know unit properties
    App::PropertyAcceleration               ::init();
    App::PropertyAmountOfSubstance          ::init();
    App::PropertyAngle                      ::init();
    App::PropertyArea                       ::init();
    App::PropertyCompressiveStrength        ::init();
    App::PropertyCurrentDensity             ::init();
    App::PropertyDensity                    ::init();
    App::PropertyDissipationRate            ::init();
    App::PropertyDistance                   ::init();
    App::PropertyDynamicViscosity           ::init();
    App::PropertyElectricalCapacitance      ::init();
    App::PropertyElectricalConductance      ::init();
    App::PropertyElectricalConductivity     ::init();
    App::PropertyElectricalInductance       ::init();
    App::PropertyElectricalResistance       ::init();
    App::PropertyElectricCharge             ::init();
    App::PropertySurfaceChargeDensity       ::init();
    App::PropertyVolumeChargeDensity        ::init();
    App::PropertyElectricCurrent            ::init();
    App::PropertyElectricPotential          ::init();
    App::PropertyElectromagneticPotential   ::init();
    App::PropertyFrequency                  ::init();
    App::PropertyForce                      ::init();
    App::PropertyHeatFlux                   ::init();
    App::PropertyInverseArea                ::init();
    App::PropertyInverseLength              ::init();
    App::PropertyInverseVolume              ::init();
    App::PropertyKinematicViscosity         ::init();
    App::PropertyLength                     ::init();
    App::PropertyLuminousIntensity          ::init();
    App::PropertyMagneticFieldStrength      ::init();
    App::PropertyMagneticFlux               ::init();
    App::PropertyMagneticFluxDensity        ::init();
    App::PropertyMagnetization              ::init();
    App::PropertyMass                       ::init();
    App::PropertyMoment                     ::init();
    App::PropertyPressure                   ::init();
    App::PropertyPower                      ::init();
    App::PropertyShearModulus               ::init();
    App::PropertySpecificEnergy             ::init();
    App::PropertySpecificHeat               ::init();
    App::PropertySpeed                      ::init();
    App::PropertyStiffness                  ::init();
    App::PropertyStiffnessDensity           ::init();
    App::PropertyStress                     ::init();
    App::PropertyTemperature                ::init();
    App::PropertyThermalConductivity        ::init();
    App::PropertyThermalExpansionCoefficient::init();
    App::PropertyThermalTransferCoefficient ::init();
    App::PropertyTime                       ::init();
    App::PropertyUltimateTensileStrength    ::init();
    App::PropertyVacuumPermittivity         ::init();
    App::PropertyVelocity                   ::init();
    App::PropertyVolume                     ::init();
    App::PropertyVolumeFlowRate             ::init();
    App::PropertyVolumetricThermalExpansionCoefficient::init();
    App::PropertyWork                       ::init();
    App::PropertyYieldStrength              ::init();
    App::PropertyYoungsModulus              ::init();

    // Extension classes
    App::Extension                     ::init();
    App::ExtensionContainer            ::init();
    App::DocumentObjectExtension       ::init();
    App::GroupExtension                ::init();
    App::GroupExtensionPython          ::init();
    App::GeoFeatureGroupExtension      ::init();
    App::GeoFeatureGroupExtensionPython::init();
    App::OriginGroupExtension          ::init();
    App::OriginGroupExtensionPython    ::init();
    App::LinkBaseExtension             ::init();
    App::LinkBaseExtensionPython       ::init();
    App::LinkExtension                 ::init();
    App::LinkExtensionPython           ::init();
    App::SuppressibleExtension         ::init();
    App::SuppressibleExtensionPython   ::init();

    // Document classes
    App::TransactionalObject       ::init();
    App::DocumentObject            ::init();
    App::GeoFeature                ::init();

    // Test features
    App::FeatureTest               ::init();
    App::FeatureTestException      ::init();
    App::FeatureTestColumn         ::init();
    App::FeatureTestRow            ::init();
    App::FeatureTestAbsAddress     ::init();
    App::FeatureTestPlacement      ::init();
    App::FeatureTestAttribute      ::init();

    // Feature class
    App::FeaturePython             ::init();
    App::GeometryPython            ::init();
    App::Document                  ::init();
    App::DocumentObjectGroup       ::init();
    App::DocumentObjectGroupPython ::init();
    App::DocumentObjectFileIncluded::init();
    Image::ImagePlane              ::init();
    App::InventorObject            ::init();
    App::VRMLObject                ::init();
    App::Annotation                ::init();
    App::AnnotationLabel           ::init();
    App::MaterialObject            ::init();
    App::MaterialObjectPython      ::init();
    App::TextDocument              ::init();
    App::Placement                 ::init();
    App::PlacementPython           ::init();
    App::DatumElement              ::init();
    App::Plane                     ::init();
    App::Line                      ::init();
    App::Point                     ::init();
    App::LocalCoordinateSystem     ::init();
    App::Part                      ::init();
    App::Origin                    ::init();
    App::Link                      ::init();
    App::LinkPython                ::init();
    App::LinkElement               ::init();
    App::LinkElementPython         ::init();
    App::LinkGroup                 ::init();
    App::LinkGroupPython           ::init();
    App::VarSet                    ::init();

    // Expression classes
    App::Expression                ::init();
    App::UnitExpression            ::init();
    App::NumberExpression          ::init();
    App::ConstantExpression        ::init();
    App::OperatorExpression        ::init();
    App::VariableExpression        ::init();
    App::ConditionalExpression     ::init();
    App::StringExpression          ::init();
    App::FunctionExpression        ::init();
    App::RangeExpression           ::init();
    App::PyObjectExpression        ::init();

    // Topological naming classes
    App::StringHasher              ::init();
    App::StringID                  ::init();

    // register transaction type
    new App::TransactionProducer<TransactionDocumentObject>
            (DocumentObject::getClassTypeId());

    // register exception producer types
    new Base::ExceptionProducer<Base::AbortException>;
    new Base::ExceptionProducer<Base::XMLBaseException>;
    new Base::ExceptionProducer<Base::XMLParseException>;
    new Base::ExceptionProducer<Base::XMLAttributeError>;
    new Base::ExceptionProducer<Base::FileException>;
    new Base::ExceptionProducer<Base::FileSystemError>;
    new Base::ExceptionProducer<Base::BadFormatError>;
    new Base::ExceptionProducer<Base::MemoryException>;
    new Base::ExceptionProducer<Base::AccessViolation>;
    new Base::ExceptionProducer<Base::AbnormalProgramTermination>;
    new Base::ExceptionProducer<Base::UnknownProgramOption>;
    new Base::ExceptionProducer<Base::ProgramInformation>;
    new Base::ExceptionProducer<Base::TypeError>;
    new Base::ExceptionProducer<Base::ValueError>;
    new Base::ExceptionProducer<Base::IndexError>;
    new Base::ExceptionProducer<Base::NameError>;
    new Base::ExceptionProducer<Base::ImportError>;
    new Base::ExceptionProducer<Base::AttributeError>;
    new Base::ExceptionProducer<Base::RuntimeError>;
    new Base::ExceptionProducer<Base::BadGraphError>;
    new Base::ExceptionProducer<Base::NotImplementedError>;
    new Base::ExceptionProducer<Base::ZeroDivisionError>;
    new Base::ExceptionProducer<Base::ReferenceError>;
    new Base::ExceptionProducer<Base::ExpressionError>;
    new Base::ExceptionProducer<Base::ParserError>;
    new Base::ExceptionProducer<Base::UnicodeError>;
    new Base::ExceptionProducer<Base::OverflowError>;
    new Base::ExceptionProducer<Base::UnderflowError>;
    new Base::ExceptionProducer<Base::UnitsMismatchError>;
    new Base::ExceptionProducer<Base::CADKernelError>;
    new Base::ExceptionProducer<Base::RestoreError>;
    new Base::ExceptionProducer<Base::PropertyError>;

    Base::registerServiceImplementation<CenterOfMassProvider>(new NullCenterOfMass);
}

namespace {

void parseProgramOptions(int ac, char ** av, const std::string& exe, boost::program_options::variables_map& vm)
{
    // Declare a group of options that will be
    // allowed only on the command line
    boost::program_options::options_description generic("Generic options");
    generic.add_options()
    ("version,v", "Prints version string")
    ("verbose", "Prints verbose version string")
    ("help,h", "Prints help message")
    ("console,c", "Starts in console mode")
    ("response-file", boost::program_options::value<std::string>(),"Can be specified with '@name', too")
    ("dump-config", "Dumps configuration")
    ("get-config", boost::program_options::value<std::string>(), "Prints the value of the requested configuration key")
    ("set-config", boost::program_options::value< std::vector<std::string> >()->multitoken(), "Sets the value of a configuration key")
    ("keep-deprecated-paths", "If set then config files are kept on the old location")
    ;

    // Declare a group of options that will be
    // allowed both on the command line and in
    // the config file
    std::stringstream descr;
    descr << "Writes " << exe << ".log to the user directory.";
    boost::program_options::options_description config("Configuration");
    config.add_options()
    ("write-log,l", descr.str().c_str())
    ("log-file", boost::program_options::value<std::string>(), "Unlike --write-log this allows logging to an arbitrary file")
    ("user-cfg,u", boost::program_options::value<std::string>(),"User config file to load/save user settings")
    ("system-cfg,s", boost::program_options::value<std::string>(),"System config file to load/save system settings")
    ("run-test,t", boost::program_options::value<std::string>()->implicit_value(""),"Run a given test case (use 0 (zero) to run all tests). If no argument is provided then return list of all available tests.")
    ("run-open,r", boost::program_options::value<std::string>()->implicit_value(""),"Run a given test case (use 0 (zero) to run all tests). If no argument is provided then return list of all available tests.  Keeps UI open after test(s) complete.")
    ("module-path,M", boost::program_options::value< std::vector<std::string> >()->composing(),"Additional module paths")
    ("macro-path,E", boost::program_options::value< std::vector<std::string> >()->composing(),"Additional macro paths")
    ("python-path,P", boost::program_options::value< std::vector<std::string> >()->composing(),"Additional python paths")
    ("disable-addon", boost::program_options::value< std::vector<std::string> >()->composing(),"Disable a given addon.")
    ("single-instance", "Allow to run a single instance of the application")
    ("safe-mode", "Force enable safe mode")
    ("pass", boost::program_options::value< std::vector<std::string> >()->multitoken(), "Ignores the following arguments and pass them through to be used by a script")
    ;


    // Hidden options, will be allowed both on the command line and
    // in the config file, but will not be shown to the user.
    boost::program_options::options_description hidden("Hidden options");
    hidden.add_options()
    ("input-file", boost::program_options::value< std::vector<std::string> >(), "input file")
    ("output",     boost::program_options::value<std::string>(),"output file")
    ("hidden",                                             "don't show the main window")
    // this are to ignore for the window system (QApplication)
    ("style",      boost::program_options::value< std::string >(), "set the application GUI style")
    ("stylesheet", boost::program_options::value< std::string >(), "set the application stylesheet")
    ("session",    boost::program_options::value< std::string >(), "restore the application from an earlier session")
    ("reverse",                                               "set the application's layout direction from right to left")
    ("widgetcount",                                           "print debug messages about widgets")
    ("graphicssystem", boost::program_options::value< std::string >(), "backend to be used for on-screen widgets and pixmaps")
    ("display",    boost::program_options::value< std::string >(), "set the X-Server")
    ("geometry ",  boost::program_options::value< std::string >(), "set the X-Window geometry")
    ("font",       boost::program_options::value< std::string >(), "set the X-Window font")
    ("fn",         boost::program_options::value< std::string >(), "set the X-Window font")
    ("background", boost::program_options::value< std::string >(), "set the X-Window background color")
    ("bg",         boost::program_options::value< std::string >(), "set the X-Window background color")
    ("foreground", boost::program_options::value< std::string >(), "set the X-Window foreground color")
    ("fg",         boost::program_options::value< std::string >(), "set the X-Window foreground color")
    ("button",     boost::program_options::value< std::string >(), "set the X-Window button color")
    ("btn",        boost::program_options::value< std::string >(), "set the X-Window button color")
    ("name",       boost::program_options::value< std::string >(), "set the X-Window name")
    ("title",      boost::program_options::value< std::string >(), "set the X-Window title")
    ("visual",     boost::program_options::value< std::string >(), "set the X-Window to color scheme")
    ("ncols",      boost::program_options::value< int    >(), "set the X-Window to color scheme")
    ("cmap",                                                  "set the X-Window to color scheme")
#if defined(FC_OS_MACOSX)
    ("psn",        boost::program_options::value< std::string >(), "process serial number")
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
    boost::program_options::options_description cmdline_options("Command-line options");
    cmdline_options.add(generic).add(config).add(hidden);

    boost::program_options::options_description config_file_options("Config");
    config_file_options.add(config).add(hidden);

    boost::program_options::options_description visible("Allowed options");
    visible.add(generic).add(config);

    boost::program_options::positional_options_description p;
    p.add("input-file", -1);

    try {
        store( boost::program_options::command_line_parser(args).
               options(cmdline_options).positional(p).extra_parser(Util::customSyntax).run(), vm);

        std::ifstream ifs("FreeCAD.cfg");
        if (ifs)
            store(parse_config_file(ifs, config_file_options), vm);
        notify(vm);
    }
    catch (const std::exception& e) {
        std::stringstream str;
        str << e.what() << '\n' << '\n' << visible << '\n';
        throw Base::UnknownProgramOption(str.str());
    }
    catch (...) {
        std::stringstream str;
        str << "Wrong or unknown option, bailing out!" << '\n' << '\n' << visible << '\n';
        throw Base::UnknownProgramOption(str.str());
    }

    if (vm.contains("help")) {
        std::stringstream str;
        str << exe << '\n' << '\n';
        str << "For a detailed description see https://www.freecad.org/wiki/Start_up_and_Configuration" << '\n'<<'\n';
        str << "Usage: " << exe << " [options] File1 File2 ..." << '\n' << '\n';
        str << visible << '\n';
        throw Base::ProgramInformation(str.str());
    }

    if (vm.contains("response-file")) {
        // Load the file and tokenize it
        std::ifstream ifs(vm["response-file"].as<std::string>().c_str());
        if (!ifs) {
            Base::Console().error("Could no open the response file\n");
            std::stringstream str;
            str << "Could no open the response file: '"
                << vm["response-file"].as<std::string>() << "'" << '\n';
            throw Base::UnknownProgramOption(str.str());
        }
        // Read the whole file into a string
        std::stringstream ss;
        ss << ifs.rdbuf();
        // Split the file content
        boost::char_separator<char> sep(" \n\r");
        boost::tokenizer<boost::char_separator<char> > tok(ss.str(), sep);
        std::vector<std::string> args2;
        copy(tok.begin(), tok.end(), back_inserter(args2));
        // Parse the file and store the options
        store( boost::program_options::command_line_parser(args2).
               options(cmdline_options).positional(p).extra_parser(Util::customSyntax).run(), vm);
    }
}

void processProgramOptions(const boost::program_options::variables_map& vm, std::map<std::string,std::string>& mConfig)
{
    if (vm.contains("version") && !vm.contains("verbose")) {
        std::stringstream str;
        str << mConfig["ExeName"] << " " << mConfig["ExeVersion"]
            << " Revision: " << mConfig["BuildRevision"] << '\n';
        throw Base::ProgramInformation(str.str());
    }

    if (vm.contains("module-path")) {
        auto  Mods = vm["module-path"].as< std::vector<std::string> >();
        std::string temp;
        for (const auto & It : Mods)
            temp += It + ";";
        temp.erase(temp.end()-1);
        mConfig["AdditionalModulePaths"] = temp;
    }

    if (vm.contains("macro-path")) {
        std::vector<std::string> Macros = vm["macro-path"].as< std::vector<std::string> >();
        std::string temp;
        for (const auto & It : Macros)
            temp += It + ";";
        temp.erase(temp.end()-1);
        mConfig["AdditionalMacroPaths"] = std::move(temp);
    }

    if (vm.contains("python-path")) {
        auto  Paths = vm["python-path"].as< std::vector<std::string> >();
        for (const auto & It : Paths)
            Base::Interpreter().addPythonPath(It.c_str());
    }

    if (vm.contains("disable-addon")) {
        auto Addons = vm["disable-addon"].as< std::vector<std::string> >();
        std::string temp;
        for (const auto & It : Addons) {
            temp += It + ";";
        }
        temp.erase(temp.end()-1);
        mConfig["DisabledAddons"] = temp;
    }

    if (vm.contains("input-file")) {
        auto  files(vm["input-file"].as< std::vector<std::string> >());
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

    if (vm.contains("output")) {
        mConfig["SaveFile"] = vm["output"].as<std::string>();
    }

    if (vm.contains("hidden")) {
        mConfig["StartHidden"] = "1";
    }

    if (vm.contains("write-log")) {
        mConfig["LoggingFile"] = "1";
        mConfig["LoggingFileName"] = mConfig["UserAppData"] + mConfig["ExeName"] + ".log";
    }

    if (vm.contains("log-file")) {
        mConfig["LoggingFile"] = "1";
        mConfig["LoggingFileName"] = vm["log-file"].as<std::string>();
    }

    if (vm.contains("user-cfg")) {
        mConfig["UserParameter"] = vm["user-cfg"].as<std::string>();
    }

    if (vm.contains("system-cfg")) {
        mConfig["SystemParameter"] = vm["system-cfg"].as<std::string>();
    }

    if (vm.contains("run-test") || vm.contains("run-open")) {
        std::string testCase = vm.contains("run-open") ? vm["run-open"].as<std::string>() : vm["run-test"].as<std::string>();

        if ( "0" == testCase) {
            testCase = "TestApp.All";
        }
        else if (testCase.empty()) {
            testCase = "TestApp.PrintAll";
        }
        mConfig["TestCase"] = std::move(testCase);
        mConfig["RunMode"] = "Internal";
        mConfig["ScriptFileName"] = "FreeCADTest";
        mConfig["ExitTests"] = vm.contains("run-open") ? "no" : "yes";
    }

    if (vm.contains("single-instance")) {
        mConfig["SingleInstance"] = "1";
    }

    if (vm.contains("dump-config")) {
        std::stringstream str;
        for (const auto & it : mConfig) {
            str << it.first << "=" << it.second << '\n';
        }
        throw Base::ProgramInformation(str.str());
    }

    if (vm.contains("get-config")) {
        auto configKey = vm["get-config"].as<std::string>();
        std::stringstream str;
        std::map<std::string,std::string>::iterator pos;
        pos = mConfig.find(configKey);
        if (pos != mConfig.end()) {
            str << pos->second;
        }
        str << '\n';
        throw Base::ProgramInformation(str.str());
    }

    if (vm.contains("set-config")) {
        auto  configKeyValue = vm["set-config"].as< std::vector<std::string> >();
        for (const auto& it : configKeyValue) {
            auto pos = it.find('=');
            if (pos != std::string::npos) {
                std::string key = it.substr(0, pos);
                std::string val = it.substr(pos + 1);
                mConfig[key] = std::move(val);
            }
        }
    }
}

}
// clang-format on

void Application::initConfig(int argc, char ** argv)
{
    // find the home path....
    mConfig["AppHomePath"] = ApplicationDirectories::findHomePath(argv[0]).string();

    // Version of the application extracted from SubWCRef into src/Build/Version.h
    // We only set these keys if not yet defined. Therefore it suffices to search
    // only for 'BuildVersionMajor'.
    if (Application::Config().find("BuildVersionMajor") == Application::Config().end()) {
        std::stringstream str;
        str << FCVersionMajor
            << "." << FCVersionMinor
            << "." << FCVersionPoint;
        Application::Config()["ExeVersion"         ] = str.str();
        Application::Config()["BuildVersionMajor"  ] = FCVersionMajor;
        Application::Config()["BuildVersionMinor"  ] = FCVersionMinor;
        Application::Config()["BuildVersionPoint"  ] = FCVersionPoint;
        Application::Config()["BuildVersionSuffix" ] = FCVersionSuffix;
        Application::Config()["BuildRevision"      ] = FCRevision;
        Application::Config()["BuildRepositoryURL" ] = FCRepositoryURL;
        Application::Config()["BuildRevisionDate"  ] = FCRevisionDate;
#if defined(FCRepositoryHash)
        Application::Config()["BuildRevisionHash"  ] = FCRepositoryHash;
#endif
#if defined(FCRepositoryBranch)
        Application::Config()["BuildRevisionBranch"] = FCRepositoryBranch;
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
            Application::Config()[it.key()] = it.value();
        }
    }

    boost::program_options::variables_map vm;
    {
        BOOST_SCOPE_EXIT_ALL(&) {
            // console-mode needs to be set (if possible) also in case parseProgramOptions
            // throws, as it's needed when reporting such exceptions
            if (vm.contains("console")) {
                mConfig["Console"] = "1";
                mConfig["RunMode"] = "Cmd";
            }
        };
        parseProgramOptions(argc, argv, mConfig["ExeName"], vm);
    }

    if (vm.contains("keep-deprecated-paths")) {
        mConfig["KeepDeprecatedPaths"] = "1";
    }

    if (vm.contains("safe-mode")) {
        mConfig["SafeMode"] = "1";
    }

    // extract home paths
    _appDirs = std::make_unique<ApplicationDirectories>(mConfig);

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
        ApplicationMethods = ApplicationPy::Methods;
        PyObject *pyModule = init_freecad_module();
        PyDict_SetItemString(sysModules, moduleName, pyModule);
        Py_DECREF(pyModule);

        moduleName = "__FreeCADBase__";
        PyImport_AddModule(moduleName);
        pyModule = init_freecad_base_module();
        PyDict_SetItemString(sysModules, moduleName, pyModule);
        Py_DECREF(pyModule);
    }

    std::string pythonpath = Base::Interpreter().init(argc,argv);
    if (!pythonpath.empty())
        mConfig["PythonSearchPath"] = pythonpath;
    else
        Base::Console().warning("Encoding of Python paths failed\n");

    // Handle the options that have impact on the init process
    processProgramOptions(vm, mConfig);

    // Init console ===========================================================
    Base::PyGILStateLocker lock;
    _pConsoleObserverStd = new Base::ConsoleObserverStd();
    Base::Console().attachObserver(_pConsoleObserverStd);
    if (mConfig["LoggingConsole"] != "1") {
        _pConsoleObserverStd->bMsg = false;
        _pConsoleObserverStd->bLog = false;
        _pConsoleObserverStd->bWrn = false;
        _pConsoleObserverStd->bErr = false;
    }

    // file logging Init ===========================================================
    if (mConfig["LoggingFile"] == "1") {
        _pConsoleObserverFile = new Base::ConsoleObserverFile(mConfig["LoggingFileName"].c_str());
        Base::Console().attachObserver(_pConsoleObserverFile);
    }
    else
        _pConsoleObserverFile = nullptr;

    // Banner ===========================================================
    if (mConfig["RunMode"] != "Cmd" && !(vm.contains("verbose") && vm.contains("version"))) {
        // Remove banner if FreeCAD is invoked via the -c command as regular
        // Python interpreter
        if (mConfig["Verbose"] != "Strict")
            Base::Console().message("%s %s, Libs: %s.%s.%s%sR%s\n%s",
                              mConfig["ExeName"].c_str(),
                              mConfig["ExeVersion"].c_str(),
                              mConfig["BuildVersionMajor"].c_str(),
                              mConfig["BuildVersionMinor"].c_str(),
                              mConfig["BuildVersionPoint"].c_str(),
                              mConfig["BuildVersionSuffix"].c_str(),
                              mConfig["BuildRevision"].c_str(),
                              mConfig["CopyrightInfo"].c_str());
        else
            Base::Console().message("%s %s, Libs: %s.%s.%s%sR%s\n",
                              mConfig["ExeName"].c_str(),
                              mConfig["ExeVersion"].c_str(),
                              mConfig["BuildVersionMajor"].c_str(),
                              mConfig["BuildVersionMinor"].c_str(),
                              mConfig["BuildVersionPoint"].c_str(),
                              mConfig["BuildVersionSuffix"].c_str(),
                              mConfig["BuildRevision"].c_str());

        if (SafeMode::SafeModeEnabled()) {
            Base::Console().message("FreeCAD is running in _SAFE_MODE_.\n"
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
                Base::Console().setDefaultLogLevel(v.second);
            }
#endif
        }
        else if (v.first == "DebugDefault") {
#ifdef FC_DEBUG
            if (v.second>=0) {
                hasDefault = true;
                Base::Console().setDefaultLogLevel(static_cast<int>(v.second));
            }
#endif
        }
        else {
            *Base::Console().getLogLevel(v.first.c_str()) = static_cast<int>(v.second);
        }
    }

    if (!hasDefault) {
#ifdef FC_DEBUG
        loglevelParam->SetInt("DebugDefault", Base::Console().logLevel(-1));
#else
        loglevelParam->SetInt("Default", Base::Console().logLevel(-1));
#endif
    }

    // Change application tmp. directory
    std::string tmpPath = _pcUserParamMngr->GetGroup("BaseApp/Preferences/General")->GetASCII("TempPath");
    Base::FileInfo di(tmpPath);
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

    if (vm.contains("verbose") && vm.contains("version")) {
        Application::_pcSingleton = new Application(mConfig);
        throw Base::ProgramInformation(Application::verboseVersionEmitMessage);
    }
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
    new Base::ScriptProducer( "CMakeVariables", CMakeVariables );
    new Base::ScriptProducer( "FreeCADInit",    FreeCADInit    );
    new Base::ScriptProducer( "FreeCADTest",    FreeCADTest    );

    // creating the application
    if (mConfig["Verbose"] != "Strict")
        Base::Console().log("Create Application\n");
    Application::_pcSingleton = new Application(mConfig);

    // set up Unit system default
    const ParameterGrp::handle hGrp = GetApplication().GetParameterGroupByPath
       ("User parameter:BaseApp/Preferences/Units");
    Base::UnitsApi::setSchema(hGrp->GetInt("UserSchema", Base::UnitsApi::getDefSchemaNum()));
    Base::UnitsApi::setDecimals(hGrp->GetInt("Decimals", Base::UnitsApi::getDecimals()));
    Base::UnitsApi::setDenominator(hGrp->GetInt("FracInch", Base::UnitsApi::getDenominator()));

#if defined (_DEBUG)
    Base::Console().log("Application is built with debug information\n");
#endif

    // starting the init script
    Base::Console().log("Run App init script\n");
    try {
        Base::Interpreter().runString(Base::ScriptFactory().ProduceScript("CMakeVariables"));
        Base::Interpreter().runString(Base::ScriptFactory().ProduceScript("FreeCADInit"));
    }
    catch (const Base::Exception& e) {
        e.reportException();
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
        files.emplace_back(mConfig[temp.str()]);
    }

    return files;
}

std::list<std::string> Application::processFiles(const std::list<std::string>& files)
{
    std::list<std::string> processed;
    Base::Console().log("Init: Processing command line files\n");
    for (const auto & it : files) {

        Base::FileInfo file(it);
        // Can we safely remove the isSymlink check and directly query the canonical
        // path for every string? The reason for avoiding it currently is that
        // getCannonicalPath will log an error if the file doesn't exist
        if (file.isSymlink()) {
            if (auto cannonicalPath = file.getCannonicalPath()) {
                file = Base::FileInfo(*cannonicalPath);
            } else {
                Base::Console().error("Failed to process symlink file: %s\n", file.filePath());
            }
        }

        Base::Console().log("Init:     Processing file: %s\n",file.filePath().c_str());

        try {
            if (file.hasExtension("fcstd") || file.hasExtension("std")) {
                // try to open
                Application::_pcSingleton->openDocument(file.filePath().c_str());
                processed.push_back(it);
            }
            else if (file.hasExtension("fcscript") || file.hasExtension("fcmacro")) {
                Base::Interpreter().runFile(file.filePath().c_str(), true);
                processed.push_back(it);
            }
            else if (file.hasExtension("py")) {
                try {
                    Base::Interpreter().addPythonPath(file.dirPath().c_str());
                    Base::Interpreter().loadModule(file.fileNamePure().c_str());
                    processed.push_back(it);
                }
                catch (const Base::PyException&) {
                    // if loading the module does not work, try just running the script (run in __main__)
                    Base::Interpreter().runFile(file.filePath().c_str(),true);
                    processed.push_back(it);
                }
            }
            else {
                std::vector<std::string> mods = GetApplication().getImportModules(file.extension());
                if (!mods.empty()) {
                    std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(file.filePath().c_str());
                    escapedstr = Base::Tools::escapeEncodeFilename(escapedstr);

                    Base::Interpreter().loadModule(mods.front().c_str());
                    Base::Interpreter().runStringArg("import %s",mods.front().c_str());
                    Base::Interpreter().runStringArg("%s.open(u\"%s\")",mods.front().c_str(),
                            escapedstr.c_str());
                    processed.push_back(it);
                    Base::Console().log("Command line open: %s.open(u\"%s\")\n",mods.front().c_str(),escapedstr.c_str());
                }
                else if (file.exists()) {
                    Base::Console().warning("File format not supported: %s \n", file.filePath().c_str());
                }
            }
        }
        catch (const Base::SystemExitException&) {
            throw; // re-throw to main() function
        }
        catch (const Base::Exception& e) {
            Base::Console().error("Exception while processing file: %s [%s]\n", file.filePath().c_str(), e.what());
        }
        catch (...) {
            Base::Console().error("Unknown exception while processing file: %s \n", file.filePath().c_str());
        }
    }

    return processed; // successfully processed files
}

void Application::processCmdLineFiles()
{
    const std::list<std::string> files = getCmdLineFiles();
    const std::list<std::string> processed = processFiles(files);

    if (files.empty()) {
        if (mConfig["RunMode"] == "Exit")
            mConfig["RunMode"] = "Cmd";
    }
    else if (processed.empty() && files.size() == 1 && mConfig["RunMode"] == "Cmd") {
        // In case we are in console mode and the argument is not a file but Python code
        // then execute it. This is to behave like the standard Python executable.
        const Base::FileInfo file(files.front());
        if (!file.exists()) {
            Base::Interpreter().runString(files.front().c_str());
            mConfig["RunMode"] = "Exit";
        }
    }

    const std::map<std::string, std::string>& cfg = Application::Config();
    const auto it = cfg.find("SaveFile");
    if (it != cfg.end()) {
        std::string output = it->second;
        output = Base::Tools::escapeEncodeFilename(output);

        const Base::FileInfo fi(output);
        try {
            const std::vector<std::string> mods = GetApplication().getExportModules(fi.extension());
            if (!mods.empty()) {
                Base::Interpreter().loadModule(mods.front().c_str());
                Base::Interpreter().runStringArg("import %s",mods.front().c_str());
                Base::Interpreter().runStringArg("%s.export(App.ActiveDocument.Objects, '%s')"
                    ,mods.front().c_str(),output.c_str());
            }
            else {
                Base::Console().warning("File format not supported: %s \n", output.c_str());
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().error("Exception while saving to file: %s [%s]\n", output.c_str(), e.what());
        }
        catch (...) {
            Base::Console().error("Unknown exception while saving to file: %s \n", output.c_str());
        }
    }
}

void Application::runApplication()
{
    // process all files given through command line interface
    processCmdLineFiles();

    if (mConfig["RunMode"] == "Cmd") {
        // Run the commandline interface
        Base::Interpreter().runCommandLine("FreeCAD Console mode");
    }
    else if (mConfig["RunMode"] == "Internal") {
        // run internal script
        Base::Console().log("Running internal script:\n");
        Base::Interpreter().runString(Base::ScriptFactory().ProduceScript(mConfig["ScriptFileName"].c_str()));
    }
    else if (mConfig["RunMode"] == "Exit") {
        // getting out
        Base::Console().log("Exiting on purpose\n");
    }
    else {
        Base::Console().log("Unknown Run mode (%d) in main()?!?\n\n", mConfig["RunMode"].c_str());
    }
}

void Application::logStatus()
{
    const std::string time_str = boost::posix_time::to_simple_string(
        boost::posix_time::second_clock::local_time());
    Base::Console().log("Time = %s\n", time_str.c_str());

    for (const auto & It : mConfig) {
        Base::Console().log("%s = %s\n", It.first.c_str(), It.second.c_str());
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
                Base::Console().warning("   Parameter does not exist, writing initial one\n");
                Base::Console().message("   This warning normally means that FreeCAD is running for the first time\n"
                                  "   or the configuration was deleted or moved. FreeCAD is generating the standard\n"
                                  "   configuration.\n");
            }
        }
    }
    catch (const Base::Exception& e) {
        // try to proceed with an empty XML document
        Base::Console().error("%s in file %s.\n"
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
                Base::Console().warning("   User settings do not exist, writing initial one\n");
                Base::Console().message("   This warning normally means that FreeCAD is running for the first time\n"
                                  "   or your configuration was deleted or moved. The system defaults\n"
                                  "   will be automatically generated for you.\n");
            }
        }
    }
    catch (const Base::Exception& e) {
        // try to proceed with an empty XML document
        Base::Console().error("%s in file %s.\n"
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
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, " "));
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
        throw Base::RuntimeError("Getting HOME path from system failed!");
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
    return userHome.isEmpty() ? getUserHome() : userHome;
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

    std::filesystem::path appData(Base::FileInfo::stringToPath(dataPath.toStdString()));

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
    tempPath = QString::fromStdString(Base::FileInfo::getTempPath());
    cacheHome = tempPath;
#endif
#endif

    return std::make_tuple(configHome, dataHome, cacheHome, tempPath);
}
}

void Application::ExtractUserPath()
{
    bool keepDeprecatedPaths = mConfig.contains("KeepDeprecatedPaths");

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
    mConfig["UserAppData"] = Base::FileInfo::pathToString(data) + PATHSEP;


    // User config path
    //
    std::filesystem::path config = findPath(configHome, customHome, subdirs, true);
    mConfig["UserConfigPath"] = Base::FileInfo::pathToString(config) + PATHSEP;


    // User cache path
    //
    std::vector<std::string> cachedirs = subdirs;
    cachedirs.emplace_back("Cache");
    std::filesystem::path cache = findPath(cacheHome, customTemp, cachedirs, true);
    mConfig["UserCachePath"] = Base::FileInfo::pathToString(cache) + PATHSEP;


    // Set application tmp. directory
    //
    std::vector<std::string> empty;
    std::filesystem::path tmp = findPath(tempPath, customTemp, empty, true);
    mConfig["AppTempPath"] = Base::FileInfo::pathToString(tmp) + PATHSEP;


    // Set the default macro directory
    //
    std::vector<std::string> macrodirs = std::move(subdirs);  // Last use in this method, just move
    macrodirs.emplace_back("Macro");
    std::filesystem::path macro = findPath(dataHome, customData, macrodirs, true);
    mConfig["UserMacroPath"] = Base::FileInfo::pathToString(macro) + PATHSEP;
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
#include <cstdlib>
#include <sys/param.h>

std::string Application::FindHomePath(const char* sCall)
{
    // If Python is initialized at this point, then we're being run from
    // MainPy.cpp, which hopefully rewrote argv[0] to point at the
    // FreeCAD shared library.
    if (!Py_IsInitialized()) {
        uint32_t sz = 0;

        // function only returns "sz" if first arg is to small to hold value
        _NSGetExecutablePath(nullptr, &sz);

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

    return sCall;
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

QString Application::prettyProductInfoWrapper()
{
    auto productName = QSysInfo::prettyProductName();
#ifdef FC_OS_MACOSX
    auto macosVersionFile =
        QStringLiteral("/System/Library/CoreServices/.SystemVersionPlatform.plist");
    auto fi = QFileInfo(macosVersionFile);
    if (fi.exists() && fi.isReadable()) {
        auto plistFile = QFile(macosVersionFile);
        plistFile.open(QIODevice::ReadOnly);
        while (!plistFile.atEnd()) {
            auto line = plistFile.readLine();
            if (line.contains("ProductUserVisibleVersion")) {
                auto nextLine = plistFile.readLine();
                if (nextLine.contains("<string>")) {
                    QRegularExpression re(QStringLiteral("\\s*<string>(.*)</string>"));
                    auto matches = re.match(QString::fromUtf8(nextLine));
                    if (matches.hasMatch()) {
                        productName = QStringLiteral("macOS ") + matches.captured(1);
                        break;
                    }
                }
            }
        }
    }
#endif
#ifdef FC_OS_WIN64
    QSettings regKey {
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        QSettings::NativeFormat};
    if (regKey.contains(QStringLiteral("CurrentBuildNumber"))) {
        auto buildNumber = regKey.value(QStringLiteral("CurrentBuildNumber")).toInt();
        if (buildNumber > 0) {
            if (buildNumber < 9200) {
                productName = QStringLiteral("Windows 7 build %1").arg(buildNumber);
            }
            else if (buildNumber < 10240) {
                productName = QStringLiteral("Windows 8 build %1").arg(buildNumber);
            }
            else if (buildNumber < 22000) {
                productName = QStringLiteral("Windows 10 build %1").arg(buildNumber);
            }
            else {
                productName = QStringLiteral("Windows 11 build %1").arg(buildNumber);
            }
        }
    }
#endif
    return productName;
}

void Application::addModuleInfo(QTextStream& str, const QString& modPath, bool& firstMod)
{
    QFileInfo mod(modPath);
    if (mod.isHidden()) {  // Ignore hidden directories
        return;
    }
    if (firstMod) {
        firstMod = false;
        str << "Installed mods: \n";
    }
    QString addonName = mod.isDir() ? QDir(modPath).dirName() : mod.fileName();
    QString versionString;
    try {
        auto metadataFile =
            std::filesystem::path(mod.absoluteFilePath().toStdString()) / "package.xml";
        if (std::filesystem::exists(metadataFile)) {
            App::Metadata metadata(metadataFile);
            if (!metadata.name().empty()) {
                addonName = QString::fromStdString(metadata.name());
            }
            if (metadata.version() != App::Meta::Version()) {
                versionString = QString::fromStdString(" " + metadata.version().str());
            }
        }
    }
    catch (const Base::Exception& e) {
        auto what = QString::fromUtf8(e.what()).trimmed().replace(QChar::fromLatin1('\n'),
                                                                  QChar::fromLatin1(' '));
        str << " (Malformed metadata: " << what << ")";
    }
    str << "  * " << addonName << versionString;
    QFileInfo disablingFile(mod.absoluteFilePath(), QStringLiteral("ADDON_DISABLED"));
    if (disablingFile.exists()) {
        str << " (Disabled)";
    }

    str << "\n";
}

QString Application::getValueOrEmpty(const std::map<std::string, std::string>& map, const std::string& key) {
    auto it = map.find(key);
    return (it != map.end()) ? QString::fromStdString(it->second) : QString();
}

void Application::getVerboseCommonInfo(QTextStream& str, const std::map<std::string,std::string>& mConfig)
{
    std::map<std::string, std::string>::iterator it;
    const QString deskEnv =
    QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_CURRENT_DESKTOP"),
                                                   QString());
    const QString deskSess =
        QProcessEnvironment::systemEnvironment().value(QStringLiteral("DESKTOP_SESSION"),
                                                    QString());
  
    const QString major = getValueOrEmpty(mConfig, "BuildVersionMajor");
    const QString minor = getValueOrEmpty(mConfig, "BuildVersionMinor");
    const QString point = getValueOrEmpty(mConfig, "BuildVersionPoint");
    const QString suffix = getValueOrEmpty(mConfig, "BuildVersionSuffix");
    const QString build = getValueOrEmpty(mConfig, "BuildRevision");
    const QString buildDate = getValueOrEmpty(mConfig, "BuildRevisionDate");

    QStringList deskInfoList;
    QString deskInfo;

    if (!deskEnv.isEmpty()) {
        deskInfoList.append(deskEnv);
    }
    if (!deskSess.isEmpty()) {
        deskInfoList.append(deskSess);
    }

    const QString sysType = QSysInfo::productType();
    if (sysType != QLatin1String("windows") && sysType != QLatin1String("macos")) {
        QString sessionType = QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_SESSION_TYPE"),
             QString());
        if (sessionType == QLatin1String("x11")) {
            sessionType = QStringLiteral("xcb");
        }
        deskInfoList.append(sessionType);
    }
    if (!deskInfoList.isEmpty()) {
        deskInfo = QLatin1String(" (") + deskInfoList.join(QLatin1String("/")) + QLatin1String(")");
    }

    str << "OS: " << prettyProductInfoWrapper() << deskInfo << '\n';
    if (QSysInfo::buildCpuArchitecture() == QSysInfo::currentCpuArchitecture()) {
        str << "Architecture: " << QSysInfo::buildCpuArchitecture() << "\n";
    }
    else {
        str << "Architecture: " << QSysInfo::buildCpuArchitecture()
            << "(running on: " << QSysInfo::currentCpuArchitecture() << ")\n";
    }
    str << "Version: " << major << "." << minor << "." << point << suffix << "." << build;

#ifdef FC_CONDA
    str << " Conda";
#endif
#ifdef FC_FLATPAK
    str << " Flatpak";
#endif
    const char* appimage = getenv("APPIMAGE");
    if (appimage) {
        str << " AppImage";
    }
    const char* snap = getenv("SNAP_REVISION");
    if (snap) {
        str << " Snap " << snap;
    }
    str << '\n';
    str << "Build date: " << buildDate << "\n";

#if defined(_DEBUG) || defined(DEBUG)
    str << "Build type: Debug\n";
#elif defined(NDEBUG)
    str << "Build type: Release\n";
#elif defined(CMAKE_BUILD_TYPE)
    str << "Build type: " << CMAKE_BUILD_TYPE << '\n';
#else
    str << "Build type: Unknown\n";
#endif
    const QString buildRevisionBranch = getValueOrEmpty(mConfig, "BuildRevisionBranch");
    if (!buildRevisionBranch.isEmpty()) {
        str << "Branch: " << buildRevisionBranch << '\n';
    }
    const QString buildRevisionHash = getValueOrEmpty(mConfig, "BuildRevisionHash");
    if (!buildRevisionHash.isEmpty()) {
        str << "Hash: " << buildRevisionHash << '\n';
    }
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python " << PY_VERSION << ", ";
    str << "Qt " << QT_VERSION_STR << ", ";
    str << "Coin " << fcCoin3dVersion << ", ";
    str << "Vtk " << fcVtkVersion << ", ";
    str << "boost " << BOOST_LIB_VERSION << ", ";
    str << "Eigen3 " << fcEigen3Version << ", ";
    str << "PySide " << fcPysideVersion << '\n';
    str << "shiboken " << fcShibokenVersion << ", ";
#ifdef SMESH_VERSION_STR
    str << "SMESH " << SMESH_VERSION_STR << ", ";
#endif
    str << "xerces-c " << fcXercescVersion << ", ";

    try {
        Base::PyGILStateLocker lock;
        Py::Module module(PyImport_ImportModule("ifcopenshell"), true);
        if (!module.isNull() && module.hasAttr("version")) {
            Py::String version(module.getAttr("version"));
            auto ver_str = static_cast<std::string>(version);
            str << "IfcOpenShell " << ver_str.c_str() << ", ";
        }
        else {
            Base::Console().log("Module 'ifcopenshell' not found (safe to ignore, unless using "
                                "the BIM workbench and IFC).\n");
        }
    }
    catch (const Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException e;
        Base::Console().log("%s\n", e.what());
    }

#if defined(HAVE_OCC_VERSION)
    str << "OCC " << OCC_VERSION_MAJOR << "." << OCC_VERSION_MINOR << "." << OCC_VERSION_MAINTENANCE
#ifdef OCC_VERSION_DEVELOPMENT
        << "." OCC_VERSION_DEVELOPMENT
#endif
        << '\n';
#endif
    QLocale loc;
    str << "Locale: " << QLocale::languageToString(loc.language()) << "/"
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        << QLocale::countryToString(loc.country())
#else
        << QLocale::territoryToString(loc.territory())
#endif
        << " (" << loc.name() << ")";
    if (loc != QLocale::system()) {
        loc = QLocale::system();
        str << " [ OS: " << QLocale::languageToString(loc.language()) << "/"
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
            << QLocale::countryToString(loc.country())
#else
            << QLocale::territoryToString(loc.territory())
#endif
            << " (" << loc.name() << ") ]";
    }
    str << "\n";

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    QString navStyle = QString::fromStdString(hGrp->GetASCII("NavigationStyle", "Gui::CADNavigationStyle"));
    // All navigation styles are named on the format "Gui::<Name>NavigationStyle"
    // so we remove the "Gui::" prefix and the "NavigationStyle" suffix before printing.
    navStyle.replace(QRegularExpression(QLatin1String("^Gui::")), {});
    navStyle.replace(QRegularExpression(QLatin1String("NavigationStyle$")), {});

    const QString orbitStyle = QStringLiteral("Turntable,Trackball,Free Turntable,Trackball Classic,Rounded Arcball")
                               .split(QLatin1Char(','))
                               .at(hGrp->GetInt("OrbitStyle", 4));
    const QString rotMode = QStringLiteral("Window center,Drag at cursor,Object center")
                            .split(QLatin1Char(','))
                            .at(hGrp->GetInt("RotationMode", 0));

    str << QStringLiteral("Navigation Style/Orbit Style/Rotation Mode: %1/%2/%3\n").arg(navStyle, orbitStyle, rotMode);
}

void Application::getVerboseAddOnsInfo(QTextStream& str, const std::map<std::string,std::string>& mConfig) {
    // Add installed module information:
    const auto modDir = fs::path(Application::getUserAppDataDir()) / "Mod";
    bool firstMod = true;
    if (fs::exists(modDir) && fs::is_directory(modDir)) {
        for (const auto& mod : fs::directory_iterator(modDir)) {
            if (!fs::is_directory(mod)) {
                continue; // Ignore files, only show directories
            }
            auto dirName = mod.path().string();
            addModuleInfo(str, QString::fromStdString(dirName), firstMod);
        }
    }
    const QString additionalModules = getValueOrEmpty(mConfig, "AdditionalModulePaths");

    if (!additionalModules.isEmpty()) {
        auto mods = additionalModules.split(QChar::fromLatin1(';'));
        for (const auto& mod : mods) {
            addModuleInfo(str, mod, firstMod);
        }
    }
}
