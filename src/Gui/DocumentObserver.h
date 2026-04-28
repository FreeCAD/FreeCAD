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
#include <fastsignals/signal.h>


namespace App
{
class Property;
}
namespace Gui
{
class Document;
class ViewProviderDocumentObject;

/**
 * The DocumentT class is a helper class to store the name of a document.
 * This can be useful when you cannot rely on that the document still exists when you have to
 * access it.
 *
 * @author Werner Mayer
 */
class GuiExport DocumentT
{
public:
    /*! Constructor */
    DocumentT();
    /*! Constructor */
    explicit DocumentT(Document*);
    /*! Constructor */
    explicit DocumentT(const std::string&);
    /*! Constructor */
    DocumentT(const DocumentT&);
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
    std::string getDocumentName() const;
    /*! Get the Gui::Document as Python command. */
    std::string getGuiDocumentPython() const;
    /*! Get the App::Document as Python command. */
    std::string getAppDocumentPython() const;

private:
    std::string document;
};

/**
 * The ViewProviderT class is a helper class to store the names of a view provider and its document.
 * This can be useful when you cannot rely on that the document or the object still exists when you
 * have to access it.
 *
 * @author Werner Mayer
 */
class GuiExport ViewProviderT
{
public:
    /*! Constructor */
    ViewProviderT();
    /*! Constructor */
    ViewProviderT(const ViewProviderT&);
    /*! Constructor */
    ViewProviderT(ViewProviderT&&);
    /*! Constructor */
    explicit ViewProviderT(const ViewProviderDocumentObject*);
    /*! Destructor */
    ~ViewProviderT();
    /*! Assignment operator */
    ViewProviderT& operator=(const ViewProviderT&);
    /*! Assignment operator */
    ViewProviderT& operator=(ViewProviderT&&);
    /*! Assignment operator */
    void operator=(const ViewProviderDocumentObject*);
    /*! Equality operator */
    bool operator==(const ViewProviderT&) const;

    /*! Get a pointer to the document or 0 if it doesn't exist any more. */
    Document* getDocument() const;
    /*! Get the name of the document. */
    const std::string& getDocumentName() const;
    /*! Get the Gui::Document as Python command. */
    std::string getGuiDocumentPython() const;
    /*! Get the App::Document as Python command. */
    std::string getAppDocumentPython() const;
    /*! Get a pointer to the document object or 0 if it doesn't exist any more. */
    ViewProviderDocumentObject* getViewProvider() const;
    /*! Get the name of the document object. */
    const std::string& getObjectName() const;
    /*! Get the document object as Python command. */
    std::string getObjectPython() const;
    /*! Get a pointer to the document or 0 if it doesn't exist any more or the type doesn't match. */
    template<typename T>
    inline T* getObjectAs() const
    {
        return freecad_cast<T*>(getViewProvider());
    }

private:
    std::string document;
    std::string object;
};

/**
 * @brief The DocumentWeakPtrT class
 */
class GuiExport DocumentWeakPtrT
{
public:
    explicit DocumentWeakPtrT(Gui::Document*) noexcept;
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
    Gui::Document* operator*() const noexcept;
    /*!
     * \brief operator ->
     * \return pointer to the document
     */
    Gui::Document* operator->() const noexcept;

    // disable
    DocumentWeakPtrT(const DocumentWeakPtrT&) = delete;
    DocumentWeakPtrT& operator=(const DocumentWeakPtrT&) = delete;

private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief The ViewProviderWeakPtrT class
 */
class GuiExport ViewProviderWeakPtrT
{
public:
    explicit ViewProviderWeakPtrT(ViewProviderDocumentObject*);

    ViewProviderWeakPtrT(ViewProviderWeakPtrT&&);
    ViewProviderWeakPtrT& operator=(ViewProviderWeakPtrT&&);

    FC_DISABLE_COPY(ViewProviderWeakPtrT);

    ~ViewProviderWeakPtrT();

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
    ViewProviderWeakPtrT& operator=(ViewProviderDocumentObject* p);
    /*!
     * \brief operator *
     * \return pointer to the document
     */
    ViewProviderDocumentObject* operator*() const noexcept;
    /*!
     * \brief operator ->
     * \return pointer to the document
     */
    ViewProviderDocumentObject* operator->() const noexcept;
    /*!
     * \brief operator ==
     * \return true if both objects are equal, false otherwise
     */
    bool operator==(const ViewProviderWeakPtrT& p) const noexcept;
    /*!
     * \brief operator !=
     * \return true if both objects are inequal, false otherwise
     */
    bool operator!=(const ViewProviderWeakPtrT& p) const noexcept;
    /*! Get a pointer to the object or 0 if it doesn't exist any more or the type doesn't match. */
    template<typename T>
    inline T* get() const noexcept
    {
        return freecad_cast<T*>(_get());
    }

private:
    ViewProviderDocumentObject* _get() const noexcept;

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
     * \brief operator *
     * \return pointer to the view provider
     */
    T* operator*() const
    {
        return ptr.get<T>();
    }
    /*!
     * \brief operator ->
     * \return pointer to the view provider
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
    WeakPtrT& operator=(const WeakPtrT&) = delete;

private:
    ViewProviderWeakPtrT ptr;
};

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable : 4251)  // MSVC emits warning C4251 too conservatively for our use-case
#endif

/**
 * The DocumentObserver class simplifies the step to write classes that listen
 * to what happens inside a document.
 * This is very useful for classes that needs to be notified when an observed
 * object has changed.
 *
 * @author Werner Mayer
 */
class GuiExport DocumentObserver
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
    /** Notifies when an object has been created. */
    virtual void slotCreatedObject(const ViewProviderDocumentObject& Obj);
    /** Notifies when the object is about to be removed. */
    virtual void slotDeletedObject(const ViewProviderDocumentObject& Obj);
    /** The property of an observed object has changed */
    virtual void slotChangedObject(const ViewProviderDocumentObject& Obj, const App::Property& Prop);
    /** Notifies when the object has been relabeled. */
    virtual void slotRelabelObject(const ViewProviderDocumentObject& Obj);
    /** Notifies when the object has been activated. */
    virtual void slotActivatedObject(const ViewProviderDocumentObject& Obj);
    /** Notifies when the object entered edit mode. */
    virtual void slotEnterEditObject(const ViewProviderDocumentObject& Obj);
    /** Notifies when the object resets edit mode. */
    virtual void slotResetEditObject(const ViewProviderDocumentObject& Obj);
    /** Notifies on undo */
    virtual void slotUndoDocument(const Document& Doc);
    /** Notifies on redo */
    virtual void slotRedoDocument(const Document& Doc);
    /** Notifies on deletion */
    virtual void slotDeleteDocument(const Document& Doc);

private:
    using Connection = fastsignals::scoped_connection;
    Connection connectDocumentCreatedObject;
    Connection connectDocumentDeletedObject;
    Connection connectDocumentChangedObject;
    Connection connectDocumentRelabelObject;
    Connection connectDocumentActivateObject;
    Connection connectDocumentEditObject;
    Connection connectDocumentResetObject;
    Connection connectDocumentUndo;
    Connection connectDocumentRedo;
    Connection connectDocumentDelete;
};

#ifdef _MSC_VER
# pragma warning(pop)
#endif

}  // namespace Gui
