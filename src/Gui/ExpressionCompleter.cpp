#include "PreCompiled.h"

#ifndef _PreComp_
#include <QStandardItem>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QAbstractItemView>
#endif

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include "ExpressionCompleter.h"
#include <App/PropertyLinks.h>

Q_DECLARE_METATYPE(App::ObjectIdentifier);

using namespace App;
using namespace Gui;

/**
 * @brief Construct an ExpressionCompleter object.
 * @param currentDoc Current document to generate the model from.
 * @param currentDocObj Current document object to generate model from.
 * @param parent Parent object owning the completer.
 */

ExpressionCompleter::ExpressionCompleter(const App::Document * currentDoc, const App::DocumentObject * currentDocObj, QObject *parent)
    : QCompleter(parent)
{
    QStandardItemModel* model = new QStandardItemModel(this);

    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    std::vector<App::Document*>::const_iterator di = docs.begin();

    std::vector<DocumentObject*> deps = currentDocObj->getInList();
    std::set<const DocumentObject*> forbidden;

    for (std::vector<DocumentObject*>::const_iterator it = deps.begin(); it != deps.end(); ++it)
        forbidden.insert(*it);

    /* Create tree with full path to all objects */
    while (di != docs.end()) {
        QStandardItem* docItem = new QStandardItem(QString::fromLatin1((*di)->getName()));

        docItem->setData(QString::fromLatin1((*di)->getName()) + QString::fromLatin1("#"), Qt::UserRole);
        createModelForDocument(*di, docItem, forbidden);

        model->appendRow(docItem);

        ++di;
    }

    /* Create branch with current document object */

    if (currentDocObj) {
        createModelForDocument(currentDocObj->getDocument(), model->invisibleRootItem(), forbidden);
        createModelForDocumentObject(currentDocObj, model->invisibleRootItem());
    }
    else {
        if (currentDoc)
            createModelForDocument(currentDoc, model->invisibleRootItem(), forbidden);
    }

    setModel(model);

    setCaseSensitivity(Qt::CaseInsensitive);
}

/**
 * @brief Create model node given document, the parent and forbidden node.
 * @param doc Document
 * @param parent Parent item
 * @param forbidden Forbidden document objects; typically those that will create a loop in the DAG if used.
 */

void ExpressionCompleter::createModelForDocument(const App::Document * doc, QStandardItem * parent,
                                                 const std::set<const DocumentObject*> & forbidden) {
    std::vector<App::DocumentObject*> docObjs = doc->getObjects();
    std::vector<App::DocumentObject*>::const_iterator doi = docObjs.begin();

    while (doi != docObjs.end()) {
        std::set<const DocumentObject*>::const_iterator it = forbidden.find(*doi);

        // Skip?
        if (it != forbidden.end()) {
            ++doi;
            continue;
        }

        QStandardItem* docObjItem = new QStandardItem(QString::fromLatin1((*doi)->getNameInDocument()));

        docObjItem->setData(QString::fromLatin1((*doi)->getNameInDocument()) + QString::fromLatin1("."), Qt::UserRole);
        createModelForDocumentObject(*doi, docObjItem);
        parent->appendRow(docObjItem);

        if (strcmp((*doi)->getNameInDocument(), (*doi)->Label.getValue()) != 0) {
            std::string label = (*doi)->Label.getValue();

            if (!ExpressionParser::isTokenAnIndentifier(label))
                label = quote(label);

            docObjItem = new QStandardItem(QString::fromUtf8(label.c_str()));

            docObjItem->setData( QString::fromUtf8(label.c_str()) + QString::fromLatin1("."), Qt::UserRole);
            createModelForDocumentObject(*doi, docObjItem);
            parent->appendRow(docObjItem);
        }

        ++doi;
    }
}

/**
 * @brief Create model nodes for document object
 * @param docObj Document object
 * @param parent Parent item
 */

void ExpressionCompleter::createModelForDocumentObject(const DocumentObject * docObj, QStandardItem * parent)
{
    std::vector<App::Property*> props;
    docObj->getPropertyList(props);

    std::vector<App::Property*>::const_iterator pi = props.begin();
    while (pi != props.end()) {

        // Skip all types of links
        if ((*pi)->isDerivedFrom(App::PropertyLink::getClassTypeId()) ||
                (*pi)->isDerivedFrom(App::PropertyLinkSub::getClassTypeId())) {
            ++pi;
            continue;
        }

        createModelForPaths(*pi, parent);
        ++pi;
    }
}

/**
 * @brief Create nodes for a property.
 * @param prop
 * @param docObjItem
 */

void ExpressionCompleter::createModelForPaths(const App::Property * prop, QStandardItem *docObjItem)
{
    std::vector<ObjectIdentifier> paths;
    std::vector<ObjectIdentifier>::const_iterator ppi;

    prop->getPaths(paths);

    for (ppi = paths.begin(); ppi != paths.end(); ++ppi) {
        QStandardItem* pathItem = new QStandardItem(Base::Tools::fromStdString(ppi->toString()));

        QVariant value;

        value.setValue(*ppi);
        pathItem->setData(value, Qt::UserRole);

        docObjItem->appendRow(pathItem);
    }
}

QString ExpressionCompleter::pathFromIndex ( const QModelIndex & index ) const
{
    QStandardItemModel * m = static_cast<QStandardItemModel*>(model());

    if (m->data(index, Qt::UserRole).canConvert<App::ObjectIdentifier>()) {
        App::ObjectIdentifier p = m->data(index, Qt::UserRole).value<App::ObjectIdentifier>();
        QString pStr = Base::Tools::fromStdString(p.toString());

        QString parentStr;
        QModelIndex parent = index.parent();
        while (parent.isValid()) {
            QString thisParentStr = m->data(parent, Qt::UserRole).toString();

            parentStr = thisParentStr + parentStr;

            parent = parent.parent();
        }

        return parentStr + pStr;
    }
    else if (m->data(index, Qt::UserRole).canConvert<QString>()) {
        QModelIndex parent = index;
        QString parentStr;

        while (parent.isValid()) {
            QString thisParentStr = m->data(parent, Qt::UserRole).toString();

            parentStr = thisParentStr + parentStr;

            parent = parent.parent();
        }

        return parentStr;
    }
    else
        return QString();
}

QStringList ExpressionCompleter::splitPath ( const QString & path ) const
{
    try {
        App::ObjectIdentifier p = ObjectIdentifier::parse(0, path.toUtf8().constData());
        QStringList l;

        if (p.getProperty()) {
            for (int i = 0; i < p.numComponents(); ++i)
                l << Base::Tools::fromStdString(p.getPropertyComponent(i).toString());
            return l;
        }
        else {
            std::vector<std::string> sl = p.getStringList();
            std::vector<std::string>::const_iterator sli = sl.begin();

            while (sli != sl.end()) {
                l << Base::Tools::fromStdString(*sli);
                ++sli;
            }

            return l;
        }
    }
    catch (const Base::Exception &) {
        return QStringList() << path;
    }
}

// Code below inspired by blog entry:
// https://john.nachtimwald.com/2009/07/04/qcompleter-and-comma-separated-tags/

void ExpressionCompleter::slotUpdate(const QString & prefix)
{
    using namespace boost::tuples;
    int j = (prefix.size() > 0 && prefix.at(0) == QChar::fromLatin1('=')) ? 1 : 0;
    std::vector<boost::tuple<int, int, std::string> > tokens = ExpressionParser::tokenize(Base::Tools::toStdString(prefix.mid(j)));
    std::string completionPrefix;

    if (tokens.size() == 0 || (prefix.size() > 0 && prefix[prefix.size() - 1] == QChar(32))) {
        if (popup())
            popup()->setVisible(false);
        return;
    }

    std::size_t i = tokens.size();

    do {
        --i;
        if (!(get<0>(tokens[i]) == ExpressionParser::IDENTIFIER ||
                get<0>(tokens[i]) == ExpressionParser::STRING ||
                get<0>(tokens[i]) == '.'))
            break;
    } while (i > 0);

    prefixStart = get<1>(tokens[i]);
    while (i < tokens.size()) {
        completionPrefix += get<2>(tokens[i]);
        ++i;
    }

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
    , block(false)
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
        completer = new ExpressionCompleter(currentDocObj->getDocument(), currentDocObj, this);
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

    block = true;
    setText(before + completionPrefix + after);
    setCursorPosition(QString(before + completionPrefix).length());
    block = false;
}

#include "moc_ExpressionCompleter.cpp"
