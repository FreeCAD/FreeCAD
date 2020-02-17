#ifndef EXPRESSIONCOMPLETER_H
#define EXPRESSIONCOMPLETER_H

#include <QObject>
#include <QCompleter>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <set>
#include <memory>
#include <App/DocumentObserver.h>

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
    ExpressionCompleter(const App::DocumentObject * currentDocObj, 
            QObject *parent = 0, bool noProperty = false);

    void getPrefixRange(int &start, int &end) const { 
        start = prefixStart;
        end = prefixEnd;
    }

    void updatePrefixEnd(int end) {
        prefixEnd = end;
    }

    void setDocumentObject(const App::DocumentObject*);

    void setNoProperty(bool enabled=true);

public Q_SLOTS:
    void slotUpdate(const QString &prefix, int pos);

private:
    void init();
    virtual QString pathFromIndex ( const QModelIndex & index ) const;
    virtual QStringList splitPath ( const QString & path ) const;

    int prefixStart = 0;
    int prefixEnd = 0;

    App::DocumentObjectT currentObj;
    bool noProperty;

};

class GuiExport ExpressionLineEdit : public QLineEdit {
    Q_OBJECT
public:
    ExpressionLineEdit(QWidget *parent = 0, bool noProperty=false);
    void setDocumentObject(const App::DocumentObject *currentDocObj);
    bool completerActive() const;
    void hideCompleter();
    void setNoProperty(bool enabled=true);
Q_SIGNALS:
    void textChanged2(QString text, int pos);
public Q_SLOTS:
    void slotTextChanged(const QString & text);
    void slotCompleteText(const QString & completionPrefix);
protected:
    void keyPressEvent(QKeyEvent * event);
private:
    ExpressionCompleter * completer;
    bool block;
    bool noProperty;
};

class GuiExport ExpressionTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    ExpressionTextEdit(QWidget *parent = 0);
    void setDocumentObject(const App::DocumentObject *currentDocObj);
    bool completerActive() const;
    void hideCompleter();
protected:
    void keyPressEvent(QKeyEvent * event);
Q_SIGNALS:
    void textChanged2(QString text, int pos);
public Q_SLOTS:
    void slotTextChanged();
    void slotCompleteText(const QString & completionPrefix);
private:
    ExpressionCompleter * completer;
    bool block;
};

}

#endif // EXPRESSIONCOMPLETER_H
