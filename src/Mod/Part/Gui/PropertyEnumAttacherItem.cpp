/***************************************************************************
 *   Copyright (c) 2017 Peter Lama <peterldev94@gmail.com>                 *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# ifdef _MSC_VER
#  define _USE_MATH_DEFINES
#  include <cmath>
# endif //_MSC_VER
#endif // _PreComp_

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "PropertyEnumAttacherItem.h"

using namespace PartGui;

PROPERTYITEM_SOURCE(PartGui::PropertyEnumAttacherItem)

PropertyEnumAttacherItem::PropertyEnumAttacherItem() = default;

QWidget* PropertyEnumAttacherItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::LabelButton* modeEditor = new Gui::LabelButton(parent);
    QObject::connect(modeEditor, SIGNAL(valueChanged(const QVariant &)), receiver, method);
    QObject::connect(modeEditor, &Gui::LabelButton::buttonClicked, this, &PropertyEnumAttacherItem::openTask);
    modeEditor->setDisabled(isReadOnly());
    return modeEditor;
}

void PropertyEnumAttacherItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::LabelButton* modeEditor = qobject_cast<Gui::LabelButton*>(editor);
    modeEditor->setValue(data);
}

QVariant PropertyEnumAttacherItem::editorData(QWidget *editor) const
{
    Gui::LabelButton* modeEditor = qobject_cast<Gui::LabelButton*>(editor);
    return modeEditor->value();
}

void PropertyEnumAttacherItem::openTask()
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    TaskDlgAttacher* task;
    task = qobject_cast<TaskDlgAttacher*>(dlg);

    if (dlg && !task) {
        // there is already another task dialog which must be closed first
        Gui::Control().showDialog(dlg);
        return;
    }
    if (!task) {
        const App::Property* prop = getFirstProperty();
        if (prop) {
            App::PropertyContainer* parent = prop->getContainer();

            if (parent->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
                App::DocumentObject* obj = static_cast<App::DocumentObject*>(parent);
                Gui::ViewProvider* view = Gui::Application::Instance->getViewProvider(obj);

                if (view->getTypeId().isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId())) {
                    task = new TaskDlgAttacher(static_cast<Gui::ViewProviderDocumentObject*>(view));
                }
            }
        }
        if (!task) {
            return;
        }
    }

    Gui::Control().showDialog(task);
}

#include "moc_PropertyEnumAttacherItem.cpp"
