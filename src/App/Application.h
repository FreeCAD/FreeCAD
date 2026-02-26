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

#pragma once

#include <fastsignals/signal.h>
#include <QtCore/qtextstream.h>

#include <deque>
#include <list>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <string>

#include <Base/Observer.h>
#include <Base/Parameter.h>
#include <Base/ProgressIndicator.h>

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
class ApplicationDirectories;
class ApplicationObserver;
class Property;
class AutoTransaction;
class ExtensionContainer;

/// Options for acquiring links.
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

/// Options for message handling.
enum class MessageOption {
    Quiet, ///< Suppress error.
    Error, ///< Print an error message.
    Throw, ///< Throw an exception. */
};

/// Options for document initialization.
struct DocumentInitFlags {
    bool createView {true}; ///< Whether to hide the document in the tree view.
    bool temporary {false}; ///< Whether the document should be a temporary one.
};

/**
 * @brief The class that represents the whole application.
 * @ingroup ApplicationGroup
 *
 * This is a singleton class that represents the application without
 * considering any GUI functionality.  You can access it using
 * App::GetApplication().  It contains functionality for transactions, system
 * and user parameters, importing modules, paths used by the application,
 * opening documents, handling links, and it defines signals used in the
 * application.
 *
 * @see App::Document
 */
class AppExport Application
{

public:
    /**
     * @name Methods for document handling
     *
     * @{
     */

    /**
     * @brief Creates a new document.
     *
     * It is possible to propose a name that is converted to be like an
     * identifier in a programming language, with no spaces and not starting
     * with a number. This name gets also forced to be unique in the
     * application. You can obtain the unique name using getDocumentName() on
     * the returned document.
     *
     * @param[in] proposedName a prototype name used to create the permanent Name for the document.
     * @param[in] proposedLabel: a UTF8 name of any kind that is normally shown to
     * the user and stored in the App::Document::Label property.
     * @param[in] CreateFlags: flags of type DocumentInitFlags to control the
     * document creation.  The default creates a non-temporary document that is
     * shown in the tree view.
     */
    App::Document* newDocument(const char* proposedName = nullptr,
                               const char* proposedLabel = nullptr,
                               DocumentInitFlags CreateFlags = DocumentInitFlags());

    /**
     * @brief Closes the document and removes it from the application.
     *
     * @param[in] doc The document to close.
     * @return Returns true if the document was found and closed, false otherwise.
     */
    bool closeDocument(const Document* doc);

    /**
     * @brief Closes the document and removes it from the application.
     *
     * @param[in] name The name of the document to close.
     * @return Returns true if the document was found and closed, false otherwise.
     */
    bool closeDocument(const char* name);

    /**
     * @brief Acquire a unique document name from a proposed name.
     *
     * @param[in] Name The proposed name.
     * @param[in] tempDoc Whether the document is temporary.
     *
     * @return Returns a unique document name based on the proposed name.
     */
    std::string getUniqueDocumentName(const char* Name, bool tempDoc = false) const;

    /**
     * @brief Open an existing document from a file.
     *
     * @param[in] FileName The file name to open.
     * @param[in] initFlags: document initialization flags of type DocumentInitFlags.  By default a
     * non-temporary, non-hidden document.
     *
     * @return Returns the opened document, or `nullptr` if failed.
     */
    App::Document* openDocument(const char* FileName = nullptr,
                                DocumentInitFlags initFlags = DocumentInitFlags {});

    /**
     * @brief Open multiple documents
     *
     * This function will also open any externally referenced files.
     *
     * @param[in] filenames The file names to open.
     * @param[in] paths Optional input file path in case it is different from
     * filenames (mainly used during recovery).
     * @param[in] labels: optional label assign to document (mainly used during recovery).
     * @param[out] errs: optional output error message corresponding to each input
     * file name. If errs is given, this function will catch all
     * Base::Exception and save the error message inside. Otherwise, it will
     * throw an exception when opening the input files.
     * @param[in] initFlags: document initialization flags of type DocumentInitFlags.  By default a
     * non-temporary, non-hidden document.
     *
     * @return Return the opened document corresponding to each input file
     * name, which maybe `nullptr` if failed.
     */
    std::vector<Document*> openDocuments(const std::vector<std::string>& filenames,
                                         const std::vector<std::string>* paths = nullptr,
                                         const std::vector<std::string>* labels = nullptr,
                                         std::vector<std::string>* errs = nullptr,
                                         DocumentInitFlags initFlags = DocumentInitFlags {});

    /// Get the active document.
    App::Document* getActiveDocument() const;

    /**
     * @brief Retrieve a document based on its name.
     *
     * @param Name: the name of the document.
     * @return Return the document with the given name, or `nullptr` if not found.
     */
    App::Document* getDocument(const char *Name) const;

    /// %Path matching modes for getDocumentByPath()
    enum class PathMatchMode
    {
        /// Match by resolving to absolute file path.
        MatchAbsolute = 0,

        /**
         * @brief Match by absolute path first.
         *
         * If not found then match by resolving to canonical file path where
         * any intermediate '.' '..' and symlinks are resolved.
         */
        MatchCanonical = 1,

        /**
         * @brief Same as MatchCanonical but also warn.
         *
         * If a document is found by canonical path match, which means the
         * document can be resolved using two different absolute path, a
         * warning is printed and the found document is not returned. This is
         * to allow the caller to intentionally load the same physical file as
         * separate documents.
         */
        MatchCanonicalWarning = 2,
    };

    /**
     * @brief Retrieve a document based on file path.
     *
     * @param[in] path: The file path.
     * @param[in] checkCanonical: file path matching mode, @sa PathMatchMode.
     *
     * @return The document found by matching with the given path or `nullptr`
     * if not successful.
     */
    App::Document* getDocumentByPath(const char *path,
                                     PathMatchMode checkCanonical = PathMatchMode::MatchAbsolute) const;

    /// Gets the (internal) name of a given document.
    const char* getDocumentName(const App::Document*) const;

    /// Get a list of all documents in the application.
    std::vector<App::Document*> getDocuments() const;

    /// Set the active document.
    void setActiveDocument(App::Document* pDoc);

    /// Set the active document.
    void setActiveDocument(const char* Name);

    /// Close all documents (without saving)
    void closeAllDocuments();

    /**
     * @brief Add a pending document to open together with the current opening
     * document.
     *
     * This function registers the filename and the object that is attempted to
     * be loaded as a pending document.  This allows it to be opened together
     * with the current document.
     *
     * @param[in] FileName: the file name of the pending document.
     * @param[in] objName: the object that is attempted to be loaded.
     * @param[in] allowPartial: whether to allow partial loading if some objects are missing.
     *
     * @return -1 if unsuccesful, 0 if the application is not restoring, and 1
     * if successfully added.
     */
    int addPendingDocument(const char* FileName, const char* objName, bool allowPartial);

    /// Check whether the application is opening (restoring) some document.
    bool isRestoring() const;

    /// Check whether the application is closing all documents.
    bool isClosingAll() const;
    /// @}

    /**
     * @name Application-wide transaction handling.
     *
     * @{
     */

    /**
     * @brief Setup a pending application-wide active transaction.
     *
     * Call this function to setup an application-wide transaction. All current
     * pending transactions of opening documents will be committed first.
     * However, no new transaction is created by this call. Any subsequent
     * changes in any current opening document will auto create a transaction
     * with the given name and ID. If more than one document is changed, the
     * transactions will share the same ID, and will be undo/redo together.
     *
     * @param[in] name The new transaction name
     * @param[in] persist By default, if the calling code is inside any invocation
     * of a command, it will be auto closed once all command within the current
     * stack exists. To disable auto closing, set to `true`.
     *
     * @return The new transaction ID.
     */
    int setActiveTransaction(const char* name, bool persist = false);

    /**
     * @brief Get the current active transaction name and ID.
     *
     * If there is no active transaction, an empty string is returned.
     *
     * @param[out] tid If not `nullptr`, the current active transaction ID is
     * returned through this pointer.
     * @return The current active transaction name.
     */
    const char* getActiveTransaction(int* tid = nullptr) const;

    /**
     * @brief Commit/abort current active transactions.
     *
     * @param[in] abort: whether to abort or commit the transactions
     * @param[in] id: by default 0 meaning that the current active transaction ID is used.
     *
     * Besides calling this function directly, it will be called by
     * automatically if 1) any new transaction is created with a different ID,
     * or 2) any transaction with the current active transaction ID is either
     * committed or aborted.
     */
    void closeActiveTransaction(bool abort=false, int id=0);
    /// @}

    // NOLINTBEGIN
    // clang-format off
    /**
     * @name Signals of the Application
     *
     * @{
     */

    /// Signal on a newly created document.
    fastsignals::signal<void (const Document&, bool)> signalNewDocument;
    /// Signal on a document getting deleted.
    fastsignals::signal<void (const Document&)> signalDeleteDocument;
    /// Signal that a document has been deleted.
    fastsignals::signal<void ()> signalDeletedDocument;
    /// Signal on relabeling the document (the user provided name).
    fastsignals::signal<void (const Document&)> signalRelabelDocument;
    /// Signal on renaming the document (internal name).
    fastsignals::signal<void (const Document&)> signalRenameDocument;
    /// Signal on activating the document.
    fastsignals::signal<void (const Document&)> signalActiveDocument;
    /// Signal on saving the document.
    fastsignals::signal<void (const Document&)> signalSaveDocument;
    /// Signal on starting to restore the document.
    fastsignals::signal<void (const Document&)> signalStartRestoreDocument;
    /// Signal after the document has restored.
    fastsignals::signal<void (const Document&)> signalFinishRestoreDocument;
    /// Signal on pending reloading of a partial document.
    fastsignals::signal<void (const Document&)> signalPendingReloadDocument;
    /// Signal on starting to save a document.
    fastsignals::signal<void (const Document&, const std::string&)> signalStartSaveDocument;
    /// Signal after a document has been saved.
    fastsignals::signal<void (const Document&, const std::string&)> signalFinishSaveDocument;
    /// Signal on an undo in a document.
    fastsignals::signal<void (const Document&)> signalUndoDocument;
    /// Signal on an application wide undo.
    fastsignals::signal<void ()> signalUndo;
    /// Signal on a redo in a document.
    fastsignals::signal<void (const Document&)> signalRedoDocument;
    /// Signal on an application wide redo.
    fastsignals::signal<void ()> signalRedo;
    /// Signal before opening an active transaction.
    fastsignals::signal<void (const std::string&)> signalBeforeOpenTransaction;
    /// Signal before closing/aborting an active transaction.
    fastsignals::signal<void (bool)> signalBeforeCloseTransaction;
    /// Signal after closing/aborting an active transaction.
    fastsignals::signal<void (bool)> signalCloseTransaction;
    /// Signal on show hidden items.
    fastsignals::signal<void (const Document&)> signalShowHidden;
    /// Signal on starting to open document(s).
    fastsignals::signal<void ()> signalStartOpenDocument;
    /// Signal after opening document(s) has finished.
    fastsignals::signal<void ()> signalFinishOpenDocument;
    /// @}


    /**
     * @name Signals of the document
     * @brief Signals for all documents.
     *
     * These signals are an aggregation of all documents. If you only require
     * the signal of a special document, connect to the document itself.
     *
     * @{
     */

    /// Signal before changing a document property.
    fastsignals::signal<void (const App::Document&, const App::Property&)> signalBeforeChangeDocument;
    /// Signal on a changed document property.
    fastsignals::signal<void (const App::Document&, const App::Property&)> signalChangedDocument;
    /// Signal on newly created object.
    fastsignals::signal<void (const App::DocumentObject&)> signalNewObject;
    //fastsignals::signal<void (const App::DocumentObject&)>     m_sig;
    /// Signal on a deleted object.
    fastsignals::signal<void (const App::DocumentObject&)> signalDeletedObject;
    /// Signal before a changed property in an object.
    fastsignals::signal<void (const App::DocumentObject&, const App::Property&)> signalBeforeChangeObject;
    /// Signal on a changed property in an object.
    fastsignals::signal<void (const App::DocumentObject&, const App::Property&)> signalChangedObject;
    /// Signal on a relabeled object.
    fastsignals::signal<void (const App::DocumentObject&)> signalRelabelObject;
    /// Signal on an activated object.
    fastsignals::signal<void (const App::DocumentObject&)> signalActivatedObject;
    /// Signal before recomputing a document.
    fastsignals::signal<void (const App::Document&)> signalBeforeRecomputeDocument;
    /// Signal on a recomputed document.
    fastsignals::signal<void (const App::Document&)> signalRecomputed;
    /// Signal on a recomputed document object.
    fastsignals::signal<void (const App::DocumentObject&)> signalObjectRecomputed;
    /// Signal on an opened transaction.
    fastsignals::signal<void (const App::Document&, std::string)> signalOpenTransaction;
    /// Signal on a committed transaction.
    fastsignals::signal<void (const App::Document&)> signalCommitTransaction;
    /// Signal on an aborted transaction.
    fastsignals::signal<void (const App::Document&)> signalAbortTransaction;
    /// @}

    /**
     * @name Signals of property changes.
     * @brief Signals for dynamic property changes.
     *
     * These signals are emitted on property additions or removal.  The changed
     * object can be any sub-class of a PropertyContainer.
     *
     * @{
     */

    /// Signal on adding a dynamic property.
    fastsignals::signal<void (const App::Property&)> signalAppendDynamicProperty;
    /// Signal on renaming a dynamic property.
    fastsignals::signal<void (const App::Property&, const char*)> signalRenameDynamicProperty;
    /// Signal before removing a dynamic property.
    fastsignals::signal<void (const App::Property&)> signalRemoveDynamicProperty;
    /// Signal before changing the editor mode of a property.
    fastsignals::signal<void (const App::Document&, const App::Property&)> signalChangePropertyEditor;
    /// @}

    /**
     * @name Signals of extension changes
     *
     * These signals are emitted on dynamic extension addition. Dynamic
     * extensions are the ones added by Python (C++ ones are part of the class
     * definition, hence not dynamic). The extension in question is provided as
     * parameter.
     *
     * @{
     */

    /// Signal before adding the extension.
    fastsignals::signal<void (const App::ExtensionContainer&, std::string extension)> signalBeforeAddingDynamicExtension;
    /// Signal after the extension was added.
    fastsignals::signal<void (const App::ExtensionContainer&, std::string extension)> signalAddedDynamicExtension;
    /// @}
    // clang-format off
    // NOLINTEND


    /**
     * @name Methods for parameter handling.
     *
     * @{
     */

    /// Get the system parameter manager.
    ParameterManager& GetSystemParameter();

    /// Get the the user parameter manager.
    ParameterManager& GetUserParameter();

    /**
     * @brief Gets a parameter group by a fully qualified path.
     *
     * To get a parameter group:
     * @code
     * // getting standard parameter
     * Base::Reference<ParameterGrp> group = App::GetApplication()
     *     .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Raytracing");
     * std::string projectPath = group->GetASCII("ProjectPath", "");
     * std::string cameraName = group->GetASCII("CameraName", "TempCamera.inc");
     * @endcode
     *
     * @param[in] sName The fully qualified path of the parameter group.
     *
     * @return Returns a reference to the parameter group.
     * @throws Base::ValueError if the parameter group does not exist or there is no path separator.
     */
    Base::Reference<ParameterGrp> GetParameterGroupByPath(const char* sName);

    /**
     * @brief Get the parameter set by name.
     *
     * Typically the names of the parameter sets are "System parameter" and
     * "User parameter".
     *
     * @sa GetSystemParameter() and GetUserParameter().
     *
     * @param[in] sName The name of the parameter set.
     * @return Returns the parameter set, or `nullptr` if not found.
     */
    ParameterManager* GetParameterSet(const char* sName) const;

    /// Get a list of all parameter sets.
    const std::map<std::string,Base::Reference<ParameterManager>> &  GetParameterSetList() const;

    /**
     * @brief Add a new parameter set.
     *
     * If the parameter set already exists, nothing is done.
     *
     * @param[in] sName The name of the new parameter set.
     */
    void AddParameterSet(const char* sName);

    /**
     * @brief Remove a parameter set.
     *
     * If the parameter set does not exist, nothing is done.
     *
     * @param[in] sName The name of the parameter set to remove.
     */
    void RemoveParameterSet(const char* sName);
    /// @}

    /**
     * @name Import and export of files
     * @brief Methods for importing and exporting file types.
     *
     * With this facility an application module can register a new file type
     * which it can handle to import or export.  The file type and module name
     * are registered and if the file type is opened, the module gets loaded
     * and importing or exporting is delegated to the module.
     *
     * @{
     */

    /**
     * @brief Register an import filetype given a filter.
     *
     * An example of a filter is: @c "STEP with colors (*.step *.STEP *.stp *.STP)".
     * An example of a module is: @c "Import" or @c "Mesh".
     *
     * @param[in] filter The filter that describes the file type and extensions.
     * @param[in] moduleName The module name that can handle this file type.
     */
    void addImportType(const char* filter, const char* moduleName);

    /**
     * @brief Change the module name of a registered filetype.
     *
     * @param[in] filter The file type filter (see addImportType())
     * @param[in] oldModuleName The old module name.
     * @param[in] newModuleName The new module name.
     */
    void changeImportModule(const char* filter, const char* oldModuleName, const char* newModuleName);

    /**
     * @brief Get a list of import modules that support the given filetype.
     *
     * @param[in] extension The file type extension.
     */
    std::vector<std::string> getImportModules(const std::string& extension) const;

    /// Get a list of all import modules.
    std::vector<std::string> getImportModules() const;

    /**
     * @brief Get a list of filetypes that are supported by a module for import.
     *
     * @param[in] Module The module name.
     * @return A list of file types (extensions) supported by the module.
     */
    std::vector<std::string> getImportTypes(const std::string& Module) const;

    /// Get a list of all import filetypes represented as extensions.
    std::vector<std::string> getImportTypes() const;

    /**
     * @brief Get the import filters with modules of a given filetype.
     *
     * @param[in] extension The file type represented by its extension.
     * @return A map of filter description to module name.
     */
    std::map<std::string, std::string> getImportFilters(const std::string& extension) const;

    /// Get a mapping of all import filters to their modules.
    std::map<std::string, std::string> getImportFilters() const;

    /**
     * @brief Register an export filetype given a filter.
     *
     * @copydetails addImportType
     */
    void addExportType(const char* filter, const char* moduleName);

    /**
     * @copydoc changeImportModule
     */
    void changeExportModule(const char* filter, const char* oldModuleName, const char* newModuleName);

    /**
     * @brief Get a list of export modules that support the given filetype.
     *
     * @copydetails getImportModules
     */
    std::vector<std::string> getExportModules(const std::string& extension) const;

    /// Get a list of all export modules.
    std::vector<std::string> getExportModules() const;

    /**
     * @brief Get a list of filetypes that are supported by a module for export.
     *
     * @copydetails App::Application::getImportTypes(const std::string&) const
     */
    std::vector<std::string> getExportTypes(const std::string& Module) const;

    /// Get a list of all export filetypes.
    std::vector<std::string> getExportTypes() const;

    /**
     * @brief Get the export filters with modules of a given filetype.
     *
     * @copydetails App::Application::getImportFilters(const std::string&) const
     */
    std::map<std::string, std::string> getExportFilters(const std::string& extension) const;

    /// Get a mapping of all export filters to their modules.
    std::map<std::string, std::string> getExportFilters() const;
    /// @}

    /**
     * @name Init, Destruct and Access methods
     *
     * @{
     */

    /**
     * @brief Initialize the application.
     *
     * @param[in] argc The argument count.
     * @param[in] argv The argument values.
     */
    static void init(int argc, char ** argv);

    /// Initialize FreeCAD's type system for objects, properties, etc.
    static void initTypes();

    /// Destroy the application.
    static void destruct();

    /// Detach from the console.
    static void destructObserver();

    /// Process files passed to command line.
    static void processCmdLineFiles();

    /// Get the command line files.
    static std::list<std::string> getCmdLineFiles();

    /**
     * @brief Process a list of files.
     *
     * @param[in] files The list of files to process.
     *
     * @return Returns the list of files that have been processed.
     */
    static std::list<std::string> processFiles(const std::list<std::string>& files);

    /// Run the application in a specific mode.
    static void runApplication();

    friend Application &GetApplication();

    /// Get the application configuration map.
    static std::map<std::string, std::string> &Config(){return mConfig;}

    /// Get the argument count that was provided at the start of the application.
    static int GetARGC(){return _argc;}

    /// Get the argument values that were provided at the start of the application.
    static char** GetARGV(){return _argv;}

    /// Get the application process id.
    static int64_t applicationPid();
    /// @}

    /**
     * @name Application directories
     *
     * @{
     */

    /// Get the home path of the application.
    static std::string getHomePath();

    /// Get the exectuable name of the application.
    static std::string getExecutableName();

    /// Get the executable name with version information.
    static std::string getNameWithVersion();

    /// Check whether this is a development version.
    static bool isDevelopmentVersion();

    /**
     * @brief Get the various application directories.
     *
     * Access to the various directories for the application.  This
     * functionality is a replacement for the get*Path methods below
     */
    static const std::unique_ptr<ApplicationDirectories>& directories();

    /**
     * @brief Get the temporary directory.
     *
     * By default, this is set to the system's temporary directory but can be
     * customized by the user.
     */
    static std::string getTempPath();

    /**
     * @brief Get a temporary file name.
     *
     * If @p FileName is provided, a unique temporary file name is generated
     * based on the file name.  Otherwise a random unique temporary file name
     * is generated.
     *
     * @param[in] FileName: optional file name to append to the temporary path.
     * @returns a unique temporary file name in the temporary path.
     */
    static std::string getTempFileName(const char* FileName=nullptr);

    /// Get the user cache path.
    static std::string getUserCachePath();

    /// Get the user configuration path.
    static std::string getUserConfigPath();

    /**
     * @brief Get the user application data directory.
     *
     * This is typically @c .local/share on Linux, @c Library/Application
     * Support on macOS, and @c AppData/Roaming on Windows.
     */
    static std::string getUserAppDataDir();

    /// Get the user macro directory.
    static std::string getUserMacroDir();

    /// Get the resource directory for the application.
    static std::string getResourceDir();

    /**
     * @brief Get the application library directory.
     *
     * This directory contains the shared libraries that the application needs
     * to load.
     */
    static std::string getLibraryDir();

    /// Get the application help directory.
    static std::string getHelpDir();
    /// @}

    /**
     * @name Verbosity Information
     *
     * @{
     */

    /**
     * @brief Get verbose information about the application.
     *
     * @param[in,out] str The text stream to write the information to.
     * @param[in] mConfig The application configuration.
     */
    static void getVerboseCommonInfo(QTextStream& str, const std::map<std::string,std::string>& mConfig);

    /**
     * @brief Get verbose information about add-ons.
     *
     * @copydetails getVerboseCommonInfo
     */
    static void getVerboseAddOnsInfo(QTextStream& str, const std::map<std::string,std::string>& mConfig);

    /**
     * @brief Add module info to the verbose output.
     *
     * This function is used to add information about a single add-on.
     *
     * @param[in,out] str The text stream to write the information to.
     * @param[in] modPath The path of the module.
     * @param[in,out] firstMod Whether this is the first module being added.
     */
    static void addModuleInfo(QTextStream& str, const QString& modPath, bool& firstMod);

    /// Get a pretty formatted product information string.
    static QString prettyProductInfoWrapper();

    /**
     * @brief Get a value from a map or an empty string.
     *
     * @param[in] map The map to search.
     * @param[in] key The key to search for.
     * @return Returns the value if found, or an empty string otherwise.
     */
    static QString getValueOrEmpty(const std::map<std::string, std::string>& map, const std::string& key);

    /**
     * Constant that request verbose version information to be printed.
     *
     * If an exception has this message, it means that we will print verbosee
     * version information.
     */
    static constexpr const char* verboseVersionEmitMessage{"verbose_version"};
    /// @}

    /**
     * @name Link handling
     *
     * @{
     */

    /**
     * @brief Check for link recursion depth.
     *
     * The function uses an internal count of all objects in all documents as
     * the limit of recursion depth.  If the depth exceeds this limit, it means
     * that there is likely a cyclic reference in the links.
     *
     * @param depth: current depth
     * @param option: whether to throw exception, print an error message or quieten any output.
     * In the latter case the caller must check the returned value.
     *
     * @return Return the maximum remaining depth.
     */
    int checkLinkDepth(int depth, MessageOption option = MessageOption::Error);

    /**
     * @brief Get the links to a given object.
     *
     * @param obj: the linked object. If `nullptr`, then all links are returned.
     * @param options: a bitmask of GetLinkOption.
     * @param maxCount: limit the number of links returned, 0 means no limit
     *
     * @return A set of objects that link to the given object.
     */
    std::set<DocumentObject*> getLinksTo(
            const DocumentObject *, int options, int maxCount=0) const;

    /// Check if there is any link to the given object
    bool hasLinksTo(const DocumentObject *obj) const;
    /// @}

    /// Gets the base progress indicator instance.
    Base::ProgressIndicator& getProgressIndicator() { return _progressIndicator; }

    friend class App::Document;

protected:
    /**
     * @name I/O of the document
     *
     * @brief Slots that get connected to all App::Documents created.
     *
     * @{
     */

    /// A slot for before a property of a document is changed.
    void slotBeforeChangeDocument(const App::Document& doc, const App::Property& prop);
    /// A slot for after a property of a document has changed.
    void slotChangedDocument(const App::Document& doc, const App::Property& prop);
    /// A slot for a newly created object.
    void slotNewObject(const App::DocumentObject& obj);
    /// A slot for a deleted object.
    void slotDeletedObject(const App::DocumentObject& obj);
    /// A slot for before a property of an object is changed.
    void slotBeforeChangeObject(const App::DocumentObject& obj, const App::Property& prop);
    /// A slot for after a property of an object has changed.
    void slotChangedObject(const App::DocumentObject& obj, const App::Property& prop);
    /// A slot for when an object is relabeled.
    void slotRelabelObject(const App::DocumentObject& obj);
    /// A slot for when an object is activated.
    void slotActivatedObject(const App::DocumentObject& obj);
    /// A slot for when an undo is performed in a document.
    void slotUndoDocument(const App::Document& doc);
    /// A slot for when an redo is performed in a document.
    void slotRedoDocument(const App::Document& doc);
    /// A slot for when an object has been recomputed.
    void slotRecomputedObject(const App::DocumentObject& obj);
    /// A slot for when a document has been recomputed.
    void slotRecomputed(const App::Document& doc);
    /// A slot for before a document is recomputed.
    void slotBeforeRecompute(const App::Document& doc);
    /// A slot for when a transaction is opened in a document.
    void slotOpenTransaction(const App::Document& doc, std::string name);
    /// A slot for when a transaction is committed in a document.
    void slotCommitTransaction(const App::Document& doc);
    /// A slot for when a transaction is aborted in a document.
    void slotAbortTransaction(const App::Document& doc);
    /// A slot for when a document is about to be saved.
    void slotStartSaveDocument(const App::Document& doc, const std::string& filename);
    /// A slot for when a document has been saved.
    void slotFinishSaveDocument(const App::Document& doc, const std::string& filename);
    /// A slot for when the editor mode of a property is changed.
    void slotChangePropertyEditor(const App::Document& doc, const App::Property& prop);
    /// @}

    /**
     * Helper class for App::Document to signal on close/abort transaction.
     *
     * This class is a RAII helper that emits signals before and after
     * close/abort of a transaction.  If the instance goes out of scope, it
     * emits the signals after if the correct conditions apply.
     */
    class AppExport TransactionSignaller {
    public:
        /**
         * Construct the signaller.
         *
         * @param[in] abort Whether to signal abort or commit.
         * @param[in] signal Whether to actually emit the signals.
         */
        TransactionSignaller(bool abort, bool signal);
        ~TransactionSignaller();
    private:
        bool abort;
    };

private:
    // Constructor. The passed configuration must last for the lifetime of the constructed Application
    // NOLINTNEXTLINE(runtime/references)
    explicit Application(std::map<std::string, std::string> &mConfig);
    // Destructor
    virtual ~Application();

    static void cleanupUnits();

    App::Document* openDocumentPrivate(const char * FileName, const char *propFileName,
            const char *label, bool isMainDoc, DocumentInitFlags initFlags, std::vector<std::string> &&objNames);

    void setActiveDocumentNoSignal(App::Document* pDoc);

    static Base::Reference<ParameterManager> _pcSysParamMngr;
    static Base::Reference<ParameterManager> _pcUserParamMngr;

    //---------------------------------------------------------------------
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++
    //---------------------------------------------------------------------
    static void setupPythonTypes();
    static void setupPythonException(PyObject*);

    friend class ApplicationObserver;

    /* Private Init, Destruct, and Access methods */
    static void initConfig(int argc, char ** argv);
    static void initApplication();
    static void logStatus();
    // the one and only pointer to the application object
    static Application *_pcSingleton;
    // checks if the environment is alright
    //static void CheckEnv(void);
    // Search for the FreeCAD home path based on argv[0]
    /*
     * There are multiple implementations of this method per-OS
     */
    static std::string FindHomePath(const char* sCall);
    // Print the help message
    static void PrintInitHelp();
    // figure out some things
    static void ExtractUserPath();
    // load the user and system parameter set
    static void LoadParameters();
    // puts the given env variable in the config
    static void SaveEnv(const char *);
    // startup configuration container
    static std::map<std::string,std::string> mConfig;
    // Management of and access to applications directories
    static std::unique_ptr<ApplicationDirectories> _appDirs;
    static int _argc;
    static char ** _argv;

    struct FileTypeItem {
        std::string filter;
        std::string module;
        std::vector<std::string> types;
    };

    // open ending information
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

    Base::ProgressIndicator _progressIndicator;

    static Base::ConsoleObserverStd  *_pConsoleObserverStd;
    static Base::ConsoleObserverFile *_pConsoleObserverFile;
};

/**
 * @brief Get the singleton Application instance.
 * @ingroup ApplicationGroup
 */
inline App::Application &GetApplication(){
    return *App::Application::_pcSingleton;
}

} // namespace App
