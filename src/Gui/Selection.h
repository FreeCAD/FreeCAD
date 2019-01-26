/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#ifndef GUI_SELECTION_H
#define GUI_SELECTION_H

// Std. configurations

#include <string>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <boost/signals2.hpp>
#include <CXX/Objects.hxx>

#include <Base/Observer.h>
#include <Base/Type.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/SelectionObject.h>

namespace App
{
  class DocumentObject;
  class Document;
}

namespace Gui
{

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
    enum MsgType {
        AddSelection,
        RmvSelection,
        SetSelection,
        ClrSelection,
        SetPreselect, // to signal observer the preselect has changed
        RmvPreselect,
        SetPreselectSignal, // to request 3D view to change preselect
        PickedListChanged,
        ShowSelection, // to show a selection
        HideSelection, // to hide a selection
        RmvPreselectSignal, // to request 3D view to remove preselect
    };

    SelectionChanges(MsgType type = ClrSelection, 
            const char *docName=0, const char *objName=0,
            const char *subName=0, const char *typeName=0,
            float x=0, float y=0, float z=0, int subtype=0)
        : Type(type),SubType(subtype)
        , x(x),y(y),z(z)
    {
        if(docName) DocName=docName;
        pDocName = DocName.c_str();
        if(objName) ObjName = objName;
        pObjectName = ObjName.c_str();
        if(subName) SubName = subName;
        pSubName = SubName.c_str();
        if(typeName) TypeName = typeName;
        pTypeName = TypeName.c_str();
    }

    SelectionChanges(MsgType type,
                     const std::string &docName, 
                     const std::string &objName, 
                     const std::string &subName,
                     const std::string &typeName = std::string(),
                     float x=0,float y=0,float z=0, int subtype=0)
        : Type(type), SubType(subtype)
        , x(x),y(y),z(z)
        , DocName(docName)
        , ObjName(objName)
        , SubName(subName)
        , TypeName(typeName)
    {
        pDocName = DocName.c_str();
        pObjectName = ObjName.c_str();
        pSubName = SubName.c_str();
        pTypeName = TypeName.c_str();
    }

    SelectionChanges(const SelectionChanges &other) {
        *this = other;
    }

    SelectionChanges &operator=(const SelectionChanges &other) {
        Type = other.Type;
        SubType = other.SubType;
        x = other.x;
        y = other.y;
        z = other.z;
        DocName = other.DocName;
        ObjName = other.ObjName;
        SubName = other.SubName;
        TypeName = other.TypeName;
        pDocName = DocName.c_str();
        pObjectName = ObjName.c_str();
        pSubName = SubName.c_str();
        pTypeName = TypeName.c_str();
        return *this;
    }

    SelectionChanges(SelectionChanges &&other) {
        *this = std::move(other);
    }

    SelectionChanges &operator=(SelectionChanges &&other) {
        Type = other.Type;
        SubType = other.SubType;
        x = other.x;
        y = other.y;
        z = other.z;
        DocName = std::move(other.DocName);
        ObjName = std::move(other.ObjName);
        SubName = std::move(other.SubName);
        TypeName = std::move(other.TypeName);
        pDocName = DocName.c_str();
        pObjectName = ObjName.c_str();
        pSubName = SubName.c_str();
        pTypeName = TypeName.c_str();
        return *this;
    }

    MsgType Type;
    int SubType;

    const char* pDocName;
    const char* pObjectName;
    const char* pSubName;
    const char* pTypeName;
    float x;
    float y;
    float z;

    // For more robust selection notification (e.g. in case user make selection
    // change inside selection notification handler), the notification message
    // shall make a copy of all the strings here.
    std::string DocName;
    std::string ObjName;
    std::string SubName;
    std::string TypeName;
};

} //namespace Gui



// Export an instance of the base class (to avoid warning C4275, see also
// C++ Language Reference/General Rules and Limitations on MSDN for more details.)
//
// For compiler gcc4.1 we need to define the template class outside namespace 'Gui'
// otherwise we get the compiler error:
// 'explicit instantiation of 'class Base::Subject<const Gui::SelectionChanges&>'
// in namespace 'Gui' (which does not enclose namespace 'Base')
//
// It seems that this construct is not longer needed for gcc4.4 and even leads to
// errors under Mac OS X. Thus, we check for version between 4.1 and 4.4.
// It seems that for Mac OS X this can be completely ignored

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && !defined(FC_OS_MACOSX)
#define GNUC_VERSION (((__GNUC__)<<16)+((__GNUC_MINOR__)<<8))
#if GNUC_VERSION >= 0x040100 && GNUC_VERSION < 0x040400
template class GuiExport Base::Subject<const Gui::SelectionChanges&>;
#endif
#undef GNUC_VERSION
#endif

namespace Gui
{
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
    /// Constructor
    SelectionObserver(bool attach = true, int resolve = 1);
    SelectionObserver(const Gui::ViewProviderDocumentObject *vp, bool attach=true, int resolve=1);
    virtual ~SelectionObserver();
    bool blockConnection(bool block);
    bool isConnectionBlocked() const;
    bool isConnectionAttached() const;

    /** Attaches to the selection. */
    void attachSelection();
    /** Detaches from the selection. */
    void detachSelection();

private:
    virtual void onSelectionChanged(const SelectionChanges& msg) = 0;
    void _onSelectionChanged(const SelectionChanges& msg);

private:
    typedef boost::signals2::connection Connection;
    Connection connectSelection;
    std::string filterDocName;
    std::string filterObjName;
    int resolve;
    bool blockSelection;
};

/**
 * The SelectionObserverPython class implements a mechanism to register
 * a Python class instance implementing the required interface in order
 * to be notified on selection changes.
 *
 * @author Werner Mayer
 */
class GuiExport SelectionObserverPython : public SelectionObserver
{

public:
    /// Constructor
    SelectionObserverPython(const Py::Object& obj, int resolve=1);
    virtual ~SelectionObserverPython();

    static void addObserver(const Py::Object& obj, int resolve=1);
    static void removeObserver(const Py::Object& obj);

private:
    void onSelectionChanged(const SelectionChanges& msg);
    void addSelection(const SelectionChanges&);
    void removeSelection(const SelectionChanges&);
    void setSelection(const SelectionChanges&);
    void clearSelection(const SelectionChanges&);
    void setPreselection(const SelectionChanges&);
    void removePreselection(const SelectionChanges&);
    void pickedListChanged();

private:
    Py::Object inst;

#define FC_PY_SEL_OBSERVER \
    FC_PY_ELEMENT(onSelectionChanged) \
    FC_PY_ELEMENT(addSelection) \
    FC_PY_ELEMENT(removeSelection) \
    FC_PY_ELEMENT(setSelection) \
    FC_PY_ELEMENT(clearSelection) \
    FC_PY_ELEMENT(setPreselection) \
    FC_PY_ELEMENT(removePreselection) \
    FC_PY_ELEMENT(pickedListChanged)

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) Py::Object py_##_name;

    FC_PY_SEL_OBSERVER

    static std::vector<SelectionObserverPython*> _instances;
};

/** SelectionGate
 * The selection gate allows or disallows selection of certain types.
 * It has to be registered to the selection.
 */
class GuiExport SelectionGate
{
public:
    virtual ~SelectionGate(){}
    virtual bool allow(App::Document*,App::DocumentObject*, const char*)=0;

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
    SelectionGateFilterExternal(const char *docName, const char *objName=0);
    virtual bool allow(App::Document*,App::DocumentObject*, const char*) override;
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
class GuiExport SelectionSingleton : public Base::Subject<const SelectionChanges&>
{
public:
    struct SelObj {
        const char* DocName;
        const char* FeatName;
        const char* SubName;
        const char* TypeName;
        App::Document* pDoc;
        App::DocumentObject*  pObject;
        float x,y,z;
    };

    /// Add to selection 
    bool addSelection(const char* pDocName, const char* pObjectName=0, const char* pSubName=0, 
            float x=0, float y=0, float z=0, const std::vector<SelObj> *pickedList = 0, bool clearPreSelect=true);
    bool addSelection2(const char* pDocName, const char* pObjectName=0, const char* pSubName=0, 
            float x=0, float y=0, float z=0, const std::vector<SelObj> *pickedList = 0)
    {
        return addSelection(pDocName,pObjectName,pSubName,x,y,z,pickedList,false);
    }

    /// Add to selection
    bool addSelection(const SelectionObject&, bool clearPreSelect=true);
    /// Add to selection with several sub-elements
    bool addSelections(const char* pDocName, const char* pObjectName, const std::vector<std::string>& pSubNames);
    /// Update a selection 
    bool updateSelection(bool show, const char* pDocName, const char* pObjectName=0, const char* pSubName=0);
    /// Remove from selection (for internal use)
    void rmvSelection(const char* pDocName, const char* pObjectName=0, const char* pSubName=0, 
            const std::vector<SelObj> *pickedList = 0);
    /// Set the selection for a document
    void setSelection(const char* pDocName, const std::vector<App::DocumentObject*>&);
    /// Clear the selection of document \a pDocName. If the document name is not given the selection of the active document is cleared.
    void clearSelection(const char* pDocName=0, bool clearPreSelect=true);
    /// Clear the selection of all documents
    void clearCompleteSelection(bool clearPreSelect=true);
    /// Check if selected
    bool isSelected(const char* pDocName, const char* pObjectName=0, 
            const char* pSubName=0, int resolve=1) const;
    /// Check if selected
    bool isSelected(App::DocumentObject*, const char* pSubName=0, int resolve=1) const;

    const char *getSelectedElement(App::DocumentObject*, const char* pSubName) const;

    /// set the preselected object (mostly by the 3D view)
    int setPreselect(const char* pDocName, const char* pObjectName, 
            const char* pSubName, float x=0, float y=0, float z=0, int signal=0);
    /// remove the present preselection
    void rmvPreselect(bool signal=false);
    /// sets different coords for the preselection
    void setPreselectCoord(float x, float y, float z);
    /// returns the present preselection
    const SelectionChanges& getPreselection(void) const;
    /// add a SelectionGate to control what is selectable
    void addSelectionGate(Gui::SelectionGate *gate, int resolve = 1);
    /// remove the active SelectionGate
    void rmvSelectionGate(void);

    int disableCommandLog();
    int enableCommandLog(bool silent=false);

    /** Returns the number of selected objects with a special object type
     * It's the convenient way to check if the right objects are selected to
     * perform an operation (GuiCommand). The check also detects base types.
     * E.g. "Part" also fits on "PartImport" or "PartTransform types.
     * If no document name is given the active document is assumed.
     *
     * Set 'resolve' to true to resolve any sub object inside selection SubName
     * field
     */
    unsigned int countObjectsOfType(const Base::Type& typeId=App::DocumentObject::getClassTypeId(),
            const char* pDocName=0, int resolve=1) const;

    /**
     * Does basically the same as the method above unless that it accepts a string literal as first argument.
     * \a typeName must be a registered type, otherwise 0 is returned.
     */
    unsigned int countObjectsOfType(const char* typeName, 
            const char* pDocName=0, int resolve=1) const;

    /** Returns a vector of objects of type \a TypeName selected for the given document name \a pDocName.
     * If no document name is specified the objects from the active document are regarded.
     * If no objects of this document are selected an empty vector is returned.
     * @note The vector reflects the sequence of selection.
     */
    std::vector<App::DocumentObject*> getObjectsOfType(const Base::Type& typeId, 
            const char* pDocName=0, int resolve=1) const;

    /**
     * Does basically the same as the method above unless that it accepts a string literal as first argument.
     * \a typeName must be a registered type otherwise an empty array is returned.
     */
    std::vector<App::DocumentObject*> getObjectsOfType(const char* typeName, 
            const char* pDocName=0, int resolve=1) const;
    /**
     * A convenience template-based method that returns an array with the correct types already.
     */
    template<typename T> inline std::vector<T*> getObjectsOfType(
            const char* pDocName=0, int resolve=1) const;

    /** Set selection object visibility
     *
     * @param visible: 1: make visible, 0: make invisible, -1: toggle visibility
     */
    void setVisible(int visible);

    /// signal on new object
    boost::signals2::signal<void (const SelectionChanges& msg)> signalSelectionChanged;

    /// signal on selection change with resolved object
    boost::signals2::signal<void (const SelectionChanges& msg)> signalSelectionChanged2;
    /// signal on selection change with resolved object and sub element map
    boost::signals2::signal<void (const SelectionChanges& msg)> signalSelectionChanged3;

    /** Returns a vector of selection objects
     *
     * @param pDocName: document name. If no document name is given the objects
     * of the active are returned. If nothing for this Document is selected an
     * empty vector is returned. If document name is "*", then all document is
     * considered. 
     * @param resolve: whether to resolve the subname reference of the selection
     * @param single: if set to true, then it will return an empty vector if
     * there is more than one selections.
     *
     * @return The returned vector reflects the sequence of selection.
     */
    std::vector<SelObj> getSelection(const char* pDocName=0, int resolve=1, bool single=false) const;
    /** Returns a vector of selection objects
     *
     * @param pDocName: document name. If no document name is given the objects
     * of the active are returned. If nothing for this Document is selected an
     * empty vector is returned. If document name is "*", then all document is
     * considered. 
     * @param typeId: specify the type of object to be returned.
     * @param resolve: whether to resolve the subname reference of the selection
     * @param single: if set to true, then it will return an empty vector if
     * there is more than one selections.
     *
     * @return The returned vector reflects the sequence of selection.
     */
    std::vector<Gui::SelectionObject> getSelectionEx(const char* pDocName=0,Base::Type typeId=App::DocumentObject::getClassTypeId(),int resolve=1, bool single=false) const;

    /**
     * @brief getAsPropertyLinkSubList fills PropertyLinkSubList with current selection.
     * @param prop (output). The property object to receive links
     * @return the number of items written to the link
     */
    int getAsPropertyLinkSubList(App::PropertyLinkSubList &prop) const;

    /** Returns a vector of all selection objects of all documents. */
    std::vector<SelObj> getCompleteSelection(int resolve=1) const;
    bool hasSelection() const;
    bool hasSelection(const char* doc, bool resolve=true) const;
    bool hasPreselection() const;

    /// Size of selected entities for all documents
    unsigned int size(void) const {
        return static_cast<unsigned int>(_SelList.size());
    }

    int selStackBackSize() const {return _SelStackBack.size();}
    int selStackForwardSize() const {return _SelStackForward.size();}
    std::vector<Gui::SelectionObject> selStackGet(const char* pDocName=0,int resolve=1,int index=0) const;
    void selStackGoBack(int count=1);
    void selStackGoForward(int count=1);
    void selStackPush(bool clearForward=true, bool overwrite=false);

    bool needPickedList() const;
    void enablePickedList(bool);
    bool hasPickedList() const;
    std::vector<SelectionSingleton::SelObj> getPickedList(const char* pDocName) const;
    std::vector<Gui::SelectionObject> getPickedListEx(const char* pDocName=0,Base::Type typeId=App::DocumentObject::getClassTypeId()) const;
    static SelectionSingleton& instance(void);
    static void destruct (void);
    friend class SelectionFilter;

    // Python interface
    static PyMethodDef    Methods[];

protected:
    static PyObject *sAddSelection        (PyObject *self,PyObject *args);
    static PyObject *sUpdateSelection     (PyObject *self,PyObject *args);
    static PyObject *sRemoveSelection     (PyObject *self,PyObject *args);
    static PyObject *sClearSelection      (PyObject *self,PyObject *args);
    static PyObject *sIsSelected          (PyObject *self,PyObject *args);
    static PyObject *sCountObjectsOfType  (PyObject *self,PyObject *args);
    static PyObject *sGetSelection        (PyObject *self,PyObject *args);
    static PyObject *sGetPreselection     (PyObject *self,PyObject *args);
    static PyObject *sRemPreselection     (PyObject *self,PyObject *args);
    static PyObject *sGetCompleteSelection(PyObject *self,PyObject *args);
    static PyObject *sGetSelectionEx      (PyObject *self,PyObject *args);
    static PyObject *sGetSelectionObject  (PyObject *self,PyObject *args);
    static PyObject *sAddSelObserver      (PyObject *self,PyObject *args);
    static PyObject *sRemSelObserver      (PyObject *self,PyObject *args);
    static PyObject *sAddSelectionGate    (PyObject *self,PyObject *args);
    static PyObject *sRemoveSelectionGate (PyObject *self,PyObject *args);
    static PyObject *sGetPickedList       (PyObject *self,PyObject *args);
    static PyObject *sEnablePickedList    (PyObject *self,PyObject *args);
    static PyObject *sPreselect           (PyObject *self,PyObject *args);
    static PyObject *sSetVisible          (PyObject *self,PyObject *args);
    static PyObject *sPushSelStack        (PyObject *self,PyObject *args);
    static PyObject *sHasSelection        (PyObject *self,PyObject *args);
    static PyObject *sGetSelectionFromStack(PyObject *self,PyObject *args);

protected:
    /// Construction
    SelectionSingleton();
    /// Destruction
    virtual ~SelectionSingleton();

    /// Observer message from the App doc
    void slotDeletedObject(const App::DocumentObject&);

    /// helper to retrieve document by name
    App::Document* getDocument(const char* pDocName=0) const;

    void slotSelectionChanged(const SelectionChanges& msg);

    SelectionChanges CurrentPreselection;

    std::deque<SelectionChanges> NotificationQueue;
    bool Notifying = false;

    void notify(SelectionChanges &&Chng);
    void notify(const SelectionChanges &Chng) { notify(SelectionChanges(Chng)); }

    struct _SelObj {
        std::string DocName;
        std::string FeatName;
        std::string SubName;
        std::string TypeName;
        App::Document* pDoc;
        App::DocumentObject* pObject;
        float x,y,z;
        bool logged = false;

        std::pair<std::string,std::string> elementName;
        App::DocumentObject* pResolvedObject = 0;

        void log(bool remove=false, bool clearPreselect=true);
    };
    mutable std::list<_SelObj> _SelList;

    mutable std::list<_SelObj> _PickedList;
    bool _needPickedList;

    typedef std::set<std::array<std::string,3> > SelStackItem;
    std::deque<SelStackItem> _SelStackBack;
    std::deque<SelStackItem> _SelStackForward;

    int checkSelection(const char *pDocName, const char *pObjectName, 
            const char *pSubName,int resolve, _SelObj &sel, const std::list<_SelObj> *selList=0) const;

    std::vector<Gui::SelectionObject> getObjectList(const char* pDocName,Base::Type typeId, std::list<_SelObj> &objs, int resolve, bool single=false) const;

    static App::DocumentObject *getObjectOfType(_SelObj &sel, Base::Type type, 
            int resolve, const char **subelement=0);

    static SelectionSingleton* _pcSingleton;

    std::string DocName;
    std::string FeatName;
    std::string SubName;
    float hx,hy,hz;

    Gui::SelectionGate *ActiveGate;
    int gateResolve;

    int logDisabled = 0;
    bool logHasSelection = false;
};

/**
 * A convenience template-based method that returns an array with the correct types already.
 */
template<typename T>
inline std::vector<T*> SelectionSingleton::getObjectsOfType(const char* pDocName, int resolve) const
{
    std::vector<T*> type;
    std::vector<App::DocumentObject*> obj = this->getObjectsOfType(T::getClassTypeId(), pDocName, resolve);
    type.reserve(obj.size());
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
        type.push_back(static_cast<T*>(*it));
    return type;
}

/// Get the global instance
inline SelectionSingleton& Selection(void)
{
    return SelectionSingleton::instance();
}

class GuiExport SelectionLogDisabler {
public:
    SelectionLogDisabler(bool silent=false) :silent(silent) {
        Selection().disableCommandLog();
    }
    ~SelectionLogDisabler() {
        Selection().enableCommandLog(silent);
    }
private:
    bool silent;
};

} //namespace Gui

#endif // GUI_SELECTION_H
