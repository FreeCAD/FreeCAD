/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef SRC_APP_DOCUMENT_H_
#define SRC_APP_DOCUMENT_H_

#include <CXX/Objects.hxx>
#include <Base/Observer.h>
#include <Base/Persistence.h>
#include <Base/Type.h>
#include <Base/Handle.h>
#include <Base/Bitmask.h>

#include "PropertyContainer.h"
#include "PropertyLinks.h"
#include "PropertyStandard.h"
#include "ExportInfo.h"

#include <map>
#include <vector>
#include <utility>
#include <list>
#include <string>

namespace Base
{
class Writer;
}

namespace App 
{
enum class AddObjectOption
{
    None = 0,
    SetNewStatus = 1,
    SetPartialStatus = 2,
    UnsetPartialStatus = 4,
    DoSetup = 8,
    ActivateObject = 16
};
using AddObjectOptions = Base::Flags<AddObjectOption>;

enum class RemoveObjectOption
{
    None = 0,
    MayRemoveWhileRecomputing = 1, 
    MayDestroyOutOfTransaction = 2,
    DestroyOnRollback = 4, 
    PreserveChildrenVisibility = 8
};
using RemoveObjectOptions = Base::Flags<RemoveObjectOption>;

}
ENABLE_BITMASK_OPERATORS(App::AddObjectOption)
ENABLE_BITMASK_OPERATORS(App::RemoveObjectOption)

namespace App
{
class TransactionalObject;
class DocumentObject;
class DocumentObjectExecReturn;
class Document;
class DocumentPy;
class Application;
class Transaction;
class StringHasher;
using StringHasherRef = Base::Reference<StringHasher>;

/**
 * @brief The document class
 * @ingroup DocumentGroup
 * @details For a more high-level discussion see the topic @ref DocumentGroup "Document".
 */
class AppExport Document: public PropertyContainer
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Document);

public:
    // clang-format off
    enum Status
    {
        SkipRecompute = 0,
        KeepTrailingDigits = 1,
        Closable = 2,
        Restoring = 3,
        Recomputing = 4,
        PartialRestore = 5,
        Importing = 6,
        PartialDoc = 7,
        AllowPartialRecompute = 8,    // allow recomputing editing object if SkipRecompute is set
        TempDoc = 9,                  // Mark as temporary document without prompt for save
        RestoreError = 10,
        LinkStampChanged = 11,        // Indicates during restore time if any linked document's time stamp has changed
        IgnoreErrorOnRecompute = 12,  // Don't report errors if the recompute failed
        RecomputeOnRestore = 13,      // Mark pending recompute on restore for migration purposes
        MigrateLCS = 14               // Migrate local coordinate system of older versions
    };
    // clang-format on

    // NOLINTBEGIN
    /** @name Properties */
    //@{
    /// holds the long name of the document (utf-8 coded)
    PropertyString Label;
    /// full qualified (with path) file name (utf-8 coded)
    PropertyString FileName;
    /// creators name (utf-8)
    PropertyString CreatedBy;
    PropertyString CreationDate;
    /// user last modified the document
    PropertyString LastModifiedBy;
    PropertyString LastModifiedDate;
    /// company name UTF8(optional)
    PropertyString Company;
    /// Unit System
    PropertyEnumeration UnitSystem;
    /// long comment or description (UTF8 with line breaks)
    PropertyString Comment;
    /// Id e.g. Part number
    PropertyString Id;
    /// unique identifier of the document
    PropertyUUID Uid;
    /// Full name of the licence e.g. "Creative Commons Attribution". See https://spdx.org/licenses/
    PropertyString License;
    /// License description/contract URL
    PropertyString LicenseURL;
    /// Meta descriptions
    PropertyMap Meta;
    /// Material descriptions, used and defined in the Material module.
    PropertyMap Material;
    /// read-only name of the temp dir created when the document is opened
    PropertyString TransientDir;
    /// Tip object of the document (if any)
    PropertyLink Tip;
    /// Tip object of the document (if any)
    PropertyString TipName;
    /// Whether to show hidden items in TreeView
    PropertyBool ShowHidden;
    /// Whether to use hasher on topological naming
    PropertyBool UseHasher;
    //@}

    /** @name Signals of the document */
    //@{
    // clang-format off
    /// signal before changing an doc property
    boost::signals2::signal<void(const Document&, const Property&)> signalBeforeChange;
    /// signal on changed doc property
    boost::signals2::signal<void(const Document&, const Property&)> signalChanged;
    /// signal on new Object
    boost::signals2::signal<void(const DocumentObject&)> signalNewObject;
    /// signal on deleted Object
    boost::signals2::signal<void(const DocumentObject&)> signalDeletedObject;
    /// signal before changing an Object
    boost::signals2::signal<void(const DocumentObject&, const Property&)> signalBeforeChangeObject;
    /// signal on changed Object
    boost::signals2::signal<void(const DocumentObject&, const Property&)> signalChangedObject;
    /// signal on manually called DocumentObject::touch()
    boost::signals2::signal<void(const DocumentObject&)> signalTouchedObject;
    /// signal on relabeled Object
    boost::signals2::signal<void(const DocumentObject&)> signalRelabelObject;
    /// signal on activated Object
    boost::signals2::signal<void(const DocumentObject&)> signalActivatedObject;
    /// signal on created object
    boost::signals2::signal<void(const DocumentObject&, Transaction*)> signalTransactionAppend;
    /// signal on removed object
    boost::signals2::signal<void(const DocumentObject&, Transaction*)> signalTransactionRemove;
    /// signal on undo
    boost::signals2::signal<void(const Document&)> signalUndo;
    /// signal on redo
    boost::signals2::signal<void(const Document&)> signalRedo;
    /** signal on load/save document
     * this signal is given when the document gets streamed.
     * you can use this hook to write additional information in
     * the file (like the Gui::Document does).
     */
    boost::signals2::signal<void(Base::Writer&)> signalSaveDocument;
    boost::signals2::signal<void(Base::XMLReader&)> signalRestoreDocument;
    boost::signals2::signal<void(const std::vector<DocumentObject*>&, Base::Writer&)> signalExportObjects;
    boost::signals2::signal<void(const std::vector<DocumentObject*>&, Base::Writer&)> signalExportViewObjects;
    boost::signals2::signal<void(const std::vector<DocumentObject*>&, Base::XMLReader&)> signalImportObjects;
    boost::signals2::signal<void(const std::vector<DocumentObject*>&, Base::Reader&,
                                 const std::map<std::string, std::string>&)> signalImportViewObjects;
    boost::signals2::signal<void(const std::vector<DocumentObject*>&)> signalFinishImportObjects;
    // signal starting a save action to a file
    boost::signals2::signal<void(const Document&, const std::string&)> signalStartSave;
    // signal finishing a save action to a file
    boost::signals2::signal<void(const Document&, const std::string&)> signalFinishSave;
    boost::signals2::signal<void(const Document&)> signalBeforeRecompute;
    boost::signals2::signal<void(const Document&, const std::vector<DocumentObject*>&)> signalRecomputed;
    boost::signals2::signal<void(const DocumentObject&)> signalRecomputedObject;
    // signal a new opened transaction
    boost::signals2::signal<void(const Document&, std::string)> signalOpenTransaction;
    // signal a committed transaction
    boost::signals2::signal<void(const Document&)> signalCommitTransaction;
    // signal an aborted transaction
    boost::signals2::signal<void(const Document&)> signalAbortTransaction;
    boost::signals2::signal<void(const Document&, const std::vector<DocumentObject*>&)> signalSkipRecompute;
    boost::signals2::signal<void(const DocumentObject&)> signalFinishRestoreObject;
    boost::signals2::signal<void(const Document&, const Property&)> signalChangePropertyEditor;
    boost::signals2::signal<void(std::string)> signalLinkXsetValue;
    // clang-format on
    //@}
    // NOLINTEND

    using PreRecomputeHook = std::function<void()>;
    void setPreRecomputeHook(const PreRecomputeHook& hook);

    void clearDocument();

    /** @name File handling of the document */
    //@{
    /// Save the Document under a new Name
    // void saveAs (const char* Name);
    /// Save the document to the file in Property Path
    bool save();
    bool saveAs(const char* file);
    bool saveCopy(const char* file) const;
    /// Restore the document from the file in Property Path
    void restore(const char* filename = nullptr,
                 bool delaySignal = false,
                 const std::vector<std::string>& objNames = {});
    bool afterRestore(bool checkPartial = false);
    bool afterRestore(const std::vector<DocumentObject*>&, bool checkPartial = false);
    enum ExportStatus
    {
        NotExporting,
        Exporting,
    };
    ExportStatus isExporting(const DocumentObject* obj) const;
    ExportInfo exportInfo() const;
    void setExportInfo(const ExportInfo& info);
    void exportObjects(const std::vector<DocumentObject*>&, std::ostream&);
    void exportGraphviz(std::ostream&) const;
    std::vector<DocumentObject*> importObjects(Base::XMLReader& reader);
    /** Import any externally linked objects
     *
     * @param objs: input list of objects. Only objects belonging to this document will
     * be checked for external links. And all found external linked object will be imported
     * to this document. Link type properties of those input objects will be automatically
     * reassigned to the imported objects. Note that the link properties of other objects
     * in the document but not included in the input list, will not be affected even if they
     * point to some object beining imported. To import all objects, simply pass in all objects
     * of this document.
     *
     * @return the list of imported objects
     */
    std::vector<DocumentObject*>
    importLinks(const std::vector<DocumentObject*>& objs = {});
    /// Opens the document from its file name
    // void open (void);
    /// Is the document already saved to a file?
    bool isSaved() const;
    /// Get the document name
    const char* getName() const;
    /// Get program version the project file was created with
    const char* getProgramVersion() const;
    /** Returned filename
     *
     * For saved document, this will be the content stored in property
     * 'Filename'. For unsaved temporary file, this will be the content of
     * property 'TransientDir'.
     */
    const char* getFileName() const;
    //@}

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    /// returns the complete document memory consumption, including all managed DocObjects and Undo
    /// Redo.
    unsigned int getMemSize() const override;

    /** @name Object handling  */
    //@{
    /** Add a feature of sType with sName (ASCII) to this document and set it active.
     * Unicode names are set through the Label property.
     * @param sType       the type of created object
     * @param pObjectName if nonNULL use that name otherwise generate a new unique name based on the
     * \a sType
     * @param isNew       if false don't call the \c DocumentObject::setupObject() callback (default
     * is true)
     * @param viewType    override object's view provider name
     * @param isPartial   indicate if this object is meant to be partially loaded
     */
    DocumentObject* addObject(const char* sType,
                              const char* pObjectName = nullptr,
                              bool isNew = true,
                              const char* viewType = nullptr,
                              bool isPartial = false);
    //@{
    /** Add a feature of T type with sName (ASCII) to this document and set it active.
     * Unicode names are set through the Label property.
     * @param pObjectName if nonNULL use that name otherwise generate a new unique name based on the
     * \a sType
     * @param isNew       if false don't call the \c DocumentObject::setupObject() callback (default
     * is true)
     * @param viewType    override object's view provider name
     * @param isPartial   indicate if this object is meant to be partially loaded
     */
    template<typename T>
    T* addObject(const char* pObjectName = nullptr,
                 bool isNew = true,
                 const char* viewType = nullptr,
                 bool isPartial = false);
    /** Add an array of features of the given types and names.
     * Unicode names are set through the Label property.
     * @param sType       The type of created object
     * @param objectNames A list of object names
     * @param isNew       If false don't call the \c DocumentObject::setupObject() callback (default
     * is true)
     */
    std::vector<DocumentObject*>
    addObjects(const char* sType, const std::vector<std::string>& objectNames, bool isNew = true);
    /// Remove a feature out of the document
    void removeObject(const char* sName);
    /** Add an existing feature with sName (ASCII) to this document and set it active.
     * Unicode names are set through the Label property.
     * This is an overloaded function of the function above and can be used to create
     * a feature outside and add it to the document afterwards.
     * \note The passed feature must not yet be added to a document, otherwise an exception
     * is raised.
     */
    void addObject(DocumentObject*, const char* pObjectName = nullptr);

    /// returns whether this is actually contains the DocumentObject.
    /// Testing the DocumentObject's pDoc pointer is not sufficient because the object
    /// removeObject and _removeObject leave _pDoc unchanged
    bool containsObject(const DocumentObject*) const;

    /** Copy objects from another document to this document
     *
     * @param objs
     * @param recursive: if true, then all objects this object depends on are
     * copied as well. By default \a recursive is false.
     *
     * @param returnAll: if true, return all copied objects including those
     * auto included by recursive searching. If false, then only return the
     * copied object corresponding to the input objects.
     *
     * @return Returns the list of objects copied.
     */
    std::vector<DocumentObject*> copyObject(const std::vector<DocumentObject*>& objs,
                                            bool recursive = false,
                                            bool returnAll = false);
    /** Move an object from another document to this document
     * If \a recursive is true then all objects this object depends on
     * are moved as well. By default \a recursive is false.
     * Returns the moved object itself or 0 if the object is already part of this
     * document..
     */
    DocumentObject* moveObject(DocumentObject* obj, bool recursive = false);
    /// Returns the active Object of this document
    DocumentObject* getActiveObject() const;
    /// Returns a Object of this document
    DocumentObject* getObject(const char* Name) const;
    /// Returns a Object of this document by its id
    DocumentObject* getObjectByID(long id) const;
    /// Returns true if the DocumentObject is contained in this document
    bool isIn(const DocumentObject* pFeat) const;
    /// Returns a Name of an Object or 0
    const char *getObjectName(const DocumentObject* pFeat) const;
    /// Returns a Name for a new Object or empty if proposedName is null or empty.
    std::string getUniqueObjectName(const char* proposedName) const;
    /// Returns a name different from any of the Labels of any objects in this document, based on the given modelName.
    std::string getStandardObjectLabel(const char* modelName, int d) const;
    /// Determine if a given DocumentObject Name and a proposed Label are based on the same base name
    bool haveSameBaseName(const std::string& name, const std::string& label);
    /// Returns a list of document's objects including the dependencies
    std::vector<DocumentObject*> getDependingObjects() const;
    /// Returns a list of all Objects
    const std::vector<DocumentObject*>& getObjects() const;
    std::vector<DocumentObject*> getObjectsOfType(const Base::Type& typeId) const;
    std::vector<DocumentObject*> getObjectsOfType(const std::vector<Base::Type>& types) const;
    /// Returns all object with given extensions. If derived=true also all objects with extensions
    /// derived from the given one
    std::vector<DocumentObject*> getObjectsWithExtension(const Base::Type& typeId,
                                                         bool derived = true) const;
    std::vector<DocumentObject*>
    findObjects(const Base::Type& typeId, const char* objname, const char* label) const;
    /// Returns an array with the correct types already.
    template<typename T>
    inline std::vector<T*> getObjectsOfType() const;
    template<typename T>
    inline int countObjectsOfType() const;
    int countObjectsOfType(const char* typeName) const;
    /// get the number of objects in the document
    int countObjects() const;
    //@}

    /** @name methods for modification and state handling
     */
    //@{
    /// Remove all modifications. After this call The document becomes Valid again.
    void purgeTouched();
    /// check if there is any touched object in this document
    bool isTouched() const;
    /// check if there is any object must execute in this document
    bool mustExecute() const;
    /// returns all touched objects
    std::vector<DocumentObject*> getTouched() const;
    /// set the document to be closable, this is on by default.
    void setClosable(bool);
    /// check whether the document can be closed
    bool isClosable() const;
    /// set the document to autoCreated, this is off by default.
    void setAutoCreated(bool);
    /// check whether the document is autoCreated.
    bool isAutoCreated() const;
    /** Recompute touched features and return the number of recalculated features
     *
     * @param objs: specify a sub set of objects to recompute. If empty, then
     * all object in this document is checked for recompute
     * @param force
     * @param hasError
     * @param options
     */
    int recompute(const std::vector<DocumentObject*>& objs = {},
                  bool force = false,
                  bool* hasError = nullptr,
                  int options = 0);
    /// Recompute only one feature
    bool recomputeFeature(DocumentObject* Feat, bool recursive = false);
    /// get the text of the error of a specified object
    const char* getErrorDescription(const DocumentObject*) const;
    /// return the status bits
    bool testStatus(Status pos) const;
    /// set the status bits
    void setStatus(Status pos, bool on);
    //@}


    /** @name methods for the UNDO REDO and Transaction handling
     *
     * Introduce a new concept of transaction ID. Each transaction must be
     * unique inside the document. Multiple transactions from different
     * documents can be grouped together with the same transaction ID.
     *
     * When undo, Gui component can query getAvailableUndo(id) to see if it is
     * possible to undo with a given ID. If there more than one undo
     * transactions, meaning that there are other transactions before the given
     * ID. The Gui component shall ask user if they want to undo multiple steps.
     * And if the user agrees, call undo(id) to unroll all transaction before
     * and including the one with the given ID. Same applies for redo.
     *
     * The new transaction ID describe here is fully backward compatible.
     * Calling the APIs with a default id=0 gives the original behavior.
     */
    //@{
    /// switch the level of Undo/Redo
    void setUndoMode(int iMode);
    /// switch the level of Undo/Redo
    int getUndoMode() const;
    /// switch the transaction mode
    void setTransactionMode(int iMode);
    /** Open a new command Undo/Redo, an UTF-8 name can be specified
     *
     * @param name: transaction name
     *
     * This function calls Application::setActiveTransaction(name) instead
     * to setup a potential transaction which will only be created if there is
     * actual changes.
     */
    void openTransaction(const char* name = nullptr);
    /// Rename the current transaction if the id matches
    void renameTransaction(const char* name, int id) const;
    /// Commit the Command transaction. Do nothing If there is no Command transaction open.
    void commitTransaction();
    /// Abort the actually running transaction.
    void abortTransaction() const;
    /// Check if a transaction is open
    bool hasPendingTransaction() const;
    /// Return the undo/redo transaction ID starting from the back
    int getTransactionID(bool undo, unsigned pos = 0) const;
    /// Check if a transaction is open and its list is empty.
    /// If no transaction is open true is returned.
    bool isTransactionEmpty() const;
    /// Set the Undo limit in Byte!
    void setUndoLimit(unsigned int UndoMemSize = 0);
    /// Returns the actual memory consumption of the Undo redo stuff.
    unsigned int getUndoMemSize() const;
    /// Set the Undo limit as stack size
    void setMaxUndoStackSize(unsigned int UndoMaxStackSize = 20);  // NOLINT
    /// Set the Undo limit as stack size
    unsigned int getMaxUndoStackSize() const;
    /// Remove all stored Undos and Redos
    void clearUndos();
    /// Returns the number of stored Undos. If greater than 0 Undo will be effective.
    int getAvailableUndos(int id = 0) const;
    /// Returns a list of the Undo names
    std::vector<std::string> getAvailableUndoNames() const;
    /// Will UNDO one step, returns False if no undo was done (Undos == 0).
    bool undo(int id = 0);
    /// Returns the number of stored Redos. If greater than 0 Redo will be effective.
    int getAvailableRedos(int id = 0) const;
    /// Returns a list of the Redo names.
    std::vector<std::string> getAvailableRedoNames() const;
    /// Will REDO one step, returns False if no redo was done (Redos == 0).
    bool redo(int id = 0);
    /// returns true if the document is in an Transaction phase, e.g. currently performing a
    /// redo/undo or rollback
    bool isPerformingTransaction() const;
    /// \internal add or remove property from a transactional object
    void addOrRemovePropertyOfObject(TransactionalObject*, const Property* prop, bool add);
    void renamePropertyOfObject(TransactionalObject*, const Property* prop, const char* newName);
    //@}

    /** @name dependency stuff */
    //@{
    /// write GraphViz file
    void writeDependencyGraphViz(std::ostream& out);
    /// checks if the graph is directed and has no cycles
    static bool checkOnCycle();
    /// get a list of all objects linking to the given object
    std::vector<DocumentObject*> getInList(const DocumentObject* me) const;

    /// Option bit flags used by getDepenencyList()
    enum DependencyOption
    {
        /// Return topological sorted list
        DepSort = 1,
        /// Do no include object linked by PropertyXLink, as it can handle external link
        DepNoXLinked = 2,
        /// Raise exception on cycles
        DepNoCycle = 4,
    };
    /** Get a complete list of all objects the given objects depend on.
     *
     * This function is defined as static because it accepts objects from
     * different documents, and the returned list will contain dependent
     * objects from all relevant documents
     *
     * @param objs: input objects to query for dependency.
     * @param options: See DependencyOption
     */
    static std::vector<DocumentObject*>
    getDependencyList(const std::vector<DocumentObject*>& objs, int options = 0);

    std::vector<Document*> getDependentDocuments(bool sort = true);
    static std::vector<Document*> getDependentDocuments(std::vector<Document*> docs,
                                                             bool sort);

    // set Changed
    // void setChanged(DocumentObject* change);
    /// get a list of topological sorted objects (https://en.wikipedia.org/wiki/Topological_sorting)
    std::vector<DocumentObject*> topologicalSort() const;
    /// get all root objects (objects no other one reference too)
    std::vector<DocumentObject*> getRootObjects() const;
    /// get all tree root objects (objects that are at the root of the object tree)
    std::vector<DocumentObject*> getRootObjectsIgnoreLinks() const;
    /// get all possible paths from one object to another following the OutList
    std::vector<std::list<DocumentObject*>>
    getPathsByOutList(const DocumentObject* from, const DocumentObject* to) const;
    //@}

    /** Called by a property during save to store its StringHasher
     *
     * @param hasher: the input hasher
     * @return Returns a pair<bool,int>. The boolean indicates if the
     * StringHasher has been added before. The integer is the hasher index.
     *
     * The StringHasher object is designed to be shared among multiple objects.
     * We must not save duplicate copies of the same hasher, and must be
     * able to restore with the same sharing relationship. This function returns
     * whether the hasher has been added before by other objects, and the index
     * of the hasher. If the hasher has not been added before, the object must
     * save the hasher by calling StringHasher::Save
     */
    std::pair<bool, int> addStringHasher(const StringHasherRef& hasher) const;

    /** Called by property to restore its StringHasher
     *
     * @param index: the index previously returned by calling addStringHasher()
     * during save. Or if is negative, then return document's own string hasher.
     *
     * @return Return the resulting string hasher.
     *
     * The caller is responsible for restoring the hasher if the caller is the first
     * owner of the hasher, i.e. if addStringHasher() returns true during save.
     */
    StringHasherRef getStringHasher(int index = -1) const;

    /** Return the links to a given object
     *
     * @param links: holds the links found
     * @param obj: the linked object. If NULL, then all links are returned.
     * @param options: @sa GetLinkOption
     * @param maxCount: limit the number of links returned, 0 means no limit
     * @param objs: optional objects to search for, if empty, then all objects
     * of this document are searched.
     */
    void getLinksTo(std::set<DocumentObject*>& links,
                    const DocumentObject* obj,
                    int options,
                    int maxCount = 0,
                    const std::vector<DocumentObject*>& objs = {}) const;

    /// Check if there is any link to the given object
    bool hasLinksTo(const DocumentObject* obj) const;

    /// Called by objects during restore to ask for recompute
    void addRecomputeObject(DocumentObject* obj);

    const std::string& getOldLabel() const
    {
        return oldLabel;
    }

    /// Function called to signal that an object identifier has been renamed
    void renameObjectIdentifiers(
        const std::map<ObjectIdentifier, ObjectIdentifier>& paths,
        const std::function<bool(const DocumentObject*)>& selector =
            [](const DocumentObject*) {
                return true;
            });

    PyObject* getPyObject() override;

    std::string getFullName() const override;

    /// Indicate if there is any document restoring/importing
    static bool isAnyRestoring();

    void registerLabel(const std ::string& newLabel);
    void unregisterLabel(const std::string& oldLabel);
    bool containsLabel(const std::string& label);
    std::string makeUniqueLabel(const std::string& modelLabel);

    friend class Application;
    /// because of transaction handling
    friend class TransactionalObject;
    friend class DocumentObject;
    friend class Transaction;
    friend class TransactionDocumentObject;

    /// Destruction
    ~Document() override;

protected:
    /// Construction
    explicit Document(const char* documentName = "");

    void _removeObject(DocumentObject* pcObject, RemoveObjectOptions options = RemoveObjectOption::DestroyOnRollback | RemoveObjectOption::PreserveChildrenVisibility);
    void _addObject(DocumentObject* pcObject, const char* pObjectName, AddObjectOptions options = AddObjectOption::ActivateObject, const char* viewType = nullptr);
    /// checks if a valid transaction is open
    void _checkTransaction(DocumentObject* pcDelObj, const Property* What, int line);
    void breakDependency(DocumentObject* pcObject, bool clear);
    std::vector<DocumentObject*> readObjects(Base::XMLReader& reader);
    void writeObjects(const std::vector<DocumentObject*>&, Base::Writer& writer) const;
    bool saveToFile(const char* filename) const;
    int countObjectsOfType(const Base::Type& typeId) const;

    void onBeforeChange(const Property* prop) override;
    void onChanged(const Property* prop) override;
    /// callback from the Document objects before property will be changed
    void onBeforeChangeProperty(const TransactionalObject* Who, const Property* What);
    /// callback from the Document objects after property was changed
    void onChangedProperty(const DocumentObject* Who, const Property* What);
    /// helper which Recompute only this feature
    /// @return 0 if succeeded, 1 if failed, -1 if aborted by user.
    int _recomputeFeature(DocumentObject* Feat);
    void _clearRedos();

    /// refresh the internal dependency graph
    void _rebuildDependencyList(
        const std::vector<DocumentObject*>& objs = std::vector<DocumentObject*>());

    std::string getTransientDirectoryName(const std::string& uuid,
                                          const std::string& filename) const;

    /** Open a new command Undo/Redo, an UTF-8 name can be specified
     *
     * @param name: transaction name
     * @param id: transaction ID, if 0 then the ID is auto generated.
     *
     * @return: Return the ID of the new transaction.
     *
     * This function creates an actual transaction regardless of Application
     * AutoTransaction setting.
     */
    int _openTransaction(const char* name = nullptr, int id = 0);
    /// Internally called by Application to commit the Command transaction.
    void _commitTransaction(bool notify = false);
    /// Internally called by Application to abort the running transaction.
    void _abortTransaction();

private:
    void changePropertyOfObject(TransactionalObject* obj, const Property* prop,
                                const std::function<void()>& changeFunc);

private:
    // # Data Member of the document
    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    std::list<Transaction*> mUndoTransactions;
    std::map<int, Transaction*> mUndoMap;
    std::list<Transaction*> mRedoTransactions;
    std::map<int, Transaction*> mRedoMap;

    struct DocumentP* d;

    std::string oldLabel;
    std::string myName;
    bool autoCreated;    // Flag to know if the document was automatically created at startup
};

template<typename T>
inline std::vector<T*> Document::getObjectsOfType() const
{
    std::vector<T*> type;
    const std::vector<DocumentObject*> obj = this->getObjectsOfType(T::getClassTypeId());
    type.reserve(obj.size());
    for (auto it : obj) {
        type.push_back(static_cast<T*>(it));
    }
    return type;
}

template<typename T>
inline int Document::countObjectsOfType() const
{
    static_assert(std::is_base_of_v<DocumentObject, T>,
                  "T must be derived from App::DocumentObject");
    return this->countObjectsOfType(T::getClassTypeId());
}

template<typename T>
T* Document::addObject(const char* pObjectName, bool isNew, const char* viewType, bool isPartial)
{
    static_assert(std::is_base_of<DocumentObject, T>::value, "T must be derived from DocumentObject");
    return static_cast<T*>(addObject(T::getClassName(), pObjectName, isNew, viewType, isPartial));
}

}  // namespace App

#endif  // SRC_APP_DOCUMENT_H_
