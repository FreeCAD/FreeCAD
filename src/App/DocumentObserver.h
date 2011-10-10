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

#include <boost/signals.hpp>
#include <set>

namespace App
{
class Document;
class DocumentObject;
class Property;

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
    virtual void slotCreatedDocument(const App::Document& Doc) = 0;
    /** Checks if the given document is about to be closed */
    virtual void slotDeletedDocument(const App::Document& Doc) = 0;
    /** Checks if a new object was added. */
    virtual void slotCreatedObject(const App::DocumentObject& Obj) = 0;
    /** Checks if the given object is about to be removed. */
    virtual void slotDeletedObject(const App::DocumentObject& Obj) = 0;
    /** The property of an observed object has changed */
    virtual void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop) = 0;

protected:
    Document* getDocument() const;

private:
    App::Document* _document;
    typedef boost::signals::connection Connection;
    Connection connectApplicationCreatedDocument;
    Connection connectApplicationDeletedDocument;
    Connection connectDocumentCreatedObject;
    Connection connectDocumentDeletedObject;
    Connection connectDocumentChangedObject;
};

/**
 * The DocumentObjectObserver class checks for a list of ojects
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
    virtual void cancelObservation() = 0;

private:
    std::set<App::DocumentObject*> _objects;
};

} //namespace App

#endif // APP_DOCUMENTOBSERVER_H
