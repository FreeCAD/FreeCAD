/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_DOCUMENTOBSERVER_H
#define APP_DOCUMENTOBSERVER_H

#include <Base/BaseClass.h>
#include <boost/signals2.hpp>
#include <set>
#include <memory>

namespace App
{
class Document;
class DocumentObject;
class Property;

/**
 * The DocumentT class is a helper class to store the name of a document.
 * This can be useful when you cannot rely on that the document still exists when you have to
 * access it.
 *
 * @author Werner Mayer
 */
class AppExport DocumentT
{
public:
    /*! Constructor */
    DocumentT();
    /*! Constructor */
    DocumentT(Document*);
    /*! Constructor */
    DocumentT(const std::string&);
    /*! Destructor */
    ~DocumentT();
    /*! Assignment operator */
    void operator=(const DocumentT&);
    /*! Assignment operator */
    void operator=(const Document*);
    /*! Assignment operator */
    void operator=(const std::string&);

    /*! Get a pointer to the document or 0 if it doesn't exist any more. */
    Document* getDocument() const;
    /*! Get the name of the document. */
    const std::string &getDocumentName() const;
    /*! Get the document as Python command. */
    std::string getDocumentPython() const;

private:
    std::string document;
};

/**
 * The DocumentObjectT class is a helper class to store the names of a document object and its document.
 * This can be useful when you cannot rely on that the document or the object still exists when you have to
 * access it.
 *
 * @author Werner Mayer
 */
class AppExport DocumentObjectT
{
public:
    /*! Constructor */
    DocumentObjectT();
    /*! Constructor */
    DocumentObjectT(const DocumentObjectT &);
    /*! Constructor */
    DocumentObjectT(DocumentObjectT &&);
    /*! Constructor */
    DocumentObjectT(const DocumentObject*);
    /*! Constructor */
    DocumentObjectT(const char *docName, const char *objName);
    /*! Constructor */
    DocumentObjectT(const Property*);
    /*! Destructor */
    ~DocumentObjectT();
    /*! Assignment operator */
    DocumentObjectT &operator=(const DocumentObjectT&);
    /*! Assignment operator */
    DocumentObjectT &operator=(DocumentObjectT &&);
    /*! Assignment operator */
    void operator=(const DocumentObject*);
    /*! Assignment operator */
    void operator=(const Property*);
    /*! Equality operator */
    bool operator==(const DocumentObjectT&) const;

    /*! Get a pointer to the document or 0 if it doesn't exist any more. */
    Document* getDocument() const;
    /*! Get the name of the document. */
    const std::string &getDocumentName() const;
    /*! Get the document as Python command. */
    std::string getDocumentPython() const;
    /*! Get a pointer to the document object or 0 if it doesn't exist any more. */
    DocumentObject* getObject() const;
    /*! Get a pointer to the property or 0 if it doesn't exist any more. */
    Property* getProperty() const;
    /*! Get the name of the document object. */
    const std::string &getObjectName() const;
    /*! Get the label of the document object. */
    const std::string &getObjectLabel() const;
    /*! Get the name of the property. */
    const std::string &getPropertyName() const;
    /*! Get the document object as Python command. */
    std::string getObjectPython() const;
    /*! Get the property as Python command. */
    std::string getPropertyPython() const;
    /*! Get a pointer to the document or 0 if it doesn't exist any more or the type doesn't match. */
    template<typename T>
    inline T* getObjectAs() const
    {
        return Base::freecad_dynamic_cast<T>(getObject());
    }
    template<typename T>
    inline T* getPropertyAs() const
    {
        return Base::freecad_dynamic_cast<T>(getProperty());
    }

private:
    std::string document;
    std::string object;
    std::string label;
    std::string property;
};

class AppExport SubObjectT : public DocumentObjectT
{
public:
    /*! Constructor */
    SubObjectT();

    /*! Constructor */
    SubObjectT(const SubObjectT &);

    /*! Constructor */
    SubObjectT(SubObjectT &&);

    /*! Constructor */
    SubObjectT(const DocumentObjectT & obj, const char *subname);

    /*! Constructor */
    SubObjectT(const DocumentObject*, const char *subname);

    /*! Constructor */
    SubObjectT(const char *docName, const char *objName, const char *subname);

    /*! Assignment operator */
    SubObjectT &operator=(const SubObjectT&);

    /*! Assignment operator */
    SubObjectT &operator=(SubObjectT &&);

    /*! Equality operator */
    bool operator==(const SubObjectT&) const;

    /// Set the subname path to the sub-object
    void setSubName(const char *subname);

    /// Return the subname path
    const std::string &getSubName() const;

    /// Return the subname path without sub-element
    std::string getSubNameNoElement() const;

    /// Return the sub-element (Face, Edge, etc) of the subname path
    const char *getElementName() const;

    /// Return the new style sub-element name
    std::string getNewElementName() const;

    /** Return the old style sub-element name
     * @param index: if given, then return the element type, and extract the index
     */
    std::string getOldElementName(int *index=0) const;

    /// Return the sub-object
    DocumentObject *getSubObject() const;

    /// Return all objects along the subname path
    std::vector<DocumentObject *> getSubObjectList() const;

    bool operator<(const SubObjectT &other) const;

    std::string getSubObjectPython(bool force=true) const;

private:
    std::string subname;
};

/**
 * @brief The DocumentWeakPtrT class
 */
class AppExport DocumentWeakPtrT
{
public:
    DocumentWeakPtrT(App::Document*) noexcept;
    ~DocumentWeakPtrT();

    /*!
     * \brief reset
     * Releases the reference to the managed object. After the call *this manages no object.
     */
    void reset() noexcept;
    /*!
     * \brief expired
     * \return true if the managed object has already been deleted, false otherwise.
     */
    bool expired() const noexcept;
    /*!
     * \brief operator ->
     * \return pointer to the document
     */
    App::Document* operator->() noexcept;

private:
    // disable
    DocumentWeakPtrT(const DocumentWeakPtrT&);
    DocumentWeakPtrT& operator=(const DocumentWeakPtrT&);

    class Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief The DocumentObjectWeakPtrT class
 */
class AppExport DocumentObjectWeakPtrT
{
public:
    DocumentObjectWeakPtrT(App::DocumentObject*) noexcept;
    ~DocumentObjectWeakPtrT();

    /*!
     * \brief reset
     * Releases the reference to the managed object. After the call *this manages no object.
     */
    void reset() noexcept;
    /*!
     * \brief expired
     * \return true if the managed object has already been deleted, false otherwise.
     */
    bool expired() const noexcept;
    /*!
     * \brief operator ->
     * \return pointer to the document
     */
    App::DocumentObject* operator->() noexcept;
    /*! Get a pointer to the object or 0 if it doesn't exist any more or the type doesn't match. */
    template<typename T>
    inline T* get() const noexcept
    {
        return Base::freecad_dynamic_cast<T>(_get());
    }

private:
    App::DocumentObject* _get() const noexcept;
    // disable
    DocumentObjectWeakPtrT(const DocumentObjectWeakPtrT&);
    DocumentObjectWeakPtrT& operator=(const DocumentObjectWeakPtrT&);

private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * The DocumentObserver class simplfies the step to write classes that listen
 * to what happens inside a document.
 * This is very useful for classes that needs to be notified when an observed
 * object has changed.
 *
 * @author Werner Mayer
 */
class AppExport DocumentObserver
{

public:
    /// Constructor
    DocumentObserver();
    DocumentObserver(Document*);
    virtual ~DocumentObserver();

    /** Attaches to another document, the old document
     * is not longer observed then.
     */
    void attachDocument(Document*);
    /** Detaches from the current document, the document
     * is not longer observed then.
     */
    void detachDocument();

private:
    /** Checks if a new document was created */
    virtual void slotCreatedDocument(const App::Document& Doc);
    /** Checks if the given document is about to be closed */
    virtual void slotDeletedDocument(const App::Document& Doc);
    /** Checks if a new object was added. */
    virtual void slotCreatedObject(const App::DocumentObject& Obj);
    /** Checks if the given object is about to be removed. */
    virtual void slotDeletedObject(const App::DocumentObject& Obj);
    /** The property of an observed object has changed */
    virtual void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);
    /** Called when a given object is recomputed */
    virtual void slotRecomputedObject(const App::DocumentObject& Obj);
    /** Called when a observed document is recomputed */
    virtual void slotRecomputedDocument(const App::Document& Doc);

protected:
    Document* getDocument() const;

private:
    App::Document* _document;
    typedef boost::signals2::connection Connection;
    Connection connectApplicationCreatedDocument;
    Connection connectApplicationDeletedDocument;
    Connection connectDocumentCreatedObject;
    Connection connectDocumentDeletedObject;
    Connection connectDocumentChangedObject;
    Connection connectDocumentRecomputedObject;
    Connection connectDocumentRecomputed;
};

/**
 * The DocumentObjectObserver class checks for a list of objects
 * which of them get removed.
 *
 * @author Werner Mayer
 */
class AppExport DocumentObjectObserver : public DocumentObserver
{

public:
    typedef std::set<App::DocumentObject*>::const_iterator const_iterator;

    /// Constructor
    DocumentObjectObserver();
    virtual ~DocumentObjectObserver();

    const_iterator begin() const;
    const_iterator end() const;
    void addToObservation(App::DocumentObject*);
    void removeFromObservation(App::DocumentObject*);

private:
    /** Checks if a new document was created */
    virtual void slotCreatedDocument(const App::Document& Doc);
    /** Checks if the given document is about to be closed */
    virtual void slotDeletedDocument(const App::Document& Doc);
    /** Checks if a new object was added. */
    virtual void slotCreatedObject(const App::DocumentObject& Obj);
    /** Checks if the given object is about to be removed. */
    virtual void slotDeletedObject(const App::DocumentObject& Obj);
    /** The property of an observed object has changed */
    virtual void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);
    /** This method gets called when all observed objects are deleted or the whole document is deleted.
      * This method can be re-implemented to perform an extra step like closing a dialog that observes
      * a document.
      */
    virtual void cancelObservation();

private:
    std::set<App::DocumentObject*> _objects;
};

} //namespace App

#endif // APP_DOCUMENTOBSERVER_H
