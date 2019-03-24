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
#include "PropertyLinks.h"

#include <map>
#include <vector>
#include <stack>
#include <functional>

#include <boost/signals2.hpp>

namespace Base {
    class Writer;
}

namespace App
{
    class TransactionalObject;
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
    enum Status {
        SkipRecompute = 0,
        KeepTrailingDigits = 1,
        Closable = 2,
        Restoring = 3,
        Recomputing = 4,
        PartialRestore = 5
    };

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
    PropertyUUID Uid;
    /** License string
      * Holds the short license string for the Item, e.g. CC-BY
      * for the Creative Commons license suit.
      */
    App::PropertyString License;
    /// License description/contract URL
    App::PropertyString LicenseURL;
    /// Meta descriptions
    App::PropertyMap Meta;
    /// Material descriptions, used and defined in the Material module.
    App::PropertyMap Material;
    /// read-only name of the temp dir created when the document is opened
    PropertyString TransientDir;
    /// Tip object of the document (if any)
    PropertyLink Tip;
    /// Tip object of the document (if any)
    PropertyString TipName;
    //@}

    /** @name Signals of the document */
    //@{
    /// signal before changing an doc property
    boost::signals2::signal<void (const App::Document&, const App::Property&)> signalBeforeChange;
    /// signal on changed doc property
    boost::signals2::signal<void (const App::Document&, const App::Property&)> signalChanged;
    /// signal on new Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalNewObject;
    //boost::signals2::signal<void (const App::DocumentObject&)>     m_sig;
    /// signal on deleted Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalDeletedObject;
    /// signal before changing an Object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalBeforeChangeObject;
    /// signal on changed Object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalChangedObject;
    /// signal on relabeled Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalRelabelObject;
    /// signal on activated Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalActivatedObject;
    /// signal on created object
    boost::signals2::signal<void (const App::DocumentObject&, Transaction*)> signalTransactionAppend;
    /// signal on removed object
    boost::signals2::signal<void (const App::DocumentObject&, Transaction*)> signalTransactionRemove;
    /// signal on undo
    boost::signals2::signal<void (const App::Document&)> signalUndo;
    /// signal on redo
    boost::signals2::signal<void (const App::Document&)> signalRedo;
    /** signal on load/save document
     * this signal is given when the document gets streamed.
     * you can use this hook to write additional information in
     * the file (like the Gui::Document does).
     */
    boost::signals2::signal<void (Base::Writer   &)> signalSaveDocument;
    boost::signals2::signal<void (Base::XMLReader&)> signalRestoreDocument;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&,
                                  Base::Writer   &)> signalExportObjects;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&,
                                  Base::Writer   &)> signalExportViewObjects;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&,
                                  Base::XMLReader&)> signalImportObjects;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&, Base::Reader&,
                                  const std::map<std::string, std::string>&)> signalImportViewObjects;
    //signal starting a save action to a file
    boost::signals2::signal<void (const App::Document&, const std::string&)> signalStartSave;
    //signal finishing a save action to a file
    boost::signals2::signal<void (const App::Document&, const std::string&)> signalFinishSave;
    boost::signals2::signal<void (const App::Document&)> signalRecomputed;
    boost::signals2::signal<void (const App::DocumentObject&)> signalRecomputedObject;
    //signal a new opened transaction
    boost::signals2::signal<void (const App::Document&, std::string)> signalOpenTransaction;
    // signal a committed transaction
    boost::signals2::signal<void (const App::Document&)> signalCommitTransaction;
    // signal an aborted transaction
    boost::signals2::signal<void (const App::Document&)> signalAbortTransaction;
    //@}

    /** @name File handling of the document */
    //@{
    /// Save the Document under a new Name
    //void saveAs (const char* Name);
    /// Save the document to the file in Property Path
    bool save (void);
    bool saveAs(const char* file);
    bool saveCopy(const char* file) const;
    /// Restore the document from the file in Property Path
    void restore (void);
    void exportObjects(const std::vector<App::DocumentObject*>&, std::ostream&);
    void exportGraphviz(std::ostream&) const;
    std::vector<App::DocumentObject*> importObjects(Base::XMLReader& reader);
    /// Opens the document from its file name
    //void open (void);
    /// Is the document already saved to a file?
    bool isSaved() const;
    /// Get the document name
    const char* getName() const;
    //@}

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    /// returns the complete document memory consumption, including all managed DocObjects and Undo Redo.
    unsigned int getMemSize (void) const;

    /** @name Object handling  */
    //@{
    /** Add a feature of sType with sName (ASCII) to this document and set it active.
     * Unicode names are set through the Label property.
     * @param sType       the type of created object
     * @param pObjectName if nonNULL use that name otherwise generate a new unique name based on the \a sType
     * @param isNew       if false don't call the \c DocumentObject::setupObject() callback (default is true)
     */
    DocumentObject *addObject(const char* sType, const char* pObjectName=0, bool isNew=true);
    /** Add an array of features of the given types and names.
     * Unicode names are set through the Label property.
     * @param sType       The type of created object
     * @param objectNames A list of object names
     * @param isNew       If false don't call the \c DocumentObject::setupObject() callback (default is true)
     */
    std::vector<DocumentObject *>addObjects(const char* sType, const std::vector<std::string>& objectNames, bool isNew=true);
    /// Remove a feature out of the document
    void removeObject(const char* sName);
    /** Add an existing feature with sName (ASCII) to this document and set it active.
     * Unicode names are set through the Label property.
     * This is an overloaded function of the function above and can be used to create
     * a feature outside and add it to the document afterwards.
     * \note The passed feature must not yet be added to a document, otherwise an exception
     * is raised.
     */
    void addObject(DocumentObject*, const char* pObjectName=0);


    /** Copy an object from another document to this document
     * If \a recursive is true then all objects this object depends on
     * are copied as well. By default \a recursive is false.
     * Returns the copy of the object or 0 if the creation failed.
     */
    DocumentObject* copyObject(DocumentObject* obj, bool recursive=false);
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
    /// Returns true if the DocumentObject is contained in this document
    bool isIn(const DocumentObject *pFeat) const;
    /// Returns a Name of an Object or 0
    const char *getObjectName(DocumentObject *pFeat) const;
    /// Returns a Name of an Object or 0
    std::string getUniqueObjectName(const char *Name) const;
    /// Returns a name of the form prefix_number. d specifies the number of digits.
    std::string getStandardObjectName(const char *Name, int d) const;
    /// Returns a list of all Objects
    std::vector<DocumentObject*> getObjects() const;
    std::vector<DocumentObject*> getObjectsOfType(const Base::Type& typeId) const;
    /// Returns all object with given extensions. If derived=true also all objects with extensions derived from the given one
    std::vector<DocumentObject*> getObjectsWithExtension(const Base::Type& typeId, bool derived = true) const;
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
    /// Remove all modifications. After this call The document becomes Valid again.
    void purgeTouched();
    /// check if there is any touched object in this document
    bool isTouched(void) const;
    /// returns all touched objects
    std::vector<App::DocumentObject *> getTouched(void) const;
    /// set the document to be closable, this is on by default.
    void setClosable(bool);
    /// check whether the document can be closed
    bool isClosable() const;
    /// Recompute all touched features and return the number of recalculated features
    int recompute();
    /// Recompute only one feature
    void recomputeFeature(DocumentObject* Feat);
    /// get the error log from the recompute run
    const std::vector<App::DocumentObjectExecReturn*> &getRecomputeLog(void)const{return _RecomputeLog;}
    /// get the text of the error of a specified object
    const char* getErrorDescription(const App::DocumentObject*) const;
    /// return the status bits
    bool testStatus(Status pos) const;
    /// set the status bits
    void setStatus(Status pos, bool on);
    //@}


    /** @name methods for the UNDO REDO and Transaction handling */
    //@{
    /// switch the level of Undo/Redo
    void setUndoMode(int iMode);
    /// switch the level of Undo/Redo
    int getUndoMode(void) const;
    /// switch the transaction mode
    void setTransactionMode(int iMode);
    /// Open a new command Undo/Redo, an UTF-8 name can be specified
    void openTransaction(const char* name=0);
    // Commit the Command transaction. Do nothing If there is no Command transaction open.
    void commitTransaction();
    /// Abort the actually running transaction.
    void abortTransaction();
    /// Check if a transaction is open
    bool hasPendingTransaction() const;
    /// Check if a transaction is open and its list is empty.
    /// If no transaction is open true is returned.
    bool isTransactionEmpty() const;
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
    /// Returns the number of stored Undos. If greater than 0 Undo will be effective.
    int getAvailableUndos() const;
    /// Returns a list of the Undo names
    std::vector<std::string> getAvailableUndoNames() const;
    /// Will UNDO one step, returns False if no undo was done (Undos == 0).
    bool undo();
    /// Returns the number of stored Redos. If greater than 0 Redo will be effective.
    int getAvailableRedos() const;
    /// Returns a list of the Redo names.
    std::vector<std::string> getAvailableRedoNames() const;
    /// Will REDO one step, returns False if no redo was done (Redos == 0).
    bool redo() ;
    /// returns true if the document is in an Transaction phase, e.g. currently performing a redo/undo or rollback
    bool isPerformingTransaction() const;
    /// \internal remove property from a transactional object with name \a name
    void removePropertyOfObject(TransactionalObject*, const char*);
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
    /// deprecated! Use In- and OutList mimic in the DocumentObject instead!
    std::vector<App::DocumentObject*> getDependencyList
        (const std::vector<App::DocumentObject*>&) const;
    // set Changed
    //void setChanged(DocumentObject* change);
    /// get a list of topological sorted objects (https://en.wikipedia.org/wiki/Topological_sorting)
    std::vector<App::DocumentObject*> topologicalSort() const;
    /// get all root objects (objects no other one reference too)
    std::vector<App::DocumentObject*> getRootObjects() const;
    /// get all possible paths from one object to another following the OutList
    std::vector<std::list<App::DocumentObject*> > getPathsByOutList
    (const App::DocumentObject* from, const App::DocumentObject* to) const;
    //@}

    /// Function called to signal that an object identifier has been renamed
    void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> & paths, const std::function<bool(const App::DocumentObject*)> &selector = [](const App::DocumentObject *) { return true; });

    virtual PyObject *getPyObject(void);

    friend class Application;
    /// because of transaction handling
    friend class TransactionalObject;
    friend class DocumentObject;
    friend class Transaction;
    friend class TransactionDocumentObject;

    /// Destruction
    virtual ~Document();

protected:
    /// Construction
    Document(void);

    void _removeObject(DocumentObject* pcObject);
    void _addObject(DocumentObject* pcObject, const char* pObjectName);
    /// checks if a valid transaction is open
    void _checkTransaction(DocumentObject* pcObject);
    void breakDependency(DocumentObject* pcObject, bool clear);
    std::vector<App::DocumentObject*> readObjects(Base::XMLReader& reader);
    void writeObjects(const std::vector<App::DocumentObject*>&, Base::Writer &writer) const;
    bool saveToFile(const char* filename) const;

    void onBeforeChange(const Property* prop);
    void onChanged(const Property* prop);
    /// callback from the Document objects before property will be changed
    void onBeforeChangeProperty(const TransactionalObject *Who, const Property *What);
    /// callback from the Document objects after property was changed
    void onChangedProperty(const DocumentObject *Who, const Property *What);
    /// helper which Recompute only this feature
    /// @return True if the recompute process of the Document shall be stopped, False if it shall be continued.
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
