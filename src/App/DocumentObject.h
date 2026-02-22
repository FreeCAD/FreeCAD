// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/TransactionalObject.h>
#include <App/PropertyExpressionEngine.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Base/SmartPtrPy.h>
#include <Base/Placement.h>

#include <bitset>
#include <unordered_map>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Base
{
class Matrix4D;
}

namespace App
{
class Document;
class DocumentObjectGroup;
class DocumentObjectPy;
class Expression;

// clang-format off
/// Defines the position of the status bits for document objects.
enum ObjectStatus
{
    Touch = 0, ///< Whether the object is marked as 'touched'.
    Error = 1, ///< Whether the object is in an error state.
    New = 2, ///< Whether the object is newly created.
    Recompute = 3, ///< Whether the object is currently recomputing.
    Restore = 4, ///< Whether the object is currently restoring from a file.
    Remove = 5, ///< Whether the object is about to be removed from the document.
    PythonCall = 6, ///< Unused flag.
    Destroy = 7, ///< Whether the object is about to be destroyed.
    Enforce = 8, ///< Whether the object is forced to be recomputed.
    Recompute2 = 9, ///< Whether the object is going to be recomputed in the second pass.
    PartialObject = 10, ///< Whether this is a partially loaded object.
    PendingRecompute = 11, ///< Whether the object is in the recomputation queue, set by Document.
    ObjImporting = 13, ///< Whether the object is being imported.
    NoTouch = 14, ///< Whether the object should be touched on a property change.
    GeoExcluded = 15, ///< Whether the object is a member but not claimed by a GeoFeatureGroup.
    Expand = 16, ///< Whether the object is expanded in the tree view.
    NoAutoExpand = 17, ///< Whether the object should have auto expansion disabled on selection.
    /// Whether the object expects a call to onUndoRedoFinished() after transaction is finished.
    PendingTransactionUpdate = 18,
    RecomputeExtension = 19, ///< Whether the extensions of this object should be recomputed.
    TouchOnColorChange = 20, ///< Whether the object should be touched on color change.
    Freeze = 21, ///< Whether the object is frozen and is excluded from recomputation.
};
// clang-format on

/**
 * @brief A return object for feature execution.
 *
 * This class is used to return the result of a feature execution and is
 * returned by the recompute(), execute() and executeExtension() methods.
 *
 * Note that these functions return a pointer to an instance and that
 * DocumentObject::StdReturn is a static instance of this class that has as
 * value `nullptr`.  Returning @ref DocumentObject::StdReturn "StdReturn"
 * indicates that the execution was successful and that no further action is
 * needed.
 */
class AppExport DocumentObjectExecReturn
{
public:
    /**
     * @brief Construct a DocumentObjectExecReturn object.
     *
     * Constructing an object of this type means that the execution was
     * unsuccessful and the reason is given in the `Why` member.
     *
     * @param[in] sWhy The reason for the failed execution.
     * @param[in] WhichObject The object that caused the failed execution.
     */
    explicit DocumentObjectExecReturn(const std::string& sWhy,
                                      DocumentObject* WhichObject = nullptr)
        : Why(sWhy)
        , Which(WhichObject)
    {}

    /// @copydoc DocumentObjectExecReturn::DocumentObjectExecReturn(const std::string&, DocumentObject*)
    explicit DocumentObjectExecReturn(const char* sWhy, DocumentObject* WhichObject = nullptr)
        : Which(WhichObject)
    {
        if (sWhy) {
            Why = sWhy;
        }
    }

    /// The reason for the failed execution.
    std::string Why;

    /// The object that caused the failed execution.
    DocumentObject* Which;
};


/**
 * @brief %Base class of all objects handled in the @ref App::Document "Document".
 * @ingroup DocumentObjectGroup
 *
 * For a more high-level overview see topic @ref DocumentObjectGroup "Document Object".
 */
class AppExport DocumentObject: public App::TransactionalObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::DocumentObject);

private:
    // store read-only property names at freeze
    // in order to retablish correct status at unfreeze
    std::vector<const char*> readOnlyProperties;

public:
    /**
     * @name Static properties
     * @{
     */

    /// A property for the label of the document object.
    PropertyString Label;

    /// A property for the description of the document object.
    PropertyString Label2;

    /// A property that manages the expressions in the document object.
    PropertyExpressionEngine ExpressionEngine;

    /// A property that controls the visibility status in App name space.
    PropertyBool Visibility;
    /// @}

    /**
     * @name Signals for property change
     * @{
     */
    // clang-format off
    /**
     * @brief Signal that the property of this object is about to change.
     *
     * Subscribers will get the current object and the property about to be modified.
     *
     * @param[in] obj  The document object whose property is changing.
     * @param[in] prop The property that is about to change.
     */
    fastsignals::signal<void(const App::DocumentObject&, const App::Property&)> signalBeforeChange;

    /**
     * @brief Signal that the property of this object has changed.
     *
     * Subscribers will get the current object and the property that has changed.
     *
     * @param[in] obj  The document object whose property just changed.
     * @param[in] prop The property that was changed.
     */
    fastsignals::signal<void(const App::DocumentObject&, const App::Property&)> signalChanged;

    /**
     * @brief Emitted immediately after any property of this object has changed.
     *
     * This is fired before the outer "document-scoped" change notification.
     *
     * @param[in] obj  The document object whose property just changed.
     * @param[in] prop The property that was changed.
     */
    fastsignals::signal<void(const App::DocumentObject&, const App::Property&)> signalEarlyChanged;
    /// @}
    // clang-format on

    /**
     * @brief Get the name of the type of the ViewProvider.
     *
     * By overriding this function, document objects can indicate what the type
     * of the belonging view provider is.  Note that this function is
     * essentially the boundary between the App and Gui namespaces.
     *
     * @return The name of the type of the ViewProvider.
     */
    virtual const char* getViewProviderName() const
    {
        return "";
    }

    /**
     * @brief Get the name of the type of the ViewProvider for Python.
     *
     * This function is introduced to allow Python feature override its view
     * provider.  The default implementation just returns \ref
     * getViewProviderName().
     *
     * The core will only accept the overridden view provider if it returns
     * true when calling Gui::ViewProviderDocumentObject::allowOverride(obj).
     * If not, the view provider will be reverted to the one returned from \ref
     * getViewProviderName().
     *
     * @return The name of the type of the ViewProvider for Python.
     */
    virtual const char* getViewProviderNameOverride() const
    {
        return getViewProviderName();
    }

    DocumentObject();

    ~DocumentObject() override;

    /// Get a value that uniquely identifies this document object.
    const char* getDagKey() const;

    /// Get the name of the object in the document.
    const char* getNameInDocument() const;

    /// Get the object ID that is unique within its owner document.
    long getID() const
    {
        return _Id;
    }

    /**
     * @brief Get the export name of the object.
     *
     * This name is safe to be exported to another document.
     *
     * @param[in] forced If true, the name is forced to be unique.
     *
     * @return The export name of the object.
     */
    std::string getExportName(bool forced = false) const;

    /**
     * @brief Get the full name of the object.
     *
     * The name is of the form `DocName#ObjName`.
     *
     * @return The full name of the object.
     */
    std::string getFullName() const override;

    /**
     * @brief Get the full label of the object.
     *
     * The label is of the form `DocName#Label`.
     *
     * @return The full label of the object.
     */
    std::string getFullLabel() const;

    bool isAttachedToDocument() const override;
    const char* detachFromDocument() override;

    /// Get the document of which this object is part.
    App::Document* getDocument() const;

    /**
     * @name Status handling
     * @{
     */

    /**
     * @brief Set this document object touched.
     *
     * With @p noRecompute it is possible to make sure that only other document
     * objects that link to it (i.e. its InList) will be recomputed.  By
     * default the document object itself will be recomputed as well.
     *
     * @param[in] noRecompute If true, the object will not be recomputed when
     * touched.
     */
    void touch(bool noRecompute = false);

    /**
     * @brief Check whether the document object is touched or not.
     *
     * @return true if document object is touched, false if not.
     */
    bool isTouched() const;

    /**
     * @brief Enforce this document object to be recomputed.
     *
     * This can be useful to recompute the feature without
     * having to change one of its input properties.
     */
    void enforceRecompute();

    /**
     * @brief Check whether the document object must be recomputed.
     *
     * This means that the 'Enforce' flag is set or that \ref mustExecute()
     * returns a value > 0.
     *
     * @return true if document object must be recomputed, false if not.
     */
    bool mustRecompute() const;

    /// Reset the touch flags of the document object.
    void purgeTouched()
    {
        StatusBits.reset(ObjectStatus::Touch);
        StatusBits.reset(ObjectStatus::Enforce);
        setPropertyStatus(0, false);
    }

    /// Check whether this document object is in an error state.
    bool isError() const
    {
        return StatusBits.test(ObjectStatus::Error);
    }

    /// Check whether this document object is in a valid state.
    bool isValid() const
    {
        return !StatusBits.test(ObjectStatus::Error);
    }

    /// Remove the error from the object.
    void purgeError()
    {
        StatusBits.reset(ObjectStatus::Error);
    }

    /// Check whether this document object is being recomputed.
    bool isRecomputing() const
    {
        return StatusBits.test(ObjectStatus::Recompute);
    }

    /// Check whether the document is restoring from file.
    bool isRestoring() const
    {
        return StatusBits.test(ObjectStatus::Restore);
    }

    /// Check whether this document object is being removed.
    bool isRemoving() const
    {
        return StatusBits.test(ObjectStatus::Remove);
    }

    /**
     * @brief Set this document object freezed.
     *
     * A freezed document object is excluded from recomputation.
     */
    void freeze();

    /**
     * @brief Set this document object unfreezed.
     *
     * A freezed document object is excluded from recomputation.  This function
     * enables recomputation and touches the object.
     *
     * @param[in] noRecompute: if true, the object will not be recomputed when
     * touched.
     *
     * @see touch()
     */
    void unfreeze(bool noRecompute = false);

    /// Check whether this document object is being freezed.
    bool isFreezed() const
    {
        return StatusBits.test(ObjectStatus::Freeze);
    }

    /// Get the status bits.
    unsigned long getStatus() const
    {
        return StatusBits.to_ulong();
    }

    /**
     * @brief Test the status of the document object.
     *
     * @param[in] pos The status bit to test.
     *
     * @return true if the status bit is set, false otherwise.
     */
    bool testStatus(ObjectStatus pos) const
    {
        return StatusBits.test(size_t(pos));
    }

    /**
     * @brief Set the status of the document object for a particular bit.
     *
     * @param[in] pos The status bit to set.
     * @param[in] on The value to set the status bit to.
     */
    void setStatus(ObjectStatus pos, bool on)
    {
        StatusBits.set(size_t(pos), on);
    }

    /// Check whether the document object is exporting.
    int isExporting() const;
    /// @}

    /**
     * @name Child element handling
     * @{
     */

    /** @brief Set the visibility of a sub-element.
     *
     * For performance reason, @p element must not contain any further
     * sub-elements, i.e. there should be no '.' inside @p element.
     *
     * @return -1 if element visibility is not supported, 0 if element is not
     * found, 1 if success
     */
    virtual int setElementVisible(const char* element, bool visible);

    /** @brief Get the visibility of a sub-element.
     *
     * @return -1 if element visibility is not supported or if the element is not found, 0
     * if element is invisible, or else 1.
     */
    virtual int isElementVisible(const char* element) const;

    /// Check whether the object has child elements.
    virtual bool hasChildElement() const;
    ///@}


    /**
     * @name DAG handling
     *
     * This part of the interface deals with viewing the document as a DAG
     * (directed acyclic graph).
     *
     * @{
     */

    /// Options for computing the OutList
    enum OutListOption
    {
        OutListNoExpression = 1, ///< Do not include links from the expression engine.
        OutListNoHidden = 2, ///< Do not hide any link (i.e. include links with LinkScopeHidden).
        OutListNoXLinked = 4, ///< Do not include links from PropertyXLink properties.
    };

    /// Get a list of objects this object links to.
    const std::vector<App::DocumentObject*>& getOutList() const;

    /**
     * @brief Get a list of objects this object links to.
     *
     * @param[in] option Options for computing the OutList.
     *
     * @return A vector of objects this object links to.
     *
     * @see OutListOption for available options.
     */
    std::vector<App::DocumentObject*> getOutList(int option) const;

    /**
     * @brief Get a list of objects this object links to.
     *
     * @param[in] option Options for computing the OutList.
     *
     * @param[in,out] res The vector to fill with the objects this object depends on.
     *
     * @see OutListOption for available options.
     */
    void getOutList(int option, std::vector<App::DocumentObject*>& res) const;

    /**
     * @brief Get a list of objects that a property links to.
     *
     * @param[in] prop The property to query for linked objects.
     * @return A vector of objects that the property links to.
     */
    std::vector<App::DocumentObject*> getOutListOfProperty(App::Property* prop) const;

    /**
     * @brief Get a list of objects this object links to, recursively.
     *
     * This function returns all objects that this object links to and all
     * further descendants.
     *
     * @return A vector of objects this object links to, recursively.
     */
    std::vector<App::DocumentObject*> getOutListRecursive() const;

    /// Clear the internal OutList cache.
    void clearOutListCache() const;

    /**
     * @brief Get all possible paths from this object to another object.
     *
     * This function returns all paths from this object to the specified object
     * by following the OutList. The paths are returned as a vector of lists,
     * where each list represents a path from this object to the target object.
     *
     * @param[in] to The target object to which paths are sought.
     *
     * @return A vector of lists, where each list contains the objects in a
     * path from this object to the target object.
     */
    std::vector<std::list<App::DocumentObject*>> getPathsByOutList(const App::DocumentObject* to) const;

    /** @brief Get a list of objects that link to this object.
     *
     * This function returns a list of objects that directly link to this
     * object, i.e. the InList.
     *
     * @return A reference to the list of objects that depend on this object.
     */
    const std::vector<App::DocumentObject*>& getInList() const;

    /**
     * @brief Get a list of objects that link to this object, recursively.
     *
     * This function returns all objects that link to this object directly and
     * indirectly.
     *
     * @return A vector of objects that link to this object, recursively.
     */
    std::vector<App::DocumentObject*> getInListRecursive() const;

    /** @brief Get a set of all objects linking to this object.
     *
     * This function returns a set of all objects that link to this object,
     * including possible external parent objects.
     *
     * @param[in,out] inSet A set containing all objects linking to this object.
     * @param[in] recursive Whether to obtain the InList recursively.
     * @param[in,out] inList An optional pointer to a vector holding the output.
     * objects, with the furthest linking object ordered last.
     */
    void getInListEx(std::set<App::DocumentObject*>& inSet,
                     bool recursive,
                     std::vector<App::DocumentObject*>* inList = nullptr) const;

    /**
     * @brief Get a set of all objects linking to this object.
     *
     * This function returns a set of all objects that link to this object,
     * including possible external parent objects.
     *
     * @param[in] recursive Whether to obtain the InList recursively.
     *
     * @return A set containing all objects linking to this object.
     */
    std::set<App::DocumentObject*> getInListEx(bool recursive) const;

    /**
     * @brief Get the group this object belongs to.
     *
     * This function returns the group that this object is part of, if any.
     *
     * @return A pointer to the group this object belongs to, or nullptr if it
     * is not part of any group.
     */
    DocumentObjectGroup* getGroup() const;

    /**
     * @brief Check if an object is (in)directly in the InList of this object.
     *
     * This function checks if the specified object is in the InList of this
     * object, either directly or recursively.
     *
     * @param[in] objToTest The object to test for presence in the InList.
     *
     * @return true if the object is in the InList, false otherwise.
     */
    bool isInInListRecursive(DocumentObject* objToTest) const;

    /**
     * @brief Check if an object is directly in the InList of this object.
     *
     * This function checks if the specified object is directly in the InList
     * of this object, without checking recursively.
     *
     * @param[in] objToTest The object to test for presence in the InList.
     *
     * @return true if the object is directly in the InList, false otherwise.
     */
    bool isInInList(DocumentObject* objToTest) const;

    /**
     * @brief Check if an object is (in)directly in the OutList of this object.
     *
     * This function checks if the specified object is in the OutList of this
     * object, either directly or recursively.
     *
     * @param[in] objToTest The object to test for presence in the OutList.
     *
     * @return true if the object is in the OutList, false otherwise.
     */
    bool isInOutListRecursive(DocumentObject* objToTest) const;

    /**
     * @brief Check if an object is directly in the OutList of this object.
     *
     * This function checks if the specified object is directly in the OutList
     * of this object, without checking recursively.
     *
     * @param[in] objToTest The object to test for presence in the OutList.
     *
     * @return true if the object is directly in the OutList, false otherwise.
     */
    bool isInOutList(DocumentObject* objToTest) const;

    /**
     * @brief Remove a back link from the InList of this object.
     *
     * This is an internal method, used by @ref App::PropertyLink
     * "PropertyLink" (and variants) to maintain DAG back links when a link
     * property's value is set.
     *
     * @param[in] obj The object to remove from the InList.
     */
    void _removeBackLink(DocumentObject* obj);

    /**
     * @brief Add a back link to the InList of this object.
     *
     * This is an internal method, used by @ref App::PropertyLink
     * "PropertyLink" (and variants) to maintain DAG back links when a link
     * property's value is set.
     *
     * @param[in] obj The object to remove from the InList.
     */
    void _addBackLink(DocumentObject*);

    /**
     * @brief Test an about to created link for circular references.
     *
     * This function checks if the link to the specified parameter is DAG
     * compatible, which means that no circular references will be created.
     *
     * @param[in] linkTo The object that this object is about to link to.
     *
     * @return true if the link can be created without a circular dependency,
     * false if it will cause a circular dependency.
     */
    bool testIfLinkDAGCompatible(DocumentObject* linkTo) const;

    /**
     * @copybrief testIfLinkDAGCompatible(DocumentObject*) const
     *
     * @param[in] linksTo A list of objects that this object is about to link to.
     *
     * @return true if the links can be created without a circular dependency,
     * false if they will cause a circular dependency.
     */
    bool testIfLinkDAGCompatible(const std::vector<DocumentObject*>& linksTo) const;

    /// @copydoc testIfLinkDAGCompatible(const std::vector<DocumentObject*>&) const
    bool testIfLinkDAGCompatible(App::PropertyLinkSubList& linksTo) const;

    /// @copydoc testIfLinkDAGCompatible(DocumentObject*) const
    bool testIfLinkDAGCompatible(App::PropertyLinkSub& linkTo) const;
    ///@}

    /**
     * @brief Get the element map version of the geometry data stored in the given property.
     *
     * @param[in] prop: the geometry property to query for element map version
     * @param[in] restored: whether to query for the restored element map version.
     *                      In case of version upgrade, the restored version may
     *                      be different from the current version.
     *
     * @return Return the element map version string.
     */
    virtual std::string getElementMapVersion(const App::Property* prop,
                                             bool restored = false) const;

    /** @brief Check the element map versionn of the property.
     *
     * This function checks whether the element map version of the given
     * property matches the given version string.  If the function returns
     * true, the geometry element names need to be recomputed.
     *
     * @param[in] prop: the geometry property to query for element map version
     * @param[in] ver: the version string to compare with
     *
     * @return true if the element map version differs and the geometry element
     * namess need to be recomputed.
     */
    virtual bool checkElementMapVersion(const App::Property* prop, const char* ver) const;

public:

    /**
     * @brief Whether the object must be executed.
     *
     * This method checks if the object was modified to be invoked.
     *
     * @return 0 if no execute is needed, 1 if the object must be executed.
     *
     * @remark If an object is marked as 'touched', then this does not
     * necessarily mean that it will be recomputed. It only means that all
     * objects that link to it (i.e. its InList) will be recomputed.
     */
    virtual short mustExecute() const;

    /**
     * @brief Recompute only this feature.
     *
     * @param recursive: set to true to recompute any dependent objects as well.
     *
     * @return true if the feature was recomputed, false if it was not.
     */
    bool recomputeFeature(bool recursive = false);

    /// Get the status message.
    const char* getStatusString() const;

    /**
     * @brief Called when a link to a DocumentObject is lost.
     *
     * This method is called by the document when a link property of this
     * object is pointing to a DocumentObject that has been deleted. The
     * standard behavior of the DocumentObject implementation is to reset the
     * links to nothing. You may override this method to implement additional
     * or different behavior.
     *
     * @param[in] obj The DocumentObject that has been lost.
     */
    virtual void onLostLinkToObject(DocumentObject* obj);

    PyObject* getPyObject() override;

    /**
     * @brief Get the sub element/object by name.
     *
     * @param[in] subname A string that is a dot separated name to refer to a
     * sub element or object. An empty string can be used to refer to the
     * object itself.
     *
     * @param[out] pyObj If not null, returns the python object corresponding to
     * this sub object. The actual type of this python object is implementation
     * dependent.  For example, The current implementation of @c Part::Feature
     * will return the TopoShapePy, even if there is no sub-element reference,
     * in which case it returns the whole shape.
     *
     * @param[in,out] mat If not null, it is used as the current
     * transformation matrix on input.  On output it is used as the accumulated
     * transformation up until and include the transformation applied by the
     * final object reference in @c subname. For @c Part::Feature, the
     * transformation is applied to the @c TopoShape inside @c pyObj before
     * returning.
     *
     * @param[in] transform: If false, then it will not apply the object's own
     * transformation to @p mat, which lets you override the object's placement
     * (and possibly scale).
     *
     * @param[in] depth: Depth limitation as hint for cyclic link detection.
     *
     * @return The last document object referred in subname. If @p subname is
     * empty, then it returns itself. If @p subname is invalid, then it returns @c
     * nullptr.
     */
    virtual DocumentObject* getSubObject(const char* subname,
                                         PyObject** pyObj = nullptr,
                                         Base::Matrix4D* mat = nullptr,
                                         bool transform = true,
                                         int depth = 0) const;

    /**
     * @brief Get a list of objects referenced by a given subname.
     *
     * This method returns a list of objects along the path that is represented
     * by the subname.  It includes this object.  With the optional argument @p
     * subsizes, it is possible to infer which part of the subname relates to
     * which document object, with the following relation:
     * @code
     * ret[i] = getSubObject(std::string(subname, subsizes[i]).c_str());
     * @endcode
     *
     * @param[in] subname: The subname path.
     * @param[in,out] subsizes: The optional subname sizes for each returned object.
     * @param[in] flatten: Whether to flatten the object hierarchies that belong to
     *                     the same geo feature group, e.g. (Part.Fusion.Box -> Part.Box)
     *
     * @return Return a list of objects along the path of the subname.
     */
    std::vector<DocumentObject*> getSubObjectList(const char* subname,
                                                  std::vector<int>* subsizes = nullptr,
                                                  bool flatten = false) const;

    /// Reason of calling getSubObjects()
    enum GSReason
    {
        /// Default, mostly for exporting shape objects
        GS_DEFAULT,
        /// For element selection
        GS_SELECT,
    };

    /**
     * @brief Get name references of all sub-objects.
     *
     * This function returns a list of sub-object names for this object.  The
     * default implementation returns all object references in PropertyLink,
     * and PropertyLinkList, if any.
     *
     * In most cases, the names returned will be the object name plus an ending
     * '.'  that can be passed directly to getSubObject() to retrieve the
     * name. The reason to return the name reference instead of the sub object
     * itself is because there may be no real sub object or the sub object is a
     * special case, for example sub objects of an array type of object.
     *
     * @param[in] reason The reason for obtaining the sub objects.
     *
     * @return A vector of subname references for all sub-objects.
     */
    virtual std::vector<std::string> getSubObjects(int reason = 0) const;

    /**
     * @brief Get the parent objects and the subnames of this object.
     *
     * This function returns a vector of pairs, where each pair contains a
     * pointer to a parent DocumentObject and its subname using thee object's
     * InList.  The @p depth parameter ensures that there is no endless loop in
     * case there is a cyclic dependency.
     *
     * @param[in] depth Optional depth that specifies on which level in parents
     * the search is.
     *
     * @return A vector of pairs containing parent DocumentObjects and their subnames.
     * @throws Base::RuntimeError If the search triggers a cycle in the dependencies in the InList.
     */
    std::vector<std::pair<App::DocumentObject*, std::string>> getParents(int depth = 0) const;

    /**
     * @brief Get the first parent object of this object that is a group.
     *
     * This function returns the first parent object that is a group, i.e. that
     * has the extension GroupExtension.
     *
     * @return A pointer to the first parent group object, or @c nullptr if no such
     * parent exists.
     */
     App::DocumentObject* getFirstParent() const;

    /**
      * @brief Get the linked object with an optional transformation.
     *
     * This method returns the linked object of this document object.  The @p
     * depth parameter indicates the current depth of recursion, which is used
     * to prevent infinite recursion in case of cyclic dependencies.
     *
     * @param recurse: If false, return the immediate linked object, or else
     * recursively call this function to return the final linked object.
     *
     * @param mat: If non-null, it is used as the current transformation matrix
     * on input.  And output as the accumulated transformation until the final
     * linked object.
     *
     * @param transform: if false, then it will not accumulate the object's own
     * placement into \c mat, which lets you override the object's placement.
     *
     * @param depth: Indicates the current depth of recursion.
     *
     * @return Return the linked object. This function returns itself if it is
     * not a link or the link is invalid.
     */
    virtual DocumentObject* getLinkedObject(bool recurse = true,
                                            Base::Matrix4D* mat = nullptr,
                                            bool transform = false,
                                            int depth = 0) const;

    /**
     * @brief Check whether this object can adopt properties from linked objects.
     *
     * This function is used to determine if the object can adopt properties
     * from linked objects.  It is typically used in the context of the
     * property view to decide whether to show properties from linked objects.
     *
     * @return true if the object can adopt properties from links, false otherwise.
     */
    virtual bool canLinkProperties() const
    {
        return true;
    }

    /// Check whether this object is a link.
    virtual bool isLink() const
    {
        return false;
    }

    /// Check whether this object is a link group.
    virtual bool isLinkGroup() const
    {
        return false;
    }

    /**
     * @brief Check whether this object allows duplicate labels.
     *
     * This function is used to determine if the object allows duplicate
     * labels.  Overriding this function and returning true allows to bypass
     * duplicate labell checking.
     *
     * @return true if duplicate labels are allowed, false otherwise.
     */
    virtual bool allowDuplicateLabel() const
    {
        return false;
    }

    /**
     * @brief Called when a new label for the document object is proposed.
     *
     * This method is called when a new label is proposed for the document
     * object and it handles label changes,including ensuring unique label values,
     * signaling onBeforeChangeLabel(), and updating linked references.
     *
     * It assumes that after returning, the label will indeed be changed to the
     * (possibly altered) value of @p newLabel.
     *
     * @param[in,out] newLabel The proposed new label for the document object.
     *
     * @return A vector of referencing (linking) properties as produced by
     * PropertyLinkBase::updateLabelReferences(), which is needed for undo/redo
     * purposes.
     */
    std::vector<std::pair<Property*, std::unique_ptr<Property>>>
    onProposedLabelChange(std::string& newLabel);

    /**
     * @brief Called to ensure that the object can control its relabeling.
     *
     * @param[in,out] newLabel The new label proposed for the object.
     *
     * @note This function is called before onBeforeChange().
     */
    virtual void onBeforeChangeLabel(std::string& newLabel)
    {
        (void)newLabel;
    }

    friend class Document;
    friend class Transaction;
    friend class ObjectExecution;

    /**
     * @brief The standard return object for document object execution.
     *
     * Its value is set to `nullptr` which means that the execution
     * was successful. If the execution fails, it is set to a pointer to an
     * instance of this class that contains the error message.
     */
    static DocumentObjectExecReturn* StdReturn;

    void Save(Base::Writer& writer) const override;

    /* Expression support */

    /**
     * @brief Set an expression for a given object identifier.
     *
     * Associate the expression @p expr with object identifier @p path in this document object.
     *
     * @param[in] path The target object identifier for the result of the expression.
     * @param[in,out] expr The Expression tree.
     */
    virtual void setExpression(const ObjectIdentifier& path, std::shared_ptr<App::Expression> expr);

    /**
     * @brief Clear the expression of an object identifier in this document object.
     *
     * @param[in] path The target object identifier.
     */
    void clearExpression(const ObjectIdentifier& path);

    /**
     * @brief Get expression information associated with an object identifier.
     *
     * @param[in] path The object identifier.
     *
     * @return Expression info that contains the expression and whether the
     * expression is busy.
     */
    virtual const PropertyExpressionEngine::ExpressionInfo
    getExpression(const ObjectIdentifier& path) const;

    /**
     * @brief Rename object identifiers in expressions.
     *
     * Invoke ExpressionEngine's renameObjectIdentifier, to possibly rewrite
     * expressions using the @p paths map with current and new identifiers.
     *
     * @param[in] paths A map of old to new object identifiers.
     */
    virtual void
    renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier>& paths);

    /// Get the old label of the object.
    const std::string& getOldLabel() const
    {
        return oldLabel;
    }

    /**
     * @brief Get the name of the view provider that was stored for this object.
     *
     * This function returns the name of the view provider that was stored for
     * this object.  This is the case when the object has a custom view
     * provider.
     *
     *  @return The name of the view provider that was stored for this object.
     */
    const char* getViewProviderNameStored() const
    {
        return _pcViewProviderName.c_str();
    }

    bool removeDynamicProperty(const char* prop) override;

    bool renameDynamicProperty(Property *prop, const char *name) override;

    App::Property* addDynamicProperty(const char* type,
                                      const char* name = nullptr,
                                      const char* group = nullptr,
                                      const char* doc = nullptr,
                                      short attr = 0,
                                      bool ro = false,
                                      bool hidden = false) override;

    /**
     * @brief Resolve the last document object referenced in the subname.
     *
     * If @p childName is not null, the method will return the name of the
     * child that can be passed to isElementVisible() or setElementVisible().
     *
     * @param[in] subname A dot separated subname.
     * @param[in,out] parent If not null, return the direct parent of the object.
     * @param[in,out] childName If not null, return the child name.
     * @param[in,out] subElement If not null, return the non-object sub-element
     * name if found. The pointer is guaranteed to be within the buffer pointed
     * to by @p subname.
     * @param[in,out] pyObj If not null, return the python object corresponding
     * to this sub object.
     * @param[in,out] mat If not null, it is used for (accumulated)
     * transformation of the sub object.
     * @param[in] transform Whether to apply the object's own transformation.
     * @param[in] depth Maintains the current depth of recursion for cyclic
     * link detection.
     *
     * @sa getSubObject()
     *
     * @return The last referenced document object in the subname or if no such
     * object in subname, itself.
     */
    App::DocumentObject* resolve(const char* subname,
                                 App::DocumentObject** parent = nullptr,
                                 std::string* childName = nullptr,
                                 const char** subElement = nullptr,
                                 PyObject** pyObj = nullptr,
                                 Base::Matrix4D* mat = nullptr,
                                 bool transform = true,
                                 int depth = 0) const;

    /**
     * @brief Resolve a link reference that is relative to this object reference.
     *
     * To avoid cyclic references, an object must not be assigned a link to any
     * of the objects in its parent. This function can be used to resolve any
     * common parents of an object and its link target.
     *
     * For example, with the following object hierarchy:
     *
     * ```
     * Group
     *   |--Group001
     *   |   |--Box
     *   |   |--Cylinder
     *   |--Group002
     *       |--Box001
     *       |--Cylinder001
     * ```
     *
     * If you want to add a link of `Group.Group002.Box001` to
     * `Group.Group001`, you can call the method with the following parameters:
     *
     * ```cpp
     *      std::string subname("Group002.");
     *      auto link = Group;
     *      std::string linkSub("Group001.Box001.");
     *      parent = Group.resolveRelativeLink(subname,link,linkSub);
     * ```
     *
     * The resolving result is as follow:
     *
     * ```
     *      return  -> Group001
     *      subname -> ""
     *      link    -> Group002
     *      linkSub -> "Box001."
     * ```
     *
     * The common parent `Group` is removed.
     *
     * @param[in,out] subname The subname reference to the object that is to be
     * assigned a link on input.  On output, the reference may be offset to get
     * rid of any common parent.
     *
     * @param[in,out] link The top parent of the link reference on input.  On
     * output, it may be altered to one of its children to get rid of any
     * common parent.
     *
     * @param[in,out] linkSub The subname of the link reference on input. On
     * output, it may be offset to get rid of any common parent.
     *
     * @return The corrected top parent of the object that is to be assigned
     * the link.  If the output @p subname is empty, then the object
     * itself is returned.
     */
    App::DocumentObject* resolveRelativeLink(std::string& subname,
                                             App::DocumentObject*& link,
                                             std::string& linkSub) const;

    /**
     * @brief Adjust relative link properties to avoid cyclic links.
     *
     * This function tries to adjust any relative link properties (i.e. link
     * properties that can hold subnames) to avoid cycles between links when
     * added to the future parent.
     *
     * @param[in] inList The recursive in-list of the future parent object,
     * including the parent itself.
     *
     * @param[in,out] visited An optional set holding the visited objects.  If
     * null then only this object is adjusted, or else all objects inside the
     * out-list of this object will be checked.
     *
     * @return True if the object has been modified, false otherwise.
     */
    virtual bool adjustRelativeLinks(const std::set<App::DocumentObject*>& inList,
                                     std::set<App::DocumentObject*>* visited = nullptr);

    /**
     * @brief Check whether the object allows partial loading of dependent objects.
     *
     * The possible return values are:
     * - 0: Do not support partial loading.
     * - 1: Allow dependent objects to be partially loaded, i.e. only create them, but do not
     * restore them.
     * - 2: This object itself can be partially loaded.
     *
     * @return Whether the object supports partial loading.
     */
    virtual int canLoadPartial() const
    {
        return 0;
    }

    /**
     * @brief Called when an element reference is updated.
     *
     * @param[in] prop The property that was updated.
     */
    virtual void onUpdateElementReference([[maybe_unused]] const Property* prop)
    {}

    /**
     * @brief Allow an object to redirect a subname path.
     *
     * This function is called by tree view to generate a subname path when an
     * item is selected in the tree.  A document object can use this function
     * to redirect the selection to some other object.
     *
     * @param[in,out] ss The current subname path from @p topParent leading to
     * just before this object as input, i.e. ends at the parent of this
     * object. As output, this method should append its own name to this path, or
     * redirect the subname to another place.
     *
     * @param[in] topParent The top parent of this subname path.
     * @param[in] child The immediate child object in the path.
     *
     * @return true if the subname was redirected, false otherwise.
     */
    virtual bool
    redirectSubName(std::ostringstream& ss, DocumentObject* topParent, DocumentObject* child) const;

    /**
     * @brief A special marker to mark the object as hidden.
     *
     * It is used by Gui::ViewProvider::getElementColors(), but exposed here
     * for convenience.
     *
     * @return A string that is used to mark the object as hidden.
     */
    static const std::string& hiddenMarker();

    /**
     * @brief Check if the subname reference ends with hidden marker.
     *
     * @param[in] subname The subname to check.
     *
     * @return A pointer to the hidden marker if it is found, or nullptr if not.
     */
    static const char* hasHiddenMarker(const char* subname);

    /**
     * @brief Find the placement of a target object as seen from this.
     *
     * If no targetObj given, the last object found in the subname is used as
     * target.
     *
     * @param[in] sub The subname that is targeted.
     * @param[in] targetObj The object that is targeted.
     *
     * @return The placement of the target object from the perspective of this
     * object.
     */
    virtual Base::Placement getPlacementOf(const std::string& sub, DocumentObject* targetObj = nullptr);

    /// Returns the Placement property value if any.
    virtual Base::Placement getPlacement() const;

    /// Returns the Placement property to use if any.
    virtual App::PropertyPlacement* getPlacementProperty() const;

protected:
    /// Recompute only this object.
    virtual App::DocumentObjectExecReturn* recompute();

    /**
     * @brief Execute the document object.
     *
     * In some contexts this is called "invoking" the object.  It is called by
     * the document to recompute this feature, normally in the processing of
     * Document::recompute().  In this method, the output properties get
     * recomputed with the data from linked objects and the properties from the
     * object itself.
     *
     * @return On success, A pointer to StdReturn is returned which is set to
     * `nullptr`.  On failure, it returns a pointer to a newly created
     * App::DocumentObjectExecReturn that containss the error message.
     */
    virtual App::DocumentObjectExecReturn* execute();

    /**
     * @brief Executes the extensions of a document object.
     *
     * @sa recompute()
     */
    App::DocumentObjectExecReturn* executeExtensions();

    /**
     * @brief Status bits of the document object.
     *
     * For the status bits, see ObjectStatus.
     */
    std::bitset<32> StatusBits;

    /// Set this object in the error state.
    void setError()
    {
        StatusBits.set(ObjectStatus::Error);
    }

    /// Reset the error state of this object.
    void resetError()
    {
        StatusBits.reset(ObjectStatus::Error);
    }

    /// Set the document this object belongs to.
    void setDocument(App::Document* doc);

    void onBeforeChange(const Property* prop) override;
    void onChanged(const Property* prop) override;
    void onEarlyChange(const Property* prop) override;

    /// Called after a document has been fully restored.
    virtual void onDocumentRestored();

    void restoreFinished() override;

    /// Called after an undo/redo transaction has finished.
    virtual void onUndoRedoFinished();

    /// Called after setting the document.
    virtual void onSettingDocument();

    /// Called after a brand new object was created.
    virtual void setupObject();

    /// Called when object is going to be removed from the document.
    virtual void unsetupObject();

    /**
     * @brief Called when a property status has changed.
     *
     * @param[in] prop The property of which the status has changed.
     * @param[in] oldStatus The old status of the property.
     */
    void onPropertyStatusChanged(const Property& prop, unsigned long oldStatus) override;

private:
    void printInvalidLinks() const;

protected:  // attributes

    /// Python object of this class and all descendent classes.
    Py::SmartPtr PythonObject;

    /// A pointer to the document this object belongs to.
    App::Document* _pDoc {nullptr};

    /// The old label that is used for renaming expressions.
    std::string oldLabel;

private:
    // pointer to the document name string (for performance)
    const std::string* pcNameInDocument {nullptr};

    // accessed by App::Document to record and restore the correct view provider type
    std::string _pcViewProviderName;

    // unique identifier (among a document) of this object.
    long _Id {0};

private:
    // Back pointer to all the fathers in a DAG of the document
    // this is used by the document (via friend) to have a effective DAG handling
    std::vector<App::DocumentObject*> _inList;
    mutable std::vector<App::DocumentObject*> _outList;
    mutable std::unordered_map<const char*, App::DocumentObject*, CStringHasher, CStringHasher>
        _outListMap;
    mutable bool _outListCached = false;
};

}  // namespace App
