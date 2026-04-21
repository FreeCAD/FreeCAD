// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <Base/BaseClass.h>
#include <Base/Bitmask.h>
#include <fastsignals/signal.h>
#include <memory>
#include <set>
#include <string>
#include <FCGlobal.h>


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
    DocumentT(Document*);  // explicit bombs
    /*! Constructor */
    explicit DocumentT(std::string);
    /*! Constructor */
    DocumentT(const DocumentT&);
    /*! Move constructor */
    DocumentT(DocumentT&&) noexcept;
    /*! Destructor */
    ~DocumentT();
    /*! Assignment operator */
    DocumentT& operator=(const DocumentT&);
    /*! Move assignment operator */
    DocumentT& operator=(DocumentT&&) noexcept;
    /*! Assignment operator */
    DocumentT& operator=(const Document*);
    /*! Assignment operator */
    DocumentT& operator=(const std::string&);

    bool operator==(const DocumentT& other) const
    {
        return document == other.document;
    }

    bool operator<(const DocumentT& other) const
    {
        return document < other.document;
    }

    explicit operator bool() const
    {
        return getDocument() != nullptr;
    }

    /*! Get a pointer to the document or 0 if it doesn't exist any more. */
    Document* getDocument() const;
    /*! Get the name of the document. */
    const std::string& getDocumentName() const;
    /*! Get the document as Python command. */
    std::string getDocumentPython() const;

private:
    std::string document;
};

/**
 * The DocumentObjectT class is a helper class to store the names of a document object and its
 * document. This can be useful when you cannot rely on that the document or the object still exists
 * when you have to access it.
 *
 * @author Werner Mayer
 */
class AppExport DocumentObjectT
{
public:
    /*! Constructor */
    DocumentObjectT();
    /*! Constructor */
    DocumentObjectT(const DocumentObjectT&);
    /*! Constructor */
    DocumentObjectT(DocumentObjectT&&) noexcept;
    /*! Constructor */
    explicit DocumentObjectT(const DocumentObject*);
    /*! Constructor */
    DocumentObjectT(const Document*, const std::string& objName);
    /*! Constructor */
    DocumentObjectT(const char* docName, const char* objName);
    /*! Constructor */
    explicit DocumentObjectT(const Property*);
    /*! Destructor */
    ~DocumentObjectT();
    /*! Assignment operator */
    DocumentObjectT& operator=(const DocumentObjectT&);
    /*! Assignment operator */
    DocumentObjectT& operator=(DocumentObjectT&&) noexcept;
    /*! Assignment operator */
    DocumentObjectT& operator=(const DocumentObject*);
    /*! Assignment operator */
    DocumentObjectT& operator=(const Property*);
    /*! Equality operator */
    bool operator==(const DocumentObjectT&) const;

    bool operator<(const DocumentObjectT& other) const
    {
        return object < other.object;
    }

    explicit operator bool() const
    {
        return getObject() != nullptr;
    }

    /*! Get a pointer to the document or 0 if it doesn't exist any more. */
    Document* getDocument() const;
    /*! Get the name of the document. */
    const std::string& getDocumentName() const;
    /*! Get the document as Python command. */
    std::string getDocumentPython() const;
    /*! Get a pointer to the document object or 0 if it doesn't exist any more. */
    DocumentObject* getObject() const;
    /*! Get a pointer to the property or 0 if it doesn't exist any more. */
    Property* getProperty() const;
    /*! Get a pointer to the property by name or 0 if it doesn't exist any more. */
    Property* getPropertyByName(const char* name) const;
    /*! Get the name of the document object. */
    const std::string& getObjectName() const;
    /*! Get the name of the document object. */
    const char* getNameInDocument() const;
    bool isAttachedToDocument() const;
    /*! Get the label of the document object. */
    const std::string& getObjectLabel() const;
    /*! Get the name of the property. */
    const std::string& getPropertyName() const;
    /*! Get the document object as Python command. */
    std::string getObjectPython() const;
    /*! Get the property as Python command. */
    std::string getPropertyPython() const;
    /*! Get a pointer to the document or 0 if it doesn't exist any more or the type doesn't match.
     */
    template<typename T>
    inline T* getObjectAs() const
    {
        return freecad_cast<T*>(getObject());
    }
    template<typename T>
    inline T* getPropertyAs() const
    {
        return freecad_cast<T*>(getProperty());
    }

private:
    std::string document;
    std::string object;
    std::string label;
    std::string property;
};

class AppExport SubObjectT: public DocumentObjectT
{
public:
    /*! Constructor */
    SubObjectT();

    /*! Destructor */
    ~SubObjectT();

    /*! Constructor */
    SubObjectT(const SubObjectT&);

    /*! Constructor */
    SubObjectT(SubObjectT&&) noexcept;

    /*! Constructor */
    SubObjectT(const DocumentObjectT& obj, const char* subname);

    /*! Constructor */
    SubObjectT(const DocumentObject*, const char* subname);

    /*! Constructor */
    SubObjectT(const DocumentObject*);  // explicit bombs

    /*! Constructor */
    SubObjectT(const char* docName, const char* objName, const char* subname);

    /*! Assignment operator */
    SubObjectT& operator=(const SubObjectT&);

    /*! Assignment operator */
    SubObjectT& operator=(SubObjectT&&) noexcept;

    /*! Assignment operator */
    SubObjectT& operator=(const DocumentObjectT&);

    /*! Assignment operator */
    SubObjectT& operator=(const App::DocumentObject*);

    /*! Equality operator */
    bool operator==(const SubObjectT&) const;

    /// Set the subname path to the sub-object
    void setSubName(const char* subname);

    /// Set the subname path to the sub-object
    void setSubName(const std::string& subname)
    {
        setSubName(subname.c_str());
    }

    /// Return the subname path
    const std::string& getSubName() const;

    /** Return docname#objname (label)
     * @param docName: optional document name. The document prefix will only be printed
     * if it is different then the given 'doc'.
     */
    std::string getObjectFullName(const char* docName = nullptr) const;

    /** Return docname#objname.subname (label)
     * @param doc: optional document name. The document prefix will only be printed
     * if it is different then the given 'doc'.
     */
    std::string getSubObjectFullName(const char* docName = nullptr) const;
    /// Return the subname path without sub-element
    std::string getSubNameNoElement() const;

    /// Return the sub-element (Face, Edge, etc) of the subname path
    const char* getElementName() const;

    /// Check if there is any sub object reference
    bool hasSubObject() const;

    /// Check if there is any sub element reference
    bool hasSubElement() const;

    /// Return the new style sub-element name
    std::string getNewElementName() const;

    /** Return the old style sub-element name
     * @param index: if given, then return the element type, and extract the index
     */
    std::string getOldElementName(int* index = nullptr) const;

    /// Return the sub-object
    DocumentObject* getSubObject() const;

    /// Return all objects along the subname path
    std::vector<DocumentObject*> getSubObjectList() const;

    bool operator<(const SubObjectT& other) const;

    std::string getSubObjectPython(bool force = true) const;

    /// Options used by normalize()
    enum class NormalizeOption : uint8_t
    {
        /// Do not include sub-element reference in the output path
        NoElement = 0x01,

        /** Do not flatten the output path. If not specified, the output path
         * will be flatten to exclude intermediate objects that belong to the
         * same geo feature group before resolving. For example,
         *      Part.Fusion.Box. -> Part.Box.
         */
        NoFlatten = 0x02,

        /** Do not change the sub-object component inside the path. Each
         * component of the subname object path can be either the object
         * internal name, the label of the object if starts with '$', or an
         * integer index. If this option is not specified, each component will
         * be converted to object internal name, except for integer index.
         */
        KeepSubName = 0x04,

        /** Convert integer index in the path to sub-object internal name */
        ConvertIndex = 0x08,
    };
    using NormalizeOptions = Base::Flags<NormalizeOption>;

    /** Normalize the subname path to use only the object internal name and old style element name
     * @return Return whether the subname has been changed
     */
    bool normalize(NormalizeOptions options = NormalizeOption());

    /// Return a normalize copy of itself
    SubObjectT normalized(NormalizeOptions options = NormalizeOption()) const;

private:
    std::string subname;
};

/**
 * The PropertyLinkT class is a helper class to create Python statements for property links.
 */
class AppExport PropertyLinkT
{
public:
    /*! Constructor */
    PropertyLinkT();

    /*! Constructor */
    explicit PropertyLinkT(DocumentObject* obj);

    /*! Constructor */
    PropertyLinkT(DocumentObject* obj, const std::vector<std::string>& subNames);

    /*! Constructor */
    explicit PropertyLinkT(const std::vector<DocumentObject*>& objs);

    /*! Constructor */
    PropertyLinkT(const std::vector<DocumentObject*>& objs,
                  const std::vector<std::string>& subNames);

    /*! Get the property as Python command. */
    std::string getPropertyPython() const;

private:
    std::string toPython;
};

/**
 * @brief The DocumentWeakPtrT class
 */
class AppExport DocumentWeakPtrT
{
public:
    explicit DocumentWeakPtrT(App::Document*) noexcept;
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
     * \brief operator *
     * \return pointer to the document
     */
    App::Document* operator*() const noexcept;
    /*!
     * \brief operator ->
     * \return pointer to the document
     */
    App::Document* operator->() const noexcept;

    // disable
    DocumentWeakPtrT(const DocumentWeakPtrT&) = delete;
    DocumentWeakPtrT(DocumentWeakPtrT&&) = delete;
    DocumentWeakPtrT& operator=(const DocumentWeakPtrT&) = delete;
    DocumentWeakPtrT& operator=(DocumentWeakPtrT&&) = delete;

private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief The DocumentObjectWeakPtrT class
 */
class AppExport DocumentObjectWeakPtrT
{
public:
    explicit DocumentObjectWeakPtrT(App::DocumentObject*);
    ~DocumentObjectWeakPtrT();

    // disable copy
    DocumentObjectWeakPtrT(const DocumentObjectWeakPtrT &) = delete;
    DocumentObjectWeakPtrT &operator=(const DocumentObjectWeakPtrT &) = delete;

    // default move
    DocumentObjectWeakPtrT(DocumentObjectWeakPtrT &&);
    DocumentObjectWeakPtrT &operator=(DocumentObjectWeakPtrT &&);

    /*!
     * \brief reset
     * Releases the reference to the managed object. After the call *this manages no object.
     */
    void reset();
    /*!
     * \brief expired
     * \return true if the managed object has already been deleted, false otherwise.
     */
    bool expired() const noexcept;
    /*!
     * \brief operator =
     * Assignment operator
     */
    DocumentObjectWeakPtrT& operator=(App::DocumentObject* p);
    /*!
     * \brief operator *
     * \return pointer to the document object
     */
    App::DocumentObject* operator*() const noexcept;
    /*!
     * \brief operator ->
     * \return pointer to the document object
     */
    App::DocumentObject* operator->() const noexcept;
    /*!
     * \brief operator ==
     * \return true if both objects are equal, false otherwise
     */
    bool operator==(const DocumentObjectWeakPtrT& p) const noexcept;
    /*!
     * \brief operator !=
     * \return true if both objects are inequal, false otherwise
     */
    bool operator!=(const DocumentObjectWeakPtrT& p) const noexcept;
    /*! Get a pointer to the object or 0 if it doesn't exist any more or the type doesn't match. */
    template<typename T>
    inline T* get() const noexcept
    {
        return freecad_cast<T*>(_get());
    }

private:
    App::DocumentObject* _get() const noexcept;
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief The WeakPtrT class
 */
template<class T>
class WeakPtrT
{
public:
    explicit WeakPtrT(T* t)
        : ptr(t)
    {}
    ~WeakPtrT() = default;

    /*!
     * \brief reset
     * Releases the reference to the managed object. After the call *this manages no object.
     */
    void reset()
    {
        ptr.reset();
    }
    /*!
     * \brief expired
     * \return true if the managed object has already been deleted, false otherwise.
     */
    bool expired() const
    {
        return ptr.expired();
    }
    /*!
     * \brief operator =
     * Assignment operator
     */
    WeakPtrT<T>& operator=(T* p)
    {
        ptr = p;
        return *this;
    }
    /*!
     * \brief operator ->
     * \return pointer to the document object
     */
    T* operator*() const
    {
        return ptr.get<T>();
    }
    /*!
     * \brief operator ->
     * \return pointer to the document object
     */
    T* operator->() const
    {
        return ptr.get<T>();
    }
    /*!
     * \brief operator ==
     * \return true if both objects are equal, false otherwise
     */
    bool operator==(const WeakPtrT<T>& p) const
    {
        return ptr == p.ptr;
    }
    /*!
     * \brief operator !=
     * \return true if both objects are inequal, false otherwise
     */
    bool operator!=(const WeakPtrT<T>& p) const
    {
        return ptr != p.ptr;
    }
    /*! Get a pointer to the object or 0 if it doesn't exist any more. */
    T* get() const noexcept
    {
        return ptr.get<T>();
    }

    // disable
    WeakPtrT(const WeakPtrT&) = delete;
    WeakPtrT(WeakPtrT&&) = delete;
    WeakPtrT& operator=(const WeakPtrT&) = delete;
    WeakPtrT& operator=(WeakPtrT&&) = delete;

private:
    DocumentObjectWeakPtrT ptr;
};

/**
 * The DocumentObserver class simplifies the step to write classes that listen
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
    explicit DocumentObserver(Document*);
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
    /** Called when a new document was created */
    virtual void slotCreatedDocument(const App::Document& Doc);
    /** Called when a document is about to be closed */
    virtual void slotDeletedDocument(const App::Document& Doc);
    /** Called when a document is activated */
    virtual void slotActivateDocument(const App::Document& Doc);
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
    using Connection = fastsignals::connection;
    Connection connectApplicationCreatedDocument;
    Connection connectApplicationDeletedDocument;
    Connection connectApplicationActivateDocument;
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
class AppExport DocumentObjectObserver: public DocumentObserver
{

public:
    using const_iterator = std::set<App::DocumentObject*>::const_iterator;

    /// Constructor
    DocumentObjectObserver();
    ~DocumentObjectObserver() override;

    const_iterator begin() const;
    const_iterator end() const;
    void addToObservation(App::DocumentObject*);
    void removeFromObservation(App::DocumentObject*);

private:
    /** Checks if a new document was created */
    void slotCreatedDocument(const App::Document& Doc) override;
    /** Checks if the given document is about to be closed */
    void slotDeletedDocument(const App::Document& Doc) override;
    /** Checks if a new object was added. */
    void slotCreatedObject(const App::DocumentObject& Obj) override;
    /** Checks if the given object is about to be removed. */
    void slotDeletedObject(const App::DocumentObject& Obj) override;
    /** The property of an observed object has changed */
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop) override;
    /** This method gets called when all observed objects are deleted or the whole document is
     * deleted. This method can be re-implemented to perform an extra step like closing a dialog
     * that observes a document.
     */
    virtual void cancelObservation();

private:
    std::set<App::DocumentObject*> _objects;
};

}  // namespace App

template<>
struct std::hash<App::DocumentObjectWeakPtrT>
{
    std::size_t operator()(const App::DocumentObjectWeakPtrT& ptr) const noexcept
    {
        return std::hash<App::DocumentObject*>{}(*ptr);
    }
};

ENABLE_BITMASK_OPERATORS(App::SubObjectT::NormalizeOption)
