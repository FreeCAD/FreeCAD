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


#ifndef APP_DOCUMENTOBJECT_H
#define APP_DOCUMENTOBJECT_H

#include <App/TransactionalObject.h>
#include <App/PropertyExpressionEngine.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Base/SmartPtrPy.h>

#include <bitset>
#include <unordered_map>

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

enum ObjectStatus {
    Touch = 0,
    Error = 1,
    New = 2,
    Recompute = 3, // set when the object is currently being recomputed
    Restore = 4,
    Remove = 5,
    PythonCall = 6,
    Destroy = 7,
    Enforce = 8,
    Recompute2 = 9, // set when the object is being recomputed in the second pass
    PartialObject = 10,
    PendingRecompute = 11, // set by Document, indicating the object is in recomputation queue
    ObjImporting = 13, // Mark the object as importing
    NoTouch = 14, // no touch on any property change
    GeoExcluded = 15, // mark as a member but not claimed by GeoFeatureGroup
    Expand = 16, // indicate the object's tree item expansion status
    NoAutoExpand = 17, // disable tree item auto expand on selection for this object
    PendingTransactionUpdate = 18, // mark that the object expects a call to onUndoRedoFinished() after transaction is finished.
    RecomputeExtension = 19, // mark the object to recompute its extensions
    TouchOnColorChange = 20, // inform view provider touch object on color change
};

/** Return object for feature execution
*/
class AppExport DocumentObjectExecReturn
{
public:
    explicit DocumentObjectExecReturn(const std::string& sWhy, DocumentObject* WhichObject=nullptr)
        : Why(sWhy), Which(WhichObject)
    {
    }
    explicit DocumentObjectExecReturn(const char* sWhy, DocumentObject* WhichObject=nullptr)
        : Which(WhichObject)
    {
        if (sWhy)
            Why = sWhy;
    }

    std::string Why;
    DocumentObject* Which;
};



/** Base class of all Classes handled in the Document
 */
class AppExport DocumentObject: public App::TransactionalObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::DocumentObject);

public:

    PropertyString Label;
    PropertyString Label2;
    PropertyExpressionEngine ExpressionEngine;

    /// Allow control visibility status in App name space
    PropertyBool Visibility;

    /// signal before changing a property of this object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalBeforeChange;
    /// signal on changed  property of this object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalChanged;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName() const {
        return "";
    }
    /**
     * This function is introduced to allow Python feature override its view provider.
     * The default implementation just returns \ref getViewProviderName().
     *
     * The core will only accept the overridden view provider if it returns
     * true when calling Gui::ViewProviderDocumentObject::allowOverride(obj).
     * If not, the view provider will be reverted to the one returned from \ref
     * getViewProviderName().
     */
    virtual const char *getViewProviderNameOverride() const {
        return getViewProviderName();
    }

    /// Constructor
    DocumentObject();
    ~DocumentObject() override;

    /// returns the name which is set in the document for this object (not the name property!)
    const char *getNameInDocument() const;
    /// Return the object ID that is unique within its owner document
    long getID() const {return _Id;}
    /// returns the name that is safe to be exported to other document
    std::string getExportName(bool forced=false) const;
    /// Return the object full name of the form DocName#ObjName
    std::string getFullName() const override;
    /// Return the object full label in the form DocName#ObjName
    std::string getFullLabel() const;
    bool isAttachedToDocument() const override;
    const char* detachFromDocument() override;
    /// gets the document in which this Object is handled
    App::Document *getDocument() const;

    /** Set the property touched -> changed, cause recomputation in Update()
     */
    //@{
    /// set this document object touched (cause recomputation on dependent features)
    void touch(bool noRecompute=false);
    /// test if this document object is touched
    bool isTouched() const;
    /// Enforce this document object to be recomputed
    void enforceRecompute();
    /// Test if this document object must be recomputed
    bool mustRecompute() const;
    /// reset this document object touched
    void purgeTouched() {
        StatusBits.reset(ObjectStatus::Touch);
        StatusBits.reset(ObjectStatus::Enforce);
        setPropertyStatus(0,false);
    }
    /// set this feature to error
    bool isError() const {return  StatusBits.test(ObjectStatus::Error);}
    bool isValid() const {return !StatusBits.test(ObjectStatus::Error);}
    /// remove the error from the object
    void purgeError(){StatusBits.reset(ObjectStatus::Error);}
    /// returns true if this objects is currently recomputing
    bool isRecomputing() const {return StatusBits.test(ObjectStatus::Recompute);}
    /// returns true if this objects is currently restoring from file
    bool isRestoring() const {return StatusBits.test(ObjectStatus::Restore);}
    /// returns true if this objects is currently removed from the document
    bool isRemoving() const {return StatusBits.test(ObjectStatus::Remove);}
    /// return the status bits
    unsigned long getStatus() const {return StatusBits.to_ulong();}
    bool testStatus(ObjectStatus pos) const {return StatusBits.test(size_t(pos));}
    void setStatus(ObjectStatus pos, bool on) {StatusBits.set(size_t(pos), on);}
    //@}

    int isExporting() const;

    /** Child element handling
     */
    //@{
    /** Set sub-element visibility
     *
     * For performance reason, \c element must not contain any further
     * sub-elements, i.e. there should be no '.' inside \c element.
     *
     * @return -1 if element visibility is not supported, 0 if element is not
     * found, 1 if success
     */
    virtual int setElementVisible(const char *element, bool visible);

    /** Get sub-element visibility
     *
     * @return -1 if element visibility is not supported or element not found, 0
     * if element is invisible, or else 1
     */
    virtual int isElementVisible(const char *element) const;

    /// return true to activate tree view group object handling and element visibility
    virtual bool hasChildElement() const;
    //@}


    /** DAG handling
        This part of the interface deals with viewing the document as
        a DAG (directed acyclic graph).
    */
    //@{
    /// OutList options
    enum OutListOption {
        /// Do not include link from expression engine
        OutListNoExpression = 1,
        /// Do not hide any link (i.e. include links with LinkScopeHidden)
        OutListNoHidden = 2,
        /// Do not include link from PropertyXLink
        OutListNoXLinked = 4,
    };
    /// returns a list of objects this object is pointing to by Links
    const std::vector<App::DocumentObject*> &getOutList() const;
    std::vector<App::DocumentObject*> getOutList(int option) const;
    void getOutList(int option, std::vector<App::DocumentObject*> &res) const;

    /// returns a list of objects linked by the property
    std::vector<App::DocumentObject*> getOutListOfProperty(App::Property*) const;
    /// returns a list of objects this object is pointing to by Links and all further descended
    std::vector<App::DocumentObject*> getOutListRecursive() const;
    /// clear internal out list cache
    void clearOutListCache() const;
    /// get all possible paths from this to another object following the OutList
    std::vector<std::list<App::DocumentObject*> > getPathsByOutList(App::DocumentObject* to) const;
#ifdef USE_OLD_DAG
    /// get all objects link to this object
    std::vector<App::DocumentObject*> getInList(void) const
#else
    const std::vector<App::DocumentObject*> &getInList() const;
#endif
    /// get all objects link directly or indirectly to this object
    std::vector<App::DocumentObject*> getInListRecursive() const;
    /** Get a set of all objects linking to this object, including possible external parent objects
     *
     * @param inSet [out]: a set containing all objects linking to this object.
     * @param recursive [in]: whether to obtain recursive in list
     * @param inList [in, out]: optional pointer to a vector holding the output
     * objects, with the furthest linking object ordered last.
     */
    void getInListEx(std::set<App::DocumentObject*> &inSet,
            bool recursive, std::vector<App::DocumentObject*> *inList=nullptr) const;
    /** Return a set of all objects linking to this object, including possible external parent objects
     * @param recursive [in]: whether to obtain recursive in list
     */
    std::set<App::DocumentObject*> getInListEx(bool recursive) const;

    /// get group if object is part of a group, otherwise 0 is returned
    DocumentObjectGroup* getGroup() const;

    /// test if this object is in the InList and recursive further down
    bool isInInListRecursive(DocumentObject* objToTest) const;
    /// test if this object is directly (non recursive) in the InList
    bool isInInList(DocumentObject* objToTest) const;
    /// test if the given object is in the OutList and recursive further down
    bool isInOutListRecursive(DocumentObject* objToTest) const;
    /// test if this object is directly (non recursive) in the OutList
    bool isInOutList(DocumentObject* objToTest) const;
    /// internal, used by PropertyLink to maintain DAG back links
    void _removeBackLink(DocumentObject*);
    /// internal, used by PropertyLink to maintain DAG back links
    void _addBackLink(DocumentObject*);
    //@}

    /**
     * @brief testIfLinkIsDAG tests a link that is about to be created for
     * circular references.
     * @param objToLinkIn (input). The object this object is to depend on after
     * the link is going to be created.
     * @return true if link can be created (no cycles will be made). False if
     * the link will cause a circular dependency and break recomputes. Throws an
     * error if the document already has a circular dependency.
     * That is, if the return is true, the link is allowed.
     */
    bool testIfLinkDAGCompatible(DocumentObject* linkTo) const;
    bool testIfLinkDAGCompatible(const std::vector<DocumentObject *> &linksTo) const;
    bool testIfLinkDAGCompatible(App::PropertyLinkSubList &linksTo) const;
    bool testIfLinkDAGCompatible(App::PropertyLinkSub &linkTo) const;

public:
    /** mustExecute
     *  We call this method to check if the object was modified to
     *  be invoked. If the object label or an argument is modified.
     *  If we must recompute the object - to call the method execute().
     *  0: no recomputation is needed
     *  1: recomputation needed
     *
     * @remark If an object is marked as 'touched' then this does not
     * necessarily mean that it will be recomputed. It only means that all
     * objects that link it (i.e. its InList) will be recomputed.
     */
    virtual short mustExecute() const;

    /** Recompute only this feature
     *
     * @param recursive: set to true to recompute any dependent objects as well
     */
    bool recomputeFeature(bool recursive=false);

    /// get the status Message
    const char *getStatusString() const;

    /** Called in case of losing a link
     * Get called by the document when a object got deleted a link property of this
     * object ist pointing to. The standard behaviour of the DocumentObject implementation
     * is to reset the links to nothing. You may override this method to implement
     * additional or different behavior.
     */
    virtual void onLostLinkToObject(DocumentObject*);
    PyObject *getPyObject() override;

    /** Get the sub element/object by name
     *
     * @param subname: a string which is dot separated name to refer to a sub
     * element or object. An empty string can be used to refer to the object
     * itself
     *
     * @param pyObj: if non zero, returns the python object corresponding to
     * this sub object. The actual type of this python object is implementation
     * dependent. For example, The current implementation of Part::Feature will
     * return the TopoShapePy, event if there is no sub-element reference, in
     * which case it returns the whole shape.
     *
     * @param mat: If non zero, it is used as the current transformation matrix
     * on input.  And output as the accumulated transformation up until and
     * include the transformation applied by the final object reference in \c
     * subname. For Part::Feature, the transformation is applied to the
     * TopoShape inside \c pyObj before returning.
     *
     * @param transform: if false, then it will not apply the object's own
     * transformation to \c mat, which lets you override the object's placement
     * (and possibly scale).
     *
     * @param depth: depth limitation as hint for cyclic link detection
     *
     * @return The last document object referred in subname. If subname is empty,
     * then it shall return itself. If subname is invalid, then it shall return
     * zero.
     */
    virtual DocumentObject *getSubObject(const char *subname, PyObject **pyObj=nullptr,
            Base::Matrix4D *mat=nullptr, bool transform=true, int depth=0) const;

    /// Return a list of objects referenced by a given subname including this object
    std::vector<DocumentObject*> getSubObjectList(const char *subname) const;

    /// reason of calling getSubObjects()
    enum GSReason {
        /// default, mostly for exporting shape objects
        GS_DEFAULT,
        /// for element selection
        GS_SELECT,
    };

    /** Return name reference of all sub-objects
     *
     * @param reason: indicate reason of obtaining the sub objects
     *
     * The default implementation returns all object references in
     * PropertyLink, and PropertyLinkList, if any
     *
     * @return Return a vector of subname references for all sub-objects. In
     * most cases, the name returned will be the object name plus an ending
     * '.', which can be passed directly to getSubObject() to retrieve the
     * name. The reason to return the name reference instead of the sub object
     * itself is because there may be no real sub object, or the sub object
     * need special transformation. For example, sub objects of an array type
     * of object.
     */
    virtual std::vector<std::string> getSubObjects(int reason=0) const;

    ///Obtain top parents and subnames of this object using its InList
    std::vector<std::pair<App::DocumentObject*,std::string> > getParents(int depth=0) const;

    /// Obtain the first parent group of this object
    App::DocumentObject* getFirstParent() const;

    /** Return the linked object with optional transformation
     *
     * @param recurse: If false, return the immediate linked object, or else
     * recursively call this function to return the final linked object.
     *
     * @param mat: If non zero, it is used as the current transformation matrix
     * on input.  And output as the accumulated transformation till the final
     * linked object.
     *
     * @param transform: if false, then it will not accumulate the object's own
     * placement into \c mat, which lets you override the object's placement.
     *
     * @return Return the linked object. This function must return itself if the
     * it is not a link or the link is invalid.
     */
    virtual DocumentObject *getLinkedObject(bool recurse=true,
            Base::Matrix4D *mat=nullptr, bool transform=false, int depth=0) const;

    /* Return true to cause PropertyView to show linked object's property */
    virtual bool canLinkProperties() const {return true;}

    /* Return true to bypass duplicate label checking */
    virtual bool allowDuplicateLabel() const {return false;}

    /*** Called to let object itself control relabeling
     *
     * @param newLabel: input as the new label, which can be modified by object itself
     *
     * This function is called before onBeforeChange()
     */
    virtual void onBeforeChangeLabel(std::string &newLabel) {(void)newLabel;}

    friend class Document;
    friend class Transaction;
    friend class ObjectExecution;

    static DocumentObjectExecReturn *StdReturn;

    void Save (Base::Writer &writer) const override;

    /* Expression support */

    virtual void setExpression(const ObjectIdentifier & path, std::shared_ptr<App::Expression> expr);

    void clearExpression(const ObjectIdentifier & path);

    virtual const PropertyExpressionEngine::ExpressionInfo getExpression(const ObjectIdentifier &path) const;

    virtual void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> & paths);

    const std::string & getOldLabel() const { return oldLabel; }

    const char *getViewProviderNameStored() const {
        return _pcViewProviderName.c_str();
    }

    bool removeDynamicProperty(const char* prop) override;

    App::Property* addDynamicProperty(
            const char* type, const char* name=nullptr,
            const char* group=nullptr, const char* doc=nullptr,
            short attr=0, bool ro=false, bool hidden=false) override;

    /** Resolve the last document object referenced in the subname
     *
     * @param subname: dot separated subname
     * @param parent: return the direct parent of the object
     * @param childName: return child name to be passed to is/setElementVisible()
     * @param subElement: return non-object sub-element name if found. The
     * pointer is guaranteed to be within the buffer pointed to by 'subname'
     *
     * @sa getSubObject()
     * @return Returns the last referenced document object in the subname. If no
     * such object in subname, return pObject.
     */
    App::DocumentObject *resolve(const char *subname, App::DocumentObject **parent=nullptr,
        std::string *childName=nullptr, const char **subElement=nullptr,
        PyObject **pyObj=nullptr, Base::Matrix4D *mat=nullptr, bool transform=true, int depth=0) const;

    /** Resolve a link reference that is relative to this object reference
     *
     * @param subname: on input, this is the subname reference to the object
     * that is to be assigned a link. On output, the reference may be offset
     * to be rid of any common parent.
     * @param link: on input, this is the top parent of the link reference. On
     * output, it may be altered to one of its child to be rid off any common
     * parent.
     * @param linkSub: on input, this the subname of the link reference. On
     * output, it may be offset to be rid off any common parent.
     *
     * @return The corrected top parent of the object that is to be assigned the
     * link. If the output 'subname' is empty, then return the object itself.
     *
     * To avoid any cyclic reference, an object must not be assign a link to any
     * of the object in its parent. This function can be used to resolve any
     * common parents of an object and its link target.
     *
     * For example, with the following object hierarchy
     *
     * Group
     *   |--Group001
     *   |   |--Box
     *   |   |--Cylinder
     *   |--Group002
     *       |--Box001
     *       |--Cylinder001
     *
     * If you want add a link of Group.Group002.Box001 to Group.Group001, you
     * can call with the following parameter (which are usually obtained from
     * Selection.getSelectionEx(), check usage in TreeWidget::onDropEvent()):
     *      std::string subname("Group002.");
     *      auto link = Group;
     *      std::string linkSub("Group001.Box001.");
     *      parent = Group.resolveRelativeLink(subname,link,linkSub);
     *
     * The resolving result is as follow:
     *      return  -> Group001
     *      subname -> ""
     *      link    -> Group002
     *      linkSub -> "Box001."
     *
     * The common parent 'Group' is removed.
     */
    App::DocumentObject *resolveRelativeLink(std::string &subname,
            App::DocumentObject *&link, std::string &linkSub) const;

    /** Called to adjust link properties to avoid cyclic links
     *
     * @param inList: the recursive in-list of the future parent object,
     * including the parent itself.
     * @param visited: optional set holding the visited objects. If null then
     * only this object is adjusted, or else all object inside the out-list of
     * this object will be checked.
     *
     * @return Return whether the object has been modified
     *
     * This function tries to adjust any relative link properties (i.e. link
     * properties that can hold subnames) to avoid cyclic when added to the
     * future parent.
     */
    virtual bool adjustRelativeLinks(const std::set<App::DocumentObject*> &inList,
            std::set<App::DocumentObject*> *visited=nullptr);

    /** allow partial loading of dependent objects
     *
     * @return Returns 0 means do not support partial loading. 1 means allow
     * dependent objects to be partially loaded, i.e. only create, but not
     * restored. 2 means this object itself can be partially loaded.
     */
    virtual int canLoadPartial() const {return 0;}

    virtual void onUpdateElementReference(const Property *) {}

    /** Allow object to redirect a subname path
     *
     * @param ss: input as the current subname path from \a topParent leading
     * just before this object, i.e. ends at the parent of this object. This
     * function should append its own name to this path, or redirect the
     * subname to other place.
     * @param topParent: top parent of this subname path
     * @param child: the immediate child object in the path
     *
     * This function is called by tree view to generate a subname path when an
     * item is selected in the tree. Document object can use this function to
     * redirect the selection to some other objects.
     */
    virtual bool redirectSubName(std::ostringstream &ss,
            DocumentObject *topParent, DocumentObject *child) const;

    /** Special marker to mark the object as hidden
     *
     * It is used by Gui::ViewProvider::getElementColors(), but exposed here
     * for convenience
     */
    static const std::string &hiddenMarker();
    /// Check if the subname reference ends with hidden marker.
    static const char *hasHiddenMarker(const char *subname);

protected:
    /// recompute only this object
    virtual App::DocumentObjectExecReturn *recompute();
    /** get called by the document to recompute this feature
      * Normally this method get called in the processing of
      * Document::recompute().
      * In execute() the output properties get recomputed
      * with the data from linked objects and objects own
      * properties.
      */
    virtual App::DocumentObjectExecReturn *execute();

    /**
     * Executes the extensions of a document object.
     */
    App::DocumentObjectExecReturn *executeExtensions();

    /** Status bits of the document object
     * The first 8 bits are used for the base system the rest can be used in
     * descendent classes to mark special statuses on the objects.
     * The bits and their meaning are listed below:
     *  0 - object is marked as 'touched'
     *  1 - object is marked as 'erroneous'
     *  2 - object is marked as 'new'
     *  3 - object is marked as 'recompute', i.e. the object gets recomputed now
     *  4 - object is marked as 'restoring', i.e. the object gets loaded at the moment
     *  5 - object is marked as 'deleting', i.e. the object gets deleted at the moment
     *  6 - reserved
     *  7 - reserved
     * 16 - object is marked as 'expanded' in the tree view
     */
    std::bitset<32> StatusBits;

    void setError(){StatusBits.set(ObjectStatus::Error);}
    void resetError(){StatusBits.reset(ObjectStatus::Error);}
    void setDocument(App::Document* doc);

    /// get called before the value is changed
    void onBeforeChange(const Property* prop) override;
    /// get called by the container when a property was changed
    void onChanged(const Property* prop) override;
    /// get called after a document has been fully restored
    virtual void onDocumentRestored();
    /// get called after an undo/redo transaction is finished
    virtual void onUndoRedoFinished();
    /// get called after setting the document
    virtual void onSettingDocument();
    /// get called after a brand new object was created
    virtual void setupObject();
    /// get called when object is going to be removed from the document
    virtual void unsetupObject();

    /// get called when a property status has changed
    void onPropertyStatusChanged(const Property &prop, unsigned long oldStatus) override;

private:
    void printInvalidLinks() const;

     /// python object of this class and all descendent
protected: // attributes
    Py::SmartPtr PythonObject;
    /// pointer to the document this object belongs to
    App::Document* _pDoc{nullptr};

    /// Old label; used for renaming expressions
    std::string oldLabel;

    // pointer to the document name string (for performance)
    const std::string *pcNameInDocument{nullptr};

private:
    // accessed by App::Document to record and restore the correct view provider type
    std::string _pcViewProviderName;

    // unique identifier (among a document) of this object.
    long _Id{0};

private:
    // Back pointer to all the fathers in a DAG of the document
    // this is used by the document (via friend) to have a effective DAG handling
    std::vector<App::DocumentObject*> _inList;
    mutable std::vector<App::DocumentObject *> _outList;
    mutable std::unordered_map<const char *, App::DocumentObject*, CStringHasher, CStringHasher> _outListMap;
    mutable bool _outListCached = false;
};

} //namespace App

#endif // APP_DOCUMENTOBJECT_H
