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


#ifndef GUI_DOCUMENTOBSERVER_H
#define GUI_DOCUMENTOBSERVER_H

#include <Base/BaseClass.h>
#include <boost_signals2.hpp>

namespace App { class Property; }
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
    DocumentT(Document*);
    /*! Constructor */
    DocumentT(const std::string&);
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
 * This can be useful when you cannot rely on that the document or the object still exists when you have to
 * access it.
 *
 * @author Werner Mayer
 */
class GuiExport ViewProviderT
{
public:
    /*! Constructor */
    ViewProviderT();
    /*! Constructor */
    ViewProviderT(ViewProviderDocumentObject*);
    /*! Constructor */
    ViewProviderT(const ViewProviderT&);
    /*! Destructor */
    ~ViewProviderT();
    /*! Assignment operator */
    void operator=(const ViewProviderT&);
    /*! Assignment operator */
    void operator=(const ViewProviderDocumentObject*);

    /*! Get a pointer to the document or 0 if it doesn't exist any more. */
    Document* getDocument() const;
    /*! Get the name of the document. */
    std::string getDocumentName() const;
    /*! Get the Gui::Document as Python command. */
    std::string getGuiDocumentPython() const;
    /*! Get the App::Document as Python command. */
    std::string getAppDocumentPython() const;
    /*! Get a pointer to the document object or 0 if it doesn't exist any more. */
    ViewProviderDocumentObject* getViewProvider() const;
    /*! Get the name of the document object. */
    std::string getObjectName() const;
    /*! Get the document object as Python command. */
    std::string getObjectPython() const;
    /*! Get a pointer to the document or 0 if it doesn't exist any more or the type doesn't match. */
    template<typename T>
    inline T* getObjectAs() const
    {
        return Base::freecad_dynamic_cast<T>(getViewProvider());
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
    DocumentWeakPtrT(Gui::Document*) noexcept;
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
    Gui::Document* operator->() noexcept;

private:
    // disable
    DocumentWeakPtrT(const DocumentWeakPtrT&);
    DocumentWeakPtrT& operator=(const DocumentWeakPtrT&);

    class Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief The ViewProviderWeakPtrT class
 */
class AppExport ViewProviderWeakPtrT
{
public:
    ViewProviderWeakPtrT(ViewProviderDocumentObject*) noexcept;
    ~ViewProviderWeakPtrT();

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
    ViewProviderDocumentObject* operator->() noexcept;
    /*! Get a pointer to the object or 0 if it doesn't exist any more or the type doesn't match. */
    template<typename T>
    inline T* get() const noexcept
    {
        return Base::freecad_dynamic_cast<T>(_get());
    }

private:
    ViewProviderDocumentObject* _get() const noexcept;
    // disable
    ViewProviderWeakPtrT(const ViewProviderWeakPtrT&);
    ViewProviderWeakPtrT& operator=(const ViewProviderWeakPtrT&);

private:
    class Private;
    std::unique_ptr<Private> d;
};

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
    virtual void slotChangedObject(const ViewProviderDocumentObject& Obj,
                                   const App::Property& Prop);
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
    typedef boost::signals2::scoped_connection Connection;
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

} //namespace Gui

#endif // GUI_DOCUMENTOBSERVER_H
