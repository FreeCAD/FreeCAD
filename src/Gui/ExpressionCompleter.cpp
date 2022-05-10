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

class ExpressionCompleterModel: public QAbstractItemModel {
public:
    ExpressionCompleterModel(QObject *parent, bool noProperty)
        :QAbstractItemModel(parent), noProperty(noProperty)
    {
    }

    void setNoProperty(bool enabled) {
        noProperty = enabled;
    }

    void setDocumentObject(const App::DocumentObject *obj, bool checkInList) {
        beginResetModel();
        if(obj) {
            currentDoc = obj->getDocument()->getName();
            currentObj = obj->getNameInDocument();
            if(!noProperty && checkInList)
                inList = obj->getInListEx(true);
        } else {
            currentDoc.clear();
            currentObj.clear();
            inList.clear();
        }
        endResetModel();

    }

    // This ExpressionCompleter model works without any pysical items.
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
    union Info {
        struct {
            qint32 doc;
            qint32 obj;
        }d;
        struct {
            qint16 doc;
            qint16 obj;
        }d32;
        void *ptr;
    };

    static void *infoId(const Info &info) {
        if(sizeof(void*) >= sizeof(info))
            return info.ptr;

        Info info32;
        info32.d32.doc = (qint16)info.d.doc;
        info32.d32.obj = (qint16)info.d.obj;
        return info32.ptr;
    };

    static Info getInfo(const QModelIndex &index) {
        Info info;
        info.ptr = index.internalPointer();
        if(sizeof(void*) >= sizeof(Info))
            return info;
        Info res;
        res.d.doc = info.d32.doc;
        res.d.obj = info.d32.obj;
        return res;
    }

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const {
        if(role!=Qt::EditRole && role!=Qt::DisplayRole && role!=Qt::UserRole)
            return QVariant();
        QVariant v;
        Info info = getInfo(index);
        _data(info,index.row(),&v,nullptr,role==Qt::UserRole);
        FC_TRACE(info.d.doc << "," << info.d.obj << "," << index.row()
                << ": " << v.toString().toUtf8().constData());
        return v;
    }

    void _data(const Info &info, int row, QVariant *v, int *count, bool sep=false) const {
        int idx;
        idx = info.d.doc<0?row:info.d.doc;
        const auto &docs = App::GetApplication().getDocuments();
        int docSize = (int)docs.size()*2;
        int objSize = 0;
        int propSize = 0;
        std::vector<std::pair<const char*, App::Property*> > props;
        App::Document *doc = nullptr;
        App::DocumentObject *obj = nullptr;
        const char *propName = nullptr;
        if(idx>=0 && idx<docSize)
            doc = docs[idx/2];
        else {
            doc = App::GetApplication().getDocument(currentDoc.c_str());
            if(!doc)
                return;
            idx -= docSize;
            if(info.d.doc<0)
                row = idx;
            const auto &objs = doc->getObjects();
            objSize = (int)objs.size()*2;
            if(idx>=0 && idx<objSize) {
                obj = objs[idx/2];
                if(inList.count(obj))
                    return;
            } else if (!noProperty) {
                auto cobj = doc->getObject(currentObj.c_str());
                if(cobj) {
                    idx -= objSize;
                    if(info.d.doc<0)
                        row = idx;
                    cobj->getPropertyNamedList(props);
                    propSize = (int)props.size();
                    if(idx >= propSize)
                        return;
                    if(idx>=0) {
                        obj = cobj;
                        propName = props[idx].first;
                    }
                }
            }
        }
        if(info.d.doc<0) {
            if(count)
                *count = docSize + objSize + propSize;
            if(idx>=0 && v) {
                QString res;
                if(propName)
                    res = QString::fromLatin1(propName);
                else if(obj) {
                    if(idx & 1)
                        res = QString::fromUtf8(quote(obj->Label.getStrValue()).c_str());
                    else
                        res = QString::fromLatin1(obj->getNameInDocument());
                    if(sep && !noProperty)
                        res += QLatin1Char('.');
                } else {
                    if(idx & 1)
                        res = QString::fromUtf8(quote(doc->Label.getStrValue()).c_str());
                    else
                        res = QString::fromLatin1(doc->getName());
                    if(sep)
                        res += QLatin1Char('#');
                }
                v->setValue(res);
            }
            return;
        }

        if(!obj) {
            idx = info.d.obj<0?row:info.d.obj;
            const auto &objs = doc->getObjects();
            objSize = (int)objs.size()*2;
            if(idx<0 || idx>=objSize || inList.count(obj))
                return;
            obj = objs[idx/2];
            if(info.d.obj<0) {
                if(count)
                    *count = objSize;
                if(v) {
                    QString res;
                    if(idx&1)
                        res = QString::fromUtf8(quote(obj->Label.getStrValue()).c_str());
                    else
                        res = QString::fromLatin1(obj->getNameInDocument());
                    if(sep && !noProperty)
                        res += QLatin1Char('.');
                    v->setValue(res);
                }
                return;
            }
        }

        if(noProperty)
            return;
        if(!propName) {
            idx = row;
            obj->getPropertyNamedList(props);
            propSize = (int)props.size();
            if(idx<0 || idx>=propSize)
                return;
            propName = props[idx].first;
            if(count)
                *count = propSize;
        }
        if(v) 
            *v = QString::fromLatin1(propName);
        return;
    }

    QModelIndex parent(const QModelIndex & index) const {
        if(!index.isValid())
            return QModelIndex();
        Info info;
        Info parentInfo;
        info = parentInfo = getInfo(index);
        if(info.d.obj>=0) {
            parentInfo.d.obj = -1;
            return createIndex(info.d.obj,0,infoId(parentInfo));
        }
        if(info.d.doc>=0) {
            parentInfo.d.doc = -1;
            return createIndex(info.d.doc,0,infoId(parentInfo));
        }
        return QModelIndex();
    }

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const {
        if(row<0)
            return QModelIndex();
        Info info;
        if(!parent.isValid()) {
            info.d.doc = -1;
            info.d.obj = -1;
        }else{
            info = getInfo(parent);
            if(info.d.doc<=0)
                info.d.doc = parent.row();
            else if(info.d.obj<=0)
                info.d.obj = parent.row();
            else
                return QModelIndex();
        }
        return createIndex(row,column,infoId(info));
    }

    int rowCount(const QModelIndex & parent = QModelIndex()) const {
        Info info;
        int row = 0;
        if(!parent.isValid()) {
            info.d.doc = -1;
            info.d.obj = -1;
            row = -1;
        }else{
            info = getInfo(parent);
            if(info.d.doc<0)
                info.d.doc = parent.row();
            else if(info.d.obj<0)
                info.d.obj = parent.row();
            else
                return 0;
        }
        int count = 0;
        _data(info,row,nullptr,&count);
        FC_TRACE(info.d.doc << "," << info.d.obj << "," << row << " row count " << count);
        return count;
    }

    int columnCount(const QModelIndex &) const {
        return 1;
    }

private:
    std::set<App::DocumentObject*> inList;
    std::string currentDoc;
    std::string currentObj;
    bool noProperty;
};

/**
 * @brief Construct an ExpressionCompleter object.
 * @param currentDoc Current document to generate the model from.
 * @param currentDocObj Current document object to generate model from.
 * @param parent Parent object owning the completer.
 */

ExpressionCompleter::ExpressionCompleter(const App::DocumentObject * currentDocObj, 
        QObject *parent, bool noProperty, bool checkInList)
    : QCompleter(parent), currentObj(currentDocObj)
    , noProperty(noProperty), checkInList(checkInList)
{
    setCaseSensitivity(Qt::CaseInsensitive);
}

void ExpressionCompleter::init() {
    if(model())
        return;

    auto m = new ExpressionCompleterModel(this,noProperty);
    m->setDocumentObject(currentObj.getObject(),checkInList);
    setModel(m);
}

void ExpressionCompleter::setDocumentObject(const App::DocumentObject *obj, bool _checkInList) {
    if(!obj || !obj->getNameInDocument())
        currentObj = App::DocumentObjectT();
    else
        currentObj = obj;
    setCompletionPrefix(QString());
    checkInList = _checkInList;
    auto m = model();
    if(m)
        static_cast<ExpressionCompleterModel*>(m)->setDocumentObject(obj, checkInList);
}

void ExpressionCompleter::setNoProperty(bool enabled) {
    noProperty = enabled;
    auto m = model();
    if(m)
        static_cast<ExpressionCompleterModel*>(m)->setNoProperty(enabled);
}

QString ExpressionCompleter::pathFromIndex ( const QModelIndex & index ) const
{
    auto m = model();
    if(!m || !index.isValid())
        return QString();

    QString res;
    auto parent = index;
    do {
        res = m->data(parent, Qt::UserRole).toString() + res;
        parent = parent.parent();
    }while(parent.isValid());

    auto info = ExpressionCompleterModel::getInfo(index);
    FC_TRACE("join path " << info.d.doc << "," << info.d.obj << "," << index.row()
            << ": " << res.toUtf8().constData());
    return res;
}

QStringList ExpressionCompleter::splitPath ( const QString & input ) const
{
    QStringList l;
    std::string path = input.toUtf8().constData();
    if(path.empty())
        return l;

    int retry = 0;
    std::string trim;
    while(1) {
        try {
            App::ObjectIdentifier p = ObjectIdentifier::parse(
                    currentObj.getObject(), path);

            std::vector<std::string> sl = p.getStringList();
            std::vector<std::string>::const_iterator sli = sl.begin();
            if(retry && sl.size())
                sl.pop_back();
            if(trim.size() && boost::ends_with(sl.back(),trim))
                sl.back().resize(sl.back().size()-trim.size());
            while (sli != sl.end()) {
                l << Base::Tools::fromStdString(*sli);
                ++sli;
            }
            FC_TRACE("split path " << path
                    << " -> " << l.join(QLatin1String("/")).toUtf8().constData());
            return l;
        }
        catch (const Base::Exception &e) {
            FC_TRACE("split path " << path << " error: " << e.what());
            if(!retry) {
                char last = path[path.size()-1];
                if(last!='#' && last!='.' && path.find('#')!=std::string::npos) {
                    path += "._self";
                    ++retry;
                    continue;
                }
            }else if(retry==1) {
                path.resize(path.size()-6);
                char last = path[path.size()-1];
                if(last!='.' && last!='<' && path.find("#<<")!=std::string::npos) {
                    path += ">>._self";
                    ++retry;
                    trim = ">>";
                    continue;
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
    init();

    std::string completionPrefix;

    // Compute start; if prefix starts with =, start parsing from offset 1.
    int start = (prefix.size() > 0 && prefix.at(0) == QChar::fromLatin1('=')) ? 1 : 0;

    std::string expression = Base::Tools::toStdString(prefix.mid(start));

    // Tokenize prefix
    std::vector<std::tuple<int, int, std::string> > tokens = ExpressionParser::tokenize(expression);

    // No tokens
    if (tokens.size() == 0) {
        if (auto p = popup())
            p->setVisible(false);
        return;
    }

    prefixEnd = prefix.size();

    // Pop those trailing tokens depending on the given position, which may be
    // in the middle of a token, and we shall include that token.
    for(auto it=tokens.begin();it!=tokens.end();++it) {
        if(get<1>(*it) >= pos) {
            // Include the immediately followed '.' or '#', because we'll be
            // inserting these separators too, in ExpressionCompleteModel::pathFromIndex()
            if(it!=tokens.begin() && get<0>(*it)!='.' && get<0>(*it)!='#')
                it = it-1;
            tokens.resize(it-tokens.begin()+1);
            prefixEnd = start + get<1>(*it) + (int)get<2>(*it).size();
            break;
        }
    }

    int trim = 0;
    if(prefixEnd > pos)
        trim = prefixEnd - pos;

    // Extract last tokens that can be rebuilt to a variable
    ssize_t i = static_cast<ssize_t>(tokens.size()) - 1;

    // First, check if we have unclosing string starting from the end
    bool stringing = false;
    for(; i>=0; --i) {
        int token = get<0>(tokens[i]);
        if(token == ExpressionParser::STRING) {
            stringing = false;
            break;
        }
        if(token==ExpressionParser::LT
            && i && get<0>(tokens[i-1])==ExpressionParser::LT)
        {
            --i;
            stringing = true;
            break;
        }
    }

    // Not an unclosed string and the last character is a space
    if(!stringing && prefix.size() && prefix[prefixEnd-1] == QChar(32)) {
        if (auto p = popup())
            p->setVisible(false);
        return;
    }

    if(!stringing) {
        i = static_cast<ssize_t>(tokens.size()) - 1;
        for(;i>=0;--i) {
            int token = get<0>(tokens[i]);
            if (token != '.' && token != '#' &&
                token != ExpressionParser::IDENTIFIER &&
                token != ExpressionParser::STRING &&
                token != ExpressionParser::UNIT)
                break;
        }
        ++i;

    }

    // Set prefix start for use when replacing later
    if (i == static_cast<ssize_t>(tokens.size()))
        prefixStart = prefixEnd;
    else
        prefixStart = start + get<1>(tokens[i]);

    // Build prefix from tokens
    while (i < static_cast<ssize_t>(tokens.size())) {
        completionPrefix += get<2>(tokens[i]);
        ++i;
    }

    if(trim && trim<(int)completionPrefix.size() )
        completionPrefix.resize(completionPrefix.size()-trim);

    // Set completion prefix
    setCompletionPrefix(Base::Tools::fromStdString(completionPrefix));

    if (!completionPrefix.empty() && widget()->hasFocus())
        complete();
    else {
        if (auto p = popup())
            p->setVisible(false);
    }
}

ExpressionLineEdit::ExpressionLineEdit(QWidget *parent, bool noProperty, char checkPrefix, bool checkInList)
    : QLineEdit(parent)
    , completer(nullptr)
    , block(true)
    , noProperty(noProperty)
    , exactMatch(false)
    , checkInList(checkInList)
    , checkPrefix(checkPrefix)
{
    connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(slotTextChanged(const QString&)));
}

void ExpressionLineEdit::setPrefix(char prefix) {
    checkPrefix = prefix;
}

void ExpressionLineEdit::setDocumentObject(const App::DocumentObject * currentDocObj, bool _checkInList)
{
    checkInList = _checkInList;
    if (completer) {
        completer->setDocumentObject(currentDocObj, checkInList);
        return;
    }
    if (currentDocObj != nullptr) {
        completer = new ExpressionCompleter(currentDocObj, this, noProperty, checkInList);
        completer->setWidget(this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        if (!exactMatch)
            completer->setFilterMode(Qt::MatchContains);
        connect(completer, SIGNAL(activated(QString)), this, SLOT(slotCompleteText(QString)));
        connect(completer, SIGNAL(highlighted(QString)), this, SLOT(slotCompleteText(QString)));
        connect(this, SIGNAL(textChanged2(QString,int)), completer, SLOT(slotUpdate(QString,int)));
    }
}

void ExpressionLineEdit::setNoProperty(bool enabled) {
    noProperty = enabled;
    if(completer)
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
        if(!text.size() || (checkPrefix && text[0]!=QLatin1Char(checkPrefix)))
            return;
        Q_EMIT textChanged2(text,cursorPosition());
    }
}

void ExpressionLineEdit::slotCompleteText(const QString & completionPrefix)
{
    int start,end;
    completer->getPrefixRange(start,end);
    QString before(text().left(start));
    QString after(text().mid(end));

    Base::FlagToggler<bool> flag(block,false);
    before += completionPrefix;
    setText(before + after);
    setCursorPosition(before.length());
    completer->updatePrefixEnd(before.length());
}

void ExpressionLineEdit::keyPressEvent(QKeyEvent *e) {
    Base::FlagToggler<bool> flag(block,true);
    QLineEdit::keyPressEvent(e);
}

void ExpressionLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
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

ExpressionTextEdit::ExpressionTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
    , completer(nullptr)
    , block(true)
    , exactMatch(false)
{
    connect(this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
}

void ExpressionTextEdit::setExactMatch(bool enabled) {
    exactMatch = enabled;
    if (completer)
        completer->setFilterMode(exactMatch ? Qt::MatchStartsWith : Qt::MatchContains);
}

void ExpressionTextEdit::setDocumentObject(const App::DocumentObject * currentDocObj)
{
    if (completer) {
        completer->setDocumentObject(currentDocObj);
        return;
    }

    if (currentDocObj != nullptr) {
        completer = new ExpressionCompleter(currentDocObj, this);
        if (!exactMatch)
            completer->setFilterMode(Qt::MatchContains);
        completer->setWidget(this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        connect(completer, SIGNAL(activated(QString)), this, SLOT(slotCompleteText(QString)));
        connect(completer, SIGNAL(highlighted(QString)), this, SLOT(slotCompleteText(QString)));
        connect(this, SIGNAL(textChanged2(QString,int)), completer, SLOT(slotUpdate(QString,int)));
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

void ExpressionTextEdit::slotCompleteText(const QString & completionPrefix)
{
    QTextCursor cursor = textCursor();
    int start,end;
    completer->getPrefixRange(start,end);
    int pos = cursor.positionInBlock();
    if(pos<end)
        cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,end-pos);
    cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor,end-start);
    Base::FlagToggler<bool> flag(block,false);
    cursor.insertText(completionPrefix);
    completer->updatePrefixEnd(cursor.positionInBlock());
}

void ExpressionTextEdit::keyPressEvent(QKeyEvent *e) {
    Base::FlagToggler<bool> flag(block,true);
    QPlainTextEdit::keyPressEvent(e);
}

void ExpressionTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
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
    static ExpressionParameter* inst = new ExpressionParameter();
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
