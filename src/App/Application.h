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

#ifndef APP_APPLICATION_H
#define APP_APPLICATION_H

#include <boost_signals2.hpp>

#include <deque>
#include <vector>

#include <Base/Observer.h>
#include <Base/Parameter.h>

// forward declarations
using PyObject = struct _object;
using PyMethodDef = struct PyMethodDef;

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
class AutoTransaction;
class ExtensionContainer;

enum GetLinkOption {
    /// Get all links (both directly and in directly) linked to the given object
    GetLinkRecursive = 1,
    /// Get link array element instead of the array
    GetLinkArrayElement = 2,
    /// Get linked object instead of the link, no effect if GetLinkRecursive
    GetLinkedObject = 4,
    /// Get only external links, no effect if GetLinkRecursive
    GetLinkExternal = 8,
};

enum class MessageOption {
    Quiet, /**< Suppress error. */
    Error, /**< Print an error message. */
    Throw, /**< Throw an exception. */
};


/** The Application
 *  The root of the whole application
 *  @see App::Document
 */
class AppExport Application
{

public:

    //---------------------------------------------------------------------
    // exported functions go here +++++++++++++++++++++++++++++++++++++++++
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
    App::Document* newDocument(const char * Name=nullptr, const char * UserName=nullptr,
            bool createView=true, bool tempDoc=false);
    /// Closes the document \a name and removes it from the application.
    bool closeDocument(const char* name);
    /// find a unique document name
    std::string getUniqueDocumentName(const char *Name, bool tempDoc=false) const;
    /// Open an existing document from a file
    App::Document* openDocument(const char * FileName=nullptr, bool createView=true);
    /** Open multiple documents
     *
     * @param filenames: input file names
     * @param paths: optional input file path in case it is different from
     * filenames (mainly used during recovery).
     * @param labels: optional label assign to document (mainly used during recovery).
     * @param errs: optional output error message corresponding to each input
     * file name. If errs is given, this function will catch all
     * Base::Exception and save the error message inside. Otherwise, it will
     * throw on exception when opening the input files.
     * @param createView: whether to signal Gui module to create view on restore.
     *
     * @return Return opened document object corresponding to each input file
     * name, which maybe NULL if failed.
     *
     * This function will also open any external referenced files.
     */
    std::vector<Document*> openDocuments(const std::vector<std::string> &filenames,
            const std::vector<std::string> *paths=nullptr,
            const std::vector<std::string> *labels=nullptr,
            std::vector<std::string> *errs=nullptr,
            bool createView = true);
    /// Retrieve the active document
    App::Document* getActiveDocument() const;
    /// Retrieve a named document
    App::Document* getDocument(const char *Name) const;

    /// Path matching mode for getDocumentByPath()
    enum class PathMatchMode {
        /// Match by resolving to absolute file path
        MatchAbsolute = 0,
        /** Match by absolute path first. If not found then match by resolving
         * to canonical file path where any intermediate '.' '..' and symlinks
         * are resolved.
         */
        MatchCanonical = 1,
        /** Same as MatchCanonical, but if a document is found by canonical
         * path match, which means the document can be resolved using two
         * different absolute path, a warning is printed and the found document
         * is not returned. This is to allow the caller to intentionally load
         * the same physical file as separate documents.
         */
        MatchCanonicalWarning = 2,
    };
    /** Retrieve a document based on file path
     *
     * @param path: file path
     * @param checkCanonical: file path matching mode, @sa PathMatchMode.
     * @return Return the document found by matching with the given path
     */
    App::Document* getDocumentByPath(const char *path,
                                     PathMatchMode checkCanonical = PathMatchMode::MatchAbsolute) const;

    /// gets the (internal) name of the document
    const char * getDocumentName(const App::Document* ) const;
    /// get a list of all documents in the application
    std::vector<App::Document*> getDocuments() const;
    /// Set the active document
    void setActiveDocument(App::Document* pDoc);
    void setActiveDocument(const char *Name);
    /// close all documents (without saving)
    void closeAllDocuments();
    /// Add pending document to open together with the current opening document
    int addPendingDocument(const char *FileName, const char *objName, bool allowPartial);
    /// Indicate whether the application is opening (restoring) some document
    bool isRestoring() const;
    /// Indicate the application is closing all document
    bool isClosingAll() const;
    //@}

    /** @name Application-wide trandaction setting */
    //@{
    /** Setup a pending application-wide active transaction
     *
     * @param name: new transaction name
     * @param persist: by default, if the calling code is inside any invocation
     * of a command, it will be auto closed once all command within the current
     * stack exists. To disable auto closing, set persist=true
     *
     * @return The new transaction ID.
     *
     * Call this function to setup an application-wide transaction. All current
     * pending transactions of opening documents will be committed first.
     * However, no new transaction is created by this call. Any subsequent
     * changes in any current opening document will auto create a transaction
     * with the given name and ID. If more than one document is changed, the
     * transactions will share the same ID, and will be undo/redo together.
     */
    int setActiveTransaction(const char *name, bool persist=false);
    /// Return the current active transaction name and ID
    const char *getActiveTransaction(int *tid=nullptr) const;
    /** Commit/abort current active transactions
     *
     * @param abort: whether to abort or commit the transactions
     *
     * Bsides calling this function directly, it will be called by automatically
     * if 1) any new transaction is created with a different ID, or 2) any
     * transaction with the current active transaction ID is either committed or
     * aborted
     */
    void closeActiveTransaction(bool abort=false, int id=0);
    //@}

    /** @name Signals of the Application */
    //@{
    /// signal on new Document
    boost::signals2::signal<void (const Document&, bool)> signalNewDocument;
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
    /// signal on pending reloading of a partial Document
    boost::signals2::signal<void (const Document&)> signalPendingReloadDocument;
    /// signal on starting to save Document
    boost::signals2::signal<void (const Document&, const std::string&)> signalStartSaveDocument;
    /// signal on saved Document
    boost::signals2::signal<void (const Document&, const std::string&)> signalFinishSaveDocument;
    /// signal on undo in document
    boost::signals2::signal<void (const Document&)> signalUndoDocument;
    /// signal on application wide undo
    boost::signals2::signal<void ()> signalUndo;
    /// signal on redo in document
    boost::signals2::signal<void (const Document&)> signalRedoDocument;
    /// signal on application wide redo
    boost::signals2::signal<void ()> signalRedo;
    /// signal before close/abort active transaction
    boost::signals2::signal<void (bool)> signalBeforeCloseTransaction;
    /// signal after close/abort active transaction
    boost::signals2::signal<void (bool)> signalCloseTransaction;
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
    boost::signals2::signal<void (const App::Document&, const App::Property&)> signalChangePropertyEditor;
    //@}

    /** @name Signals of extension changes
     * These signals are emitted on dynamic extension addition. Dynamic extensions are the ones added by python (c++ ones are part
     * of the class definition, hence not dynamic)
     * The extension in question is provided as parameter.
     */
    //@{
    /// signal before adding the extension
    boost::signals2::signal<void (const App::ExtensionContainer&, std::string extension)> signalBeforeAddingDynamicExtension;
    /// signal after the extension was added
    boost::signals2::signal<void (const App::ExtensionContainer&, std::string extension)> signalAddedDynamicExtension;
     //@}


    /** @name methods for parameter handling */
    //@{
    /// returns the system parameter
    ParameterManager &                                GetSystemParameter();
    /// returns the user parameter
    ParameterManager &                                GetUserParameter();
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
    const std::map<std::string,Base::Reference<ParameterManager>> &  GetParameterSetList() const;
    void AddParameterSet(const char* sName);
    void RemoveParameterSet(const char* sName);
    //@}

    /** @name methods for the open handler
     *  With this facility an Application module can register
     *  an ending (filetype) which it can handle to open.
     *  The ending and the module name are stored and if the file
     *  type is opened the module gets loaded and needs to register an
     *  OpenHandler class in the OpenHandlerFactorySingleton.
     *  After the module is loaded, an OpenHandler of this type is created
     *  and the file gets loaded.
     *  @see OpenHandler
     *  @see OpenHandlerFactorySingleton
     */
    //@{
    /// Register an import filetype and a module name
    void addImportType(const char* Type, const char* ModuleName);
    /// Change the module name of a registered filetype
    void changeImportModule(const char* Type, const char* OldModuleName, const char* NewModuleName);
    /// Return a list of modules that support the given filetype.
    std::vector<std::string> getImportModules(const char* Type) const;
    /// Return a list of all modules.
    std::vector<std::string> getImportModules() const;
    /// Return a list of filetypes that are supported by a module.
    std::vector<std::string> getImportTypes(const char* Module) const;
    /// Return a list of all filetypes.
    std::vector<std::string> getImportTypes() const;
    /// Return the import filters with modules of a given filetype.
    std::map<std::string, std::string> getImportFilters(const char* Type) const;
    /// Return a list of all import filters.
    std::map<std::string, std::string> getImportFilters() const;
    //@}
    //@{
    /// Register an export filetype and a module name
    void addExportType(const char* Type, const char* ModuleName);
    /// Change the module name of a registered filetype
    void changeExportModule(const char* Type, const char* OldModuleName, const char* NewModuleName);
    /// Return a list of modules that support the given filetype.
    std::vector<std::string> getExportModules(const char* Type) const;
    /// Return a list of all modules.
    std::vector<std::string> getExportModules() const;
    /// Return a list of filetypes that are supported by a module.
    std::vector<std::string> getExportTypes(const char* Module) const;
    /// Return a list of all filetypes.
    std::vector<std::string> getExportTypes() const;
    /// Return the export filters with modules of a given filetype.
    std::map<std::string, std::string> getExportFilters(const char* Type) const;
    /// Return a list of all export filters.
    std::map<std::string, std::string> getExportFilters() const;
    //@}

    /** @name Init, Destruct an Access methods */
    //@{
    static void init(int argc, char ** argv);
    static void initTypes();
    static void destruct();
    static void destructObserver();
    static void processCmdLineFiles();
    static std::list<std::string> getCmdLineFiles();
    static std::list<std::string> processFiles(const std::list<std::string>&);
    static void runApplication();
    friend Application &GetApplication();
    static std::map<std::string, std::string> &Config(){return mConfig;}
    static int GetARGC(){return _argc;}
    static char** GetARGV(){return _argv;}
    //@}

    /** @name Application directories */
    //@{
    static std::string getHomePath();
    static std::string getExecutableName();
    /*!
     Returns the temporary directory. By default, this is set to the
     system's temporary directory but can be customized by the user.
     */
    static std::string getTempPath();
    static std::string getTempFileName(const char* FileName=nullptr);
    static std::string getUserCachePath();
    static std::string getUserConfigPath();
    static std::string getUserAppDataDir();
    static std::string getUserMacroDir();
    static std::string getResourceDir();
    static std::string getLibraryDir();
    static std::string getHelpDir();
    //@}

    /** @name Link handling */
    //@{

    /** Check for link recursion depth
     *
     * @param depth: current depth
     * @param option: whether to throw exception, print an error message or quieten any output.
     * In the latter case the caller must check the returned value.
     *
     * @return Return the maximum remaining depth.
     *
     * The function uses an internal count of all objects in all documents as
     * the limit of recursion depth.
     */
    int checkLinkDepth(int depth, MessageOption option = MessageOption::Error);

    /** Return the links to a given object
     *
     * @param obj: the linked object. If NULL, then all links are returned.
     * @param option: @sa App::GetLinkOption
     * @param maxCount: limit the number of links returned, 0 means no limit
     */
    std::set<DocumentObject*> getLinksTo(
            const DocumentObject *, int options, int maxCount=0) const;

    /// Check if there is any link to the given object
    bool hasLinksTo(const DocumentObject *obj) const;
    //@}

    friend class App::Document;

protected:
    /// get called by the document when the name is changing
    void renameDocument(const char *OldName, const char *NewName);

    /** @name I/O of the document
     * This slot gets connected to all App::Documents created
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
    void slotChangePropertyEditor(const App::Document&, const App::Property &);
    //@}

    /// open single document only
    App::Document* openDocumentPrivate(const char * FileName, const char *propFileName,
            const char *label, bool isMainDoc, bool createView, std::vector<std::string> &&objNames);

    /// Helper class for App::Document to signal on close/abort transaction
    class AppExport TransactionSignaller {
    public:
        TransactionSignaller(bool abort, bool signal);
        ~TransactionSignaller();
    private:
        bool abort;
    };

private:
    /// Constructor
    explicit Application(std::map<std::string, std::string> &mConfig);
    /// Destructor
    virtual ~Application();

    static void cleanupUnits();

    /** @name member for parameter */
    //@{
    static Base::Reference<ParameterManager> _pcSysParamMngr;
    static Base::Reference<ParameterManager> _pcUserParamMngr;
    //@}

    //---------------------------------------------------------------------
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++
    //---------------------------------------------------------------------
    static void setupPythonTypes();
    static void setupPythonException(PyObject*);

    // static python wrapper of the exported functions
    static PyObject* sGetParam          (PyObject *self, PyObject *args);
    static PyObject* sSaveParameter     (PyObject *self, PyObject *args);
    static PyObject* sGetVersion        (PyObject *self, PyObject *args);
    static PyObject* sGetConfig         (PyObject *self, PyObject *args);
    static PyObject* sSetConfig         (PyObject *self, PyObject *args);
    static PyObject* sDumpConfig        (PyObject *self, PyObject *args);
    static PyObject* sAddImportType     (PyObject *self, PyObject *args);
    static PyObject* sChangeImportModule(PyObject *self, PyObject *args);
    static PyObject* sGetImportType     (PyObject *self, PyObject *args);
    static PyObject* sAddExportType     (PyObject *self, PyObject *args);
    static PyObject* sChangeExportModule(PyObject *self, PyObject *args);
    static PyObject* sGetExportType     (PyObject *self, PyObject *args);
    static PyObject* sGetResourcePath   (PyObject *self, PyObject *args);
    static PyObject* sGetLibraryPath    (PyObject *self, PyObject *args);
    static PyObject* sGetTempPath       (PyObject *self, PyObject *args);
    static PyObject* sGetUserCachePath  (PyObject *self, PyObject *args);
    static PyObject* sGetUserConfigPath (PyObject *self, PyObject *args);
    static PyObject* sGetUserAppDataPath(PyObject *self, PyObject *args);
    static PyObject* sGetUserMacroPath  (PyObject *self, PyObject *args);
    static PyObject* sGetHelpPath       (PyObject *self, PyObject *args);
    static PyObject* sGetHomePath       (PyObject *self, PyObject *args);

    static PyObject* sLoadFile          (PyObject *self,PyObject *args);
    static PyObject* sOpenDocument      (PyObject *self,PyObject *args, PyObject *kwd);
    static PyObject* sSaveDocument      (PyObject *self,PyObject *args);
    static PyObject* sSaveDocumentAs    (PyObject *self,PyObject *args);
    static PyObject* sNewDocument       (PyObject *self,PyObject *args, PyObject *kwd);
    static PyObject* sCloseDocument     (PyObject *self,PyObject *args);
    static PyObject* sActiveDocument    (PyObject *self,PyObject *args);
    static PyObject* sSetActiveDocument (PyObject *self,PyObject *args);
    static PyObject* sGetDocument       (PyObject *self,PyObject *args);
    static PyObject* sListDocuments     (PyObject *self,PyObject *args);
    static PyObject* sAddDocObserver    (PyObject *self,PyObject *args);
    static PyObject* sRemoveDocObserver (PyObject *self,PyObject *args);
    static PyObject *sIsRestoring       (PyObject *self,PyObject *args);

    static PyObject *sSetLogLevel       (PyObject *self,PyObject *args);
    static PyObject *sGetLogLevel       (PyObject *self,PyObject *args);

    static PyObject *sCheckLinkDepth    (PyObject *self,PyObject *args);
    static PyObject *sGetLinksTo        (PyObject *self,PyObject *args);

    static PyObject *sGetDependentObjects(PyObject *self,PyObject *args);

    static PyObject *sSetActiveTransaction  (PyObject *self,PyObject *args);
    static PyObject *sGetActiveTransaction  (PyObject *self,PyObject *args);
    static PyObject *sCloseActiveTransaction(PyObject *self,PyObject *args);
    static PyObject *sCheckAbort(PyObject *self,PyObject *args);
    static PyMethodDef    Methods[];

    friend class ApplicationObserver;

    /** @name  Private Init, Destruct an Access methods */
    //@{
    static void initConfig(int argc, char ** argv);
    static void initApplication();
    static void logStatus();
    // the one and only pointer to the application object
    static Application *_pcSingleton;
    /// checks if the environment is alright
    //static void CheckEnv(void);
    /// Search for the FreeCAD home path based on argv[0]
    /*!
     * There are multiple implementations of this method per-OS
     */
    static std::string FindHomePath(const char* sCall);
    /// Print the help message
    static void PrintInitHelp();
    /// figure out some things
    static void ExtractUserPath();
    /// load the user and system parameter set
    static void LoadParameters();
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
    mutable std::map<std::string,Document*> DocFileMap;
    std::map<std::string,Base::Reference<ParameterManager>> mpcPramManager;
    std::map<std::string,std::string> &_mConfig;
    App::Document* _pActiveDoc{nullptr};

    std::deque<std::string> _pendingDocs;
    std::deque<std::string> _pendingDocsReopen;
    std::map<std::string,std::vector<std::string> > _pendingDocMap;

    // To prevent infinite recursion of reloading a partial document due a truly
    // missing object
    std::map<std::string,std::set<std::string> > _docReloadAttempts;

    bool _isRestoring{false};
    bool _allowPartial{false};
    bool _isClosingAll{false};

    // for estimate max link depth
    int _objCount{-1};

    friend class AutoTransaction;

    std::string _activeTransactionName;
    int _activeTransactionID{0};
    int _activeTransactionGuard{0};
    bool _activeTransactionTmpName{false};

    static Base::ConsoleObserverStd  *_pConsoleObserverStd;
    static Base::ConsoleObserverFile *_pConsoleObserverFile;
};

/// Singleton getter of the Application
inline App::Application &GetApplication(){
    return *App::Application::_pcSingleton;
}

} // namespace App


#endif // APP_APPLICATION_H
