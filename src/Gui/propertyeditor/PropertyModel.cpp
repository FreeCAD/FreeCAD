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

#include <boost/algorithm/string/predicate.hpp>

#include "PropertyModel.h"
#include "PropertyItem.h"
#include "PropertyView.h"

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
            if (fabs(d-v) > DBL_EPSILON)
                return item->setData(value);
        }
        // Special case handling for quantities
        else if (data.canConvert<Base::Quantity>() && value.canConvert<Base::Quantity>()) {
            const Base::Quantity& val1 = data.value<Base::Quantity>();
            const Base::Quantity& val2 = value.value<Base::Quantity>();
            if (!(val1 == val2))
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

struct PropItemInfo {
    const std::string &name;
    const std::vector<App::Property*> &props;

    PropItemInfo(const std::string &n, const std::vector<App::Property*> &p)
        :name(n),props(p)
    {}
};

static void setPropertyItemName(PropertyItem *item, const char *propName, QString groupName) {
    QString name = QString::fromLatin1(propName);
    if(name.size()>groupName.size()+1 
            && name.startsWith(groupName + QLatin1Char('_')))
        name = name.right(name.size()-groupName.size()-1);

    item->setPropertyName(name);
}

void PropertyModel::buildUp(const PropertyModel::PropertyList& props)
{
    beginResetModel();

    // fill up the listview with the properties
    rootItem->reset();

    // sort the properties into their groups
    std::map<std::string, std::vector<PropItemInfo> > propGroup;
    PropertyModel::PropertyList::const_iterator jt;
    for (jt = props.begin(); jt != props.end(); ++jt) {
        App::Property* prop = jt->second.front();
        const char* group = prop->getGroup();
        bool isEmpty = (group == 0 || group[0] == '\0');
        std::string grp = isEmpty ? QT_TRANSLATE_NOOP("App::Property", "Base") : group;
        propGroup[grp].emplace_back(jt->first,jt->second);
    }

    for (auto kt = propGroup.begin(); kt != propGroup.end(); ++kt) {
        // set group item
        PropertyItem* group = static_cast<PropertyItem*>(PropertySeparatorItem::create());
        group->setParent(rootItem);
        rootItem->appendChild(group);
        QString groupName = QString::fromLatin1(kt->first.c_str());
        group->setPropertyName(groupName);

        // setup the items for the properties
        for (auto it = kt->second.begin(); it != kt->second.end(); ++it) {
            const auto &info = *it;
            App::Property* prop = info.props.front();
            std::string editor(prop->getEditorName());
            if(editor.empty() && PropertyView::showAll())
                editor = "Gui::PropertyEditor::PropertyItem";
            if (!editor.empty()) {
                PropertyItem* item = PropertyItemFactory::instance().createPropertyItem(editor.c_str());
                if (!item) {
                    qWarning("No property item for type %s found\n", editor.c_str());
                    continue;
                }
                else {
                    if(boost::ends_with(info.name,"*"))
                        item->setLinked(true);
                    PropertyItem* child = (PropertyItem*)item;
                    child->setParent(rootItem);
                    rootItem->appendChild(child);

                    setPropertyItemName(child,prop->getName(),groupName);

                    child->setPropertyData(info.props);
                }
            }
        }
    }

    endResetModel();
}

void PropertyModel::updateProperty(const App::Property& prop)
{
    int column = 1;
    int numChild = rootItem->childCount();
    for (int row=0; row<numChild; row++) {
        PropertyItem* child = rootItem->child(row);
        if (child->hasProperty(&prop)) {
            child->updateData();
            QModelIndex data = this->index(row, column, QModelIndex());
            if (data.isValid()) {
                child->assignProperty(&prop);
                dataChanged(data, data);
                updateChildren(child, column, data);
            }
            break;
        }
    }
}

void PropertyModel::appendProperty(const App::Property& prop)
{
    std::string editor(prop.getEditorName());
    if(editor.empty() && PropertyView::showAll())
        editor = "Gui::PropertyEditor::PropertyItem";
    if (!editor.empty()) {
        PropertyItem* item = PropertyItemFactory::instance().createPropertyItem(editor.c_str());
        if (!item) {
            qWarning("No property item for type %s found\n", editor.c_str());
            return;
        }

        const char* group = prop.getGroup();
        bool isEmpty = (group == 0 || group[0] == '\0');
        std::string grp = isEmpty ? QT_TRANSLATE_NOOP("App::Property", "Base") : group;
        QString groupName = QString::fromStdString(grp);

        // go through all group names and check if one matches
        int index = -1;
        for (int i=0; i<rootItem->childCount(); i++) {
            PropertyItem* child = rootItem->child(i);
            if (child->isSeparator()) {
                if (groupName == child->propertyName()) {
                    index = i+1;
                    break;
                }
            }
        }

        int numChilds = rootItem->childCount();
        int first = 0;
        int last = 0;
        if (index < 0) {
            // create a new group
            first = numChilds;
            last = first + 1;
        }
        else {
            // the group exists, determine the position before the next group
            // or at the end if there is no further group
            for (int i=index; i<rootItem->childCount(); i++) {
                index++;
                PropertyItem* child = rootItem->child(i);
                if (child->isSeparator()) {
                    index = i;
                    break;
                }
            }

            first = index;
            last = index;
        }

        // notify system to add new row
        beginInsertRows(QModelIndex(), first, last);

        // create a new group at the end with the property
        if (index < 0) {
            PropertyItem* group = static_cast<PropertyItem*>(PropertySeparatorItem::create());
            group->setParent(rootItem);
            rootItem->appendChild(group);
            group->setPropertyName(groupName);

            item->setParent(rootItem);
            rootItem->appendChild(item);
        }
        // add the property at the end of its group
        else if (index < numChilds) {
            item->setParent(rootItem);
            rootItem->insertChild(index, item);
        }
        // add the property at end
        else {
            item->setParent(rootItem);
            rootItem->appendChild(item);
        }

        std::vector<App::Property*> data;
        data.push_back(const_cast<App::Property*>(&prop));

        setPropertyItemName(item,prop.getName(),groupName);
        item->setPropertyData(data);

        endInsertRows();
    }
}

void PropertyModel::removeProperty(const App::Property& prop)
{
    int numChild = rootItem->childCount();
    for (int row=0; row<numChild; row++) {
        PropertyItem* child = rootItem->child(row);
        if (child->hasProperty(&prop)) {
            if (child->removeProperty(&prop)) {
                removeRow(row, QModelIndex());
            }
            break;
        }
    }
}

void PropertyModel::updateChildren(PropertyItem* item, int column, const QModelIndex& parent)
{
    int numChild = item->childCount();
    if (numChild > 0) {
        QModelIndex topLeft = this->index(0, column, parent);
        QModelIndex bottomRight = this->index(numChild, column, parent);
        dataChanged(topLeft, bottomRight);
#if 0 // It seems we don't have to inform grand children
        for (int row=0; row<numChild; row++) {
            PropertyItem* child = item->child(row);
            QModelIndex data = this->index(row, column, parent);
            if (data.isValid()) {
                updateChildren(child, column, data);
            }
        }
#endif
    }
}

bool PropertyModel::removeRows(int row, int count, const QModelIndex& parent)
{
    PropertyItem* item;
    if (!parent.isValid())
        item = rootItem;
    else
        item = static_cast<PropertyItem*>(parent.internalPointer());

    int start = row;
    int end = row+count-1;
    beginRemoveRows(parent, start, end);
    item->removeChildren(start, end);
    endRemoveRows();
    return true;
}

#include "moc_PropertyModel.cpp"
