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
#include <boost/signals.hpp>
#include <QFlags>

namespace App { class Property; }
namespace Gui
{
class Document;
class ViewProviderDocumentObject;

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
    enum Notification {
        None = 0x0000,
        Create = 0x0001,
        Delete = 0x0002,
        Change = 0x0004,
        Relabel = 0x0008,
        Activate = 0x0010,
        Edit = 0x0020,
        Reset = 0x0040,
        Undo = 0x0080,
        Redo = 0x0100,
        All = 0x01ff
    };
    Q_DECLARE_FLAGS(Notifications, Notification)

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
    /** Activates the connection depending on the given value.
     */
    void enableNotifications(Notifications value);

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
    typedef boost::BOOST_SIGNALS_NAMESPACE::scoped_connection Connection;
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
