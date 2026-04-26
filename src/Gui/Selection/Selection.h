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

#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <Base/Observer.h>

#include "SelectionChanges.h"
#include "SelectionGate.h"
#include "SelectionObject.h"
#include "SelectionObserver.h"
#include "SelectionTypes.h"


namespace App
{
class DocumentObject;
class Document;
class PropertyLinkSubList;
}  // namespace App

namespace Gui
{

class SelectionObject;
class SelectionFilter;

/** The Selection class
 *  The selection singleton keeps track of the selection state of
 *  the whole application. It gets messages from all entities which can
 *  alter the selection (e.g. tree view and 3D-view) and sends messages
 *  to entities which need to keep track on the selection state.
 *
 *  The selection consists mainly out of following information per selected object:
 *  - document (pointer)
 *  - Object   (pointer)
 *  - list of subelements (list of strings)
 *  - 3D coordinates where the user clicks to select (Vector3d)
 *
 *  Also the preselection is managed. That means you can add a filter to prevent selection
 *  of unwanted objects or subelements.
 */
class GuiExport SelectionSingleton: public Base::Subject<const SelectionChanges&>
{
public:
    struct SelObj
    {
        const char* DocName;
        const char* FeatName;
        const char* SubName;
        const char* TypeName;
        App::Document* pDoc;
        App::DocumentObject* pObject;
        App::DocumentObject* pResolvedObject;
        float x, y, z;
    };

    /// Add to selection
    bool addSelection(
        const char* pDocName,
        const char* pObjectName = nullptr,
        const char* pSubName = nullptr,
        float x = 0,
        float y = 0,
        float z = 0,
        const std::vector<SelObj>* pickedList = nullptr,
        bool clearPreSelect = true
    );
    bool addSelection2(
        const char* pDocName,
        const char* pObjectName = nullptr,
        const char* pSubName = nullptr,
        float x = 0,
        float y = 0,
        float z = 0,
        const std::vector<SelObj>* pickedList = nullptr
    )
    {
        return addSelection(pDocName, pObjectName, pSubName, x, y, z, pickedList, false);
    }

    /// Add to selection
    bool addSelection(const SelectionObject&, bool clearPreSelect = true);
    /// Add to selection with several sub-elements
    bool addSelections(
        const char* pDocName,
        const char* pObjectName,
        const std::vector<std::string>& pSubNames
    );
    /// Update a selection
    bool updateSelection(
        bool show,
        const char* pDocName,
        const char* pObjectName = nullptr,
        const char* pSubName = nullptr
    );
    /// Remove from selection (for internal use)
    void rmvSelection(
        const char* pDocName,
        const char* pObjectName = nullptr,
        const char* pSubName = nullptr,
        const std::vector<SelObj>* pickedList = nullptr
    );
    /// Set the selection for a document
    void setSelection(const char* pDocName, const std::vector<App::DocumentObject*>&);
    /// Clear the selection of document \a pDocName. If the document name is not given the selection
    /// of the active document is cleared.
    void clearSelection(const char* pDocName = nullptr, bool clearPreSelect = true);
    /// Clear the selection of all documents
    void clearCompleteSelection(const char* pDocName = nullptr, bool clearPreSelect = true);
    /// Check if selected
    bool isSelected(
        const char* pDocName,
        const char* pObjectName = nullptr,
        const char* pSubName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;
    /// Check if selected
    bool isSelected(
        App::DocumentObject*,
        const char* pSubName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;

    std::string getSelectedElement(App::DocumentObject*, const char* pSubName) const;

    /// set the preselected object (mostly by the 3D view)
    int setPreselect(
        const char* pDocName,
        const char* pObjectName,
        const char* pSubName,
        float x = 0,
        float y = 0,
        float z = 0,
        SelectionChanges::MsgSource signal = SelectionChanges::MsgSource::Any
    );
    /// remove the present preselection
    void rmvPreselect(bool signal = false);
    /// sets different coords for the preselection
    void setPreselectCoord(float x, float y, float z);
    /// returns the present preselection
    const SelectionChanges& getPreselection() const;
    /// add a SelectionGate to control what is selectable in a document's scope, by default the
    /// active document is selected
    // which is usually the intended behavior
    void addSelectionGate(
        Gui::SelectionGate* gate,
        ResolveMode resolve = ResolveMode::OldStyleElement,
        const char* pDocName = nullptr
    );

    /** @brief get the pointer to the selection gate
     * It will be nullptr when no selection filter active
     */
    const Gui::SelectionGate* getSelectionGate(const App::Document* document) const;
    /// remove the document's SelectionGate, by default the active document is selected, which is
    /// usually the intended behavior
    void rmvSelectionGate(const char* pDocName = nullptr);
    /// remove the document's SelectionGate (assumes valid pointer)
    void rmvSelectionGate(App::Document* doc);

    int disableCommandLog();
    int enableCommandLog(bool silent = false);

    /** Returns the number of selected objects with a special object type
     * It's the convenient way to check if the right objects are selected to
     * perform an operation (GuiCommand). The check also detects base types.
     * E.g. "Part" also fits on "PartImport" or "PartTransform types.
     * If no document name is given the active document is assumed.
     *
     * Set 'resolve' to true to resolve any sub object inside selection SubName
     * field.
     *
     * The typename T must be based on App::DocumentObject.
     */
    template<typename T>
    inline unsigned int countObjectsOfType(
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;

    /**
     * Does basically the same as the method above unless that it accepts a string literal as first
     * argument.
     * \a typeName must be a registered type, otherwise 0 is returned.
     */
    unsigned int countObjectsOfType(
        const char* typeName,
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;

    /** Returns a vector of objects of type \a TypeName selected for the given document name \a
     * pDocName. If no document name is specified the objects from the active document are regarded.
     * If no objects of this document are selected an empty vector is returned.
     * @note The vector reflects the sequence of selection.
     */
    std::vector<App::DocumentObject*> getObjectsOfType(
        const Base::Type& typeId,
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;

    /**
     * Does basically the same as the method above unless that it accepts a string literal as first
     * argument.
     * \a typeName must be a registered type otherwise an empty array is returned.
     */
    std::vector<App::DocumentObject*> getObjectsOfType(
        const char* typeName,
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;
    /**
     * A convenience template-based method that returns an array with the correct types already.
     */
    template<typename T>
    inline std::vector<T*> getObjectsOfType(
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;

    /// Visible state used by setVisible()
    enum VisibleState
    {
        /// Hide the selection
        VisHide = 0,
        /// Show the selection
        VisShow = 1,
        /// Toggle visibility of the selection
        VisToggle = -1,
    };

    /** Set selection object visibility
     *
     * @param visible: see VisibleState
     * @param pDocName: name of the document that scopes the request, defaults to active document
     */
    void setVisible(VisibleState visible, const char* pDocName = nullptr);

    bool isClarifySelectionActive();
    void setClarifySelectionActive(bool active);

    /// signal on new object
    fastsignals::signal<void(const SelectionChanges& msg)> signalSelectionChanged;

    /// signal on selection change with resolved object
    fastsignals::signal<void(const SelectionChanges& msg)> signalSelectionChanged2;
    /// signal on selection change with resolved object and sub element map
    fastsignals::signal<void(const SelectionChanges& msg)> signalSelectionChanged3;

    /** Returns a vector of selection objects
     *
     * @param pDocName: document name. If no document name is given the objects
     * of the active are returned. If nothing for this Document is selected an
     * empty vector is returned. If document name is "*", then all document is
     * considered.
     * @param resolve: sub-object resolving mode
     * @param single: if set to true, then it will return an empty vector if
     * there is more than one selections.
     *
     * @return The returned vector reflects the sequence of selection.
     */
    std::vector<SelObj> getSelection(
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement,
        bool single = false
    ) const;
    /** Returns a vector of selection objects
     *
     * @param pDocName: document name. If no document name is given the objects
     * of the active are returned. If nothing for this Document is selected an
     * empty vector is returned. If document name is "*", then all documents are
     * considered.
     * @param typeId: specify the type of object to be returned.
     * @param resolve: sub-object resolving mode.
     * @param single: if set to true, then it will return an empty vector if
     * there is more than one selection.
     *
     * @return The returned vector reflects the sequence of selection.
     */
    std::vector<Gui::SelectionObject> getSelectionEx(
        const char* pDocName = nullptr,
        Base::Type typeId = App::DocumentObject::getClassTypeId(),
        ResolveMode resolve = ResolveMode::OldStyleElement,
        bool single = false
    ) const;
    /** Returns a vector of selection objects children of an object
     *
     * @param obj: Object within which you want to find a selection.
     * The selection objects returned will be direct children of Obj. The rest of the subname will
     * be unresolved. So this is equivalent to ResolveMode::NoResolve, but starting from obj. For
     * instance if you have : Assembly.Part.Body.LCS.PlaneXY
     * - If obj = Assembly : SelectionObject = Part, subname = "Body.LCS.PlaneXY."
     * - If obj = Part : SelectionObject = Body, subname = "LCS.PlaneXY."
     * - If obj = Body : SelectionObject = LCS, subname = "PlaneXY."
     * The docname used is the document of obj.
     * if obj is nullptr, this acts as getSelectionEx with empty docName
     *
     * @param typeId: specify the type of object to be returned.
     * @param single: if set to true, then it will return an empty vector if
     * there is more than one selection.
     *
     * @return The returned vector reflects the sequence of selection.
     */
    std::vector<Gui::SelectionObject> getSelectionIn(
        App::DocumentObject* obj = nullptr,
        Base::Type typeId = App::DocumentObject::getClassTypeId(),
        bool single = false
    ) const;

    /**
     * @brief getAsPropertyLinkSubList fills PropertyLinkSubList with current selection.
     * @param prop (output). The property object to receive links
     * @return the number of items written to the link
     */
    int getAsPropertyLinkSubList(App::PropertyLinkSubList& prop) const;

    /** Returns a vector of all selection objects of all documents. */
    std::vector<SelObj> getCompleteSelection(ResolveMode resolve = ResolveMode::OldStyleElement) const;

    /// Check if there is any selection
    bool hasSelection(const char* pDocName = nullptr) const;

    /** Check if there is any selection within a given document
     *
     * @param doc: specify the document to check for selection. If NULL, then
     *             check the current active document.
     * @param resolve: whether to resolve the selected sub-object
     *
     * If \c resolve is true, then the selection is first resolved before
     * matching its owner document. So in case the selected sub-object is
     * linked from an external document, it may not match the input \c doc.
     * If \c resolve is false, then the match is only done with the top
     * level parent object.
     */
    bool hasSelection(const char* doc, ResolveMode resolve) const;

    /** Check if there is any sub-element selection
     *
     * @param doc: optional document to check for selection
     * @param subElement: whether to count sub-element only selection
     *
     * Example sub selections are face, edge or vertex. If \c subElement is false,
     * then sub-object (i.e. a group child object) selection is also counted
     * even if it selects the whole sub-object.
     */
    bool hasSubSelection(const char* doc = nullptr, bool subElement = false) const;

    /// Check if there is any preselection
    bool hasPreselection() const;

    /// Size of selected entities for all documents
    unsigned int size(const char* pDocName = nullptr) const;

    /** @name Selection stack functions
     *
     * Selection stack is for storing selection history so that the user can go
     * back and forward to previous selections.
     */
    //@{
    /// Return the current selection stack size
    std::size_t selStackBackSize(const char* pDocName = nullptr) const;

    /// Return the current forward selection stack size
    std::size_t selStackForwardSize(const char* pDocName = nullptr) const;

    /** Obtain selected objects from stack
     *
     * @param pDocName: optional filtering document, NULL for current active
     *                  document
     * @param resolve: sub-object resolving mode.
     * @param index: optional position in the stack
     */
    std::vector<Gui::SelectionObject> selStackGet(
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement,
        int index = 0
    ) const;

    /** Go back selection history
     *
     * @param count: optional number of steps to go back
     * @param pDocName: the name of the document to index the context, defaults to active document
     *
     * This function pops the selection stack, and populate the current
     * selection with the content of the last pop'd entry
     */
    void selStackGoBack(int count = 1, const char* pDocName = nullptr);

    /** Go forward selection history
     *
     * @param count: optional number of steps to go back
     * @param pDocName: the name of the document to index the context, defaults to active document
     *
     * This function pops the selection stack, and populate the current
     * selection with the content of the last pop'd entry
     */
    void selStackGoForward(int count = 1, const char* pDocName = nullptr);

    /** Save the current selection on to the stack
     *
     * @param clearForward: whether to clear forward selection stack
     * @param overwrite: whether to overwrite the current top entry of the
     *                   stack instead of pushing a new entry.
     * @param pDocName: the name of the document to index the context, defaults to active document
     */
    void selStackPush(bool clearForward = true, bool overwrite = false, const char* pDocName = nullptr);
    //@}

    /** @name Picked list functions
     *
     * Picked list stores all selected geometry elements that intersects the
     * 3D pick point. The list population is done by SoFCUnifiedSelection through
     * addSelection() with the pickedList argument.
     */
    //@{
    /// Check whether picked list is enabled
    bool needPickedList(const char* pDocName = nullptr) const;
    /// Turn on or off picked list
    void enablePickedList(bool, const char* pDocName = nullptr);
    /// Check if there is any selection inside picked list
    bool hasPickedList(const char* pDocName = nullptr) const;
    /// Return select objects inside picked list
    std::vector<SelectionSingleton::SelObj> getPickedList(const char* pDocName) const;
    /// Return selected object inside picked list grouped by top level parents
    std::vector<Gui::SelectionObject> getPickedListEx(
        const char* pDocName = nullptr,
        Base::Type typeId = App::DocumentObject::getClassTypeId()
    ) const;
    //@}

    /** @name Selection style functions
     *
     * The selection style changes the way selection works. In Greedy selection
     * it is as if you were pressing Ctrl.
     */
    //@{
    enum class SelectionStyle
    {
        NormalSelection,
        GreedySelection
    };
    /// Changes the style of selection between greedy and normal.
    void setSelectionStyle(SelectionStyle selStyle, const char* pDocName = nullptr);
    /// Get the style of selection.
    SelectionStyle getSelectionStyle(const char* pDocName = nullptr);
    //@}

    static SelectionSingleton& instance();
    static void destruct();
    friend class SelectionFilter;

protected:
    /// Construction
    SelectionSingleton();
    /// Destruction
    ~SelectionSingleton() override;

    /// Observer message from the App doc
    void slotDeletedObject(const App::DocumentObject&);
    void slotClosedDocument(const App::Document&);

    /// helper to retrieve document by name
    App::Document* getDocument(const char* pDocName = nullptr) const;

    void slotSelectionChanged(const SelectionChanges& msg);

    void notify(SelectionChanges&& Chng);
    void notify(const SelectionChanges& Chng)
    {
        notify(SelectionChanges(Chng));
    }

    struct SelectionDescription
    {
        std::string DocName;
        std::string FeatName;
        std::string SubName;
        std::string TypeName;
        App::Document* pDoc = nullptr;
        App::DocumentObject* pObject = nullptr;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        bool logged = false;

        App::ElementNamePair elementName;
        App::DocumentObject* pResolvedObject = nullptr;

        void log(bool remove = false, bool clearPreselect = true);
        std::string getSubString() const;
    };

    enum class SelectionCheckResult
    {
        Invalid,
        Available,
        Selected
    };

    SelectionCheckResult checkSelection(
        const char* pDocName,
        const char* pObjectName,
        const char* pSubName,
        ResolveMode resolve,
        SelectionDescription& sel,
        const std::list<SelectionDescription>* selList = nullptr
    ) const;
    SelectionCheckResult resolveSelectionDescription(
        const char* pDocName,
        const char* pObjectName,
        const char*& pSubName,
        ResolveMode resolve,
        SelectionDescription& sel,
        std::string& subNamePrefix,
        bool reportErrors
    ) const;
    const std::list<SelectionDescription>* selectionListForCheck(
        const char* pDocName,
        const std::list<SelectionDescription>* selList
    ) const;
    static SelectionCheckResult findSelectionMatch(
        const char* pDocName,
        const char* pSubName,
        const std::string& subNamePrefix,
        ResolveMode resolve,
        const SelectionDescription& sel,
        const std::list<SelectionDescription>& selList
    );

    std::vector<Gui::SelectionObject> getObjectList(
        const char* pDocName,
        Base::Type typeId,
        const std::list<SelectionDescription>& objs,
        ResolveMode resolve,
        bool single = false
    ) const;
    static bool appendSelectionSubElement(
        SelectionObject& selection,
        const char* subelement,
        const SelectionDescription& sel,
        ResolveMode resolve
    );
    static bool appendObjectListEntry(
        std::vector<SelectionObject>& selections,
        std::map<App::DocumentObject*, size_t>& objectIndices,
        App::DocumentObject* obj,
        const char* subelement,
        const SelectionDescription& sel,
        ResolveMode resolve,
        bool single
    );
    struct SelectionInResult
    {
        App::DocumentObject* root {nullptr};
        std::string subName;
    };
    bool selectionInResult(
        SelectionObject& sel,
        const std::string& subName,
        App::DocumentObject* container,
        Base::Type typeId,
        App::Document*& doc,
        bool& containerPassed,
        SelectionInResult& result
    ) const;
    static bool appendSelectionInResult(
        std::vector<SelectionObject>& selections,
        std::map<App::DocumentObject*, size_t>& objectIndices,
        const SelectionInResult& result,
        const Base::Vector3d& pickedPoint,
        bool single
    );

    static App::DocumentObject* getObjectOfType(
        const SelectionDescription& sel,
        Base::Type type,
        ResolveMode resolve,
        const char** subelement = nullptr
    );

    unsigned int countObjectsOfType(
        const Base::Type& typeId = App::DocumentObject::getClassTypeId(),
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;

    using SelStackItem = std::set<App::SubObjectT>;
    // Each document has a description context
    struct SelectionInfo
    {
        Gui::SelectionGate* gate {nullptr};
        ResolveMode resolveMode {ResolveMode::OldStyleElement};

        std::list<SelectionDescription> selList;
        std::list<SelectionDescription> pickedList;
        bool needPickedList {false};

        std::deque<SelStackItem> selStackBack;
        std::deque<SelStackItem> selStackForward;

        SelectionStyle selectionStyle {SelectionStyle::NormalSelection};
    };
    struct SelectionContext
    {
        SelectionInfo* info;
        std::string docName;
    };
    struct SelectionConstContext
    {
        const SelectionInfo* info;
        std::string docName;
    };
    struct VisibilitySelection
    {
        std::string DocName;
        std::string FeatName;
        std::string SubName;
        VisibilitySelection(
            const std::string& docName,
            const std::string& featName,
            const std::string& subName
        )
            : DocName(docName)
            , FeatName(featName)
            , SubName(subName)
        {}
    };
    struct VisibilityTarget
    {
        App::DocumentObject* object {nullptr};
        App::DocumentObject* parent {nullptr};
        std::string elementName;
    };
    enum class VisibilityElementResult
    {
        Handled,
        Fallback
    };
    using VisibilityFilter = std::set<std::pair<App::DocumentObject*, App::DocumentObject*>>;
    struct VisibilityOperation
    {
        explicit VisibilityOperation(int visible)
            : visible(visible)
        {}

        int visible;
        VisibilityFilter filter;
    };
    std::vector<SelectionChanges> removeDeletedObjectSelections(
        SelectionInfo& info,
        const App::DocumentObject& obj
    );
    bool removeDeletedObjectFromPickedList(SelectionInfo& info, const App::DocumentObject& obj);
    SelStackItem selectionStackItem(const SelectionContext& context) const;
    bool restoreSelectionStackItem(const SelStackItem& item);
    void logBulkSelection(
        const char* pDocName,
        const char* pObjectName,
        const std::vector<std::string>& subNames
    ) const;
    bool appendBulkSelection(
        SelectionContext& context,
        const char* pDocName,
        const char* pObjectName,
        const std::string& pSubName,
        std::vector<std::string>* loggedSubNames
    );
    void notifyBulkSelectionAdded(const SelectionContext& context, const SelectionDescription& sel);
    std::vector<VisibilitySelection> visibilitySelectionSnapshot(const SelectionContext& context) const;
    bool resolveVisibilityTarget(const VisibilitySelection& sel, VisibilityTarget& target) const;
    VisibilityElementResult applyElementVisibility(
        const VisibilitySelection& sel,
        const VisibilityTarget& target,
        VisibilityOperation& operation
    );
    void applyObjectVisibility(
        const VisibilitySelection& sel,
        App::DocumentObject* obj,
        VisibilityOperation& operation
    );

    // Returns a selection context or nullptr if the document is not found
    SelectionContext getSelectionContext(const char* pDocName);
    SelectionConstContext getSelectionContext(const char* pDocName) const;
    bool selectionGateAllows(const SelectionContext& context, const SelectionDescription& sel) const;
    bool preselectionGateAllows(
        const SelectionContext& context,
        App::Document* pDoc,
        const char* pDocName,
        const char* pObjectName,
        const char* pSubName,
        SelectionChanges::MsgSource signal
    ) const;
    std::vector<SelectionChanges> removeSelectionMatches(
        SelectionContext& context,
        const SelectionDescription& sel
    );
    bool clearDocumentSelectionEntries(SelectionContext& context);
    void notifyViewProvidersOfClearSelection(
        const std::list<SelectionDescription>& selections,
        const char* pDocName
    ) const;
    void logDocumentClearSelection(const std::string& docName, bool clearPreSelect) const;
    void logCompleteClearSelection(bool clearPreSelect) const;
    void clearPickedList(SelectionContext& context);
    void replacePickedList(SelectionContext& context, const std::vector<SelObj>& pickedList);
    void notifySelectionRemovals(std::vector<SelectionChanges>& changes);

    struct PreselectionState
    {
        std::string docName;
        std::string objectName;
        std::string subName;

        bool empty() const
        {
            return docName.empty();
        }

        bool matches(const char* document, const char* object, const char* subElement) const
        {
            return docName == document && objectName == object && subName == subElement;
        }

        void set(const std::string& document, const char* object, const char* subElement)
        {
            docName = document;
            objectName = object;
            subName = subElement;
        }

        void reset()
        {
            docName.clear();
            objectName.clear();
            subName.clear();
        }
    };

    static SelectionSingleton* _pcSingleton;


    std::map<App::Document*, SelectionInfo> docSelectionContext;

    struct SelectionAllowance
    {
        bool allowed {false};
        std::string reason;
    };

    /** @brief Checks if a selection is allowed through the selecetion filter.
     * Uses SelectionGate (which has a SelectionFilter).
     * @param context The selection context.
     * @param sel The object to be selected.
     * @returns SelectionAllowance
     */
    SelectionAllowance isSelectionAllowed(
        const SelectionContext& context,
        const SelectionDescription& sel
    );
    // Preselection helpers, it's a mess, needs clarifying -theo-vt
    std::string DocName;
    std::string FeatName;
    std::string SubName;
    float hx {0.0f}, hy {0.0f}, hz {0.0f};
    PreselectionState preselection;
    SelectionChanges CurrentPreselection;


    int logDisabled {0};
    bool logHasSelection {false};
    bool clarifySelectionActive {false};

    std::deque<SelectionChanges> NotificationQueue;
    bool Notifying {false};
};

/**
 * A convenience template-based method that returns the number of objects of the given type.
 */
template<typename T>
inline unsigned int SelectionSingleton::countObjectsOfType(const char* pDocName, ResolveMode resolve) const
{
    static_assert(
        std::is_base_of<App::DocumentObject, T>::value,
        "Template parameter T must be derived from App::DocumentObject"
    );
    return this->countObjectsOfType(T::getClassTypeId(), pDocName, resolve);
}

/**
 * A convenience template-based method that returns an array with the correct types already.
 */
template<typename T>
inline std::vector<T*> SelectionSingleton::getObjectsOfType(const char* pDocName, ResolveMode resolve) const
{
    std::vector<T*> type;
    std::vector<App::DocumentObject*> obj
        = this->getObjectsOfType(T::getClassTypeId(), pDocName, resolve);
    type.reserve(obj.size());
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        type.push_back(static_cast<T*>(*it));
    }
    return type;
}

/// Get the global instance
inline SelectionSingleton& Selection()
{
    return SelectionSingleton::instance();
}

/** Helper class to disable logging selection action to MacroManager
 */
class GuiExport SelectionLogDisabler
{
public:
    explicit SelectionLogDisabler(bool silent = false)
        : silent(silent)
    {
        Selection().disableCommandLog();
    }
    ~SelectionLogDisabler()
    {
        Selection().enableCommandLog(silent);
    }

private:
    bool silent;
};

}  // namespace Gui
