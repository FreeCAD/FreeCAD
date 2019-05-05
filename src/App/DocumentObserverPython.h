/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_DOCUMENTOBSERVERPYTHON_H
#define APP_DOCUMENTOBSERVERPYTHON_H

#include <CXX/Objects.hxx>

#include <boost/signals2.hpp>
#include <boost/bind.hpp>

namespace App
{

/**
 * The DocumentObserverPython class is used to notify registered Python instances
 * whenever something happens to a document, like creation, destruction, adding or
 * removing objects or when property changes.
 *
 * @author Werner Mayer
 */
class AppExport DocumentObserverPython
{

public:
    /// Constructor
    DocumentObserverPython(const Py::Object& obj);
    virtual ~DocumentObserverPython();

    static void addObserver(const Py::Object& obj);
    static void removeObserver(const Py::Object& obj);

private:
    /** Checks if a new document was created */
    void slotCreatedDocument(const App::Document& Doc);
    /** Checks if the given document is about to be closed */
    void slotDeletedDocument(const App::Document& Doc);
    /** Checks if the given document is relabeled */
    void slotRelabelDocument(const App::Document& Doc);
    /** Checks if the given document is activated */
    void slotActivateDocument(const App::Document& Doc);
    /** The property of an observed document has changed */
    void slotBeforeChangeDocument(const App::Document& Obj, const App::Property& Prop);
    /** The property of an observed document has changed */
    void slotChangedDocument(const App::Document& Obj, const App::Property& Prop);
    /** Checks if a new object was added. */
    void slotCreatedObject(const App::DocumentObject& Obj);
    /** Checks if the given object is about to be removed. */
    void slotDeletedObject(const App::DocumentObject& Obj);
    /** The property of an observed object has changed */
    void slotBeforeChangeObject(const App::DocumentObject& Obj, const App::Property& Prop);
    /** The property of an observed object has changed */
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);
    /** Undoes the last transaction of the document */
    void slotUndoDocument(const App::Document& Doc);
    /** Redoes the last undone transaction of the document */
    void slotRedoDocument(const App::Document& Doc);
    /** Called when a given object is recomputed */
    void slotRecomputedObject(const App::DocumentObject& Obj);
    /** Called before an observed document is recomputed */
    void slotBeforeRecomputeDocument(const App::Document& Doc);
    /** Called when an observed document is recomputed */
    void slotRecomputedDocument(const App::Document& Doc);
    /** Called when an observed document opens a transaction */
    void slotOpenTransaction(const App::Document& Doc, std::string str);
    /** Called when an observed document commits a transaction */
    void slotCommitTransaction(const App::Document& Doc);
    /** Called when an observed document aborts a transaction */
    void slotAbortTransaction(const App::Document& Doc);
    /** Called when an object gets a new dynamic property added*/
    void slotAppendDynamicProperty(const App::Property& Prop);
    /** Called when an object gets a dynamic property removed*/
    void slotRemoveDynamicProperty(const App::Property& Prop);
    /** Called when an object property gets a new editor relevant status like hidden or read only*/
    void slotChangePropertyEditor(const App::Document &Doc, const App::Property& Prop);
    /** Called when a document is about to be saved*/
    void slotStartSaveDocument(const App::Document&, const std::string&);
    /** Called when an document has been saved*/
    void slotFinishSaveDocument(const App::Document&, const std::string&);

private:
    Py::Object inst;
    static std::vector<DocumentObserverPython*> _instances;

    typedef boost::signals2::connection Connection;

#define FC_PY_DOC_OBSERVER \
    FC_PY_ELEMENT(CreatedDocument,_1) \
    FC_PY_ELEMENT(DeletedDocument,_1) \
    FC_PY_ELEMENT(RelabelDocument,_1) \
    FC_PY_ELEMENT(ActivateDocument,_1) \
    FC_PY_ELEMENT(UndoDocument,_1) \
    FC_PY_ELEMENT(RedoDocument,_1) \
    FC_PY_ELEMENT(BeforeChangeDocument,_1,_2) \
    FC_PY_ELEMENT(ChangedDocument,_1,_2) \
    FC_PY_ELEMENT(CreatedObject,_1) \
    FC_PY_ELEMENT(DeletedObject,_1) \
    FC_PY_ELEMENT(BeforeChangeObject,_1,_2) \
    FC_PY_ELEMENT(ChangedObject,_1,_2) \
    FC_PY_ELEMENT(RecomputedObject,_1) \
    FC_PY_ELEMENT(BeforeRecomputeDocument,_1) \
    FC_PY_ELEMENT(RecomputedDocument,_1) \
    FC_PY_ELEMENT(OpenTransaction,_1,_2) \
    FC_PY_ELEMENT(CommitTransaction,_1) \
    FC_PY_ELEMENT(AbortTransaction,_1) \
    FC_PY_ELEMENT(StartSaveDocument,_1,_2) \
    FC_PY_ELEMENT(FinishSaveDocument,_1,_2) \
    FC_PY_ELEMENT(AppendDynamicProperty,_1) \
    FC_PY_ELEMENT(RemoveDynamicProperty,_1) \
    FC_PY_ELEMENT(ChangePropertyEditor,_1,_2)

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name,...) \
    Connection connect##_name;\
    Py::Object py##_name;

    FC_PY_DOC_OBSERVER
};

} //namespace App

#endif // APP_DOCUMENTOBSERVERPYTHON_H
