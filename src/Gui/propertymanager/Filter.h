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

#ifndef FILTER_H
#define FILTER_H

#include "PreCompiled.h"

#ifndef _PreComp_
# include <boost/algorithm/string/predicate.hpp>
# include <QAbstractItemModel>
# include <QString>
# include <map>
# include <unordered_map>
# include <vector>
# include <unordered_set>
#endif

#include <Base/Persistence.h>
#include <App/Document.h>

namespace App {
class Property;
}

namespace Gui::PropertyEditor {

class DocumentItem;
class DocumentObjectItem;

template<typename T>
class UniqueVector {
public:

    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    template<typename... Args>
    UniqueVector(Args... args) {
        addAll(args...);
    }

    template<typename... Args>
    void addAll(T first, Args... rest) {
        push_back(first);
        addAll(rest...);
    }    

    void addAll() {}
    
    iterator begin() {
        return vector.begin();
    }

    const_iterator begin() const {
        return vector.begin();
    }

    iterator end() {
        return vector.end();
    }

    const_iterator end() const {
        return vector.end();
    }

    void push_back(const T& value) {
        if (set.insert(value).second) { // Insertion was successful, i.e., value was unique
            vector.push_back(value);     // Add value to the vector
        }
    }

    const T& operator[](size_t index) const {
        return vector[index];
    }

    size_t size() const {
        return vector.size();
    }

private:
    std::vector<T> vector;  // Container for maintaining insertion order
    std::unordered_set<T> set;        // Container for ensuring uniqueness
};

    
class Filter
{
public:
    static const QString STAR;
    
    static const QString ACTIVE_DOC;// = "\"Active document\"";
    static const QString BASE_GROUP;// = "Base";
    //static const QString ACTIVE_OBJ;// = "\"Active object\"";

    Filter();
    // Filter(const Filter&) = default;
    // Filter(Filter&&) = delete;
    // Filter& operator=(const Filter&) = default;
    // Filter& operator=(Filter&&) = delete;
    ~Filter() = default;

    static QString getType(Base::Persistence* obj);

    UniqueVector<QString> getFilterValuesDocument();
    UniqueVector<QString> getFilterValuesTypeObject();
    UniqueVector<QString> getFilterValuesNameObject();
    UniqueVector<QString> getFilterValuesGroup();
    UniqueVector<QString> getFilterValuesTypeProperty();
    UniqueVector<QString> getFilterValuesNameProperty();

    void filter(QString &selectedDocument,
                QString &selectedTypeObject, QString &selectedNameObject,
                QString &selectedGroup,
                QString &selectedTypeProperty,
                QString &selecteNameProperty);
    
    std::vector<App::Document*> getDocumentsFiltered();
    std::vector<App::DocumentObject*> getObjectsFiltered(App::Document* doc);
    std::vector<App::Property*> getPropertiesFiltered(App::DocumentObject* obj);

    QString getName();
    void setName(QString& name);
    
private:
    QString name;
    
    QString filterDocument;
    QString filterTypeObject;
    QString filterNameObject;
    QString filterGroup;
    QString filterTypeProp;
    QString filterNameProp;
};

} // namespace Gui::PropertyEditor

#endif
