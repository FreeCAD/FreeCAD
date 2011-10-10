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


#ifndef PROPERTYMODEL_H
#define PROPERTYMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <vector>
#include <map>

namespace App {
class Property;
}
namespace Gui {
namespace PropertyEditor {

class PropertyItem;
class PropertyModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    PropertyModel(QObject* parent);
    virtual ~PropertyModel();

    QModelIndex buddy (const QModelIndex & index) const;
    int columnCount (const QModelIndex & parent = QModelIndex()) const;
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData (const QModelIndex & idx, const QVariant & value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex parent (const QModelIndex & index) const;
    int rowCount (const QModelIndex & parent = QModelIndex()) const;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setHeaderData (int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole);
    void buildUp(const std::map<std::string, std::vector<App::Property*> >& props);
    QStringList propertyPathFromIndex(const QModelIndex&) const;
    QModelIndex propertyIndexFromPath(const QStringList&) const;

private:
    PropertyItem *rootItem;
};

} //namespace PropertyEditor
} //namespace Gui


#endif //PROPERTYMODEL_H
