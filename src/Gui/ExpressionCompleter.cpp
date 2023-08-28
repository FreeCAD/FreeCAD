/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
# include <boost/algorithm/string/predicate.hpp>
# include <QAbstractItemView>
# include <QContextMenuEvent>
# include <QLineEdit>
# include <QMenu>
# include <QTextBlock>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/ObjectIdentifier.h>
#include <Base/Tools.h>
#include <CXX/Extensions.hxx>

#include "ExpressionCompleter.h"


FC_LOG_LEVEL_INIT("Completer", true, true)

Q_DECLARE_METATYPE(App::ObjectIdentifier)

using namespace App;
using namespace Gui;

class ExpressionCompleterModel: public QAbstractItemModel
{
public:
    ExpressionCompleterModel(QObject* parent, bool noProperty)
        :QAbstractItemModel(parent), noProperty(noProperty)
    {
    }

    void setNoProperty(bool enabled) {
        noProperty = enabled;
    }

    void setDocumentObject(const App::DocumentObject* obj, bool checkInList)
    {
        beginResetModel();
        if (obj) {
            currentDoc = obj->getDocument()->getName();
            currentObj = obj->getNameInDocument();
            if (!noProperty && checkInList) {
                inList = obj->getInListEx(true);
            }
        }
        else {
            currentDoc.clear();
            currentObj.clear();
            inList.clear();
        }
        endResetModel();
    }

    // This ExpressionCompleter model works without any physical items.
    // Everything item related is stored inside QModelIndex.InternalPointer/InternalId(),
    // using the following Info structure.
    //
    // The Info contains two indices, one for document and the other for object.
    // For 32-bit system, the index is 16bit which limits the size to 64K. For
    // 64-bit system, the index is 32bit.
    //
    // The "virtual" items are organized as a tree. The root items are special,
    // which consists of three types in the following order,
    //
    // * Document, even index contains item using document's name, while
    //   odd index with quoted document label.
    // * Objects of the current document, even index with object's internal
    //   name, and odd index with quoted object label.
    // * Properties of the current object.
    //
    // Document item contains object item as child, and object item contains
    // property item.
    //
    // The QModelIndex of a root item has both the doc field and obj field set
    // to -1, and uses the row as the item index. We can figure out the type of
    // the item solely based on this row index.
    //
    // QModelIndex of a non-root object item has doc field as the document
    // index, and obj field set to -1.
    //
    // QModelIndex of a non-root property item has doc field as the document
    // index, and obj field as the object index.
    //
    // An item is uniquely identified by the pair (row, father_link) in the QModelIndex
    //
    // The completion tree structure created takes into account the current document and object
    //
    // It is done as such, in order to have contextual completion (prop -> object -> files):
    // * root (-1,-1)
    // |
    // |----- documents
    // |----- current documents' objects [externally set]
    // |----- current objects' props  [externally set]
    //
    // This complicates the decoding schema for the root, where the childcount will be
    // doc.size() + current_doc.Objects.size() + current_obj.Props.size().
    //
    // This is reflected in the complexity of the DATA function.
    //
    // Example encoding of a QMODEL Index
    //
    //    ROOT (row -1, [-1,-1,-1,0]), info represented as [-1,-1,-1,0]
    //    |-- doc 1 (non contextual)                 - (row 0, [-1,-1,-1,0]) = encode as parent => [0,-1,-1,0]
    //    |-- doc 2 (non contextual)                 - (row 1, [-1,-1,-1,0]) = encode as parent => [1,-1,-1,0]
    //    |   |- doc 2.obj1                          - (row 0, [1,-1,-1,0])  = encode as parent => [1, 0,-1,0]
    //    |   |- doc 2.obj2                          - (row 1, [1,-1,-1,0])  = encode as parent => [1, 1,-1,0]
    //    |   |- doc 2.obj3                          - (row 2, [1,-1,-1,0])  = encode as parent => [1, 2,-1,0]
    //    |      |- doc 2.obj3.prop1                 - (row 0, [1, 2,-1,0])  = encode as parent => [1, 2, 0,0]
    //    |      |- doc 2.obj3.prop2                 - (row 1, [1, 2,-1,0])  = encode as parent => [1, 2, 1,0]
    //    |      |- doc 2.obj3.prop3                 - (row 2, [1, 2,-1,0])  = encode as parent => [1, 2, 2,0]
    //    |         |- doc 2.obj3.prop3.path0        - (row 0, [1, 2, 2,0])  = encode as parent => INVALID, LEAF ITEM
    //    |         |- doc 2.obj3.prop3.path1        - (row 1, [1, 2, 2,0])  = encode as parent => INVALID, LEAF ITEM
    //    |
    //    |
    //    |-- doc 3 (non contextual)                 - (row 2, [-1,-1,-1,0]) = encode as parent => [2,-1,-1,0]
    //    |
    //    |-- obj1 (current doc - contextual)        - (row 3, [-1,-1,-1,0]) = encode as parent => [3,-1,-1,1]
    //    |-- obj2 (current doc - contextual)        - (row 4, [-1,-1,-1,0]) = encode as parent => [4,-1,-1,1]
    //    |   |- obj2.prop1 (contextual)             - (row 0, [4,-1,-1,1])  = encode as parent => [4,-1,0,1]
    //    |   |- obj2.prop2 (contextual)             - (row 1, [4,-1,-1,1])  = encode as parent => [4,-1,1,1]
    //    |      | - obj2.prop2.path1 (contextual)   - (row 0, [4,-1,0 ,1])  = encode as parent => INVALID, LEAF ITEM
    //    |      | - obj2.prop2.path2 (contextual)   - (row 1, [4,-1,1 ,1])  = encode as parent => INVALID, LEAF ITEM
    //    |
    //    |-- prop1 (current obj - contextual)       - (row 5, [-1,-1,-1,0]) = encode as parent => [5,-1,-1,1]
    //    |-- prop2 (current obj - contextual)       - (row 6, [-1,-1,-1,0]) = encode as parent => [6,-1,-1,1]
    //        |-- prop2.path1 (contextual)           - (row 0, [ 6,-1,-1,0]) = encode as parent => INVALID, LEAF ITEM
    //        |-- prop2.path2 (contextual)           - (row 1, [ 6,-1,-1,0]) = encode as parent => INVALID, LEAF ITEM
    //

    struct Info
    {
        qint32 doc;
        qint32 obj;
        qint32 prop;
        quint32 contextualHierarchy : 1;

        static const Info root;
    };

    static const quint64 k_numBitsProp                  = 16ULL;    // 0 .. 15
    static const quint64 k_numBitsObj                   = 24ULL;    // 16.. 39
    static const quint64 k_numBitsContextualHierarchy   = 1;        // 40
    static const quint64 k_numBitsDocuments             = 23ULL;    // 41.. 63

    static const quint64 k_offsetProp                   = 0;
    static const quint64 k_offsetObj                    = k_offsetProp + k_numBitsProp;
    static const quint64 k_offsetContextualHierarchy    = k_offsetObj + k_numBitsObj;
    static const quint64 k_offsetDocuments              = k_offsetContextualHierarchy + k_numBitsContextualHierarchy;

    static const quint64 k_maskProp                     = ((1ULL << k_numBitsProp) - 1);
    static const quint64 k_maskObj                      = ((1ULL << k_numBitsObj) - 1);
    static const quint64 k_maskContextualHierarchy      = ((1ULL << k_numBitsContextualHierarchy) - 1);
    static const quint64 k_maskDocuments                = ((1ULL << k_numBitsDocuments) - 1);

    union InfoPtrEncoding
    {
        quint64 d_enc;
        struct
        {
            quint8 doc;
            quint8 prop;
            quint16 obj : 15;
            quint16 contextualHierarchy : 1;
        } d32;
        void* ptr;

        InfoPtrEncoding(const Info& info)
            : d_enc(0)
        {
            if (sizeof(void*) < sizeof(InfoPtrEncoding)) {
                d32.doc = (quint8)(info.doc + 1);
                d32.obj = (quint16)(info.obj + 1);
                d32.prop = (quint8)(info.prop + 1);
                d32.contextualHierarchy = info.contextualHierarchy;
            }
            else {
                d_enc = ((quint64(info.doc + 1) & k_maskDocuments) << k_offsetDocuments)
                    | ((quint64(info.contextualHierarchy) & k_maskContextualHierarchy)
                       << k_offsetContextualHierarchy)
                    | ((quint64(info.obj + 1) & k_maskObj) << k_offsetObj)
                    | ((quint64(info.prop + 1) & k_maskProp) << k_offsetProp);
            }
        }
        InfoPtrEncoding(void* pointer)
            : d_enc(0)
        {
            this->ptr = pointer;
        }

        Info DecodeInfo()
        {
            Info info;
            if (sizeof(void*) < sizeof(InfoPtrEncoding)) {
                info.doc = qint32(d32.doc) - 1;
                info.obj = qint32(d32.obj) - 1;
                info.prop = qint32(d32.prop) - 1;
                info.contextualHierarchy = d32.contextualHierarchy;
            }
            else {
                info.doc = ((d_enc >> k_offsetDocuments) & k_maskDocuments) - 1;
                info.contextualHierarchy =
                    ((d_enc >> k_offsetContextualHierarchy) & k_maskContextualHierarchy);
                info.obj = ((d_enc >> k_offsetObj) & k_maskObj) - 1;
                info.prop = ((d_enc >> k_offsetProp) & k_maskProp) - 1;
            }
            return info;
        }
    };

    static void* infoId(const Info& info)
    {
        InfoPtrEncoding ptrEnc(info);
        return ptrEnc.ptr;
    }

    static Info getInfo(const QModelIndex& index)
    {
        InfoPtrEncoding enc(index.internalPointer());
        return enc.DecodeInfo();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::EditRole && role != Qt::DisplayRole && role != Qt::UserRole)
            return {};
        QVariant variant;
        Info info = getInfo(index);
        _data(info, index.row(), &variant, nullptr, role == Qt::UserRole);
        FC_TRACE(info.doc << "," << info.obj << "," << info.prop << "," << info.contextualHierarchy
                          << "," << index.row() << ": " << variant.toString().toUtf8().constData());
        return variant;
    }

    static std::vector<App::ObjectIdentifier> retrieveSubPaths(const App::Property* prop)
    {
        std::vector<App::ObjectIdentifier> result;
        if (prop) {
            prop->getPaths(result);
            // need to filter out irrelevant paths (len 1, aka just this object identifier)
            auto res = std::remove_if(
                result.begin(), result.end(), [](const App::ObjectIdentifier& path) -> bool {
                    return path.getComponents().empty();
                });
            result.erase(res, result.end());
        }
        return result;
    }

    // The completion tree structure created takes into account the current document and object
    //
    // It is done as such:
    // * root (-1,-1)
    // |
    // |----- documents
    // |----- current documents' objects [externally set]
    // |----- current objects' props  [externally set]
    //
    // This complicates the decoding schema for the root, where the childcount will be
    // doc.size() + current_doc.Objects.size() + current_obj.Props.size().
    //
    // this function is called in two modes:
    // - obtain the count of a node identified by Info,row => count != nullptr, v==nullptr
    // - get the text of an item. This text will contain separators but NO full path
    void _data(const Info& info, int row, QVariant* v, int* count, bool sep = false) const
    {
        int idx;
        // identify the document index. For any children of the root, it is given by traversing
        // the flat list and identified by [row]
        idx = info.doc < 0 ? row : info.doc;
        const auto &docs = App::GetApplication().getDocuments();
        int docSize = (int)docs.size() * 2;
        int objSize = 0;
        int propSize = 0;
        std::vector<std::pair<const char*, App::Property*> > props;
        App::Document* doc = nullptr;
        App::DocumentObject* obj = nullptr;
        const char* propName = nullptr;
        App::Property* prop = nullptr;
        // check if the document is uniquely identified: either the correct index in info.doc
        // OR if, the node is a descendant of the root, its row lands within 0...docsize
        if (idx >= 0 && idx < docSize) {
            doc = docs[idx / 2];
        }
        else {
            // if we're looking at the ROOT, or the row identifies one of the other ROOT elements
            // |----- current documents' objects, rows: docs.size             ... docs.size + objs.size
            // |----- current objects' props,     rows: docs.size + objs.size ... docs.size + objs.size+  props.size
            //
            // We need to process the ROOT so we get the correct count for its children
            doc = App::GetApplication().getDocument(currentDoc.c_str());
            if (!doc)// no current, there are no additional objects
                return;

            // move to the current documents' objects' range
            idx -= docSize;
            if (info.doc < 0)
                row = idx;

            const auto &objs = doc->getObjects();
            objSize = (int)objs.size() * 2;
            // if this is a valid object, we found our object and break.
            // if not, this may be the root or one of current object's properties
            if (idx >= 0 && idx < objSize) {
                obj = objs[idx / 2];
                // if they are in the ignore list skip
                if (inList.count(obj))
                    return;
            }
            else if (!noProperty) {
                // need to check the current object's props range, or we're parsing the ROOT
                auto cobj = doc->getObject(currentObj.c_str());
                if (cobj) {
                    // move to the props range of the current object
                    idx -= objSize;
                    if (info.doc < 0)
                        row = idx;
                    // get the properties
                    cobj->getPropertyNamedList(props);
                    propSize = (int)props.size();

                    // if this is an invalid index, bail out
                    // if it's the ROOT break!
                    if (idx >= propSize)
                        return;
                    if (idx >= 0) {
                        obj = cobj; // we only set the active object if we're not processing the root.
                        propName = props[idx].first;
                        prop = props[idx].second;
                    }
                }
            }
        }
        // the item is the ROOT or a CHILD of the root
        if (info.doc < 0) {
            // and we're asking for a count, compute it
            if (count) {
                // note that if we're dealing with a valid DOC node (row>0, ROOT_info)
                // objSize and propSize will be zero because of the early exit above
                *count = docSize + objSize + propSize;
            }
            if (idx >= 0 && v) {
                // we're asking for this child's data, and IT's NOT THE ROOT
                QString res;
                // we resolved the property
                if (propName) {
                    res = QString::fromLatin1(propName);
                    // resolve the property
                    if (sep && !noProperty && !retrieveSubPaths(prop).empty())
                        res += QLatin1Char('.');
                }
                else if (obj) {
                    // the object has been resolved, use the saved idx to figure out quotation or not.
                    if (idx & 1)
                        res = QString::fromUtf8(quote(obj->Label.getStrValue()).c_str());
                    else
                        res = QString::fromLatin1(obj->getNameInDocument());
                    if (sep && !noProperty)
                        res += QLatin1Char('.');
                }
                else {
                    // the document has been resolved, use the saved idx to figure out quotation or not.
                    if (idx & 1)
                        res = QString::fromUtf8(quote(doc->Label.getStrValue()).c_str());
                    else
                        res = QString::fromLatin1(doc->getName());
                    if (sep)
                        res += QLatin1Char('#');
                }
                v->setValue(res);
            }
            // done processing the ROOT or any child items
            return;
        }

        // object not resolved
        if (!obj) {
            // are we pointing to an object item, or our father (info) is an object
            idx = info.obj < 0 ? row : info.obj;
            const auto &objs = doc->getObjects();
            objSize = (int)objs.size() * 2;
            // if invalid index, or in the ignore list bail out
            if (idx < 0 || idx >= objSize || inList.count(obj))
                return;
            obj = objs[idx / 2];

            if (info.obj < 0) {
                // if this is AN actual Object item and not a root
                if (count)
                    *count = objSize; // set the correct count if requested
                if (v) {
                    // resolve the name
                    QString res;
                    if (idx & 1)
                        res = QString::fromUtf8(quote(obj->Label.getStrValue()).c_str());
                    else
                        res = QString::fromLatin1(obj->getNameInDocument());
                    if (sep && !noProperty)
                        res += QLatin1Char('.');
                    v->setValue(res);
                }
                return;
            }
        }

        if (noProperty)
            return;
        if (!propName) {
            idx = info.prop < 0 ? row : info.prop;
            obj->getPropertyNamedList(props);
            propSize = (int)props.size();
            // return if the property is invalid
            if (idx < 0 || idx >= propSize) {
                return;
            }
            propName = props[idx].first;
            prop = props[idx].second;
            // if this is a root object item
            if (info.prop < 0) {
                // set the property size count
                if (count) {
                    *count = propSize;
                }
                if (v) {
                    QString res = QString::fromLatin1(propName);

                    // check to see if we have accessible paths from this prop name?
                    if (sep && !retrieveSubPaths(prop).empty())
                        res += QLatin1Char('.');
                    *v = res;
                }
                return;
            }
        }

        // resolve paths
        if (prop) {
            // idx identifies the path
            idx = row;
            std::vector<App::ObjectIdentifier> paths = retrieveSubPaths(prop);

            if (count) {
                *count = paths.size();
            }

            // check to see if this is a valid path
            if (idx < 0 || idx >= static_cast<int>(paths.size())) {
                return;
            }

            if (v) {
                auto str = paths[idx].getSubPathStr();
                if (str.size() && (str[0] == '.' || str[0] == '#')) {
                    // skip the "."
                    *v = QString::fromLatin1(str.c_str() + 1);
                }
                else {
                    *v = QString::fromLatin1(str.c_str());
                }
            }
        }
        return;
    }

    QModelIndex parent(const QModelIndex & index) const override {
        if (!index.isValid())
            return {};

        Info parentInfo = getInfo(index);
        Info grandParentInfo = parentInfo;

        if (parentInfo.contextualHierarchy) {
            // for contextual hierarchy we have this:
            // ROOT -> Object in Current Doc -> Prop In Object -> PropPath
            // ROOT -> prop in Current Object -> prop Path

            if (parentInfo.prop >= 0) {
                grandParentInfo.prop = -1;
                return createIndex(parentInfo.prop, 0, infoId(grandParentInfo));
            }
            // if the parent is the object or a prop attached to the root, we just need the below line
            return createIndex(parentInfo.doc, 0, infoId(Info::root));
        }
        else {
            if (parentInfo.prop >= 0) {
                grandParentInfo.prop = -1;
                return createIndex(parentInfo.prop, 0, infoId(grandParentInfo));
            }
            if (parentInfo.obj >= 0) {
                grandParentInfo.obj = -1;
                return createIndex(parentInfo.obj, 0, infoId(grandParentInfo));
            }
            if (parentInfo.doc >= 0) {
                grandParentInfo.doc = -1;
                return createIndex(parentInfo.doc, 0, infoId(grandParentInfo));
            }
        }


        return {};
    }

    // returns true if successful, false if 'element' identifies a Leaf
    bool modelIndexToParentInfo(QModelIndex element, Info & info) const
    {
        Info parentInfo;
        info = Info::root;

        if (element.isValid()) {
            parentInfo = getInfo(element);
            info = parentInfo;

            // Our wonderful element is a child of the root
            if (parentInfo.doc < 0) {
                // need special casing to properly identify this model's object
                const auto& docs = App::GetApplication().getDocuments();
                auto docsSize = static_cast<int>(docs.size() * 2);

                info.doc = element.row();

                // if my element is a contextual descendant of root (current doc object list, current object prop list)
                // mark it as such
                if (element.row() >= docsSize) {
                    info.contextualHierarchy = 1;
                }
            }
            else if (parentInfo.contextualHierarchy) {
                const auto& docs = App::GetApplication().getDocuments();
                auto cdoc = App::GetApplication().getDocument(currentDoc.c_str());

                if (cdoc) {
                    int objsSize = static_cast<int>(cdoc->getObjects().size() * 2);
                    int idx = parentInfo.doc - static_cast<int>(docs.size());
                    if (idx < objsSize) {
                        //  |-- Parent (OBJECT)   - (row 4, [-1,-1,-1,0]) = encode as element => [parent.row,-1,-1,1]
                        //      |- element (PROP) - (row 0, [parent.row,-1,-1,1]) = encode as element => [parent.row,-1,parent.row,1]

                        info.doc = parentInfo.doc;
                        info.obj = -1;// object information is determined by the DOC index actually
                        info.prop = element.row();
                        info.contextualHierarchy = 1;
                    }
                    else {
                        // if my parent (parentInfo) is a prop, it means that our element is a prop path
                        // and that is a leaf item (we don't split prop paths further)
                        // we can't encode leaf items into an "Info"
                        return false;
                    }
                }
                else {
                    // no contextual document
                    return false;
                }

            }
            // regular hierarchy
            else if (parentInfo.obj <= 0) {
                info.obj = element.row();
            }
            else if (parentInfo.prop <= 0) {
                info.prop = element.row();
            }
            else {
                return false;
            }
        }
        return true;
    }

    QModelIndex index(int row, int column,
        const QModelIndex & parent = QModelIndex()) const override {
        if (row < 0)
            return {};
        Info myParentInfoEncoded = Info::root;

        // encode the parent's QModelIndex into an 'Info' structure
        bool parentCanHaveChildren = modelIndexToParentInfo(parent, myParentInfoEncoded);
        if (!parentCanHaveChildren) {
            return {};
        }
        return createIndex(row, column, infoId(myParentInfoEncoded));
    }

    // function returns how many children the QModelIndex parent has
    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Info info;
        int row = 0;
        if (!parent.isValid()) {
            // we're getting the row count for the root
            // that is: document hierarchy _and_ contextual completion
            info = Info::root;
            row = -1;
        }
        else {
            // try to encode the parent's QModelIndex into an info structure
            // if the paren't can't have any children, return 0
            if (!modelIndexToParentInfo(parent, info)) {
                return 0;
            }
        }
        int count = 0;
        _data(info, row, nullptr, &count);
        FC_TRACE(info.doc << "," << info.obj << "," << info.prop << "," << info.contextualHierarchy
                          << "," << row << " row count " << count);
        return count;
    }

    int columnCount(const QModelIndex&) const override
    {
        return 1;
    }

private:
    std::set<App::DocumentObject*> inList;
    std::string currentDoc;
    std::string currentObj;
    bool noProperty;
};

const ExpressionCompleterModel::Info ExpressionCompleterModel::Info::root = {-1, -1, -1, 0};

/**
 * @brief Construct an ExpressionCompleter object.
 * @param currentDoc Current document to generate the model from.
 * @param currentDocObj Current document object to generate model from.
 * @param parent Parent object owning the completer.
 */

ExpressionCompleter::ExpressionCompleter(const App::DocumentObject* currentDocObj,
    QObject* parent, bool noProperty, bool checkInList)
    : QCompleter(parent), currentObj(currentDocObj)
    , noProperty(noProperty), checkInList(checkInList)
{
    setCaseSensitivity(Qt::CaseInsensitive);
}

void ExpressionCompleter::init() {
    if (model())
        return;

    auto m = new ExpressionCompleterModel(this,noProperty);
    m->setDocumentObject(currentObj.getObject(),checkInList);
    setModel(m);
}

void ExpressionCompleter::setDocumentObject(const App::DocumentObject* obj, bool _checkInList)
{
    if (!obj || !obj->getNameInDocument())
        currentObj = App::DocumentObjectT();
    else
        currentObj = obj;
    setCompletionPrefix(QString());
    checkInList = _checkInList;
    auto m = model();
    if (m)
        static_cast<ExpressionCompleterModel*>(m)->setDocumentObject(obj, checkInList);
}

void ExpressionCompleter::setNoProperty(bool enabled) {
    noProperty = enabled;
    auto m = model();
    if (m)
        static_cast<ExpressionCompleterModel*>(m)->setNoProperty(enabled);
}

QString ExpressionCompleter::pathFromIndex(const QModelIndex& index) const
{
    auto m = model();
    if (!m || !index.isValid())
        return {};

    QString res;
    auto parent = index;
    do {
        res = m->data(parent, Qt::UserRole).toString() + res;
        parent = parent.parent();
    } while (parent.isValid());

    auto info = ExpressionCompleterModel::getInfo(index);
    FC_TRACE("join path " << info.doc << "," << info.obj << "," << info.prop << ","
                          << info.contextualHierarchy << "," << index.row()
                          << ": " << res.toUtf8().constData());
    return res;
}

QStringList ExpressionCompleter::splitPath(const QString& input) const
{
    QStringList resultList;
    std::string path = input.toUtf8().constData();
    if (path.empty())
        return resultList;

    int retry = 0;
    std::string lastElem;  // used to recover in case of parse failure after ".".
    std::string trim;      // used to delete ._self added for another recovery path
    while (true) {
        try {
            // this will not work for incomplete Tokens at the end
            // "Sketch." will fail to parse and complete.

            App::ObjectIdentifier ident = ObjectIdentifier::parse(
                    currentObj.getObject(), path);

            std::vector<std::string> stringList = ident.getStringList();
            auto stringListIter = stringList.begin();
            if (retry > 1 && !stringList.empty())
                stringList.pop_back();

            if (!stringList.empty()) {
                if (!trim.empty() && boost::ends_with(stringList.back(), trim))
                    stringList.back().resize(stringList.back().size() - trim.size());
                while (stringListIter != stringList.end()) {
                    resultList << Base::Tools::fromStdString(*stringListIter);
                    ++stringListIter;
                }
            }
            if (lastElem.size()) {
                // if we finish in a trailing separator
                if (!lastElem.empty()) {
                    // erase the separator
                    lastElem.erase(lastElem.begin());
                    resultList << Base::Tools::fromStdString(lastElem);
                } else {
                    // add empty string to allow completion after "." or "#"
                    resultList << QString();
                }
            }
            FC_TRACE("split path " << path << " -> "
                                   << resultList.join(QLatin1String("/")).toUtf8().constData());
            return resultList;
        }
        catch (const Base::Exception& except) {
            FC_TRACE("split path " << path << " error: " << except.what());
            if (retry == 0) {
                size_t lastElemStart = path.rfind('.');

                if (lastElemStart == std::string::npos) {
                    lastElemStart = path.rfind('#');
                }
                if (lastElemStart != std::string::npos) {
                    lastElem = path.substr(lastElemStart);
                    path = path.substr(0, lastElemStart);
                }
                retry++;
                continue;
            }
            else if (retry == 1) {
                // restore path from retry 0
                if (lastElem.size() > 1) {
                    path = path + lastElem;
                    lastElem = "";
                }
                // else... we don't reset lastElem if it's a '.' or '#' to allow chaining completions
                if (!path.empty()) {
                    char last = path[path.size() - 1];
                    if (last != '#' && last != '.' && path.find('#') != std::string::npos) {
                        path += "._self";
                        ++retry;
                        continue;
                    }
                }
            }
            else if (retry == 2) {
                if (path.size() >= 6) {
                    path.resize(path.size() - 6);
                }
                if (!path.empty()) {
                    char last = path[path.size() - 1];
                    if (last != '.' && last != '<' && path.find("#<<") != std::string::npos) {
                        path += ">>._self";
                        ++retry;
                        trim = ">>";
                        continue;
                    }
                }
            }
            return QStringList() << input;
        }
    }
}

// Code below inspired by blog entry:
// https://john.nachtimwald.com/2009/07/04/qcompleter-and-comma-separated-tags/

void ExpressionCompleter::slotUpdate(const QString & prefix, int pos)
{
    FC_TRACE("SlotUpdate:" << prefix.toUtf8().constData());

    init();

    QString completionPrefix = tokenizer.perform(prefix, pos);
    if (completionPrefix.isEmpty()) {
        if (auto itemView = popup())
            itemView->setVisible(false);
        return;
    }

    FC_TRACE("Completion Prefix:" << completionPrefix.toUtf8().constData());
    // Set completion prefix
    setCompletionPrefix(completionPrefix);

    if (widget()->hasFocus()) {
        FC_TRACE("Complete on Prefix" << completionPrefix.toUtf8().constData());
        complete();
        FC_TRACE("Complete Done");

    }
    else if (auto itemView = popup()) {
        itemView->setVisible(false);
    }
}

ExpressionLineEdit::ExpressionLineEdit(QWidget* parent, bool noProperty,
    char checkPrefix, bool checkInList)
    : QLineEdit(parent)
    , completer(nullptr)
    , block(true)
    , noProperty(noProperty)
    , exactMatch(false)
    , checkInList(checkInList)
    , checkPrefix(checkPrefix)
{
    connect(this, &QLineEdit::textEdited, this, &ExpressionLineEdit::slotTextChanged);
}

void ExpressionLineEdit::setPrefix(char prefix) {
    checkPrefix = prefix;
}

void ExpressionLineEdit::setDocumentObject(const App::DocumentObject* currentDocObj,
    bool _checkInList)
{
    checkInList = _checkInList;
    if (completer) {
        completer->setDocumentObject(currentDocObj, checkInList);
        return;
    }
    if (currentDocObj) {
        completer = new ExpressionCompleter(currentDocObj, this, noProperty, checkInList);
        completer->setWidget(this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        if (!exactMatch)
            completer->setFilterMode(Qt::MatchContains);
        connect(completer, qOverload<const QString&>(&QCompleter::activated),
                this, &ExpressionLineEdit::slotCompleteTextSelected);
        connect(completer,
                qOverload<const QString&>(&QCompleter::highlighted),
                this, &ExpressionLineEdit::slotCompleteTextHighlighted);
        connect(this, &ExpressionLineEdit::textChanged2,
                completer, &ExpressionCompleter::slotUpdate);
    }
}

void ExpressionLineEdit::setNoProperty(bool enabled) {
    noProperty = enabled;
    if (completer)
        completer->setNoProperty(enabled);
}

void ExpressionLineEdit::setExactMatch(bool enabled) {
    exactMatch = enabled;
    if (completer)
        completer->setFilterMode(exactMatch ? Qt::MatchStartsWith : Qt::MatchContains);

}

bool ExpressionLineEdit::completerActive() const
{
    return completer && completer->popup() && completer->popup()->isVisible();
}

void ExpressionLineEdit::hideCompleter()
{
    if (completer && completer->popup())
        completer->popup()->setVisible(false);
}

void ExpressionLineEdit::slotTextChanged(const QString & text)
{
    if (!block) {
        if (!text.size() || (checkPrefix && text[0] != QLatin1Char(checkPrefix)))
            return;
        Q_EMIT textChanged2(text,cursorPosition());
    }
}

void ExpressionLineEdit::slotCompleteText(const QString & completionPrefix, bool isActivated)
{
    int start,end;
    completer->getPrefixRange(start,end);
    QString before(text().left(start));
    QString after(text().mid(end));

    {
        Base::FlagToggler<bool> flag(block, false);
        before += completionPrefix;
        setText(before + after);
        setCursorPosition(before.length());
        completer->updatePrefixEnd(before.length());
    }

    // chain completions if we select an entry from the completer drop down
    // and that entry ends with '.' or '#'
    if (isActivated) {
        std::string textToComplete = completionPrefix.toUtf8().constData();
        if (textToComplete.size()
            && (*textToComplete.crbegin() == '.' || *textToComplete.crbegin() == '#')) {
            Base::FlagToggler<bool> flag(block, true);
            slotTextChanged(before + after);
        }
    }
}

void ExpressionLineEdit::slotCompleteTextHighlighted(const QString& completionPrefix)
{
    slotCompleteText(completionPrefix, false);
}

void ExpressionLineEdit::slotCompleteTextSelected(const QString& completionPrefix)
{
    slotCompleteText(completionPrefix, true);
}


void ExpressionLineEdit::keyPressEvent(QKeyEvent* e)
{
    Base::FlagToggler<bool> flag(block,true);
    QLineEdit::keyPressEvent(e);
}

void ExpressionLineEdit::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = createStandardContextMenu();
    menu->addSeparator();
    QAction* match = menu->addAction(tr("Exact match"));

    if (completer) {
        match->setCheckable(true);
        match->setChecked(completer->filterMode() == Qt::MatchStartsWith);
    }
    else {
        match->setVisible(false);
    }

    QAction* action = menu->exec(event->globalPos());

    if (completer) {
        if (action == match)
            setExactMatch(match->isChecked());
    }

    delete menu;
}


///////////////////////////////////////////////////////////////////////

ExpressionTextEdit::ExpressionTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
    , completer(nullptr)
    , block(true)
    , exactMatch(false)
{
    connect(this, &QPlainTextEdit::textChanged, this, &ExpressionTextEdit::slotTextChanged);
}

void ExpressionTextEdit::setExactMatch(bool enabled)
{
    exactMatch = enabled;
    if (completer)
        completer->setFilterMode(exactMatch ? Qt::MatchStartsWith : Qt::MatchContains);
}

void ExpressionTextEdit::setDocumentObject(const App::DocumentObject* currentDocObj)
{
    if (completer) {
        completer->setDocumentObject(currentDocObj);
        return;
    }

    if (currentDocObj) {
        completer = new ExpressionCompleter(currentDocObj, this);
        if (!exactMatch)
            completer->setFilterMode(Qt::MatchContains);
        completer->setWidget(this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        connect(completer, qOverload<const QString&>(&QCompleter::activated), this,
            &ExpressionTextEdit::slotCompleteText);
        connect(completer, qOverload<const QString&>(&QCompleter::highlighted), this,
            &ExpressionTextEdit::slotCompleteText);
        connect(this, &ExpressionTextEdit::textChanged2, completer,
            &ExpressionCompleter::slotUpdate);
    }
}

bool ExpressionTextEdit::completerActive() const
{
    return completer && completer->popup() && completer->popup()->isVisible();
}

void ExpressionTextEdit::hideCompleter()
{
    if (completer && completer->popup())
        completer->popup()->setVisible(false);
}

void ExpressionTextEdit::slotTextChanged()
{
    if (!block) {
        QTextCursor cursor = textCursor();
        Q_EMIT textChanged2(cursor.block().text(),cursor.positionInBlock());
    }
}

void ExpressionTextEdit::slotCompleteText(const QString& completionPrefix)
{
    QTextCursor cursor = textCursor();
    int start,end;
    completer->getPrefixRange(start,end);
    int pos = cursor.positionInBlock();
    if (pos < end) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, end - pos);
    }
    cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor,end-start);
    Base::FlagToggler<bool> flag(block,false);
    cursor.insertText(completionPrefix);
    completer->updatePrefixEnd(cursor.positionInBlock());
}

void ExpressionTextEdit::keyPressEvent(QKeyEvent* e)
{
    Base::FlagToggler<bool> flag(block,true);
    QPlainTextEdit::keyPressEvent(e);
}

void ExpressionTextEdit::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = createStandardContextMenu();
    menu->addSeparator();
    QAction* match = menu->addAction(tr("Exact match"));

    if (completer) {
        match->setCheckable(true);
        match->setChecked(completer->filterMode() == Qt::MatchStartsWith);
    }
    else {
        match->setVisible(false);
    }

    QAction* action = menu->exec(event->globalPos());

    if (completer) {
        if (action == match)
            setExactMatch(match->isChecked());
    }

    delete menu;
}

///////////////////////////////////////////////////////////////////////

ExpressionParameter* ExpressionParameter::instance()
{
    static auto inst = new ExpressionParameter();
    return inst;
}

bool ExpressionParameter::isCaseSensitive() const
{
    auto handle = GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Expression");
    return handle->GetBool("CompleterCaseSensitive", false);
}

bool ExpressionParameter::isExactMatch() const
{
    auto handle = GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Expression");
    return handle->GetBool("CompleterMatchExact", false);
}

#include "moc_ExpressionCompleter.cpp"
