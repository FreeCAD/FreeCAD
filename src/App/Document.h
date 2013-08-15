/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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

#ifndef APP_DOCUMENT_H
#define APP_DOCUMENT_H

#include <CXX/Objects.hxx>
#include <Base/Observer.h>
#include <Base/Persistence.h>
#include <Base/Type.h>

#include "PropertyContainer.h"
#include "PropertyStandard.h"

#include <map>
#include <vector>
#include <stack>

#include <boost/signals.hpp>
#include <boost/graph/adjacency_list.hpp>


namespace Base {
    class Writer;
}

namespace App
{
    class DocumentObject;
    class DocumentObjectExecReturn;
    class Document;
    class DocumentPy; // the python document class
    class Application;
    class Transaction;
}

namespace App
{

/// The document class
class AppExport Document : public App::PropertyContainer
{
    PROPERTY_HEADER(App::Document);

public:
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
    /// long comment or description (UTF8 with line breaks)
    PropertyString Comment;
    /// Id e.g. Part number
    PropertyString Id;
    /// unique identifier of the document
    PropertyUUID   Uid;
    /** License string
      * Holds the short license string for the Item, e.g. CC-BY
      * for the Creative Commons license suit. 
      */
    App::PropertyString  License;
    /// License descripton/contract URL
    App::PropertyString  LicenseURL;
    /// Meta descriptons
    App::PropertyMap     Meta;
    /// Meta descriptons
    App::PropertyMap     Material;
    /// read-only name of the temp dir created wen the document is opened
    PropertyString TransientDir;
    //@}

    /** @name Signals of the document */
    //@{
    /// signal on new Object
    boost::signal<void (const App::DocumentObject&)> signalNewObject;
    //boost::signal<void (const App::DocumentObject&)>     m_sig;
    /// signal on deleted Object
    boost::signal<void (const App::DocumentObject&)> signalDeletedObject;
    /// signal on changed Object
    boost::signal<void (const App::DocumentObject&, const App::Property&)> signalChangedObject;
    /// signal on renamed Object
    boost::signal<void (const App::DocumentObject&)> signalRenamedObject;
    /// signal on activated Object
    boost::signal<void (const App::DocumentObject&)> signalActivatedObject;
    /// signal on undo
    boost::signal<void (const App::Document&)> signalUndo;
    /// signal on redo
    boost::signal<void (const App::Document&)> signalRedo;
    /** signal on load/save document
     * this signal is given when the document gets streamed.
     * you can use this hook to write additional information in 
     * the file (like the Gui::Document it does).
     */
    boost::signal<void (Base::Writer   &)> signalSaveDocument;
    boost::signal<void (Base::XMLReader&)> signalRestoreDocument;
    boost::signal<void (const std::vector<App::DocumentObject*>&,
                        Base::Writer   &)> signalExportObjects;
    boost::signal<void (const std::vector<App::DocumentObject*>&,
                        Base::XMLReader&)> signalImportObjects;
    //@}

    /** @name File handling of the document */
    //@{
    /// Save the Document under a new Name
    //void saveAs (const char* Name);
    /// Save the document to the file in Property Path
    bool save (void);
    bool saveAs(const char* file);
    /// Restore the document from the file in Property Path
    void restore (void);
    void exportObjects(const std::vector<App::DocumentObject*>&, std::ostream&);
    void exportGraphviz(std::ostream&);
    std::vector<App::DocumentObject*> importObjects(Base::XMLReader& reader);
    /// Opens the document from its file name
    //void open (void);
    /// Is the document already saved to a file
    bool isSaved() const;
    /// Get the document name
    const char* getName() const;
    //@}

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    /// returns the complet document mermory consumption, including all managed DocObjects and Undo Redo.
    unsigned int getMemSize (void) const;

    /** @name Object handling  */
    //@{
    /// Add a feature of sType with sName (ASCII) to this document and set it active. Unicode names are set through the Label propery
    DocumentObject *addObject(const char* sType, const char* pObjectName=0);
    /// Remove a feature out of the document
    void remObject(const char* sName);
    /** Copy an object from another document to this document
     * If \a recursive is true then all objects this object depends on
     * are copied as well. By default \a recursive is false.
     * Returns the copy of the object or 0 if the creation failed.
     */
    DocumentObject* copyObject(DocumentObject* obj, bool recursive=false, bool keepdigitsatend=false);
    /** Move an object from another document to this document
     * If \a recursive is true then all objects this object depends on
     * are moved as well. By default \a recursive is false.
     * Returns the moved object itself or 0 if the object is already part of this
     * document..
     */
    DocumentObject* moveObject(DocumentObject* obj, bool recursive=false);
    /// Returns the active Object of this document
    DocumentObject *getActiveObject(void) const;
    /// Returns a Object of this document
    DocumentObject *getObject(const char *Name) const;
    /// Returns a Name of an Object or 0
    const char *getObjectName(DocumentObject *pFeat) const;
    /// Returns a Name of an Object or 0
    std::string getUniqueObjectName(const char *Name) const;
    /// Returns a name of the form prefix_number. d specifies the number of digits.
    std::string getStandardObjectName(const char *Name, int d) const;
    /// Returns a list of all Objects
    std::vector<DocumentObject*> getObjects() const;
    std::vector<DocumentObject*> getObjectsOfType(const Base::Type& typeId) const;
    std::vector<DocumentObject*> findObjects(const Base::Type& typeId, const char* objname) const;
    /// Returns an array with the correct types already.
    template<typename T> inline std::vector<T*> getObjectsOfType() const;
    int countObjectsOfType(const Base::Type& typeId) const;
    /// get the number of objects in the document
    int countObjects(void) const;
    //@}

    /** @name methods for modification and state handling
     */
    //@{
    /// Remove all modifications. After this call The document becomes again Valid.
    void purgeTouched();
    /// check if there is any touched object in this document
    bool isTouched(void) const;
    /// returns all touched objects
    std::vector<App::DocumentObject *> getTouched(void) const;
    /// set the document to be closable, this is on by default.
    void setClosable(bool);
    /// check whether the document can be closed
    bool isClosable() const;
    /// Recompute all touched features
    void recompute();
    /// Recompute only one feature
    void recomputeFeature(DocumentObject* Feat);
    /// get the error log from the recompute run
    const std::vector<App::DocumentObjectExecReturn*> &getRecomputeLog(void)const{return _RecomputeLog;}
    /// get the text of the error of a spezified object
    const char* getErrorDescription(const App::DocumentObject*) const;
    //@}


    /** @name methods for the UNDO REDO and Transaction handling */
    //@{
    /// switch the level of Undo/Redo
    void setUndoMode(int iMode);  
    /// switch the level of Undo/Redo
    int getUndoMode(void) const;  
    /// switch the tranaction mode
    void setTransactionMode(int iMode);
    /// Open a new command Undo/Redo, an UTF-8 name can be specified
    void openTransaction(const char* name=0);
    // Commit the Command transaction. Do nothing If there is no Command transaction open.
    void commitTransaction();
    /// Abort the  actually running transaction. 
    void abortTransaction();
    /// Check if a transaction is open
    bool hasPendingTransaction() const;
    /// Set the Undo limit in Byte!
    void setUndoLimit(unsigned int UndoMemSize=0);
    /// Returns the actual memory consumption of the Undo redo stuff.
    unsigned int getUndoMemSize (void) const;
    /// Set the Undo limit as stack size
    void setMaxUndoStackSize(unsigned int UndoMaxStackSize=20);
    /// Set the Undo limit as stack size
    unsigned int getMaxUndoStackSize(void)const;
    /// Remove all stored Undos and Redos
    void clearUndos();
    /// Returns the  number  of stored Undos. If greater than 0 Undo will be effective.
    int getAvailableUndos() const;
    /// Returns a list of the Undo names
    std::vector<std::string> getAvailableUndoNames() const;
    /// Will UNDO  one step, returns  False if no undo was done (Undos == 0).
    bool undo();
    /// Returns the number of stored Redos. If greater than 0 Redo will be effective.
    int getAvailableRedos() const;
    /// Returns a list of the Redo names.
    std::vector<std::string> getAvailableRedoNames() const;
    /// Will REDO  one step, returns  False if no redo was done (Redos == 0).
    bool redo() ;
    //@}

    /** @name dependency stuff */
    //@{
    /// write GraphViz file
    void writeDependencyGraphViz(std::ostream &out);
    /// checks if the graph is directed and has no cycles
    bool checkOnCycle(void);
    /// get a list of all objects linking to the given object
    std::vector<App::DocumentObject*> getInList(const DocumentObject* me) const;
    /// Get a complete list of all objects the given objects depend on. The list
    /// also contains the given objects!
    std::vector<App::DocumentObject*> getDependencyList
        (const std::vector<App::DocumentObject*>&) const;
    // set Changed
    //void setChanged(DocumentObject* change);
    //@}

    virtual PyObject *getPyObject(void);

    friend class Application;
    /// because of transaction handling
    friend class DocumentObject;
    friend class Transaction;
    friend class TransactionObject;

    /// Destruction 
    virtual ~Document();

protected:
    /// Construction
    Document(void);

    void _remObject(DocumentObject* pcObject);
    void _addObject(DocumentObject* pcObject, const char* pObjectName);
    DocumentObject* _copyObject(DocumentObject* obj, std::map<DocumentObject*, 
        DocumentObject*>&, bool recursive=false, bool keepdigitsatend=false);
    /// checks if a valid transaction is open
    void _checkTransaction(DocumentObject* pcObject);
    void breakDependency(DocumentObject* pcObject, bool clear);
    std::vector<App::DocumentObject*> readObjects(Base::XMLReader& reader);
    void writeObjects(const std::vector<App::DocumentObject*>&, Base::Writer &writer) const;

    void onChanged(const Property* prop);
    /// callback from the Document objects before property will be changed
    void onBeforeChangeProperty(const DocumentObject *Who, const Property *What);
    /// callback from the Document objects after property was changed
    void onChangedProperty(const DocumentObject *Who, const Property *What);
    /// helper which Recompute only this feature
    bool _recomputeFeature(DocumentObject* Feat);
    void _clearRedos();
    /// refresh the internal dependency graph
    void _rebuildDependencyList(void);
    std::string getTransientDirectoryName(const std::string& uuid, const std::string& filename) const;


private:
    // # Data Member of the document +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    std::list<Transaction*> mUndoTransactions;
    std::list<Transaction*> mRedoTransactions;
    // recompute log
    std::vector<App::DocumentObjectExecReturn*> _RecomputeLog;

    // pointer to the python class
    Py::Object DocumentPythonObject;
    struct DocumentP* d;
};

template<typename T>
inline std::vector<T*> Document::getObjectsOfType() const
{
    std::vector<T*> type;
    std::vector<App::DocumentObject*> obj = this->getObjectsOfType(T::getClassTypeId());
    type.reserve(obj.size());
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
        type.push_back(static_cast<T*>(*it));
    return type;
}


} //namespace App

#endif // APP_DOCUMENT_H
