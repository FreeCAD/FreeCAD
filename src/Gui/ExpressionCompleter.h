/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
 *   Copyright (c) 2020 Zheng Lei <realthunder.dev@gmail.com>              *
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
                        QObject *parent = nullptr,
                        bool noProperty = false,
                        bool checkInList = true,
                        char leadChar = 0);

    void getPrefixRange(QString &prefix, int &start, int &end, int &offset) const;

    void updatePrefixEnd(int end) {
        prefixEnd = end;
    }

    void setDocumentObject(const App::DocumentObject*, bool checkInList=true);

    void setNoProperty(bool enabled=true);
    void setRequireLeadingEqualSign(bool enabled);

    void setSearchUnit(bool enabled=true);

    bool isInsideString() const {return insideString;}

    void setLeadChar(char lead) {leadChar = lead;}

    void setupContextMenu(QMenu *menu);

public Q_SLOTS:
    void slotUpdate(const QString &prefix, int pos);

protected:
    bool eventFilter(QObject *o, QEvent *e);

Q_SIGNALS:
    void visibilityChanged(bool visible);

private:
    void init();
    virtual QString pathFromIndex ( const QModelIndex & index ) const;
    virtual QStringList splitPath ( const QString & path ) const;
    void showPopup(bool show);

    int prefixStart = 0;
    int prefixEnd = 0;
    bool closeString = false;
    bool insideString = false;
    QString currentPrefix;
    QString savedPrefix;

    App::DocumentObjectT currentObj;
    bool noProperty = false;
    bool checkInList = true;
    bool searchUnit = false;
    char leadChar = 0;
};

class GuiExport ExpressionLineEdit : public QLineEdit {
    Q_OBJECT
public:
    ExpressionLineEdit(QWidget *parent = nullptr, bool noProperty=false,
            char leadChar=0, bool checkInList=true);
    void setDocumentObject(const App::DocumentObject *currentDocObj, bool checkInList=true);
    void setLeadChar(char lead);
    bool completerActive() const;
    void hideCompleter();
    void setNoProperty(bool enabled=true);
    void setSearchUnit(bool enabled=true);
Q_SIGNALS:
    void textChanged2(QString text, int pos);
    void sizeChanged();
    void completerVisibilityChanged(bool visible);
public Q_SLOTS:
    void slotTextChanged(const QString & text);
    void slotCompleteText(QString completionPrefix);
protected:
    void contextMenuEvent(QContextMenuEvent * event);
    void resizeEvent(QResizeEvent *);
private:
    ExpressionCompleter * completer;
    bool noProperty;
    bool checkInList;
    char leadChar;
    bool searchUnit;
};

class GuiExport ExpressionTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    ExpressionTextEdit(QWidget *parent = nullptr, char leadChar=0);
    void setLeadChar(char lead);
    void setDocumentObject(const App::DocumentObject *currentDocObj);
    bool completerActive() const;
    void hideCompleter();
protected:
    void keyPressEvent(QKeyEvent * event);
    void contextMenuEvent(QContextMenuEvent * event);
Q_SIGNALS:
    void textChanged2(QString text, int pos);
    void completerVisibilityChanged(bool visible);
public Q_SLOTS:
    void slotTextChanged();
    void slotCompleteText(QString completionPrefix);
private:
    ExpressionCompleter * completer;
    bool block;
    char leadChar;
};

}

#endif // EXPRESSIONCOMPLETER_H
