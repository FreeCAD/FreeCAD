#include "PreCompiled.h"

#ifndef _PreComp_
#include <QStandardItem>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QTextBlock>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Tools.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <App/ObjectIdentifier.h>
#include "ExpressionCompleter.h"
#include <App/ExpressionParser.h>
#include <App/PropertyLinks.h>

FC_LOG_LEVEL_INIT("Completer",true,true);

Q_DECLARE_METATYPE(App::ObjectIdentifier);

using namespace App;
using namespace Gui;

class ExpressionCompleterModel: public QAbstractItemModel {
public:
    ExpressionCompleterModel(QObject *parent, const App::DocumentObject *obj)
        :QAbstractItemModel(parent)
    {
        if(obj) {
            currentDoc = obj->getDocument()->getName();
            currentObj = obj->getNameInDocument();
            inList = obj->getInListEx(true);
        }
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
    // which are consists of three types in the following order,
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
        _data(info,index.row(),&v,0,role==Qt::UserRole);
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
        std::vector<App::Property*> props;
        App::Document *doc = 0;
        App::DocumentObject *obj = 0;
        App::Property *prop = 0;
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
            } else {
                auto cobj = doc->getObject(currentObj.c_str());
                if(cobj) {
                    idx -= objSize;
                    if(info.d.doc<0)
                        row = idx;
                    cobj->getPropertyList(props);
                    propSize = (int)props.size();
                    if(idx >= propSize)
                        return;
                    if(idx>=0) {
                        obj = cobj;
                        prop = props[idx];
                    }
                }
            }
        }
        if(info.d.doc<0) {
            if(count) 
                *count = docSize + objSize + propSize;
            if(idx>=0 && v) {
                QString res;
                if(prop)
                    res = QString::fromLatin1(prop->getName());
                else if(obj) {
                    if(idx & 1)
                        res = QString::fromUtf8(quote(obj->Label.getStrValue()).c_str());
                    else
                        res = QString::fromLatin1(obj->getNameInDocument());
                    if(sep)
                        res += QLatin1Char('.');
                }else {
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
                    if(sep)
                        res += QLatin1Char('.');
                    v->setValue(res);
                }
                return;
            }
        }

        if(!prop) {
            idx = row;
            obj->getPropertyList(props);
            propSize = (int)props.size();
            if(idx<0 || idx>=propSize)
                return;
            prop = props[idx];
            if(count)
                *count = propSize;
        }
        if(v) 
            *v = QString::fromLatin1(prop->getName());
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
        const auto &docs = App::GetApplication().getDocuments();
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
        _data(info,row,0,&count);
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
};

/**
 * @brief Construct an ExpressionCompleter object.
 * @param currentDoc Current document to generate the model from.
 * @param currentDocObj Current document object to generate model from.
 * @param parent Parent object owning the completer.
 */

ExpressionCompleter::ExpressionCompleter(const App::DocumentObject * currentDocObj, QObject *parent)
    : QCompleter(parent), prefixStart(0), currentObj(new App::DocumentObjectT(currentDocObj))
{
    setCaseSensitivity(Qt::CaseInsensitive);
}

void ExpressionCompleter::init() {
    if(model())
        return;

    setModel(new ExpressionCompleterModel(this,currentObj->getObject()));
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
                    currentObj->getObject(), path);

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

void ExpressionCompleter::slotUpdate(const QString & prefix)
{
    init();

    using namespace boost::tuples;
    std::string completionPrefix;

    // Compute start; if prefix starts with =, start parsing from offset 1.
    int start = (prefix.size() > 0 && prefix.at(0) == QChar::fromLatin1('=')) ? 1 : 0;

    std::string expression = Base::Tools::toStdString(prefix.mid(start));

    // Tokenize prefix
    std::vector<boost::tuple<int, int, std::string> > tokens = ExpressionParser::tokenize(expression);

    // No tokens, or last char is a space?
    if (tokens.size() == 0 || (prefix.size() > 0 && prefix[prefix.size() - 1] == QChar(32))) {
        if (popup())
            popup()->setVisible(false);
        return;
    }

    // Extract last tokens that can be rebuilt to a variable
    ssize_t i = static_cast<ssize_t>(tokens.size()) - 1;
    while (i >= 0) {
        int token = get<0>(tokens[i]);
        int tok = ExpressionParser::translateToken(token);
        const auto &s = get<2>(tokens[i]);

        if (token != '.' && token != '#' && s != "<" && s != "<<" && 
            tok != ExpressionParser::FC_TOK_IDENTIFIER && 
            tok != ExpressionParser::FC_TOK_STRING && 
            tok != ExpressionParser::FC_TOK_UNIT)
            break;
        --i;
    }

    ++i;

    // Set prefix start for use when replacing later
    if (i == static_cast<ssize_t>(tokens.size()))
        prefixStart = prefix.size();
    else
        prefixStart = (prefix.at(0) == QChar::fromLatin1('=') ? 1 : 0) + get<1>(tokens[i]);

    // Build prefix from tokens
    while (i < static_cast<ssize_t>(tokens.size())) {
        completionPrefix += get<2>(tokens[i]);
        ++i;
    }

    // Set completion prefix
    FC_TRACE("completion prefix " << completionPrefix);
    setCompletionPrefix(Base::Tools::fromStdString(completionPrefix));

    if (!completionPrefix.empty() && widget()->hasFocus())
        complete();
    else {
        if (popup())
            popup()->setVisible(false);
    }
}

ExpressionLineEdit::ExpressionLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , completer(0)
    , block(true)
{
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged(const QString&)));
}

void ExpressionLineEdit::setDocumentObject(const App::DocumentObject * currentDocObj)
{
    if (completer) {
        delete completer;
        completer = 0;
    }

    if (currentDocObj != 0) {
        completer = new ExpressionCompleter(currentDocObj, this);
        completer->setWidget(this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        connect(completer, SIGNAL(activated(QString)), this, SLOT(slotCompleteText(QString)));
        connect(completer, SIGNAL(highlighted(QString)), this, SLOT(slotCompleteText(QString)));
        connect(this, SIGNAL(textChanged2(QString)), completer, SLOT(slotUpdate(QString)));
    }
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
        Q_EMIT textChanged2(text.left(cursorPosition()));
    }
}

void ExpressionLineEdit::slotCompleteText(const QString & completionPrefix)
{
    int start = completer->getPrefixStart();
    QString before(text().left(start));
    QString after(text().mid(cursorPosition()));

    Base::FlagToggler<bool> flag(block,false);
    setText(before + completionPrefix + after);
    setCursorPosition(QString(before + completionPrefix).length());
}

void ExpressionLineEdit::keyPressEvent(QKeyEvent *e) {
    Base::FlagToggler<bool> flag(block,true);
    QLineEdit::keyPressEvent(e);
}


///////////////////////////////////////////////////////////////////////

ExpressionTextEdit::ExpressionTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
    , completer(0)
    , block(true)
{
    connect(this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
}

void ExpressionTextEdit::setDocumentObject(const App::DocumentObject * currentDocObj)
{
    if (completer) {
        delete completer;
        completer = 0;
    }

    if (currentDocObj != 0) {
        completer = new ExpressionCompleter(currentDocObj, this);
        completer->setWidget(this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        connect(completer, SIGNAL(activated(QString)), this, SLOT(slotCompleteText(QString)));
        connect(completer, SIGNAL(highlighted(QString)), this, SLOT(slotCompleteText(QString)));
        connect(this, SIGNAL(textChanged2(QString)), completer, SLOT(slotUpdate(QString)));
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
        Q_EMIT textChanged2(cursor.block().text().left(cursor.positionInBlock()));
    }
}

void ExpressionTextEdit::slotCompleteText(const QString & completionPrefix)
{
    QTextCursor cursor = textCursor();
    int start = completer->getPrefixStart();
    int pos = cursor.positionInBlock();
    if(pos>=start) {
        Base::FlagToggler<bool> flag(block,false);
        if(pos>start)
            cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor,pos-start);
        cursor.insertText(completionPrefix);
    }
}

void ExpressionTextEdit::keyPressEvent(QKeyEvent *e) {
    Base::FlagToggler<bool> flag(block,true);
    QPlainTextEdit::keyPressEvent(e);
}

#include "moc_ExpressionCompleter.cpp"
