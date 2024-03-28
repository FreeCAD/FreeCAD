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

#ifndef PROPERTY_MANAGER_MODEL_H
#define PROPERTY_MANAGER_MODEL_H

#include <QAbstractItemModel>
#include <QString>
#include <map>
#include <unordered_map>
#include <vector>
#include "PropertyModel2.h"
#include "Filter.h"


namespace App {
class Property;
}

namespace Gui::PropertyEditor {

class DocumentItem;
class DocumentObjectItem;

class PropertyManagerModel : public PropertyModel2
{
    Q_OBJECT

public:
    using PropertyList = std::vector< std::pair< std::string, std::vector<App::Property*> > >;


    explicit PropertyManagerModel(QObject* parent);
    ~PropertyManagerModel() override;

    void addProperties(QModelIndex & parentIndex, int parentRow);
    void addDocumentObjects(QModelIndex & parentInex, int parentRow);
    void init();

    UniqueVector<QString> getFilterValuesDocument();
    UniqueVector<QString> getFilterValuesTypeObject();
    UniqueVector<QString> getFilterValuesNameObject();
    UniqueVector<QString> getFilterValuesGroup();
    UniqueVector<QString> getFilterValuesTypeProperty();
    UniqueVector<QString> getFilterValuesNameProperty();

    UniqueVector<QString> getGroupNames();

    void filterSelected(QString &selectedDocument,
                        QString &selectedTypeObject, QString &selectedNameObject,
                        QString &selectedGroup,
                        QString &selectedTypeProperty, QString &selectedNameProperty);

    void refresh();
    App::DocumentObject* getUniqueObject();
    QString getUniqueTypeProperty();
    QString getUniqueGroup();

    // scopes

    bool scopeExists(QString& name);
    void selectScope(QString& name);
    void insertScope(QString& name);
    UniqueVector<QString> getNamesScope();

private:

    QList<QModelIndex> childrenAtLevel(int level);
    void childrenAtLevel(QList<QModelIndex>& children, QModelIndex& parent, int level);

    void removeItemsWithNoChildren();
    void removeItemsWithNoChildren(QModelIndex& parent, int level, int upToLevel);
        
    void getGroupNames(UniqueVector<QString> &groupNames);

    
    template<class T>
    void getFilterValues(
            UniqueVector<QString>& filterValues,
            int level,
            std::function<void(T*)> funcAct,
            std::function<QString(T*)> funcProduce);


private:
    static std::unordered_map<QString, PropertyEditor::Filter> scopes;
    Filter filter;
};

} // namespace Gui


#endif // PROPERTY_MANAGER_MODEL_H
