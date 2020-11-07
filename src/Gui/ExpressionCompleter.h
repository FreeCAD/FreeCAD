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
    void setExactMatch(bool enabled=true);
Q_SIGNALS:
    void textChanged2(QString text, int pos);
public Q_SLOTS:
    void slotTextChanged(const QString & text);
    void slotCompleteText(const QString & completionPrefix);
protected:
    void keyPressEvent(QKeyEvent * event);
    void contextMenuEvent(QContextMenuEvent * event);
private:
    ExpressionCompleter * completer;
    bool block;
    bool noProperty;
    bool exactMatch;
};

class GuiExport ExpressionTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    ExpressionTextEdit(QWidget *parent = 0);
    void setDocumentObject(const App::DocumentObject *currentDocObj);
    bool completerActive() const;
    void hideCompleter();
    void setExactMatch(bool enabled=true);
protected:
    void keyPressEvent(QKeyEvent * event);
    void contextMenuEvent(QContextMenuEvent * event);
Q_SIGNALS:
    void textChanged2(QString text, int pos);
public Q_SLOTS:
    void slotTextChanged();
    void slotCompleteText(const QString & completionPrefix);
private:
    ExpressionCompleter * completer;
    bool block;
    bool exactMatch;
};

class GuiExport ExpressionParameter {
public:
    static ExpressionParameter *instance();
    bool isCaseSensitive() const;
    bool isExactMatch() const;
};

}

#endif // EXPRESSIONCOMPLETER_H
