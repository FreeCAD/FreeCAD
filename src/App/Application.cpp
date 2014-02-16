/***************************************************************************
 *   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
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
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/



#include "PreCompiled.h"

#ifndef _PreComp_
# include <iostream>
# include <sstream>
# include <exception>
# if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
# include <unistd.h>
# include <pwd.h>
# include <sys/types.h>
# endif
# include <ctime>
# include <csignal>
# include <boost/program_options.hpp>
#endif

#ifdef FC_OS_WIN32
# include <Shlobj.h>
// Doesn't seem to work with VS2010
# if (defined(_MSC_VER) && (_MSC_VER < 1600))
# include <Shfolder.h>
# endif
#endif



#include "Application.h"
#include "Document.h"

// FreeCAD Base header
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <Base/Factory.h>
#include <Base/FileInfo.h>
#include <Base/Type.h>
#include <Base/BaseClass.h>
#include <Base/Persistence.h>
#include <Base/Reader.h>
#include <Base/MatrixPy.h>
#include <Base/VectorPy.h>
#include <Base/AxisPy.h>
#include <Base/BoundBoxPy.h>
#include <Base/PlacementPy.h>
#include <Base/RotationPy.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/QuantityPy.h>
#include <Base/UnitPy.h>

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
#include "Document.h"
#include "DocumentObjectGroup.h"
#include "DocumentObjectFileIncluded.h"
#include "InventorObject.h"
#include "VRMLObject.h"
#include "Annotation.h"
#include "MeasureDistance.h"
#include "Placement.h"
#include "Plane.h"
#include "MaterialObject.h"

// If you stumble here, run the target "BuildExtractRevision" on Windows systems
// or the Python script "SubWCRev.py" on Linux based systems which builds
// src/Build/Version.h. Or create your own from src/Build/Version.h.in!
#include <Build/Version.h>

#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>
#include <boost/signals.hpp>
#include <boost/bind.hpp>
#include <boost/version.hpp>
#include <QDir>

using namespace App;
using namespace std;
using namespace boost;
using namespace boost::program_options;


// scriptings (scripts are build in but can be overridden by command line option)
#include "InitScript.h"
#include "TestScript.h"

#ifdef _MSC_VER // New handler for Microsoft Visual C++ compiler
# include <new.h>
#else // Ansi C/C++ new handler
# include <new>
#endif

#ifdef MemDebugOn
# define new DEBUG_CLIENTBLOCK
#endif


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

Application::Application(ParameterManager * /*pcSysParamMngr*/,
                         ParameterManager * /*pcUserParamMngr*/,
                         std::map<std::string,std::string> &mConfig)
    ://_pcSysParamMngr(pcSysParamMngr),
    //_pcUserParamMngr(pcUserParamMngr),
    _mConfig(mConfig),
    _pActiveDoc(0)
{
    //_hApp = new ApplicationOCC;
    mpcPramManager["System parameter"] = _pcSysParamMngr;
    mpcPramManager["User parameter"] = _pcUserParamMngr;


    // setting up Python binding
    Base::PyGILStateLocker lock;
    PyObject* pAppModule = Py_InitModule3("FreeCAD", Application::Methods, FreeCAD_doc);
    Py::Module(pAppModule).setAttr(std::string("ActiveDocument"),Py::None());

    PyObject* pConsoleModule = Py_InitModule3("__FreeCADConsole__", ConsoleSingleton::Methods, Console_doc);

    // introducing additional classes

    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    if (PyType_Ready(&Base::VectorPy::Type) < 0) return;
    union PyType_Object pyVecType = {&Base::VectorPy::Type};
    PyModule_AddObject(pAppModule, "Vector", pyVecType.o);

    if (PyType_Ready(&Base::MatrixPy::Type) < 0) return;
    union PyType_Object pyMtxType = {&Base::MatrixPy::Type};
    PyModule_AddObject(pAppModule, "Matrix", pyMtxType.o);

    if (PyType_Ready(&Base::BoundBoxPy::Type) < 0) return;
    union PyType_Object pyBoundBoxType = {&Base::BoundBoxPy::Type};
    PyModule_AddObject(pAppModule, "BoundBox", pyBoundBoxType.o);

    if (PyType_Ready(&Base::PlacementPy::Type) < 0) return;
    union PyType_Object pyPlacementPyType = {&Base::PlacementPy::Type};
    PyModule_AddObject(pAppModule, "Placement", pyPlacementPyType.o);

    if (PyType_Ready(&Base::RotationPy::Type) < 0) return;
    union PyType_Object pyRotationPyType = {&Base::RotationPy::Type};
    PyModule_AddObject(pAppModule, "Rotation", pyRotationPyType.o);

    if (PyType_Ready(&Base::AxisPy::Type) < 0) return;
    union PyType_Object pyAxisPyType = {&Base::AxisPy::Type};
    PyModule_AddObject(pAppModule, "Axis", pyAxisPyType.o);

    // Note: Create an own module 'Base' which should provide the python
    // binding classes from the base module. At a later stage we should
    // remove these types from the FreeCAD module.
    PyObject* pBaseModule = Py_InitModule3("__FreeCADBase__", NULL,
        "The Base module contains the classes for the geometric basics\n"
        "like vector, matrix, bounding box, placement, rotation, axis, ...");
    Base::Interpreter().addType(&Base::VectorPy     ::Type,pBaseModule,"Vector");
    Base::Interpreter().addType(&Base::MatrixPy     ::Type,pBaseModule,"Matrix");
    Base::Interpreter().addType(&Base::BoundBoxPy   ::Type,pBaseModule,"BoundBox");
    Base::Interpreter().addType(&Base::PlacementPy  ::Type,pBaseModule,"Placement");
    Base::Interpreter().addType(&Base::RotationPy   ::Type,pBaseModule,"Rotation");
    Base::Interpreter().addType(&Base::AxisPy       ::Type,pBaseModule,"Axis");

    //insert Base and Console
    Py_INCREF(pBaseModule);
    PyModule_AddObject(pAppModule, "Base", pBaseModule);
    Py_INCREF(pConsoleModule);
    PyModule_AddObject(pAppModule, "Console", pConsoleModule);

    //insert Units module
    PyObject* pUnitsModule = Py_InitModule3("Units", Base::UnitsApi::Methods,
          "The Unit API");
    Base::Interpreter().addType(&Base::QuantityPy  ::Type,pUnitsModule,"Quantity");
    Base::Interpreter().addType(&Base::UnitPy      ::Type,pUnitsModule,"Unit");

    Py_INCREF(pUnitsModule);
    PyModule_AddObject(pAppModule, "Units", pUnitsModule);

    Base::ProgressIndicatorPy::init_type();
    Base::Interpreter().addType(Base::ProgressIndicatorPy::type_object(),
        pBaseModule,"ProgressIndicator");
}

Application::~Application()
{
}

//**************************************************************************
// Interface

/// get called by the document when the name is changing
void Application::renameDocument(const char *OldName, const char *NewName)
{
    std::map<std::string,Document*>::iterator pos;
    pos = DocMap.find(OldName);

    if (pos != DocMap.end()) {
        Document* temp;
        temp = pos->second;
        DocMap.erase(pos);
        DocMap[NewName] = temp;
        signalRenameDocument(*temp);
    }
    else
        Base::Exception("Application::renameDocument(): no document with this name to rename!");

}

Document* Application::newDocument(const char * Name, const char * UserName)
{
    // get anyway a valid name!
    if (!Name || Name[0] == '\0')
        Name = "Unnamed";
    string name = getUniqueDocumentName(Name);

    // create the FreeCAD document
    auto_ptr<Document> newDoc(new Document() );

    // add the document to the internal list
    DocMap[name] = newDoc.release(); // now owned by the Application
    _pActiveDoc = DocMap[name];


    // connect the signals to the application for the new document
    _pActiveDoc->signalNewObject.connect(boost::bind(&App::Application::slotNewObject, this, _1));
    _pActiveDoc->signalDeletedObject.connect(boost::bind(&App::Application::slotDeletedObject, this, _1));
    _pActiveDoc->signalChangedObject.connect(boost::bind(&App::Application::slotChangedObject, this, _1, _2));
    _pActiveDoc->signalRenamedObject.connect(boost::bind(&App::Application::slotRenamedObject, this, _1));
    _pActiveDoc->signalActivatedObject.connect(boost::bind(&App::Application::slotActivatedObject, this, _1));
    _pActiveDoc->signalUndo.connect(boost::bind(&App::Application::slotUndoDocument, this, _1));
    _pActiveDoc->signalRedo.connect(boost::bind(&App::Application::slotRedoDocument, this, _1));

    // make sure that the active document is set in case no GUI is up
    {
        Base::PyGILStateLocker lock;
        Py::Object active(_pActiveDoc->getPyObject(), true);
        Py::Module("FreeCAD").setAttr(std::string("ActiveDocument"),active);
    }

    signalNewDocument(*_pActiveDoc);

    // set the UserName after notifying all observers
    if (UserName)
        _pActiveDoc->Label.setValue(UserName);
    else
        _pActiveDoc->Label.setValue(name);

    return _pActiveDoc;
}

bool Application::closeDocument(const char* name)
{
    map<string,Document*>::iterator pos = DocMap.find( name );
    if (pos == DocMap.end()) // no such document
        return false;

    // Trigger observers before removing the document from the internal map.
    // Some observers might rely on that this document is still there.
    signalDeleteDocument(*pos->second);

    // For exception-safety use a smart pointer
    if (_pActiveDoc == pos->second)
        setActiveDocument((Document*)0);
    auto_ptr<Document> delDoc (pos->second);
    DocMap.erase( pos );

    // Trigger observers after removing the document from the internal map.
    signalDeletedDocument();

    return true;
}

void Application::closeAllDocuments(void)
{
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

std::string Application::getUniqueDocumentName(const char *Name) const
{
    if (!Name || *Name == '\0')
        return std::string();
    std::string CleanName = Base::Tools::getIdentifier(Name);

    // name in use?
    std::map<string,Document*>::const_iterator pos;
    pos = DocMap.find(CleanName);

    if (pos == DocMap.end()) {
        // if not, name is OK
        return CleanName;
    }
    else {
        std::vector<std::string> names;
        names.reserve(DocMap.size());
        for (pos = DocMap.begin();pos != DocMap.end();++pos) {
            names.push_back(pos->first);
        }
        return Base::Tools::getUniqueName(CleanName, names);
    }
}

Document* Application::openDocument(const char * FileName)
{
    FileInfo File(FileName);

    if (!File.exists()) {
        std::stringstream str;
        str << "File '" << FileName << "' does not exist!";
        throw Base::Exception(str.str().c_str());
    }

    // Before creating a new document we check whether the document is already open
    std::string filepath = File.filePath();
    for (std::map<std::string,Document*>::iterator it = DocMap.begin(); it != DocMap.end(); ++it) {
        // get unique path separators
        std::string fi = FileInfo(it->second->FileName.getValue()).filePath();
        if (filepath == fi) {
            std::stringstream str;
            str << "The project '" << FileName << "' is already open!";
            throw Base::Exception(str.str().c_str());
        }
    }

    // Use the same name for the internal and user name.
    // The file name is UTF-8 encoded which means that the internal name will be modified
    // to only contain valid ASCII characters but the user name will be kept.
    Document* newDoc = newDocument(File.fileNamePure().c_str(), File.fileNamePure().c_str());

    newDoc->FileName.setValue(File.filePath());

    // read the document
    newDoc->restore();

    return newDoc;
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
    // Allows that no active document is set.
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
        throw Base::Exception(s.str());
    }
}

const char* Application::GetHomePath(void) const
{
    return _mConfig["AppHomePath"].c_str();
}

const char* Application::getExecutableName(void) const
{
    return _mConfig["ExeName"].c_str();
}

std::string Application::getUserAppDataDir()
{
    return mConfig["UserAppData"];
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

    // is there a path seperator ?
    if (pos == std::string::npos) {
        throw Base::Exception("Application::GetParameterGroupByPath() no parameter set name specified");
    }
    // assigning the parameter set name
    cTemp.assign(cName,0,pos);
    cName.erase(0,pos+1);

    // test if name is valid
    std::map<std::string,ParameterManager *>::iterator It = mpcPramManager.find(cTemp.c_str());
    if (It == mpcPramManager.end())
        throw Base::Exception("Application::GetParameterGroupByPath() unknown parameter set name specified");

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

    // Due to branding stuff replace FreeCAD through the application name
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

    // Due to branding stuff replace FreeCAD through the application name
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
void Application::slotNewObject(const App::DocumentObject&O)
{
    this->signalNewObject(O);
}

void Application::slotDeletedObject(const App::DocumentObject&O)
{
    this->signalDeletedObject(O);
}

void Application::slotChangedObject(const App::DocumentObject&O, const App::Property& P)
{
    this->signalChangedObject(O,P);
}

void Application::slotRenamedObject(const App::DocumentObject&O)
{
    this->signalRenamedObject(O);
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

//**************************************************************************
// Init, Destruct and singleton

Application * Application::_pcSingleton = 0;

int Application::_argc;
char ** Application::_argv;


void Application::destruct(void)
{
    // saving system parameter
    Console().Log("Saving system parameter...\n");
    _pcSysParamMngr->SaveDocument(mConfig["SystemParameter"].c_str());
    // saving the User parameter
    Console().Log("Saving system parameter...done\n");
    Console().Log("Saving user parameter...\n");
    _pcUserParamMngr->SaveDocument(mConfig["UserParameter"].c_str());
    Console().Log("Saving user parameter...done\n");
    // clean up
    delete _pcSysParamMngr;
    delete _pcUserParamMngr;

    // not initialized or doubel destruct!
    assert(_pcSingleton);
    delete _pcSingleton;

    // We must detach from console and delete the observer to save our file
    destructObserver();

    Base::Interpreter().finalize();

    ScriptFactorySingleton::Destruct();
    InterpreterSingleton::Destruct();
    Base::Type::destruct();
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
        _pConsoleObserverFile = 0;
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

void segmentation_fault_handler(int sig)
{
    switch (sig) {
        case SIGSEGV:
            std::cerr << "Illegal storage access..." << std::endl;
            break;
        case SIGABRT:
            std::cerr << "Abnormal program termination..." << std::endl;
            break;
        default:
            std::cerr << "Unknown error occurred..." << std::endl;
            break;
    }

#if defined(__GNUC__)
    // According to the documentation to C signals we should exit.
    exit(3);
#endif
}

void unhandled_exception_handler()
{
    std::cerr << "Unhandled exception..." << std::endl;
}

void unexpection_error_handler()
{
    std::cerr << "Unexpected error occurred..." << std::endl;
    terminate();
}


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
#ifdef _MSC_VER // Microsoft compiler
        std::signal(SIGSEGV,segmentation_fault_handler);
        std::signal(SIGABRT,segmentation_fault_handler);
        std::set_terminate(unhandled_exception_handler);
        std::set_unexpected(unexpection_error_handler);
#endif

        initTypes();

#if (BOOST_VERSION < 104600) || (BOOST_FILESYSTEM_VERSION == 2)
        boost::filesystem::path::default_name_check(boost::filesystem::no_check);
#endif

        initConfig(argc,argv);
        initApplication();
    }
    catch (...) {
        // force to flush the log
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
    Base::Persistence               ::init();

    // Complex data classes
    Data::ComplexGeoData            ::init();
    Data::Segment                   ::init();

    // Properties
    App ::Property                  ::init();
    App ::PropertyContainer         ::init();
    App ::PropertyLists             ::init();
    App ::PropertyBool              ::init();
    App ::PropertyFloat             ::init();
    App ::PropertyFloatList         ::init();
    App ::PropertyFloatConstraint   ::init();
    App ::PropertyQuantity          ::init();
    App ::PropertyQuantityConstraint::init();
    App ::PropertyAngle             ::init();
    App ::PropertyDistance          ::init();
    App ::PropertyLength            ::init();
    App ::PropertySpeed             ::init();
    App ::PropertyAcceleration      ::init();
    App ::PropertyForce             ::init();
    App ::PropertyPressure          ::init();
    App ::PropertyInteger           ::init();
    App ::PropertyIntegerConstraint ::init();
    App ::PropertyPercent           ::init();
    App ::PropertyEnumeration       ::init();
    App ::PropertyIntegerList       ::init();
    App ::PropertyIntegerSet        ::init();
    App ::PropertyMap               ::init();
    App ::PropertyString            ::init();
    App ::PropertyUUID              ::init();
    App ::PropertyFont              ::init();
    App ::PropertyStringList        ::init();
    App ::PropertyLink              ::init();
    App ::PropertyLinkSub           ::init();
    App ::PropertyLinkList          ::init();
    App ::PropertyLinkSubList       ::init();
    App ::PropertyMatrix            ::init();
    App ::PropertyVector            ::init();
    App ::PropertyVectorList        ::init();
    App ::PropertyPlacement         ::init();
    App ::PropertyPlacementLink     ::init();
    App ::PropertyGeometry          ::init();
    App ::PropertyComplexGeoData    ::init();
    App ::PropertyColor             ::init();
    App ::PropertyColorList         ::init();
    App ::PropertyMaterial          ::init();
    App ::PropertyPath              ::init();
    App ::PropertyFile              ::init();
    App ::PropertyFileIncluded      ::init();
    App ::PropertyPythonObject      ::init();
    // Document classes
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
    App ::Placement                 ::init();
    App ::Plane                     ::init();
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

    // extract home paths
    ExtractUserPath();

#   ifdef FC_DEBUG
    mConfig["Debug"] = "1";
#   else
    mConfig["Debug"] = "0";
#   endif

    // init python
    mConfig["PythonSearchPath"] = Interpreter().init(argc,argv);

    // Parse the options which have impact to the init process
    ParseOptions(argc,argv);

    // Init console ===========================================================
    Base::PyGILStateLocker lock;
    _pConsoleObserverStd = new ConsoleObserverStd();
    Console().AttachObserver(_pConsoleObserverStd);
    if (mConfig["Verbose"] == "Strict")
        Console().SetMode(ConsoleSingleton::Verbose);

    // file logging Init ===========================================================
    if (mConfig["LoggingFile"] == "1") {
        _pConsoleObserverFile = new ConsoleObserverFile(mConfig["LoggingFileName"].c_str());
        Console().AttachObserver(_pConsoleObserverFile);
    }
    else
        _pConsoleObserverFile = 0;

    // Banner ===========================================================
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

    LoadParameters();

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
    new ScriptProducer( "FreeCADInit",    FreeCADInit    );
    new ScriptProducer( "FreeCADTest",    FreeCADTest    );

    // creating the application
    if (!(mConfig["Verbose"] == "Strict")) Console().Log("Create Application\n");
    Application::_pcSingleton = new Application(0,0,mConfig);

    // set up Unit system default
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
       ("User parameter:BaseApp/Preferences/Units");
    UnitsApi::setSchema((UnitSystem)hGrp->GetInt("UserSchema",0));

    // starting the init script
    Interpreter().runString(Base::ScriptFactory().ProduceScript("FreeCADInit"));
}

void Application::processCmdLineFiles(void)
{
    Base::Console().Log("Init: Processing command line files\n");

    // cycling through all the open files
    unsigned short count = 0;
    count = atoi(mConfig["OpenFileCount"].c_str());
    std::string File;

    if (count == 0 && mConfig["RunMode"] == "Exit")
        mConfig["RunMode"] = "Cmd";

    for (unsigned short i=0; i<count; i++) {
        // getting file name
        std::ostringstream temp;
        temp << "OpenFile" << i;

        FileInfo File(mConfig[temp.str()].c_str());

        std::string Ext = File.extension();
        Base::Console().Log("Init:     Processing file: %s\n",File.filePath().c_str());
        try {

            if (File.hasExtension("fcstd") || File.hasExtension("std")) {
                // try to open
                Application::_pcSingleton->openDocument(File.filePath().c_str());
            }
            else if (File.hasExtension("fcscript")||File.hasExtension("fcmacro")) {
                Base::Interpreter().runFile(File.filePath().c_str(), true);
            }
            else if (File.hasExtension("py")) {
                try {
                    Base::Interpreter().loadModule(File.fileNamePure().c_str());
                }
                catch(const PyException&) {
                    // if module load not work, just try run the script (run in __main__)
                    Base::Interpreter().runFile(File.filePath().c_str(),true);
                }
            }
            else {
                std::vector<std::string> mods = App::GetApplication().getImportModules(Ext.c_str());
                if (!mods.empty()) {
                    Base::Interpreter().loadModule(mods.front().c_str());
                    Base::Interpreter().runStringArg("import %s",mods.front().c_str());
                    Base::Interpreter().runStringArg("%s.open(\"%s\")",mods.front().c_str(),File.filePath().c_str());
                    Base::Console().Log("Command line open: %s.Open(\"%s\")\n",mods.front().c_str(),File.filePath().c_str());
                }
                else {
                    Console().Warning("File format not supported: %s \n", File.filePath().c_str());
                }
            }
        }
        catch (const Base::SystemExitException&) {
            throw; // re-throw to main() function
        }
        catch (const Base::Exception& e) {
            Console().Error("Exception while processing file: %s [%s]\n", File.filePath().c_str(), e.what());
        }
        catch (...) {
            Console().Error("Unknown exception while processing file: %s \n", File.filePath().c_str());
        }
    }

    const std::map<std::string,std::string>& cfg = Application::Config();
    std::map<std::string,std::string>::const_iterator it = cfg.find("SaveFile");
    if (it != cfg.end()) {
        std::string output = it->second;
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
        // Run the comandline interface
        Interpreter().runCommandLine("FreeCAD Console mode");
    }
    else if (mConfig["RunMode"] == "Internal") {
        // run internal script
        Console().Log("Running internal script:\n");
        Interpreter().runString(Base::ScriptFactory().ProduceScript(mConfig["ScriptFileName"].c_str()));
    }
    else if (mConfig["RunMode"] == "Exit") {
        // geting out
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

    for (std::map<std::string,std::string>::iterator It = mConfig.begin();It!= mConfig.end();It++) {
        Console().Log("%s = %s\n",It->first.c_str(),It->second.c_str());
    }
}

void Application::LoadParameters(void)
{
    // create standard parameter sets
    _pcSysParamMngr = new ParameterManager();
    _pcUserParamMngr = new ParameterManager();

    // Init parameter sets ===========================================================
    //
    if (mConfig.find("UserParameter") == mConfig.end())
        mConfig["UserParameter"]   = mConfig["UserAppData"] + "user.cfg";
    if (mConfig.find("SystemParameter") == mConfig.end())
        mConfig["SystemParameter"] = mConfig["UserAppData"] + "system.cfg";


    try {
        if (_pcSysParamMngr->LoadOrCreateDocument(mConfig["SystemParameter"].c_str()) && !(mConfig["Verbose"] == "Strict")) {
            // Configuration file optional when using as Python module
            if (!Py_IsInitialized()) {
                Console().Warning("   Parameter not existing, write initial one\n");
                Console().Message("   This warning normally means that FreeCAD is running the first time\n"
                                  "   or the configuration was deleted or moved. Build up the standard\n"
                                  "   configuration.\n");
            }
        }
    }
    catch (const Base::Exception& e) {
        // try to proceed with an empty XML document
        Base::Console().Error("%s in file %s.\n"
                              "Continue with an empty configuration.",
                              e.what(), mConfig["SystemParameter"].c_str());
        _pcSysParamMngr->CreateDocument();
    }

    try {
        if (_pcUserParamMngr->LoadOrCreateDocument(mConfig["UserParameter"].c_str()) && !(mConfig["Verbose"] == "Strict")) {
            // Configuration file optional when using as Python module
            if (!Py_IsInitialized()) {
                Console().Warning("   User settings not existing, write initial one\n");
                Console().Message("   This warning normally means that FreeCAD is running the first time\n"
                                  "   or your configuration was deleted or moved. The system defaults\n"
                                  "   will be reestablished for you.\n");
            }
        }
    }
    catch (const Base::Exception& e) {
        // try to proceed with an empty XML document
        Base::Console().Error("%s in file %s.\n"
                              "Continue with an empty configuration.",
                              e.what(), mConfig["UserParameter"].c_str());
        _pcUserParamMngr->CreateDocument();
    }
}


#if defined(_MSC_VER)
// fix weird error while linking boost (all versions of VC)
// VS2010: http://forum.freecadweb.org/viewtopic.php?f=4&t=1886&p=12553&hilit=boost%3A%3Afilesystem%3A%3Aget#p12553
namespace boost { namespace program_options { std::string arg="arg"; } }
#if (defined (BOOST_VERSION) && (BOOST_VERSION >= 104100))
namespace boost { namespace program_options {
    const unsigned options_description::m_default_line_length = 80;
} }
#endif
#endif

#if 0 // it seemse the SUSE has fixed the broken boost package
// reported for SUSE in issue #0000208
#if defined(__GNUC__)
#if BOOST_VERSION == 104400
namespace boost { namespace filesystem {
    bool no_check( const std::string & ) { return true; }
} }
#endif
#endif
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
    // allowed only on command line
    options_description generic("Generic options");
    generic.add_options()
    ("version,v", "Prints version string")
    ("help,h", "Prints help message")
    ("console,c", "Starts in console mode")
    ("response-file", value<string>(),"Can be specified with '@name', too")
    ;

    // Declare a group of options that will be
    // allowed both on command line and in
    // config file
    std::string descr("Writes a log file to:\n");
    descr += mConfig["UserAppData"];
    descr += mConfig["ExeName"];
    descr += ".log";
    boost::program_options::options_description config("Configuration");
    config.add_options()
    //("write-log,l", value<string>(), "write a log file")
    ("write-log,l", descr.c_str())
    ("log-file", value<string>(), "Unlike to --write-log this allows to log to an arbitrary file")
    ("user-cfg,u", value<string>(),"User config file to load/save user settings")
    ("system-cfg,s", value<string>(),"Systen config file to load/save system settings")
    ("run-test,t",   value<int>()   ,"Test level")
    ("module-path,M", value< vector<string> >()->composing(),"Additional module paths")
    ("python-path,P", value< vector<string> >()->composing(),"Additional python paths")
    ;


    // Hidden options, will be allowed both on command line and
    // in config file, but will not be shown to the user.
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
    ("visual",     boost::program_options::value< string >(), "set the X-Window to color scema")
    ("ncols",      boost::program_options::value< int    >(), "set the X-Window to color scema")
    ("cmap",                                                  "set the X-Window to color scema")
#if defined(FC_OS_MACOSX)
    ("psn",        boost::program_options::value< string >(), "process serial number")
#endif
    ;

    // Ignored options, will be savely ignored. Mostly uses by underlaying libs.
    //boost::program_options::options_description x11("X11 options");
    //x11.add_options()
    //    ("display",  boost::program_options::value< string >(), "set the X-Server")
    //    ;
    //0000723: improper handling of qt specific comand line arguments
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
        str << "For detailed descripton see http://www.freecadweb.org" << endl<<endl;
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
        int level = vm["run-test"].as<int>();
        switch (level) {
        case '0':
            // test script level 0
            mConfig["RunMode"] = "Internal";
            mConfig["ScriptFileName"] = "FreeCADTest";
            //sScriptName = FreeCADTest;
            break;
        default:
            //default testing level 0
            mConfig["RunMode"] = "Internal";
            mConfig["ScriptFileName"] = "FreeCADTest";
            //sScriptName = FreeCADTest;
            break;
        };
    }

}

void Application::ExtractUserPath()
{
    // std paths
    mConfig["BinPath"] = mConfig["AppHomePath"] + "bin" + PATHSEP;
    mConfig["DocPath"] = mConfig["AppHomePath"] + "doc" + PATHSEP;

#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD)
    // Default paths for the user depending stuff on the platform
    struct passwd *pwd = getpwuid(getuid());
    if (pwd == NULL)
        throw Base::Exception("Getting HOME path from system failed!");
    mConfig["UserHomePath"] = pwd->pw_dir;
    std::string appData = pwd->pw_dir;
    Base::FileInfo fi(appData.c_str());
    if (!fi.exists()) {
        // This should never ever happen
        std::stringstream str;
        str << "Application data directory " << appData << " does not exist!";
        throw Base::Exception(str.str());
    }

    // Try to write into our data path, therefore we must create some directories, first.
    // If 'AppDataSkipVendor' is defined the value of 'ExeVendor' must not be part of
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
                throw Base::Exception(error);
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
            throw Base::Exception(error);
        }
    }

    // Actually the name of the directory where the parameters are stored should be the name of
    // the application due to branding reasons.
    appData += PATHSEP;
    mConfig["UserAppData"] = appData;

#elif defined(FC_OS_MACOSX)
    // Default paths for the user depending stuff on the platform
    struct passwd *pwd = getpwuid(getuid());
    if (pwd == NULL)
        throw Base::Exception("Getting HOME path from system failed!");
    mConfig["UserHomePath"] = pwd->pw_dir;
    std::string appData = pwd->pw_dir;
    appData += PATHSEP;
    appData += "Library";
    appData += PATHSEP;
    appData += "Preferences";
    Base::FileInfo fi(appData.c_str());
    if (!fi.exists()) {
        // This should never ever happen
        std::stringstream str;
        str << "Application data directory " << appData << " does not exist!";
        throw Base::Exception(str.str());
    }

    // Try to write into our data path, therefore we must create some directories, first.
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
                throw Base::Exception(error);
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
            throw Base::Exception(error);
        }
    }

    // Actually the name of the directory where the parameters are stored should be the name of
    // the application due to branding reasons.
    appData += PATHSEP;
    mConfig["UserAppData"] = appData;

#elif defined(FC_OS_WIN32)
    WCHAR szPath[MAX_PATH];
    TCHAR dest[MAX_PATH*3];
    // Get the default path where we can save our documents. It seems that
    // 'CSIDL_MYDOCUMENTS' doesn't work on all machines, so we use 'CSIDL_PERSONAL'
    // which does the same.
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, szPath))) {
        WideCharToMultiByte(CP_UTF8, 0, szPath, -1,dest, 256, NULL, NULL);
        mConfig["UserHomePath"] = dest;
    }
    else
        mConfig["UserHomePath"] = mConfig["AppHomePath"];

    // In the second step we want the directory where user settings of the application can be
    // kept. There we create a directory with name of the vendor and a sub-directory with name
    // of the application.
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        // convert to UTF8
        WideCharToMultiByte(CP_UTF8, 0, szPath, -1,dest, 256, NULL, NULL);

        std::string appData = dest;
        Base::FileInfo fi(appData.c_str());
        if (!fi.exists()) {
            // This should never ever happen
            std::stringstream str;
            str << "Application data directory " << appData << " does not exist!";
            throw Base::Exception(str.str());
        }

        // Try to write into our data path, therefore we must create some directories, first.
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
                    throw Base::Exception(error);
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
                throw Base::Exception(error);
            }
        }

        // Actually the name of the directory where the parameters are stored should be the name of
        // the application due to branding reasons.
        appData += PATHSEP;
        mConfig["UserAppData"] = appData;
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
    // We have three ways to start this application either use one of the both executables or
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
        // Find the path of the executable. Theoretically, there could  occur a
        // race condition when using readlink, but we only use  this method to
        // get the absolute path of the executable to compute the actual home
        // path. In the worst case we simply get q wrong path and FreeCAD is not
        // able to load its modules.
        char resolved[PATH_MAX];
        int nchars = readlink("/proc/self/exe", resolved, PATH_MAX);
        if (nchars < 0 || nchars >= PATH_MAX)
            throw Base::Exception("Cannot determine the absolute path of the executable");
        resolved[nchars] = '\0'; // enfore null termination
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
    uint32_t sz = 0;
    char *buf;

    _NSGetExecutablePath(NULL, &sz); //function only returns "sz" if first arg is to small to hold value
    buf = (char*) malloc(++sz);

    if (_NSGetExecutablePath(buf, &sz) == 0) {
        char resolved[PATH_MAX];
        char* path = realpath(buf, resolved);
        free(buf);

        if (path) {
            std::string Call(resolved), TempHomePath;
            std::string::size_type pos = Call.find_last_of(PATHSEP);
            TempHomePath.assign(Call,0,pos);
            pos = TempHomePath.find_last_of(PATHSEP);
            TempHomePath.assign(TempHomePath,0,pos+1);
            return TempHomePath;
        }
    }

    return call; // error
}

#elif defined (FC_OS_WIN32)
std::string Application::FindHomePath(const char* sCall)
{
    // We have three ways to start this application either use one of the both executables or
    // import the FreeCAD.pyd module from a running Python session. In the latter case the
    // Python interpreter is already initialized.
    char  szFileName [MAX_PATH] ;
    if (Py_IsInitialized()) {
        GetModuleFileName(GetModuleHandle(sCall),szFileName, MAX_PATH-1);
    }
    else {
        GetModuleFileName(0, szFileName, MAX_PATH-1);
    }

    std::string Call(szFileName), TempHomePath;
    std::string::size_type pos = Call.find_last_of(PATHSEP);
    TempHomePath.assign(Call,0,pos);
    pos = TempHomePath.find_last_of(PATHSEP);
    TempHomePath.assign(TempHomePath,0,pos+1);

    // switch to posix style
    for (std::string::iterator i=TempHomePath.begin();i!=TempHomePath.end();++i) {
        if (*i == '\\')
            *i = '/';
    }

    return TempHomePath;
}

#else
# error "std::string Application::FindHomePath(const char*) not implemented"
#endif
