#ifndef EXPRESSIONCOMPLETER_H
#define EXPRESSIONCOMPLETER_H

#include <QObject>
#include <QCompleter>
#include <QLineEdit>
#include <set>

class QStandardItem;

namespace App {
class Document;
class DocumentObject;
class Property;
class ObjectIdentifier;
}

namespace Gui {

/**
 * @brief The ExpressionCompleter class extends the QCompleter class to provide a completer model of documentobject names and properties.
 */

class GuiExport ExpressionCompleter : public QCompleter
{
    Q_OBJECT
public:
    ExpressionCompleter(const App::Document * currentDoc, const App::DocumentObject * currentDocObj, QObject *parent = 0);

    int getPrefixStart() const { return prefixStart; }

public Q_SLOTS:
    void slotUpdate(const QString &prefix);

private:
    void createModelForDocument(const App::Document * doc, QStandardItem * parent, const std::set<const App::DocumentObject *> &forbidden);
    void createModelForDocumentObject(const App::DocumentObject * docObj, QStandardItem * parent);
    void createModelForPaths(const App::Property * prop, QStandardItem *docObjItem);

    virtual QString pathFromIndex ( const QModelIndex & index ) const;
    virtual QStringList splitPath ( const QString & path ) const;

    int prefixStart;

};

class GuiExport ExpressionLineEdit : public QLineEdit {
    Q_OBJECT
public:
    ExpressionLineEdit(QWidget *parent = 0);
    void setDocumentObject(const App::DocumentObject *currentDocObj);
    bool completerActive() const;
    void hideCompleter();
Q_SIGNALS:
    void textChanged2(QString text);
public Q_SLOTS:
    void slotTextChanged(const QString & text);
    void slotCompleteText(const QString & completionPrefix);
private:
    ExpressionCompleter * completer;
    bool block;
};

}

#endif // EXPRESSIONCOMPLETER_H
