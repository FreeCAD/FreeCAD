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

#ifndef PROPERTY_MANAGER_ITEM_H
#define PROPERTY_MANAGER_ITEM_H

#include "ViewProviderDocumentObject.h"
#include "propertyeditor/PropertyItem.h"

namespace Gui::PropertyEditor {

class GuiExport PropertyManagerItem : public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

public:
    ~PropertyManagerItem() override;
    virtual bool isSeparator() const override { return true; }

protected:
    PropertyManagerItem();
};


class GuiExport DocumentItem : public PropertyManagerItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

public:
    DocumentItem(App::Document* doc);
    ~DocumentItem() override = default;
    App::Document* getDocument();
    QVariant data (int column, int role = Qt::DisplayRole) const override;
    
private:
    App::Document* doc;
};

class GuiExport DocumentObjectItem : public PropertyManagerItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

public:
    DocumentObjectItem(App::DocumentObject* obj);
    ~DocumentObjectItem() override = default;
    App::DocumentObject* getObject();
    QVariant data (int column, int role = Qt::DisplayRole) const override;

    struct GroupInfo {
        PropertySeparatorItem *groupItem = nullptr;
        std::vector<PropertyItem *> children;
    };

    std::map<QString, GroupInfo> &getGroupItems();
    std::unordered_map<App::Property*, QPointer<PropertyItem>> &getItemMap();

private:
    App::DocumentObject* obj;

    std::unordered_map<App::Property*, QPointer<PropertyItem> > itemMap;

    std::map<QString, GroupInfo> groupItems;

    ViewProviderDocumentObject* getViewProvider() const;
};

    
} // namespace Gui::PropertyEditor

#endif // PROPERTY_MANAGER_ITEM_H
