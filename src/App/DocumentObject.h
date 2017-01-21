/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de)          *
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
#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/PropertyExpressionEngine.h>

#include <Base/TimeInfo.h>
#include <CXX/Objects.hxx>

#include <bitset>
#include <boost/signals.hpp>

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
    Recompute = 3,
    Restore = 4,
    Delete = 5,
    PythonCall = 6,
    Expand = 16
};

/** Return object for feature execution
*/
class AppExport DocumentObjectExecReturn
{
public:
    DocumentObjectExecReturn(const std::string& sWhy, DocumentObject* WhichObject=0)
        : Why(sWhy), Which(WhichObject)
    {
    }
    DocumentObjectExecReturn(const char* sWhy, DocumentObject* WhichObject=0)
        : Which(WhichObject)
    {
        if(sWhy)
            Why = sWhy;
    }

    std::string Why;
    DocumentObject* Which;
};



/** Base class of all Classes handled in the Document
 */
class AppExport DocumentObject: public App::TransactionalObject
{
    PROPERTY_HEADER(App::DocumentObject);

public:

    PropertyString Label;
    PropertyExpressionEngine ExpressionEngine;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "";
    }
    /// Constructor
    DocumentObject(void);
    virtual ~DocumentObject();

    /// returns the name which is set in the document for this object (not the name property!)
    const char *getNameInDocument(void) const;
    virtual bool isAttachedToDocument() const;
    virtual const char* detachFromDocument();
    /// gets the document in which this Object is handled
    App::Document *getDocument(void) const;

    /** Set the property touched -> changed, cause recomputation in Update()
     */
    //@{
    /// set this feature touched (cause recomputation on depndend features)
    void touch(void);
    /// test if this feature is touched
    bool isTouched(void) const;
    /// reset this feature touched
    void purgeTouched(void){StatusBits.reset(0);setPropertyStatus(0,false);}
    /// set this feature to error
    bool isError(void) const {return  StatusBits.test(1);}
    bool isValid(void) const {return !StatusBits.test(1);}
    /// remove the error from the object
    void purgeError(void){StatusBits.reset(1);}
    /// returns true if this objects is currently recomputing
    bool isRecomputing() const {return StatusBits.test(3);}
    /// returns true if this objects is currently restoring from file
    bool isRestoring() const {return StatusBits.test(4);}
    /// returns true if this objects is currently restoring from file
    bool isDeleting() const {return StatusBits.test(5);}
    /// return the status bits
    unsigned long getStatus() const {return StatusBits.to_ulong();}
    bool testStatus(ObjectStatus pos) const {return StatusBits.test((size_t)pos);}
    void setStatus(ObjectStatus pos, bool on) {StatusBits.set((size_t)pos, on);}
    //@}

    /** DAG handling
        This part of the interface deals with viewing the document as
        an DAG (directed acyclic graph). 
    */
    //@{
    /// returns a list of objects this object is pointing to by Links
    std::vector<App::DocumentObject*> getOutList(void) const;
    /// returns a list of objects this object is pointing to by Links and all further descended 
    std::vector<App::DocumentObject*> getOutListRecursive(void) const;
    /// get all objects link to this object
    std::vector<App::DocumentObject*> getInList(void) const;
    /// get all objects link directly or indirectly to this object 
    std::vector<App::DocumentObject*> getInListRecursive(void) const;
    /// get group if object is part of a group, otherwise 0 is returned
    DocumentObjectGroup* getGroup() const;
#if USE_OLD_DAG
#else
    /// test if this object is in the InList and recursive further down
    bool isInInListRecursive(DocumentObject* objToTest) const;
    /// test if this object is directly (non recursive) in the InList
    bool isInInList(DocumentObject* objToTest) const;
    /// test if the given object is in the OutList and recursive further down
    bool isInOutListRecursive(DocumentObject* objToTest) const;
    /// test if this object is directly (non recursive) in the OutList
    bool isInOutList(DocumentObject* objToTest) const;
    /// internal, used by ProperyLink to maintain DAG back links
    void _removeBackLink(DocumentObject*);
    /// internal, used by ProperyLink to maintain DAG back links
    void _addBackLink(DocumentObject*);
#endif //USE_OLD_DAG
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
     *  0: no recompution is needed
     *  1: recompution needed
     * -1: the document examine all links of this object and if one is touched -> recompute
     */
    virtual short mustExecute(void) const;

    /// Recompute only this feature
    bool recomputeFeature();

    /// get the status Message
    const char *getStatusString(void) const;

    /** Called in case of loosing a link
     * Get called by the document when a object got deleted a link property of this
     * object ist pointing to. The standard behaviour of the DocumentObject implementation
     * is to reset the links to nothing. You may overide this method to implement
     *additional or different behavior.
     */
    virtual void onLostLinkToObject(DocumentObject*);
    virtual PyObject *getPyObject(void);
    /// its used to get the python sub objects by name (e.g. by the selection)
    virtual std::vector<PyObject *> getPySubObjects(const std::vector<std::string>&) const;

    friend class Document;
    friend class Transaction;
    friend class ObjectExecution;

    static DocumentObjectExecReturn *StdReturn;

    virtual void Save (Base::Writer &writer) const;

    /* Expression support */

    virtual void setExpression(const ObjectIdentifier & path, boost::shared_ptr<App::Expression> expr, const char *comment = 0);

    virtual const PropertyExpressionEngine::ExpressionInfo getExpression(const ObjectIdentifier &path) const;

    virtual void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> & paths);

    virtual void connectRelabelSignals();

    const std::string & getOldLabel() const { return oldLabel; }

protected:
    /// recompute only this object
    virtual App::DocumentObjectExecReturn *recompute(void);
    /** get called by the document to recompute this feature
      * Normaly this method get called in the processing of
      * Document::recompute().
      * In execute() the outpupt properties get recomputed
      * with the data from linked objects and objects own
      * properties.
      */
    virtual App::DocumentObjectExecReturn *execute(void);

    /** Status bits of the document object
     * The first 8 bits are used for the base system the rest can be used in
     * descendent classes to to mark special stati on the objects.
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

    void setError(void){StatusBits.set(1);}
    void resetError(void){StatusBits.reset(1);}
    void setDocument(App::Document* doc);

    /// get called before the value is changed
    virtual void onBeforeChange(const Property* prop);
    /// get called by the container when a property was changed
    virtual void onChanged(const Property* prop);
    /// get called after a document has been fully restored
    virtual void onDocumentRestored() {}
    /// get called after setting the document
    virtual void onSettingDocument();
    /// get called after a brand new object was created
    virtual void setupObject();
    /// get called when object is going to be removed from the document
    virtual void unsetupObject();

     /// python object of this class and all descendend
protected: // attributes
    Py::Object PythonObject;
    /// pointer to the document this object belongs to
    App::Document* _pDoc;

    // Connections to track relabeling of document and document objects
    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection onRelabledDocumentConnection;
    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection onRelabledObjectConnection;
    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection onDeletedObjectConnection;

    /// Old label; used for renaming expressions
    std::string oldLabel;

    // pointer to the document name string (for performance)
    const std::string *pcNameInDocument;
    
private:
    // Back pointer to all the fathers in a DAG of the document
    // this is used by the document (via friend) to have a effective DAG handling
    std::vector<App::DocumentObject*> _inList;
    // helper for isInInListRecursive()
    bool _isInInListRecursive(const DocumentObject *act, const DocumentObject* test, const DocumentObject* checkObj, int depth) const;
    // helper for isInOutListRecursive()
    bool _isInOutListRecursive(const DocumentObject *act, const DocumentObject* test, const DocumentObject* checkObj, int depth) const;
};

class AppExport ObjectStatusLocker
{
public:
    ObjectStatusLocker(ObjectStatus s, DocumentObject* o) : status(s), obj(o)
    { obj->setStatus(status, true); }
    ~ObjectStatusLocker()
    { obj->setStatus(status, false); }
private:
    ObjectStatus status;
    DocumentObject* obj;
};

} //namespace App

#endif // APP_DOCUMENTOBJECT_H
