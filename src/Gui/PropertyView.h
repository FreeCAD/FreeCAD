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



#ifndef GUI_DOCKWND_PROPERTYVIEW_H
#define GUI_DOCKWND_PROPERTYVIEW_H


#include "DockWindow.h"
#include "Selection.h"

class QPixmap;
class QTabWidget;

namespace App {
  class PropertyContainer;
}

namespace Gui {
namespace PropertyEditor {

class EditableListView;
class EditableItem;
class PropertyEditor;

} // namespace PropertyEditor
} // namespace Gui

namespace Gui {


/** The property view class.
 */
class PropertyView : public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    PropertyView(QWidget *parent=0);
    virtual ~PropertyView();

    Gui::PropertyEditor::PropertyEditor* propertyEditorView;
    Gui::PropertyEditor::PropertyEditor* propertyEditorData;

protected:
    void changeEvent(QEvent *e);

private:
    void onSelectionChanged(const SelectionChanges& msg);

private:
    QTabWidget* tabs;
};

namespace DockWnd {

/** A dock window with the embedded property view.
 */
class PropertyDockView : public Gui::DockWindow
{
    Q_OBJECT

public:
    PropertyDockView(Gui::Document*  pcDocument, QWidget *parent=0);
    virtual ~PropertyDockView();
};

} // namespace DockWnd
} // namespace Gui

#endif // GUI_DOCKWND_PROPERTYVIEW_H
