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

#include <App/PropertyContainer.h>
#include <App/PropertyStandard.h>

#include <Base/TimeInfo.h>
#include <CXX/Objects.hxx>

#include <bitset>


namespace App
{
class Document;
class DocumentObjectPy;

enum ObjectStatus {
    Touch = 0,
    Error = 1,
    New = 2,
    Recompute = 3,
    Restore = 4,
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
class AppExport DocumentObject: public App::PropertyContainer
{
    PROPERTY_HEADER(App::DocumentObject);

public:

    PropertyString Label;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "";
    }
    /// Constructor
    DocumentObject(void);
    virtual ~DocumentObject();

    /// returns the name which is set in the document for this object (not the name property!)
    const char *getNameInDocument(void) const;
    /// gets the document in which this Object is handled
    App::Document *getDocument(void) const;

    /** Set the property touched -> changed, cause recomputation in Update()
     */
    //@{
    /// set this feature touched (cause recomputation on depndend features)
    void touch(void);
    /// test if this feature is touched
    bool isTouched(void) const {return StatusBits.test(0);}
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
    /// recompute only this object
    virtual App::DocumentObjectExecReturn *recompute(void);
    /// return the status bits
    unsigned long getStatus() const {return StatusBits.to_ulong();}
    bool testStatus(ObjectStatus pos) const {return StatusBits.test((size_t)pos);}
    void setStatus(ObjectStatus pos, bool on) {StatusBits.set((size_t)pos, on);}
    //@}

    /// returns a list of objects this object is pointing to by Links
    std::vector<App::DocumentObject*> getOutList(void) const;
    /// get all objects link to this object
    std::vector<App::DocumentObject*> getInList(void) const;


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

protected:
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
     *  5 - reserved
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
    /// get called after duplicating an object
    virtual void onFinishDuplicating() {}
    /// get called after setting the document
    virtual void onSettingDocument() {}

     /// python object of this class and all descendend
protected: // attributes
    Py::Object PythonObject;
    /// pointer to the document this object belongs to
    App::Document* _pDoc;

    // pointer to the document name string (for performance)
    const std::string *pcNameInDocument;
};

} //namespace App


#endif // APP_DOCUMENTOBJECT_H
