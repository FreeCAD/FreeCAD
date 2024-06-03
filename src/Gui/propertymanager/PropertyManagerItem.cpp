/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
# include <QApplication>
# include <QComboBox>
# include <QFontDatabase>
# include <QLocale>
# include <QMessageBox>
# include <QPalette>
# include <QPixmap>
# include <QTextStream>
# include <QTimer>
# include <QtGlobal>
# include <QMenu>
#endif

#include <Gui/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include "PropertyManagerItem.h"
#include "PropertyManager.h"


using namespace Gui::PropertyEditor;
using namespace Gui::Dialog;

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyManagerItem)

PropertyManagerItem::PropertyManagerItem() : PropertyItem()
{
    setExpanded(true);
}

PropertyManagerItem::~PropertyManagerItem() = default;


DocumentItem::DocumentItem(App::Document* doc) :
    doc(doc)
{
}

QVariant DocumentItem::data (int column, int role) const {
    if (column == 0) {
        if (role == Qt::DecorationRole) {
            if (DlgPropertyManager::documentPixmap.get()) {
                return QVariant::fromValue(QIcon(doc->testStatus(App::Document::PartialDoc) ?
                                                 *DlgPropertyManager::documentPartialPixmap :
                                                 *DlgPropertyManager::documentPixmap));
            }
        }
    }
    return PropertyItem::data(column, role);
}



App::Document* DocumentItem::getDocument()
{
    return doc;
}



DocumentObjectItem::DocumentObjectItem(App::DocumentObject* obj) :
    obj(obj)
{
}

App::DocumentObject* DocumentObjectItem::getObject()
{
    return obj;
}

std::map<QString, DocumentObjectItem::GroupInfo> &DocumentObjectItem::getGroupItems()
{
    return groupItems;
}

std::unordered_map<App::Property*,QPointer<PropertyItem>> &DocumentObjectItem::getItemMap()
{
    return itemMap;
}

Gui::ViewProviderDocumentObject* DocumentObjectItem::getViewProvider() const
{
    return Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Gui::Application::Instance->getViewProvider(obj));
}

QVariant DocumentObjectItem::data (int column, int role) const
{
    if (column == 0) {
        if (role == Qt::DecorationRole) {
            ViewProviderDocumentObject* vp = getViewProvider();
            if (vp) {
                return QVariant::fromValue(vp->getIcon());
            }
        }
    }
    return PropertyItem::data(column, role);
}

#include "moc_PropertyManagerItem.cpp"
