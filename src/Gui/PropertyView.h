/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "DockWindow.h"
#include "Selection.h"


class QPixmap;
class QTabWidget;

namespace App
{
class Property;
class PropertyContainer;
class DocumentObject;
}  // namespace App

namespace Gui
{
namespace PropertyEditor
{

class EditableListView;
class EditableItem;
class PropertyEditor;

}  // namespace PropertyEditor
}  // namespace Gui

namespace Gui
{
class ViewProvider;

/** The property view class.
 */
class PropertyView: public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit PropertyView(QWidget* parent = nullptr);
    ~PropertyView() override;

    Gui::PropertyEditor::PropertyEditor* propertyEditorView;
    Gui::PropertyEditor::PropertyEditor* propertyEditorData;
    void clearPropertyItemSelection();
    static bool showAll();
    static void setShowAll(bool);
    static bool isPropertyHidden(const App::Property*);

public Q_SLOTS:
    /// Stores a preference for the last tab selected
    void tabChanged(int index);
    void onTimer();

protected:
    void changeEvent(QEvent* e) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

private:
    void onSelectionChanged(const SelectionChanges& msg) override;
    void slotChangePropertyData(const App::Property&);
    void slotChangePropertyView(const Gui::ViewProvider&, const App::Property&);
    void slotAppendDynamicProperty(const App::Property&);
    void slotRemoveDynamicProperty(const App::Property&);
    void slotRenameDynamicProperty(const App::Property&, const char* oldName);
    void slotChangePropertyEditor(const App::Document&, const App::Property&);
    void slotRollback();
    void slotActiveDocument(const Gui::Document&);
    void slotDeleteDocument(const Gui::Document&);
    void slotDeletedViewObject(const Gui::ViewProvider&);
    void slotDeletedObject(const App::DocumentObject&);

    void checkEnable(const char* doc = nullptr);

private:
    struct PropInfo;
    struct PropFind;
    using Connection = fastsignals::connection;
    Connection connectPropData;
    Connection connectPropView;
    Connection connectPropAppend;
    Connection connectPropRemove;
    Connection connectPropRename;
    Connection connectPropChange;
    Connection connectUndoDocument;
    Connection connectRedoDocument;
    Connection connectActiveDoc;
    Connection connectDelDocument;
    Connection connectDelObject;
    Connection connectDelViewObject;
    Connection connectChangedDocument;
    QTabWidget* tabs;
    QTimer* timer;
    bool updating = false;
};

namespace DockWnd
{

/** A dock window with the embedded property view.
 */
class PropertyDockView: public Gui::DockWindow
{
    Q_OBJECT

public:
    explicit PropertyDockView(Gui::Document* pcDocument, QWidget* parent = nullptr);
    ~PropertyDockView() override;
};

}  // namespace DockWnd
}  // namespace Gui
