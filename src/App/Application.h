/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *
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

#ifndef APP_APPLICATION_H
#define APP_APPLICATION_H

#include <boost/signals2.hpp>

#include <vector>
#include <deque>

#include <Base/PyObjectBase.h>
#include <Base/Parameter.h>
#include <Base/Observer.h>


namespace Base
{
    class ConsoleObserverStd;
    class ConsoleObserverFile;
}

namespace App
{

class Document;
class DocumentObject;
class ApplicationObserver;
class Property;

enum GetLinkOption {
    /// Get all links (both directly and in directly) linked to the given object
    GetLinkRecursive = 1,
    /// Get link array instead of the array element
    GetLinkArray = 2,
};


/** The Application
 *  The root of the whole application
 *  @see App::Document
 */
class AppExport Application
{

public:

    //---------------------------------------------------------------------
    // exported functions goes here +++++++++++++++++++++++++++++++++++++++
    //---------------------------------------------------------------------

    /** @name methods for document handling */
    //@{
    /** Creates a new document
     * The first name is a the identifier and some kind of an internal (english)
     * name. It has to be like an identifier in a programming language, with no
     * spaces and not starting with a number. This name gets also forced to be unique
     * in this Application. You can avoid the renaming by using getUniqueDocumentName()
     * to get a unique name before calling newDoucument().
     * The second name is a UTF8 name of any kind. It's that name normally shown to
     * the user and stored in the App::Document::Name property.
     */
    App::Document* newDocument(const char * Name=0l, const char * UserName=0l);
    /// Closes the document \a name and removes it from the application.
    bool closeDocument(const char* name);
    /// find a unique document name
    std::string getUniqueDocumentName(const char *Name) const;
    /// Open an existing document from a file
    App::Document* openDocument(const char * FileName=0l);
    /** Open multiple documents
     *
     * @param filenames: input file names
     * @param pathes: optional input file path in case it is different from
     * filenames (mainly used during recovery).
     * @param labels: optional label assign to document (mainly used during recovery).
     * @param errs: optional output error message corresponding to each input
     * file name. If errs is given, this function will catch all
     * Base::Exception and save the error message inside. Otherwise, it will
     * throw on exception when opening the input files.
     *
     * @return Return opened document object corresponding to each input file
     * name, which maybe NULL if failed.
     *
     * This function will also open any external referenced files.
     */
    std::vector<Document*> openDocuments(const std::vector<std::string> &filenames, 
            const std::vector<std::string> *pathes=0,
            const std::vector<std::string> *labels=0,
            std::vector<std::string> *errs=0);
    /// Retrieve the active document
    App::Document* getActiveDocument(void) const;
    /// Retrieve a named document
    App::Document* getDocument(const char *Name) const;
    /// gets the (internal) name of the document
    const char * getDocumentName(const App::Document* ) const;
    /// get a list of all documents in the application
    std::vector<App::Document*> getDocuments() const;
    /// Set the active document
    void setActiveDocument(App::Document* pDoc);
    void setActiveDocument(const char *Name);
    /// close all documents (without saving)
    void closeAllDocuments(void);
    /// Add pending document to open together with the current opening document
    int addPendingDocument(const char *FileName, const char *objName, bool allowPartial);
    /// Indicate whether the application is opening (restoring) some document
    bool isRestoring() const;
    //@}
    
    /** @name Application-wide trandaction setting */
    //@{
    /** Setup a pending application-wide active transaction
     *
     * @param name: new transaction name
     *
     * @return The new transaction ID.
     *
     * Call this function to setup an application-wide transaction. All current
     * pending transactions of opening documents will be commited first.
     * However, no new transaction is created by this call. Any subsequent
     * changes in any current opening document will auto create a transaction
     * with the given name and ID. If more than one document is changed, the
     * transactions will share the same ID, and will be undo/redo together.
     */
    int setActiveTransaction(const char *name);
    /// Return the current active transaction name and ID
    const char *getActiveTransaction(int *tid=0) const;
    /** Commit/abort current active transactions
     *
     * @param abort: whether to abort or commit the transactions
     *
     * Bsides calling this function directly, it will be called by automatically
     * if 1) any new transaction is created with a different ID, or 2) any
     * transaction with the current active transaction ID is either commited or
     * aborted
     */
    void closeActiveTransaction(bool abort=false, int id=0);
    /** Return auto transaction parameter setting
     *
     * When enabled, any transaction created on non-active document will create
     * a new transaction in the active document if there isn't one open.
     */
    bool autoTransaction();
    //@}

    /** @name Signals of the Application */
    //@{
    /// signal on new Document
    boost::signals2::signal<void (const Document&)> signalNewDocument;
    /// signal on document getting deleted
    boost::signals2::signal<void (const Document&)> signalDeleteDocument;
    /// signal on already deleted Document
    boost::signals2::signal<void ()> signalDeletedDocument;
    /// signal on relabeling Document (user name)
    boost::signals2::signal<void (const Document&)> signalRelabelDocument;
    /// signal on renaming Document (internal name)
    boost::signals2::signal<void (const Document&)> signalRenameDocument;
    /// signal on activating Document
    boost::signals2::signal<void (const Document&)> signalActiveDocument;
    /// signal on saving Document
    boost::signals2::signal<void (const Document&)> signalSaveDocument;
    /// signal on starting to restore Document
    boost::signals2::signal<void (const Document&)> signalStartRestoreDocument;
    /// signal on restoring Document
    boost::signals2::signal<void (const Document&)> signalFinishRestoreDocument;
    /// signal on starting to save Document
    boost::signals2::signal<void (const Document&, const std::string&)> signalStartSaveDocument;
    /// signal on saved Document
    boost::signals2::signal<void (const Document&, const std::string&)> signalFinishSaveDocument;
    /// signal on undo in document
    boost::signals2::signal<void (const Document&)> signalUndoDocument;
    /// signal on redo in document
    boost::signals2::signal<void (const Document&)> signalRedoDocument;
    /// signal on transaction abort in document
    boost::signals2::signal<void (const Document&)> signalTransactionAbort;
    /// signal on show hidden items
    boost::signals2::signal<void (const Document&)> signalShowHidden;
    /// signal on start opening document(s)
    boost::signals2::signal<void ()> signalStartOpenDocument;
    /// signal on finished opening document(s)
    boost::signals2::signal<void ()> signalFinishOpenDocument;
    //@}


    /** @name Signals of the document
     * This signals are an aggregation of all document. If you only
     * the signal of a special document connect to the document itself
     */
    //@{
    /// signal before change of doc property
    boost::signals2::signal<void (const App::Document&, const App::Property&)> signalBeforeChangeDocument;
    /// signal on changed doc property
    boost::signals2::signal<void (const App::Document&, const App::Property&)> signalChangedDocument;
    /// signal on new Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalNewObject;
    //boost::signals2::signal<void (const App::DocumentObject&)>     m_sig;
    /// signal on deleted Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalDeletedObject;
    /// signal on changed Object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalBeforeChangeObject;
    /// signal on changed Object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalChangedObject;
    /// signal on relabeled Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalRelabelObject;
    /// signal on activated Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalActivatedObject;
    /// signal before recomputed document
    boost::signals2::signal<void (const App::Document&)> signalBeforeRecomputeDocument;
    /// signal on recomputed document
    boost::signals2::signal<void (const App::Document&)> signalRecomputed;
    /// signal on recomputed document object
    boost::signals2::signal<void (const App::DocumentObject&)> signalObjectRecomputed;
    // signal on opened transaction
    boost::signals2::signal<void (const App::Document&, std::string)> signalOpenTransaction;
    // signal a committed transaction
    boost::signals2::signal<void (const App::Document&)> signalCommitTransaction;
    // signal an aborted transaction
    boost::signals2::signal<void (const App::Document&)> signalAbortTransaction;
    //@}

    /** @name Signals of property changes
     * These signals are emitted on property additions or removal.
     * The changed object can be any sub-class of PropertyContainer.
     */
    //@{
    /// signal on adding a dynamic property
    boost::signals2::signal<void (const App::Property&)> signalAppendDynamicProperty;
    /// signal on about removing a dynamic property
    boost::signals2::signal<void (const App::Property&)> signalRemoveDynamicProperty;
    /// signal on about changing the editor mode of a property
    boost::signals2::signal<void (const App::Property&)> signalChangePropertyEditor;
    //@}


    /** @name methods for parameter handling */
    //@{
    /// returns the system parameter
    ParameterManager &                                GetSystemParameter(void) ;
    /// returns the user parameter
    ParameterManager &                                GetUserParameter(void) ;
    /** Gets a parameter group by a full qualified path
     * It's an easy method to get a group:
     * \code
     * // getting standard parameter
     * ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Raytracing");
     * std::string cDir             = hGrp->GetASCII("ProjectPath", "");
     * std::string cCameraName      = hGrp->GetASCII("CameraName", "TempCamera.inc");
     * \endcode
     */
    Base::Reference<ParameterGrp>                     GetParameterGroupByPath(const char* sName);

    ParameterManager *                                GetParameterSet(const char* sName) const;
    const std::map<std::string,ParameterManager *> &  GetParameterSetList(void) const;
    void AddParameterSet(const char* sName);
    void RemoveParameterSet(const char* sName);
    //@}

    /** @name methods for the open handler
     *  With this facility a Application module can register
     *  a ending (filetype) which he can handle to open.
     *  The ending and the module name are stored and if the file
     *  type is opened the module get loaded and need to register a
     *  OpenHandler class in the OpenHandlerFactorySingleton.
     *  After the module is loaded a OpenHandler of this type is created
     *  and the file get loaded.
     *  @see OpenHandler
     *  @see OpenHandlerFactorySingleton
     */
    //@{
    /// Register an import filetype and a module name
    void addImportType(const char* Type, const char* ModuleName);
    /// Return a list of modules that support the given filetype.
    std::vector<std::string> getImportModules(const char* Type) const;
    /// Return a list of all modules.
    std::vector<std::string> getImportModules() const;
    /// Return a list of filetypes that are supported by a module.
    std::vector<std::string> getImportTypes(const char* Module) const;
    /// Return a list of all filetypes.
    std::vector<std::string> getImportTypes(void) const;
    /// Return the import filters with modules of a given filetype.
    std::map<std::string, std::string> getImportFilters(const char* Type) const;
    /// Return a list of all import filters.
    std::map<std::string, std::string> getImportFilters(void) const;
    //@}
    //@{
    /// Register an export filetype and a module name
    void addExportType(const char* Type, const char* ModuleName);
    /// Return a list of modules that support the given filetype.
    std::vector<std::string> getExportModules(const char* Type) const;
    /// Return a list of all modules.
    std::vector<std::string> getExportModules() const;
    /// Return a list of filetypes that are supported by a module.
    std::vector<std::string> getExportTypes(const char* Module) const;
    /// Return a list of all filetypes.
    std::vector<std::string> getExportTypes(void) const;
    /// Return the export filters with modules of a given filetype.
    std::map<std::string, std::string> getExportFilters(const char* Type) const;
    /// Return a list of all export filters.
    std::map<std::string, std::string> getExportFilters(void) const;
    //@}

    /** @name Init, Destruct an Access methods */
    //@{
    static void init(int argc, char ** argv);
    static void initTypes(void);
    static void destruct(void);
    static void destructObserver(void);
    static void processCmdLineFiles(void);
    static std::list<std::string> getCmdLineFiles();
    static std::list<std::string> processFiles(const std::list<std::string>&);
    static void runApplication(void);
    friend Application &GetApplication(void);
    static std::map<std::string,std::string> &Config(void){return mConfig;}
    static int GetARGC(void){return _argc;}
    static char** GetARGV(void){return _argv;}
    //@}

    /** @name Application directories */
    //@{
    const char* getHomePath(void) const;
    const char* getExecutableName(void) const;
    /*!
     Returns the temporary directory. By default, this is set to the
     system's temporary directory but can be customized by the user.
     */
    static std::string getTempPath();
    static std::string getTempFileName(const char* FileName=0);
    static std::string getUserAppDataDir();
    static std::string getUserMacroDir();
    static std::string getResourceDir();
    static std::string getHelpDir();
    //@}

    /** @name Link handling */
    //@{
    int checkLinkDepth(int depth, bool no_exception=true);

    std::set<DocumentObject*> getLinksTo(
            const DocumentObject *, int options, int maxCount=0) const;

    bool hasLinksTo(const DocumentObject *obj) const {
        return !getLinksTo(obj,GetLinkArray,1).empty();
    }
    //@}

    friend class App::Document;

protected:
    /// get called by the document when the name is changing
    void renameDocument(const char *OldName, const char *NewName);

    /** @name I/O of the document
     * This slot get connected to all App::Documents created
     */
    //@{
    void slotBeforeChangeDocument(const App::Document&, const App::Property&);
    void slotChangedDocument(const App::Document&, const App::Property&);
    void slotNewObject(const App::DocumentObject&);
    void slotDeletedObject(const App::DocumentObject&);
    void slotBeforeChangeObject(const App::DocumentObject&, const App::Property& Prop);
    void slotChangedObject(const App::DocumentObject&, const App::Property& Prop);
    void slotRelabelObject(const App::DocumentObject&);
    void slotActivatedObject(const App::DocumentObject&);
    void slotUndoDocument(const App::Document&);
    void slotRedoDocument(const App::Document&);
    void slotRecomputedObject(const App::DocumentObject&);
    void slotRecomputed(const App::Document&);
    void slotBeforeRecompute(const App::Document&);
    void slotOpenTransaction(const App::Document&, std::string);
    void slotCommitTransaction(const App::Document&);
    void slotAbortTransaction(const App::Document&);
    void slotStartSaveDocument(const App::Document&, const std::string&);
    void slotFinishSaveDocument(const App::Document&, const std::string&);
    //@}

    /// open single document only
    App::Document* openDocumentPrivate(const char * FileName, const char *propFileName,
            const char *label, bool isMainDoc, const std::set<std::string> &objNames);

private:
    /// Constructor
    Application(std::map<std::string,std::string> &mConfig);
    /// Destructor
    virtual ~Application();

    /** @name member for parameter */
    //@{
    static ParameterManager *_pcSysParamMngr;
    static ParameterManager *_pcUserParamMngr;
    //@}

    //---------------------------------------------------------------------
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++
    //---------------------------------------------------------------------

    // static python wrapper of the exported functions
    static PyObject* sGetParam          (PyObject *self,PyObject *args);
    static PyObject* sSaveParameter     (PyObject *self,PyObject *args);
    static PyObject* sGetVersion        (PyObject *self,PyObject *args);
    static PyObject* sGetConfig         (PyObject *self,PyObject *args);
    static PyObject* sSetConfig         (PyObject *self,PyObject *args);
    static PyObject* sDumpConfig        (PyObject *self,PyObject *args);
    static PyObject* sTemplateAdd       (PyObject *self,PyObject *args);
    static PyObject* sTemplateDelete    (PyObject *self,PyObject *args);
    static PyObject* sTemplateGet       (PyObject *self,PyObject *args);
    static PyObject* sAddImportType     (PyObject *self,PyObject *args);
    static PyObject* sGetImportType     (PyObject *self,PyObject *args);
    static PyObject* sAddExportType     (PyObject *self,PyObject *args);
    static PyObject* sGetExportType     (PyObject *self,PyObject *args);
    static PyObject* sGetResourceDir    (PyObject *self,PyObject *args);
    static PyObject* sGetUserAppDataDir (PyObject *self,PyObject *args);
    static PyObject* sGetUserMacroDir   (PyObject *self,PyObject *args);
    static PyObject* sGetHelpDir        (PyObject *self,PyObject *args);
    static PyObject* sGetHomePath       (PyObject *self,PyObject *args);

    static PyObject* sLoadFile          (PyObject *self,PyObject *args);
    static PyObject* sOpenDocument      (PyObject *self,PyObject *args);
    static PyObject* sSaveDocument      (PyObject *self,PyObject *args);
    static PyObject* sSaveDocumentAs    (PyObject *self,PyObject *args);
    static PyObject* sNewDocument       (PyObject *self,PyObject *args);
    static PyObject* sCloseDocument     (PyObject *self,PyObject *args);
    static PyObject* sActiveDocument    (PyObject *self,PyObject *args);
    static PyObject* sSetActiveDocument (PyObject *self,PyObject *args);
    static PyObject* sGetDocument       (PyObject *self,PyObject *args);
    static PyObject* sListDocuments     (PyObject *self,PyObject *args);
    static PyObject* sAddDocObserver    (PyObject *self,PyObject *args);
    static PyObject* sRemoveDocObserver (PyObject *self,PyObject *args);
    static PyObject* sTranslateUnit     (PyObject *self,PyObject *args);
    static PyObject *sIsRestoring       (PyObject *self,PyObject *args);

    static PyObject *sSetLogLevel       (PyObject *self,PyObject *args);
    static PyObject *sGetLogLevel       (PyObject *self,PyObject *args);

    static PyObject *sCheckLinkDepth    (PyObject *self,PyObject *args);
    static PyObject *sGetLinksTo        (PyObject *self,PyObject *args);

    static PyObject *sGetDependentObjects(PyObject *self,PyObject *args);

    static PyObject *sSetActiveTransaction  (PyObject *self,PyObject *args);
    static PyObject *sGetActiveTransaction  (PyObject *self,PyObject *args);
    static PyObject *sCloseActiveTransaction(PyObject *self,PyObject *args);
    static PyObject *sAutoTransaction       (PyObject *self,PyObject *args);

    static PyObject *sDumpSWIG(PyObject *self,PyObject *args);

    static PyObject *sCheckAbort(PyObject *self,PyObject *args);

    static PyMethodDef    Methods[]; 

    friend class ApplicationObserver;

    /** @name  Private Init, Destruct an Access methods */
    //@{
    static void initConfig(int argc, char ** argv);
    static void initApplication(void);
    static void logStatus(void);
    // the one and only pointer to the application object
    static Application *_pcSingleton;
    /// argument helper function
    static void ParseOptions(int argc, char ** argv);
    /// checks if the environment is allreight
    //static void CheckEnv(void);
    /// Search for the FreeCAD home path based on argv[0]
    /*!
     * There are multiple implementations of this method per-OS
     */
    static std::string FindHomePath(const char* sCall);
    /// Print the help message
    static void PrintInitHelp(void);
    /// figure out some things
    static void ExtractUserPath();
    /// load the user and system parameter set
    static void LoadParameters(void);
    /// puts the given env variable in the config
    static void SaveEnv(const char *);
    /// startup configuration container
    static std::map<std::string,std::string> mConfig;
    static int _argc;
    static char ** _argv;
    //@}

    struct FileTypeItem {
        std::string filter;
        std::string module;
        std::vector<std::string> types;
    };

    /// open ending information
    std::vector<FileTypeItem> _mImportTypes;
    std::vector<FileTypeItem> _mExportTypes;
    std::map<std::string,Document*> DocMap;
    std::map<std::string,ParameterManager *> mpcPramManager;
    std::map<std::string,std::string> &_mConfig;
    App::Document* _pActiveDoc;

    std::deque<const char *> _pendingDocs;
    std::deque<const char *> _pendingDocsReopen;
    std::map<std::string,std::set<std::string> > _pendingDocMap;
    bool _isRestoring;
    bool _allowPartial;

    // for estimate max link depth
    int _objCount;

    std::string _activeTransactionName;
    int _activeTransactionID;

    static Base::ConsoleObserverStd  *_pConsoleObserverStd;
    static Base::ConsoleObserverFile *_pConsoleObserverFile;
};

/// Singleton getter of the Application
inline App::Application &GetApplication(void){
    return *App::Application::_pcSingleton;
}

class AppExport AutoTransaction {
public:
    AutoTransaction(const char *name) :tid(0) {
        if(!GetApplication().getActiveTransaction())
            tid = GetApplication().setActiveTransaction(name);
    }
    ~AutoTransaction() {
        close(false);
    }

    void close(bool abort=false) {
        int id = 0;
        if(tid && GetApplication().getActiveTransaction(&id) && tid==id)
            GetApplication().closeActiveTransaction(abort);
    }

    int tid;
};

} // namespace App


#endif // APP_APPLICATION_H
