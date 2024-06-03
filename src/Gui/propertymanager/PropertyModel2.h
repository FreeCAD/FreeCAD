/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PROPERTYMODEL2_H
#define PROPERTYMODEL2_H

#include <QAbstractItemModel>
#include <QStringList>
#include <map>
#include <unordered_map>
#include <vector>
#include "propertyeditor/PropertyItem.h"
#include "PropertyManagerItem.h"


namespace App {
class Property;
}

namespace Gui {
namespace PropertyEditor {


class PropertyModel2 : public QAbstractItemModel
{
    Q_OBJECT

public:
    using PropertyList = std::vector< std::pair< std::string, std::vector<App::Property*> > >;

    PropertyModel2(QObject* parent);
    ~PropertyModel2() override;

    QModelIndex buddy (const QModelIndex & index) const override;
    int columnCount (const QModelIndex & parent = QModelIndex()) const override;
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData (const QModelIndex & idx, const QVariant & value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent (const QModelIndex & index) const override;
    int rowCount (const QModelIndex & parent = QModelIndex()) const override;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData (int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole) override;
    void buildUp(QModelIndex& parentIndex, const PropertyList & props);

    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;

    //void updateProperty(const App::Property&);
    //void appendProperty(const App::Property&);
    //void removeProperty(const App::Property&);
    
    // QStringList propertyPathFromIndex(const QModelIndex&) const;
    // QModelIndex propertyIndexFromPath(const QStringList&) const;

private:
    void resetGroups(QModelIndex& parentIndex);
    void initGroups(QModelIndex& parentIndex);
    void updateChildren(PropertyItem* item, int column, const QModelIndex& parent);
    void findOrCreateChildren(QModelIndex& parentIndex, const PropertyList& props);
    void insertOrMoveChildren(QModelIndex& parentIndex);
    void removeChildren(QModelIndex& parentIndex);

    // struct GroupInfo {
    //     PropertySeparatorItem *groupItem = nullptr;
    //     std::vector<PropertyItem *> children;
    // };
    DocumentObjectItem::GroupInfo &getGroupInfo(QModelIndex& parentIndex, App::Property *);
    void getRange(const DocumentObjectItem::GroupInfo&, int& first, int& last) const;

protected:
    PropertyItem *rootItem;

private:

};

} //namespace PropertyEditor
} //namespace Gui


#endif //PROPERTYMODEL2_H
