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
#include <string>
#include <vector>

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <Base/Observer.h>

#include "SelectionObject.h"


using PyObject = struct _object;

namespace App
{
class DocumentObject;
class Document;
class PropertyLinkSubList;
}  // namespace App

namespace Gui
{

enum class ResolveMode
{
    /// No resolve
    NoResolve,
    /// Sub-object with old style element name
    OldStyleElement,
    /// Sub-object with new style element name
    NewStyleElement,
    /// Resolve sub-object and follow link
    FollowLink
};

class SelectionObject;
class SelectionFilter;

/** Transport the changes of the Selection
 * This class transports closer information what was changed in the
 * selection. It's an optional information and not all commands set this
 * information. If not set all observer of the selection assume a full change
 * and update everything (e.g 3D view). This is not a very good idea if, e.g. only
 * a small parameter has changed. Therefore one can use this class and make the
 * update of the document much faster!
 * @see Base::Observer
 */
class GuiExport SelectionChanges
{
public:
    enum MsgType
    {
        AddSelection,
        RmvSelection,
        SetSelection,
        ClrSelection,
        SetPreselect,  // to signal observer the preselect has changed
        RmvPreselect,
        SetPreselectSignal,  // to request 3D view to change preselect
        PickedListChanged,
        ShowSelection,       // to show a selection
        HideSelection,       // to hide a selection
        RmvPreselectSignal,  // to request 3D view to remove preselect
        MovePreselect,       // to signal observer the mouse movement when preselect
    };
    enum class MsgSource
    {
        Any = 0,
        Internal = 1,
        TreeView = 2
    };

    SelectionChanges(
        MsgType type = ClrSelection,
        const char* docName = nullptr,
        const char* objName = nullptr,
        const char* subName = nullptr,
        const char* typeName = nullptr,
        float x = 0,
        float y = 0,
        float z = 0,
        MsgSource subtype = MsgSource::Any
    )
        : Type(type)
        , SubType(subtype)
        , x(x)
        , y(y)
        , z(z)
        , Object(docName, objName, subName)
    {
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        if (typeName) {
            TypeName = typeName;
        }
        pTypeName = TypeName.c_str();
    }  // explicit bombs

    SelectionChanges(
        MsgType type,
        const std::string& docName,
        const std::string& objName,
        const std::string& subName,
        const std::string& typeName = std::string(),
        float x = 0,
        float y = 0,
        float z = 0,
        MsgSource subtype = MsgSource::Any
    )
        : Type(type)
        , SubType(subtype)
        , x(x)
        , y(y)
        , z(z)
        , Object(docName.c_str(), objName.c_str(), subName.c_str())
        , TypeName(typeName)
    {
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        pTypeName = TypeName.c_str();
    }

    SelectionChanges(const SelectionChanges& other)
    {
        *this = other;
    }

    SelectionChanges& operator=(const SelectionChanges& other)
    {
        Type = other.Type;
        SubType = other.SubType;
        x = other.x;
        y = other.y;
        z = other.z;
        Object = other.Object;
        TypeName = other.TypeName;
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        pTypeName = TypeName.c_str();
        pOriginalMsg = other.pOriginalMsg;
        return *this;
    }

    SelectionChanges(SelectionChanges&& other)
    {
        *this = std::move(other);
    }

    SelectionChanges& operator=(SelectionChanges&& other)
    {
        Type = other.Type;
        SubType = other.SubType;
        x = other.x;
        y = other.y;
        z = other.z;
        Object = std::move(other.Object);
        TypeName = std::move(other.TypeName);
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        pTypeName = TypeName.c_str();
        pOriginalMsg = other.pOriginalMsg;
        return *this;
    }

    MsgType Type;
    MsgSource SubType;

    const char* pDocName;
    const char* pObjectName;
    const char* pSubName;
    const char* pTypeName;
    float x;
    float y;
    float z;

    App::SubObjectT Object;
    std::string TypeName;

    // Original selection message in case resolve!=0
    const SelectionChanges* pOriginalMsg = nullptr;
};

class ViewProviderDocumentObject;

/**
 * The SelectionObserver class simplifies the step to write classes that listen
 * to what happens to the selection.
 *
 * @author Werner Mayer
 */
class GuiExport SelectionObserver
{

public:
    /** Constructor
     *
     * @param attach: whether to attach this observer on construction
     * @param resolve: sub-object resolving mode.
     */
    explicit SelectionObserver(bool attach = true, ResolveMode resolve = ResolveMode::OldStyleElement);
    /** Constructor
     *
     * @param vp: filtering view object.
     * @param attach: whether to attach this observer on construction
     * @param resolve: sub-object resolving mode.
     *
     * Constructs an selection observer that receives only selection event of
     * objects within the same document as the input view object.
     */
    explicit SelectionObserver(
        const Gui::ViewProviderDocumentObject* vp,
        bool attach = true,
        ResolveMode resolve = ResolveMode::OldStyleElement
    );

    virtual ~SelectionObserver();
    bool blockSelection(bool block);
    bool isSelectionBlocked() const;
    bool isSelectionAttached() const;

    /** Attaches to the selection. */
    void attachSelection();
    /** Detaches from the selection. */
    void detachSelection();

private:
    virtual void onSelectionChanged(const SelectionChanges& msg) = 0;
    void _onSelectionChanged(const SelectionChanges& msg);

private:
    using Connection = fastsignals::connection;
    Connection connectSelection;
    std::string filterDocName;
    std::string filterObjName;
    ResolveMode resolve;
    bool blockedSelection;
};

/** SelectionGate
 * The selection gate allows or disallows selection of certain types.
 * It has to be registered to the selection.
 */
class GuiExport SelectionGate
{
public:
    virtual ~SelectionGate() = default;
    virtual bool allow(App::Document*, App::DocumentObject*, const char*) = 0;

    /**
     * @brief notAllowedReason is a string that sets the message to be
     * displayed in statusbar for cluing the user on why is the selection not
     * allowed. Set this variable in allow() implementation. Enclose the
     * literal into QT_TR_NOOP() for translatability.
     */
    std::string notAllowedReason;
};

/** SelectionGateFilterExternal
 * The selection gate disallows any external object
 */
class GuiExport SelectionGateFilterExternal: public SelectionGate
{
public:
    explicit SelectionGateFilterExternal(const char* docName, const char* objName = nullptr);
    bool allow(App::Document*, App::DocumentObject*, const char*) override;

private:
    std::string DocName;
    std::string ObjName;
};

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
    void clearCompleteSelection(bool clearPreSelect = true);
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

    const char* getSelectedElement(App::DocumentObject*, const char* pSubName) const;

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
    /// add a SelectionGate to control what is selectable
    void addSelectionGate(Gui::SelectionGate* gate, ResolveMode resolve = ResolveMode::OldStyleElement);
    /// remove the active SelectionGate
    void rmvSelectionGate();

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
     */
    void setVisible(VisibleState visible);

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
    bool hasSelection() const;

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
    bool hasSelection(const char* doc, ResolveMode resolve = ResolveMode::OldStyleElement) const;

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
    unsigned int size() const
    {
        return static_cast<unsigned int>(_SelList.size());
    }

    /** @name Selection stack functions
     *
     * Selection stack is for storing selection history so that the user can go
     * back and forward to previous selections.
     */
    //@{
    /// Return the current selection stack size
    std::size_t selStackBackSize() const
    {
        return _SelStackBack.size();
    }

    /// Return the current forward selection stack size
    std::size_t selStackForwardSize() const
    {
        return _SelStackForward.size();
    }

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
     *
     * This function pops the selection stack, and populate the current
     * selection with the content of the last pop'd entry
     */
    void selStackGoBack(int count = 1);

    /** Go forward selection history
     *
     * @param count: optional number of steps to go back
     *
     * This function pops the selection stack, and populate the current
     * selection with the content of the last pop'd entry
     */
    void selStackGoForward(int count = 1);

    /** Save the current selection on to the stack
     *
     * @param clearForward: whether to clear forward selection stack
     * @param overwrite: whether to overwrite the current top entry of the
     *                   stack instead of pushing a new entry.
     */
    void selStackPush(bool clearForward = true, bool overwrite = false);
    //@}

    /** @name Picked list functions
     *
     * Picked list stores all selected geometry elements that intersects the
     * 3D pick point. The list population is done by SoFCUnifiedSelection through
     * addSelection() with the pickedList argument.
     */
    //@{
    /// Check whether picked list is enabled
    bool needPickedList() const;
    /// Turn on or off picked list
    void enablePickedList(bool);
    /// Check if there is any selection inside picked list
    bool hasPickedList() const;
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
    void setSelectionStyle(SelectionStyle selStyle);
    /// Get the style of selection.
    SelectionStyle getSelectionStyle();
    //@}

    static SelectionSingleton& instance();
    static void destruct();
    friend class SelectionFilter;

    // Python interface
    static PyMethodDef Methods[];

protected:
    static PyObject* sAddSelection(PyObject* self, PyObject* args);
    static PyObject* sUpdateSelection(PyObject* self, PyObject* args);
    static PyObject* sRemoveSelection(PyObject* self, PyObject* args);
    static PyObject* sClearSelection(PyObject* self, PyObject* args);
    static PyObject* sIsSelected(PyObject* self, PyObject* args);
    static PyObject* sCountObjectsOfType(PyObject* self, PyObject* args);
    static PyObject* sGetSelection(PyObject* self, PyObject* args);
    static PyObject* sSetPreselection(PyObject* self, PyObject* args, PyObject* kwd);
    static PyObject* sGetPreselection(PyObject* self, PyObject* args);
    static PyObject* sRemPreselection(PyObject* self, PyObject* args);
    static PyObject* sGetCompleteSelection(PyObject* self, PyObject* args);
    static PyObject* sGetSelectionEx(PyObject* self, PyObject* args);
    static PyObject* sGetSelectionObject(PyObject* self, PyObject* args);
    static PyObject* sSetSelectionStyle(PyObject* self, PyObject* args);
    static PyObject* sAddSelObserver(PyObject* self, PyObject* args);
    static PyObject* sRemSelObserver(PyObject* self, PyObject* args);
    static PyObject* sAddSelectionGate(PyObject* self, PyObject* args);
    static PyObject* sRemoveSelectionGate(PyObject* self, PyObject* args);
    static PyObject* sGetPickedList(PyObject* self, PyObject* args);
    static PyObject* sEnablePickedList(PyObject* self, PyObject* args);
    static PyObject* sPreselect(PyObject* self, PyObject* args);
    static PyObject* sSetVisible(PyObject* self, PyObject* args);
    static PyObject* sPushSelStack(PyObject* self, PyObject* args);
    static PyObject* sHasSelection(PyObject* self, PyObject* args);
    static PyObject* sHasSubSelection(PyObject* self, PyObject* args);
    static PyObject* sGetSelectionFromStack(PyObject* self, PyObject* args);

protected:
    /// Construction
    SelectionSingleton();
    /// Destruction
    ~SelectionSingleton() override;

    /// Observer message from the App doc
    void slotDeletedObject(const App::DocumentObject&);

    /// helper to retrieve document by name
    App::Document* getDocument(const char* pDocName = nullptr) const;

    void slotSelectionChanged(const SelectionChanges& msg);

    SelectionChanges CurrentPreselection;

    std::deque<SelectionChanges> NotificationQueue;
    bool Notifying = false;

    void notify(SelectionChanges&& Chng);
    void notify(const SelectionChanges& Chng)
    {
        notify(SelectionChanges(Chng));
    }

    struct _SelObj
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
    mutable std::list<_SelObj> _SelList;

    mutable std::list<_SelObj> _PickedList;
    bool _needPickedList {false};

    using SelStackItem = std::set<App::SubObjectT>;
    std::deque<SelStackItem> _SelStackBack;
    std::deque<SelStackItem> _SelStackForward;

    int checkSelection(
        const char* pDocName,
        const char* pObjectName,
        const char* pSubName,
        ResolveMode resolve,
        _SelObj& sel,
        const std::list<_SelObj>* selList = nullptr
    ) const;

    std::vector<Gui::SelectionObject> getObjectList(
        const char* pDocName,
        Base::Type typeId,
        std::list<_SelObj>& objs,
        ResolveMode resolve,
        bool single = false
    ) const;

    static App::DocumentObject* getObjectOfType(
        _SelObj& sel,
        Base::Type type,
        ResolveMode resolve,
        const char** subelement = nullptr
    );

    unsigned int countObjectsOfType(
        const Base::Type& typeId = App::DocumentObject::getClassTypeId(),
        const char* pDocName = nullptr,
        ResolveMode resolve = ResolveMode::OldStyleElement
    ) const;

    static SelectionSingleton* _pcSingleton;

    std::string DocName;
    std::string FeatName;
    std::string SubName;
    float hx, hy, hz;

    Gui::SelectionGate* ActiveGate;
    ResolveMode gateResolve;

    int logDisabled = 0;
    bool logHasSelection = false;
    bool clarifySelectionActive = false;

    SelectionStyle selectionStyle;
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
