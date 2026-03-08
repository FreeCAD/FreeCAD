// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

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
 * @brief A class that represents a FreeCAD document.
 *
 * A document is a container for all objects that are part of a FreeCAD
 * project.  Besides managing the objects, it also maintains properties itself.
 * As such, it is also a PropertyContainer.
 *
 * @ingroup DocumentGroup
 * @details For a more high-level discussion see the topic @ref DocumentGroup "Document".
 */
class AppExport Document: public PropertyContainer
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Document);

public:
    // clang-format off
    /// Defines the position of the status bits for documents.
    enum Status
    {
        /// Avoid recomputing the document.
        SkipRecompute = 0,
        /// Keep trailing digits in the object names when reading objects.
        KeepTrailingDigits = 1,
        /// Whether the document can be closed.
        Closable = 2,
        /// Whether the document is currently being restored.
        Restoring = 3,
        /// Whether the document is currently recomputing.
        Recomputing = 4,
        /** Whether the document is being partially restored,
         * typically meaning that there was an error.
         */
        PartialRestore = 5,
        /// Whether the document is currently importing objects.
        Importing = 6,
        /** Whether the document is a partial document,
         * meaning that it is not fully loaded.
         */
        PartialDoc = 7,
        /// Whether only the editing or active object is recomputed if SkipRecompute is set.
        AllowPartialRecompute = 8,
        /// Whether the document is a temporary document, meaning that it is not saved to a file.
        TempDoc = 9,
        /// Whether the document has an error during restore.
        RestoreError = 10,
        /// Whether any linked document's time stamp has changed during restore time.
        LinkStampChanged = 11,
        /// Whether errors are reported if the recompute failed.
        IgnoreErrorOnRecompute = 12,
        /// Whether a recompute is necessary on restore for migration purposes.
        RecomputeOnRestore = 13,
        /// Whether the local coordinate system of older versions should be migrated.
        MigrateLCS = 14
    };
    // clang-format on

    // NOLINTBEGIN
    /** @name Properties
     * @{
     */

    /// The long name of the document (utf-8 coded).
    PropertyString Label;

    /// The fully qualified (with path) file name (utf-8 coded).
    PropertyString FileName;

    /// The creator's name (utf-8).
    PropertyString CreatedBy;
    /// The date when the document was created.
    PropertyString CreationDate;
    /// The user that last modified the document.
    PropertyString LastModifiedBy;
    /// The date when the document was last modified.
    PropertyString LastModifiedDate;
    /// The company name (utf-8, optional).
    PropertyString Company;
    /// The Unit System for this document.
    PropertyEnumeration UnitSystem;
    /// A long comment or description (utf-8 with line breaks).
    PropertyString Comment;
    /// The Id, e.g. a Part number.
    PropertyString Id;
    /// A unique identifier of the document.
    PropertyUUID Uid;
    /// The full name of the licence e.g. "Creative Commons Attribution". See https://spdx.org/licenses/.
    PropertyString License;
    /// The URL to the license description or contract.
    PropertyString LicenseURL;
    /// Meta descriptions.
    PropertyMap Meta;
    /// Material descriptions, used and defined in the Material module.
    PropertyMap Material;
    /// Read-only name of the temp dir created when the document is opened.
    PropertyString TransientDir;
    /// Tip object of the document (if any).
    PropertyLink Tip;
    /// Tip object name of the document (if any).
    PropertyString TipName;
    /// Whether to show hidden items in TreeView.
    PropertyBool ShowHidden;
    /// Whether to use hasher on topological naming.
    PropertyBool UseHasher;
    /// @}

    /** @name Signals of the document
     * @{
     */
    // clang-format off

    /// Signal before changing a document property.
    fastsignals::signal<void(const Document&, const Property&)> signalBeforeChange;
    /// Signal on a changed document property.
    fastsignals::signal<void(const Document&, const Property&)> signalChanged;
    /// Signal on new object.
    fastsignals::signal<void(const DocumentObject&)> signalNewObject;
    /// Signal on a deleted object.
    fastsignals::signal<void(const DocumentObject&)> signalDeletedObject;
    /// Signal before changing an object.
    fastsignals::signal<void(const DocumentObject&, const Property&)> signalBeforeChangeObject;
    /// Signal on a changed object.
    fastsignals::signal<void(const DocumentObject&, const Property&)> signalChangedObject;
    /// Signal on a manually called DocumentObject::touch().
    fastsignals::signal<void(const DocumentObject&)> signalTouchedObject;
    /// Signal on relabeled object.
    fastsignals::signal<void(const DocumentObject&)> signalRelabelObject;
    /// Signal on an activated object.
    fastsignals::signal<void(const DocumentObject&)> signalActivatedObject;
    /// Signal on a created object.
    fastsignals::signal<void(const DocumentObject&, Transaction*)> signalTransactionAppend;
    /// Signal on a removed object.
    fastsignals::signal<void(const DocumentObject&, Transaction*)> signalTransactionRemove;
    /// Signal on undo.
    fastsignals::signal<void(const Document&)> signalUndo;
    /// Signal on redo.
    fastsignals::signal<void(const Document&)> signalRedo;

    /**
     * @brief Signal on load/save document.
     *
     * This signal is given when the document gets streamed.  You can use this
     * hook to write additional information in the file (like Gui::Document
     * does).
     */
    fastsignals::signal<void(Base::Writer&)> signalSaveDocument;
    /// Signal on restoring a document.
    fastsignals::signal<void(Base::XMLReader&)> signalRestoreDocument;
    /// Signal on exporting objects.
    fastsignals::signal<void(const std::vector<DocumentObject*>&, Base::Writer&)> signalExportObjects;
    /// Signal on exporting view objects.
    fastsignals::signal<void(const std::vector<DocumentObject*>&, Base::Writer&)> signalExportViewObjects;
    /// Signal on importing objects.
    fastsignals::signal<void(const std::vector<DocumentObject*>&, Base::XMLReader&)> signalImportObjects;
    /// Signal on importing view objects.
    fastsignals::signal<void(const std::vector<DocumentObject*>&, Base::Reader&,
                                 const std::map<std::string, std::string>&)> signalImportViewObjects;
    /// Signal after finishing importing objects.
    fastsignals::signal<void(const std::vector<DocumentObject*>&)> signalFinishImportObjects;
    /// Signal starting a save action to a file.
    fastsignals::signal<void(const Document&, const std::string&)> signalStartSave;
    /// Signal finishing a save action to a file.
    fastsignals::signal<void(const Document&, const std::string&)> signalFinishSave;
    /// Signal before recomputing the document.
    fastsignals::signal<void(const Document&)> signalBeforeRecompute;
    /// Signal after recomputing the document.
    fastsignals::signal<void(const Document&, const std::vector<DocumentObject*>&)> signalRecomputed;
    /// Signal after recomputing an object.
    fastsignals::signal<void(const DocumentObject&)> signalRecomputedObject;
    /// Signal on a new opened transaction.
    fastsignals::signal<void(const Document&, std::string)> signalOpenTransaction;
    /// Signal on a committed transaction.
    fastsignals::signal<void(const Document&)> signalCommitTransaction;
    /// Signal on an aborted transaction.
    fastsignals::signal<void(const Document&)> signalAbortTransaction;
    /// Signal on a skipping a recompute.
    fastsignals::signal<void(const Document&, const std::vector<DocumentObject*>&)> signalSkipRecompute;
    /// Signal on finishing restoring an object.
    fastsignals::signal<void(const DocumentObject&)> signalFinishRestoreObject;
    /// Signal on a changed property in the property editor.
    fastsignals::signal<void(const Document&, const Property&)> signalChangePropertyEditor;
    /// Signal on setting a value in an external link.
    fastsignals::signal<void(std::string)> signalLinkXsetValue;

    // clang-format on
    /// @}
    // NOLINTEND

    using PreRecomputeHook = std::function<void()>;

    /** @brief Set a hook for before a recompute.
     *
     * @param[in] hook: The function to be called before a recompute.
     */
    void setPreRecomputeHook(const PreRecomputeHook& hook);

    /// Clear the document of all its administration.
    void clearDocument();

    /** @name File handling of the document
     * @{
     */

    /// Save the document to the file in Property Path
    bool save();

    /**
     * @brief Save the document to a specified file.
     *
     * @param[in] file: The file name to save the document to.
     */
    bool saveAs(const char* file);

    /**
     * @brief Save a copy of the document to a specified file.
     *
     * @param[in] file: The file name to save the copy to.
     */
    bool saveCopy(const char* file) const;

    /**
     * @brief Restore the document from the file in Property Path.
     *
     * @param[in] filename: The file name to restore the document from. If nullptr, then
     * use the file name in property 'FileName'.
     * @param[in] delaySignal: if true, the signals for restored objects are delayed until
     * all objects are restored.
     * @param[in] objNames: if not empty, only restore the objects with these
     * names.  This is useful to partially restore a document.  If empty,
     * restore all objects in the document.
     */
    void restore(const char* filename = nullptr,
                 bool delaySignal = false,
                 const std::vector<std::string>& objNames = {});

    /**
     * @brief Called after a document is restored.
     *
     * This function can be delayed in restore() if `delaySignal` is true.
     *
     * @param[in] checkPartial: Check if the document is partially loaded.
     *
     * @returns True if the document is restored.  Returning false means that
     * the document must have a full reload.
     */
    bool afterRestore(bool checkPartial = false);

    /**
     * @brief Called after a document is restored to check a list of objects.
     *
     * @param[in] checkPartial: Check if the document is partially loaded.
     * @param[in] objs: The list of objects to check.
     *
     * @returns True if the document is restored.  Returning false means that
     * the document must have a full reload.
     */
    bool afterRestore(const std::vector<DocumentObject*>& objs, bool checkPartial = false);

    /// The status of export.
    enum ExportStatus
    {
        NotExporting,
        Exporting,
    };

    /**
     * @brief Check if an object is being exported.
     * @param[in] obj: The object to check.
     * @return The export status.
     */
    ExportStatus isExporting(const DocumentObject* obj) const;

    /// Get the export info.
    ExportInfo exportInfo() const;

    /**
     * @brief Set the export info in this document.
     *
     * @param[in] info: The export info to set.
     */
    void setExportInfo(const ExportInfo& info);

    /**
     *@brief Export a list of objects to a stream.
     *
     * @param[in] objs: The list of objects to export.
     * @param[in, out] out: The output stream to write to.
     */
    void exportObjects(const std::vector<DocumentObject*>& objs, std::ostream& out);

    /**
     * @brief Write the dependency graph of this document.
     *
     * The output is in the DOT format of Graphviz.
     *
     * @param[in, out] out: The output stream to write to.
     */
    void exportGraphviz(std::ostream& out) const;

    /**
     * @brief Import objects from a stream.
     *
     * @param[in, out] reader: The XML reader to read from.
     * @return The list of imported objects.
     */
    std::vector<DocumentObject*> importObjects(Base::XMLReader& reader);

    /**
     * @brief Import any externally linked objects
     *
     * @param[in] objs: input list of objects. Only objects belonging to this document will
     * be checked for external links. And all found external linked object will be imported
     * to this document. Link type properties of those input objects will be automatically
     * reassigned to the imported objects. Note that the link properties of other objects
     * in the document but not included in the input list, will not be affected even if they
     * point to some object beining imported. To import all objects, simply pass in all objects
     * of this document.
     *
     * @return the list of imported objects
     */
    std::vector<DocumentObject*> importLinks(const std::vector<DocumentObject*>& objs = {});

    // Opens the document from its file name
    // void open (void);

    /// Check if the document has been saved.
    bool isSaved() const;

    /// Get the document name.
    const char* getName() const;

    /// Get program version the project file was created with.
    const char* getProgramVersion() const;

    /**
     * @brief Get the filename of the document.
     *
     * @return For a saved document, this will be the content stored in
     * property 'Filename'.  For an unsaved temporary file, this will be the
     * content of property 'TransientDir'.
     */
    const char* getFileName() const;
    /// @}

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    unsigned int getMemSize() const override;

    /** @name Object handling
     * @{
     */

    /**
     * @brief Add an object of given type to the document.
     *
     * Add an object of given type with @p pObjectName that should be ASCII to
     * this document and set it active.  Unicode names are set through the
     * Label property.
     *
     * @param[in] sType       the type of created object
     * @param[in] pObjectName if `nullptr` generate a new unique name based on @p
     * sType, otherwise use this name.
     * @param[in] isNew if false don't call the DocumentObject::setupObject()
     * callback (default is true)
     * @param[in] viewType    override the object's view provider name
     * @param[in] isPartial   indicate if this object is meant to be partially loaded
     *
     * @return The newly added object.
     */
    DocumentObject* addObject(const char* sType,
                              const char* pObjectName = nullptr,
                              bool isNew = true,
                              const char* viewType = nullptr,
                              bool isPartial = false);

    /**
     * @brief Add an object of a given type to the document.
     *
     * Add an object of a given type with @p pObjectName that should be ASCII
     * to this document and set it active.  Unicode names are set through the
     * Label property.
     *
     * @tparam T The type of created object.
     * @param[in] pObjectName if `nullptr` generate a new unique name based on
     * @p T, otherwise use this name.
     * @param[in] isNew if `false` don't call the DocumentObject::setupObject()
     * callback (default * is true)
     * @param[in] viewType    override object's view provider name
     * @param[in] isPartial   indicate if this object is meant to be partially loaded
     *
     * @return The newly added object.
     */
    template<typename T>
    T* addObject(const char* pObjectName = nullptr,
                 bool isNew = true,
                 const char* viewType = nullptr,
                 bool isPartial = false);

    /**
     * @brief Add multiple objects of a given type to the document.
     *
     * Add multiple objects of a given type with @p objectNames that should be
     * ASCII to this document.  Unicode names are set through the Label
     * property.
     *
     * @param[in] sType       The type of created object
     * @param[in] objectNames A list of object names
     * @param[in] isNew If false don't call the DocumentObject::setupObject()
     * callback (default is true)
     */
    std::vector<DocumentObject*>
    addObjects(const char* sType, const std::vector<std::string>& objectNames, bool isNew = true);

    /**
     * @brief Remove an object from the document.
     *
     * @param[in] object The object to remove.
     */
    void removeObject(const DocumentObject* object);

    /**
     * @brief Remove an object from the document.
     *
     * @param[in] sName The name of the object to remove.
     */
    void removeObject(const char* sName);

    /**
     * @brief Add an existing object to the document.
     *
     * Add an existing feature with @p pObjectName (ASCII) to this document and set it active.
     * Unicode names are set through the Label property.
     *
     * This is an overloaded function of the function above and can be used to
     * create a feature outside and add it to the document afterwards.
     *
     * @param[in] obj The object to add.
     * @param[in] name if `nullptr` generate a new unique name based on @p
     * this name.
     *
     * @throws Base::RuntimeError If the object is already in the document.
     */
    void addObject(DocumentObject* obj, const char* name = nullptr);

    /**
     * @brief Check whether the document contains a document object.
     *
     * @note Testing the @c DocumentObject's @c pDoc pointer is not sufficient
     * because removeObject() leaves @c _pDoc unchanged.
     *
     * @param[in] pcObject The object to check.
     *
     * @return Returns true if the object is in this document, false otherwise.
     */
    bool containsObject(const DocumentObject* pcObject) const;

    /**
     * @brief Copy objects from another document to this document.
     *
     * @param[in] objs The objects to copy.
     * @param[in] recursive if true, then all objects this object depends on are
     * copied as well. By default @a recursive is false.
     *
     * @param[in] returnAll: if true, return all copied objects including those
     * auto included by recursive searching. If false, then only return the
     * copied object corresponding to the input objects.
     *
     * @return Returns the list of objects copied.
     */
    std::vector<DocumentObject*> copyObject(const std::vector<DocumentObject*>& objs,
                                            bool recursive = false,
                                            bool returnAll = false);

    /**
     * @brief Move an object from another document to this document.
     *
     * @param[in] obj The object to move.
     * @param[in] recursive If true then all objects this object depends on
     * are moved as well.  By default @a recursive is false.
     *
     * @returns The moved object itself or `nullptr` if the object is already part of this
     * document.
     */
    DocumentObject* moveObject(DocumentObject* obj, bool recursive = false);

    /// Get the active Object of this document.
    DocumentObject* getActiveObject() const;

    /**
     * @brief Get the object with the given name.
     *
     * @param[in] Name The name of the object to get.
     *
     * @return The document object with the given name or `nullptr` if no such
     * object exists.
     */
    DocumentObject* getObject(const char* Name) const;

    /**
     * @brief Get the object with the given id.
     *
     * @param[in] id The id of the object to get.
     * @return The document object with the given id or `nullptr` if no such
     * object exists.
     */
    DocumentObject* getObjectByID(long id) const;

    /**
     * @brief Check whether the object is in this document.
     *
     * @param[in] pFeat The object to check.
     * @return Returns true if the object is in this document, false otherwise.
     */
    bool isIn(const DocumentObject* pFeat) const;

    /**
     * @brief Get the name of an object.
     *
     * @param[in] pFeat The object to get the name of.
     *
     * @return The name of the object or `nullptr` if the object is not in this document.
     */
    const char* getObjectName(const DocumentObject* pFeat) const;

    /**
     * @brief Get a unique name for an object given a proposed name.
     *
     * @param[in] proposedName The proposed name for the object.
     *
     * @return A unique name for the object or an empty string if the proposed
     * name is empty.
     */
    std::string getUniqueObjectName(const char* proposedName) const;

    /**
     * @brief Get a unique label for an object.
     *
     * The name is different from any of the labels of any object in this
     * document, based on the given modelname.
     *
     * @param[in] modelName The base name to use for generating the label.
     * @param[in] digitCount The number of digits to use for the numeric suffix.
     *
     * @return A unique label for the object.
     */
    std::string getStandardObjectLabel(const char* modelName, int digitCount) const;

    /**
     * @brief Check if an object name and a label have the same base.
     *
     * Determine if a given DocumentObject Name and a proposed Label are based
     * on the same base name.
     *
     * @param[in] name The object name.
     * @param[in] label The proposed label.
     *
     * @return Returns true if the base names are the same, false otherwise.
     */
    bool haveSameBaseName(const std::string& name, const std::string& label);

    /// Get a list of the document's objects including the dependencies.
    std::vector<DocumentObject*> getDependingObjects() const;

    /// Get a list of all document objects.
    const std::vector<DocumentObject*>& getObjects() const;

    /**
     * @brief Get all objects of a given type.
     *
     * @param[in] typeId The type to search for.
     * @return A vector of objects of the given type.
     */
    std::vector<DocumentObject*> getObjectsOfType(const Base::Type& typeId) const;

    /**
     * @brief Get all objects of any of the given types.
     *
     * @param[in] types The types to search for.
     * @return A vector of objects of any of the given types.
     */
    std::vector<DocumentObject*> getObjectsOfType(const std::vector<Base::Type>& types) const;

    /**
     * @brief Get all objects with a given extension.
     *
     * @param[in] typeId The extension type to search for.
     * @param[in] derived If true, also include objects with extensions
     * derived from the given one.
     *
     * @return A vector of objects with the given extension.
     */
    std::vector<DocumentObject*> getObjectsWithExtension(const Base::Type& typeId,
                                                         bool derived = true) const;

    /**
     * @brief Find objects by type, name, and label.
     *
     * Find the objects that match all criteria.  It is possible to ignore the
     * name or label.
     *
     * @param[in] typeId The type to search for.
     * @param[in] objname The name of the object to search for, or `nullptr` to
     * ignore searching for the name.
     * @param[in] label The label of the object to search for, or `nullptr` to
     * ignore searching for the label.
     *
     *@return A vector of objects matching the given criteria.
     */
    std::vector<DocumentObject*>
    findObjects(const Base::Type& typeId, const char* objname, const char* label) const;

    /**
     * @brief Get all objects of a given type.
     *
     * @tparam T The type to search for.
     * @return A vector of objects of the given type.
     */
    template<typename T>
    inline std::vector<T*> getObjectsOfType() const;

    /**
     * @brief Count the number of objects of a given type.
     *
     * @tparam T The type to search for.
     * @return The number of objects of the given type.
     */
    template<typename T>
    inline int countObjectsOfType() const;

    /**
     * @brief Count the number of objects of a given type.
     *
     * @param[in] typeName The name of the type to search for.
     * @return The number of objects of the given type.
     */
    int countObjectsOfType(const char* typeName) const;

    /// Get the number of objects in the document
    int countObjects() const;
    /// @}

    /** @name Methods for modification and state handling.
     * @{
     */

    /**
     * @brief Remove all modifications.
     *
     * After this call The document becomes Valid again.
     */
    void purgeTouched();

    /// Check if there is any touched object in this document.
    bool isTouched() const;

    /// Check if there is any object must execute in this document.
    bool mustExecute() const;

    /// Get all touched objects.
    std::vector<DocumentObject*> getTouched() const;

    /**
     * @brief Set the document to be closable.
     *
     * This is on by default.
     */
    void setClosable(bool c);

    /// Check whether the document can be closed.
    bool isClosable() const;

    /// Set the document to autoCreated, this is off by default.
    void setAutoCreated(bool);

    /// Check whether the document is autoCreated.
    bool isAutoCreated() const;

    /**
     * @brief Recompute touched features.
     *
     * @param[in] objs The subset of objects to recompute. If empty, then all
     * object in this document are checked to be recomputed.
     *
     * @param[in] force If true, force a recompute even if the document is
     * marked to skip recomputes.
     * @param[out] hasError If not `nullptr`, set to true if there was any error.
     * @param[in] options A bitmask of DependencyOption.
     *
     * @returns The number of objects recomputed.
     */
    int recompute(const std::vector<DocumentObject*>& objs = {},
                  bool force = false,
                  bool* hasError = nullptr,
                  int options = 0);

    /**
     * @brief Recompute a single object.
     *
     * @param[in] Feat The object to recompute.
     * @param[in] recursive If true, then all objects depending on this object
     * are recomputed as well.
     *
     * @return True if the object was recomputed, false if there was an error.
     */
    bool recomputeFeature(DocumentObject* Feat, bool recursive = false);

    /**
     * @brief Get the text of the error for a specified object.
     * @param[in] Obj The object to get the error text for.
     *
     * @return The error text, or `nullptr` if there is no error.
     */
    const char* getErrorDescription(const DocumentObject* Obj) const;

    /**
     * @brief Get the status of this document for a given status bit.
     *
     * @param[in] pos The status bit to check.
     * @return The status of the given bit.
     */
    bool testStatus(Status pos) const;

    /**
     * @brief Set or unset a status bit of this document.
     *
     * @param[in] pos The status bit to set or unset.
     * @param[in] on If true, set the bit, if false, unset the bit.
     */
    void setStatus(Status pos, bool on);
    /// @}


    /** @name Methods for the Undo, Redo, and Transaction handling.
     *
     * The concept of a transaction ID ensures that each transaction is unique
     * inside the document. Multiple transactions from different documents can
     * be grouped together with the same transaction ID.
     *
     * When undoing, a Gui component can query getAvailableUndo(id) to see if
     * it is possible to undo with a given ID.  If there are more than one undo
     * transactions, this means that there are other transactions before the
     * given ID. The Gui component shall ask the user if they want to undo
     * multiple steps.  And if the user agrees, calling undo(id) unrolls all
     * transactions before and including the one with the given ID. The same
     * applies for redo.
     *
     * The transaction ID described here is fully backward compatible.  Calling
     * the APIs with a default id=0 gives the original behavior.
     *
     * @{
     */

    /**
     * @brief Set the level of Undo/Redo.
     *
     * A mode of 0 disables Undo/Redo completely, while a nonzero value turns
     * it on.
     *
     * @param[in] iMode The Undo/Redo mode.
     */
    void setUndoMode(int iMode);

    /// Get the Undo/Redo mode.
    int getUndoMode() const;

    /// Set the transaction mode.
    void setTransactionMode(int iMode);

    /**
     * @brief Open a new command Undo/Redo.
     *
     * A UTF-8 name can be specified.
     *
     * @param[in] name The transaction name.
     *
     * This function calls Application::setActiveTransaction(name) instead of
     * setup a potential transaction that will only be created if there are
     * actual changes.
     */
    void openTransaction(const char* name = nullptr);

    /**
     * @brief Rename the current transaction.
     *
     * Rename the current transaction if it matches the given ID.
     *
     * @param[in] name The new name of the transaction.
     * @param[in] id The transaction ID to match.
     */
    void renameTransaction(const char* name, int id) const;

    /**
     * @brief Commit the Command transaction.
     *
     * Do nothing If there is no Command transaction open.
     */
    void commitTransaction();

    /// Abort the currently running transaction.
    void abortTransaction() const;

    /// Check whether a transaction is open.
    bool hasPendingTransaction() const;

    /**
     * @brief Get the undo or redo transaction ID.
     *
     * Get the transaction ID starting from the back.
     *
     * @param[in] undo If true, get the undo transaction ID, if false get the
     * redo transaction ID.
     * @param[in] pos The position from the back, 0 is the last transaction,
     * 1 is the one before last, and so on.
     *
     * @return The transaction ID, or 0 if @p pos is out of range.
     */
    int getTransactionID(bool undo, unsigned pos = 0) const;

    /**
     * @brief Check if a transaction is open and its list is empty.
     *
     * @return True if there is a transaction open and its list is empty, true
     * if there is no open transaction, false if a transaction is open but its
     * list is not empty.
     */
    bool isTransactionEmpty() const;

    /**
     * @brief Set the undo limit.
     * @param[in] UndoMemSize The maximum memory in bytes.
     */
    void setUndoLimit(unsigned int UndoMemSize = 0);

    /**
     * @brief Get the undo memory size.
     * @return The memory used by the undo stack in bytes.
     */
    unsigned int getUndoMemSize() const;

    /**
     * @brief Set the Undo limit as stack size.
     *
     * @param[in] UndoMaxStackSize The maximum number of undos to store.
     */
    void setMaxUndoStackSize(unsigned int UndoMaxStackSize = 20);  // NOLINT
                                                                   //
    /// Get the maximum number of undos that can be stored.
    unsigned int getMaxUndoStackSize() const;

    /// Remove all stored undos and redos.
    void clearUndos();

    /**
     * @brief Get the number of undos stored.
     *
     * Get the number of stored for a given transaction ID. If @p id is 0, then
     * return the total number of undos stored.  If the number returned is greater
     * than 0, then this means that an undo will be effective.
     *
     * @param[in] id The transaction ID to match or 0 to get the total number of undos.
     *
     * @return The number of undos stored.
     */
    int getAvailableUndos(int id = 0) const;

    /// Get a list of the undo names.
    std::vector<std::string> getAvailableUndoNames() const;

    /**
     * @brief Undo one or multiple steps.
     *
     * If @p id is 0, then undo one step.  If @p id is not 0, then undo
     * multiple steps until the transaction with the given ID is undone.
     *
     * @param[in] id The transaction ID to match or 0 to undo one step.
     *
     * @return Returns true if an undo was done, false if no undo was done.
     */
    bool undo(int id = 0);

    /**
     * @brief Get the number of redos stored.
     *
     * Get the number of stored redos for a given transaction ID. If @p id is 0,
     * then return the total number of redos stored.  If the number returned is
     * greater than 0, then this means that a redo will be effective.
     *
     * @param[in] id The transaction ID to match or 0 to get the total number of redos.
     *
     * @return The number of redos stored.
     */
    int getAvailableRedos(int id = 0) const;

    /// Get a list of the redo names.
    std::vector<std::string> getAvailableRedoNames() const;

    /**
     * @brief Redo one or multiple steps.
     *
     * If @p id is 0, then redo one step.  If @p id is not 0, then redo
     * multiple steps until the transaction with the given ID is redone.
     *
     * @param[in] id The transaction ID to match or 0 to redo one step.
     *
     * @return Returns true if a redo was done, false if no redo was done.
     */
    bool redo(int id = 0);

    /**
     * @brief Check if the document is performing a transaction.
     *
     * This function returns true if the document is in a transaction phase,
     * i.e. currently performing a redo/undo or rollback.
     *
     * @return True if the document is performing a transaction, false otherwise.
     */
    bool isPerformingTransaction() const;

    /**
     * @brief Register that a property of an object has changed in a transaction.
     *
     * @param[in] obj The object whose property has changed.
     * @param[in] prop The property that has changed.
     * @param[in] add If true, the property was added, if false it was removed.
     *
     * @warning This function is only for internal use.
     */

    void addOrRemovePropertyOfObject(TransactionalObject* obj, const Property* prop, bool add);
    /**
     * @brief Register that a property of an object has been renamed in a transaction.
     *
     * @param[in] obj The object whose property has changed.
     * @param[in] prop The property that has changed.
     * @param[in] newName The new name of the property.
     *
     * @warning This function is only for internal use.
     */
    void renamePropertyOfObject(TransactionalObject* obj, const Property* prop, const char* newName);
    /// @}

    /** @name Dependency items.
     * @{
     */

    /**
     * @brief Write the dependency graph of this document.
     *
     * The dependency graph is in Graphviz DOT format.
     *
     * @param[in, out] out The output stream to write to.
     */
    void writeDependencyGraphViz(std::ostream& out);

    /// Checks if the dependency graph is directed and has no cycles.
    static bool checkOnCycle();

    /**
     * @brief Get the Inlist of an object.
     *
     * The Inlist represents the list of objects that have an edge pointing to
     * @p me.  This means that these objects depend on @p me and that on a
     * recompute, @p me should be computed first.
     *
     * @param[in] me The object to get the Inlist for.
     *
     * @return The Inlist of the object.
     */
    std::vector<DocumentObject*> getInList(const DocumentObject* me) const;

    /// The position of option flags used by getDependencyList()
    enum DependencyOption
    {
        DepSort = 1,       ///< For a topologically sorted list
        DepNoXLinked = 2,  ///< Ignore external links
        DepNoCycle = 4,    ///< Ignore cyclic links
    };

    /**
     * @brief Get a list of all objects that the given objects depend on.
     *
     * This function is defined as static because it accepts objects from
     * different documents, and the returned list will contain dependent
     * objects from all relevant documents.
     *
     * @param[in] objs Objects for which we want to find the dependencies.
     * @param[in] options A bitmask of DependencyOption.
     *
     * @return A list of all objects that the given objects depend on.
     */
    static std::vector<DocumentObject*>
    getDependencyList(const std::vector<DocumentObject*>& objs, int options = 0);

    /**
     * @brief Get a list of documents that depend on this document.
     *
     * @param[in] sort If true, the returned list is topologically sorted.
     *
     * @return A list of documents that depend on this document.
     */
    std::vector<Document*> getDependentDocuments(bool sort = true);

    /**
     * @brief Get a list of documents that depend on the given documents.
     *
     * @param[in] docs The documents for which we want to find the dependencies.
     * @param[in] sort If true, the returned list is topologically sorted.
     *
     * @return A list of documents that depend on the given documents.
     */
    static std::vector<Document*> getDependentDocuments(std::vector<Document*> docs,
                                                        bool sort);

    // set Changed
    // void setChanged(DocumentObject* change);

    /**
     * @brief Get a list of topologically sorted objects.
     *
     * For more information on topological sorting see
     * https://en.wikipedia.org/wiki/Topological_sorting.
     *
     * @return A list of the topologically sorted objects of this document.
     */
    std::vector<DocumentObject*> topologicalSort() const;

    /**
     * @brief Get all root objects in the document.
     *
     * Root objects are objects that no other objects references.
     *
     * @return A list of all root objects.
     */
    std::vector<DocumentObject*> getRootObjects() const;

    /**
     * @brief Get all tree root objects in the document.
     *
     * These are objects that are at the root of the object tree.
     *
     * @return A list of all tree root objects.
     */
    std::vector<DocumentObject*> getRootObjectsIgnoreLinks() const;

    /**
     * @brief Get all possible paths from one object to another.
     *
     * This functions follows the outlist to find all paths from @p from to @p to.
     *
     * @param[in] from The object to start from.
     * @param[in] to The object to end at.
     *
     * @return A vector of object lists, each list is one path from @p from to
     * @p to.
     */
    std::vector<std::list<DocumentObject*>>
    getPathsByOutList(const DocumentObject* from, const DocumentObject* to) const;
    /// @}

    /**
     * @brief Add a string hasher to the document.
     *
     * This function is called by a property during save to store its
     * StringHasher. The StringHasher object is designed to be shared among
     * multiple objects.  We must not save duplicate copies of the same hasher,
     * and must be able to restore with the same sharing relationship. This
     * function returns whether the hasher has been added before by other
     * objects, and the index of the hasher. If the hasher has not been added
     * before, the object must save the hasher by calling StringHasher::Save
     *
     * @param[in] hasher The input hasher.
     *
     * @return Returns a pair<bool,int>. The boolean indicates if the
     * StringHasher has been added before. The integer is the hasher index.
     */
    std::pair<bool, int> addStringHasher(const StringHasherRef& hasher) const;

    /**
     * @brief Get a string hasher from the document given an index.
     *
     * This function is called by a property to restore its StringHasher.  The
     * caller is responsible for restoring the hasher if the caller is the
     * first owner of the hasher, i.e. if addStringHasher() returns true during
     * save.
     *
     * @param[in] index The index previously returned by calling
     * addStringHasher() during save. Or if the index is negative, return the
     * document's own string hasher.
     *
     * @return Return the resulting string hasher or a new one if the index is
     * not found.
     */
    StringHasherRef getStringHasher(int index = -1) const;

    /**
     * @brief Get the links to a given object.
     *
     * Get the links to an object that is contained in this document.  If the
     * object is `nullptr`, then all links in this document are returned.
     *
     * @param[in, out] links Holds the links found
     * @param[in] obj The linked object. If `nullptr`, then all links are returned.
     * @param[in] options: A bitmask of type GetLinkOption
     * @param[in] maxCount: limit the number of links returned, 0 means no limit
     * @param[in] objs: optional objects to search for, if empty, then all objects
     * of this document are searched.
     */
    void getLinksTo(std::set<DocumentObject*>& links,
                    const DocumentObject* obj,
                    int options,
                    int maxCount = 0,
                    const std::vector<DocumentObject*>& objs = {}) const;

    /**
     * @brief Check if there is any link to the given object.
     *
     * @param[in] obj The linked object.
     *
     * @return True if there is at least one link to the given object, false otherwise.
     */
    bool hasLinksTo(const DocumentObject* obj) const;

    /**
     * @brief Mark an object for recompute during restore.
     *
     * Called by objects during restore to ask for recompute.
     *
     * @param[in] obj The object to mark for recompute.
     */
    void addRecomputeObject(DocumentObject* obj);

    /// Get the old label of an object before it was changed.
    const std::string& getOldLabel() const
    {
        return oldLabel;
    }

    /**
     * @brief Rename object identifiers.
     *
     * @param[in] paths A map of old to new object identifiers.
     * @param[in] selector A function that returns true for objects that should
     * be renamed, and false for objects that should be skipped.  By default
     * all objects are renamed.
     */
    void renameObjectIdentifiers(
        const std::map<ObjectIdentifier, ObjectIdentifier>& paths,
        const std::function<bool(const DocumentObject*)>& selector =
            [](const DocumentObject*) {
                return true;
            });

    PyObject* getPyObject() override;

    std::string getFullName() const override;

    /// Check if there is any document restoring/importing.
    static bool isAnyRestoring();

    /// Register a new label.
    void registerLabel(const std ::string& newLabel);
    /// Unregister a label.
    void unregisterLabel(const std::string& oldLabel);
    /// Check if a label exists.
    bool containsLabel(const std::string& label);
    /// Create a unique label based on the given modelLabel.
    std::string makeUniqueLabel(const std::string& modelLabel);

    friend class Application;
    // because of transaction handling
    friend class TransactionalObject;
    friend class DocumentObject;
    friend class Transaction;
    friend class TransactionDocumentObject;

    ~Document() override;

protected:
    /**
     * @brief Construct a new Document object.
     *
     * @param[in] documentName The name of the document.  The default value is
     * the empty string.
     */
    explicit Document(const char* documentName = "");

    /**
     *@brief Remove an object from the document.
     *
     * @param[in] pcObject The object to remove.
     * @param[in] options A bitmask of RemoveObjectOptions.
     */
    void _removeObject(DocumentObject* pcObject,
                       RemoveObjectOptions options = RemoveObjectOption::DestroyOnRollback
                           | RemoveObjectOption::PreserveChildrenVisibility);

    /**
     * @brief Add an object to the document.
     *
     * @param[in] pcObject The object to add.
     * @param[in] pObjectName if `nullptr` generate a new unique name based on @p
     * pcObject type, otherwise use this name.
     * @param[in] options A bitmask of AddObjectOptions.
     * @param[in] viewType Override object's view provider name.
     */
    void _addObject(DocumentObject* pcObject, const char* pObjectName, AddObjectOptions options = AddObjectOption::ActivateObject, const char* viewType = nullptr);

    /**
     * @brief Check if a valid transaction is open.
     *
     * Check if a valid transaction is open, and if not, open a new
     * transaction.  With the arguments we can check what kind of transaction
     * we expect to be open.
     *
     * @param[in] pcDelObj The object being deleted, or `nullptr` if no
     * object is being deleted.
     * @param[in] What The property being changed, or `nullptr` if no property
     * is being changed.
     * @param[in] line The line number where this function is called.
     */
    void _checkTransaction(DocumentObject* pcDelObj, const Property* What, int line);

    /**
     * @brief Break dependencies of an object.
     *
     * Break all dependencies of an object, i.e. remove all links to and from
     * the object.
     *
     * @param[in] pcObject The object to break dependencies for.
     * @param[in] clear If true, then also clear all link properties of @p pcObject.
     */
    void breakDependency(DocumentObject* pcObject, bool clear);

    /**
     * @brief Read objects from an XML reader.
     *
     * @param[in, out] reader The XML reader to read from.
     * @return A vector of objects read.
     */
    std::vector<DocumentObject*> readObjects(Base::XMLReader& reader);

    /**
     * @brief Write objects to an XML writer.
     *
     * @param[in] objs The objects to write.
     * @param[in, out] writer The XML writer to write to.
     */
    void writeObjects(const std::vector<DocumentObject*>& objs, Base::Writer& writer) const;

    void writeObjectDeps(const std::vector<DocumentObject*>& objs, Base::Writer& writer) const;
    void writeObjectType(const std::vector<DocumentObject*>& objs, Base::Writer& writer) const;
    void writeObjectData(const std::vector<DocumentObject*>& objs, Base::Writer& writer) const;

    /**
     * @brief Save the document to a file.
     *
     * @param[in] filename The name of the file to save to.
     * @return True if the document was saved successfully.
     * @throw Base::FileException if the file could not be written.
     */
    bool saveToFile(const char* filename) const;

    /**
     * @brief Count the object of a given type.
     *
     * @param[in] typeId The type to count.
     * @return The number of objects of the given type.
     */
    int countObjectsOfType(const Base::Type& typeId) const;

    void onBeforeChange(const Property* prop) override;
    void onChanged(const Property* prop) override;

    /**
     * @brief Notify the document that a property is about to be changed.
     *
     * @param[in] Who The object whose property is about to be changed.
     * @param[in] What The property that is about to be changed.
     */
    void onBeforeChangeProperty(const TransactionalObject* Who, const Property* What);

    /**
     * @brief Notify the document that a property has changed.
     *
     * @param[in] Who The object whose property has changed.
     * @param[in] What The property that has changed.
     */
    void onChangedProperty(const DocumentObject* Who, const Property* What);

    /**
     * @brief Recompute a single object.
     * @param[in] Feat The object to recompute.
     * @return 0 if succeeded, 1 if failed, -1 if aborted by user.
     */
    int _recomputeFeature(DocumentObject* Feat);

    /// Clear the redos.
    void _clearRedos();

    /**
     * @brief Get the name of the transient directory for a given UUID and filename.
     *
     * @param[in] uuid The UUID of the document.
     * @param[in] filename The name of the file.
     *
     * @return The name of the transient directory.
     */
    std::string getTransientDirectoryName(const std::string& uuid,
                                          const std::string& filename) const;

    /**
     * @brief Open a new command Undo/Redo.
     *
     * A UTF-8 name can be specified
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

    /**
     * @brief Commit the Command transaction.
     *
     * This method is internally called by Application to commit the Command
     * transaction.
     *
     * @param notify If true, notify the application to close the transaction.
     */
    void _commitTransaction(bool notify = false);

    /**
     * @brief Abort the running transaction.
     *
     * This method is internally called by Application to abort the running
     * transaction.
     */
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
