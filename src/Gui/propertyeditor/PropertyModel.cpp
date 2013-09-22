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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <cfloat>
#endif

#include "PropertyModel.h"
#include "PropertyItem.h"

using namespace Gui::PropertyEditor;


/* TRANSLATOR Gui::PropertyEditor::PropertyModel */

PropertyModel::PropertyModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    rootItem = static_cast<PropertyItem*>(PropertyItem::create());
}

PropertyModel::~PropertyModel()
{
    delete rootItem;
}

QModelIndex PropertyModel::buddy ( const QModelIndex & index ) const
{
    if (index.column() == 1)
        return index;
    return index.sibling(index.row(), 1);
}

int PropertyModel::columnCount ( const QModelIndex & parent ) const
{
    // <property, value>, hence always 2
    if (parent.isValid())
        return static_cast<PropertyItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant PropertyModel::data ( const QModelIndex & index, int role ) const
{
    if (!index.isValid())
        return QVariant();

    PropertyItem *item = static_cast<PropertyItem*>(index.internalPointer());
    return item->data(index.column(), role);
}

bool PropertyModel::setData(const QModelIndex& index, const QVariant & value, int role)
{
    if (!index.isValid())
        return false;

    // we check whether the data has really changed, otherwise we ignore it
    if (role == Qt::EditRole) {
        PropertyItem *item = static_cast<PropertyItem*>(index.internalPointer());
        QVariant data = item->data(index.column(), role);
        if (data.type() == QVariant::Double && value.type() == QVariant::Double) {
            // since we store some properties as floats we get some round-off
            // errors here. Thus, we use an epsilon here.
            // NOTE: Since 0.14 PropertyFloat uses double precision, so this is maybe unnecessary now?
            double d = data.toDouble();
            double v = value.toDouble();
            if (fabs(d-v) > FLT_EPSILON)
                return item->setData(value);
        }
        else if (data != value)
            return item->setData(value);
    }

    return true;
}

Qt::ItemFlags PropertyModel::flags(const QModelIndex &index) const
{
    PropertyItem *item = static_cast<PropertyItem*>(index.internalPointer());
    return item->flags(index.column());
}

QModelIndex PropertyModel::index ( int row, int column, const QModelIndex & parent ) const
{
    PropertyItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<PropertyItem*>(parent.internalPointer());

    PropertyItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex PropertyModel::parent ( const QModelIndex & index ) const
{
    if (!index.isValid())
        return QModelIndex();

    PropertyItem *childItem = static_cast<PropertyItem*>(index.internalPointer());
    PropertyItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int PropertyModel::rowCount ( const QModelIndex & parent ) const
{
    PropertyItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<PropertyItem*>(parent.internalPointer());

    return parentItem->childCount();
}

QVariant PropertyModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role != Qt::DisplayRole)
            return QVariant();
        if (section == 0)
            return tr("Property");
        if (section == 1)
            return tr("Value");
    }

    return QVariant();
}

bool PropertyModel::setHeaderData (int, Qt::Orientation, const QVariant &, int)
{
    return false;
}

QStringList PropertyModel::propertyPathFromIndex(const QModelIndex& index) const
{
    QStringList path;
    if (index.isValid()) {
        PropertyItem* item = static_cast<PropertyItem*>(index.internalPointer());
        if (!item->isSeparator()) {
            do {
                path.push_front(item->propertyName());
                item = item->parent();
            }
            while (item != this->rootItem && item != 0);
        }
    }

    return path;
}

QModelIndex PropertyModel::propertyIndexFromPath(const QStringList& path) const
{
    QModelIndex parent;
    for (QStringList::const_iterator it = path.begin(); it != path.end(); ++it) {
        int rows = this->rowCount(parent);
        for (int i=0; i<rows; i++) {
            QModelIndex index = this->index(i, 0, parent);
            if (index.isValid()) {
                PropertyItem* item = static_cast<PropertyItem*>(index.internalPointer());
                if (item->propertyName() == *it) {
                    parent = index;
                    break;
                }
            }
        }
    }
    return parent;
}

void PropertyModel::buildUp(const std::map<std::string, std::vector<App::Property*> >& props)
{
    // fill up the listview with the properties
    rootItem->reset();

    // sort the properties into their groups
    std::map<std::string, std::vector<std::vector<App::Property*> > > propGroup;
    std::map<std::string, std::vector<App::Property*> >
        ::const_iterator jt;
    for (jt = props.begin(); jt != props.end(); ++jt) {
        App::Property* prop = jt->second.front();
        const char* group = prop->getGroup();
        std::string grp = group ? group : "Base";
        propGroup[grp].push_back(jt->second);
    }

    std::map<std::string, std::vector<std::vector<App::Property*> > >
        ::const_iterator kt;
    for (kt = propGroup.begin(); kt != propGroup.end(); ++kt) {
        // set group item
        PropertyItem* group = static_cast<PropertyItem*>(PropertySeparatorItem::create());
        group->setParent(rootItem);
        rootItem->appendChild(group);
        group->setPropertyName(QString::fromAscii(kt->first.c_str()));

        // setup the items for the properties
        std::vector<std::vector<App::Property*> >::const_iterator it;
        for (it = kt->second.begin(); it != kt->second.end(); ++it) {
            App::Property* prop = it->front();
            QString editor = QString::fromAscii(prop->getEditorName());
            if (!editor.isEmpty()) {
                Base::BaseClass* item = 0;
                try {
                    item = static_cast<Base::BaseClass*>(Base::Type::
                        createInstanceByName(prop->getEditorName(),true));
                }
                catch (...) {
                }
                if (!item) {
                    qWarning("No property item for type %s found\n", prop->getEditorName());
                    continue;
                }
                if (item->getTypeId().isDerivedFrom(PropertyItem::getClassTypeId())) {
                    PropertyItem* child = (PropertyItem*)item;
                    child->setParent(rootItem);
                    rootItem->appendChild(child);
                    child->setPropertyName(QString::fromAscii(prop->getName()));
                    child->setPropertyData(*it);
                }
            }
        }
    }

    reset();
}

#include "moc_PropertyModel.cpp"
