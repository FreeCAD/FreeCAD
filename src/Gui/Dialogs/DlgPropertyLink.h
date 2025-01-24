/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_DIALOG_DLGPROPERTYLINK_H
#define GUI_DIALOG_DLGPROPERTYLINK_H

#include <QDialog>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include "Selection.h"

#define FC_XLINK_VALUE_INDEX 5

class QTreeWidgetItem;

namespace Gui { namespace Dialog {

class Ui_DlgPropertyLink;
class DlgPropertyLink : public QDialog, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit DlgPropertyLink(QWidget* parent = nullptr);
    ~DlgPropertyLink() override;

    void accept() override;

    QList<App::SubObjectT> currentLinks() const;
    QList<App::SubObjectT> originalLinks() const;

    void init(const App::DocumentObjectT &prop, bool tryFilter=true);

    static QString linksToPython(const QList<App::SubObjectT>& links);

    static QList<App::SubObjectT> getLinksFromProperty(const App::PropertyLinkBase *prop);

    static QString formatObject(App::Document *ownerDoc, App::DocumentObject *obj, const char *sub);

    static inline QString formatObject(App::Document *ownerDoc, const App::SubObjectT &sobj) {
        return formatObject(ownerDoc, sobj.getObject(), sobj.getSubName().c_str());
    }

    static QString formatLinks(App::Document *ownerDoc, QList<App::SubObjectT> links);

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void closeEvent (QCloseEvent * e) override;
    void leaveEvent(QEvent *) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

    void detachObserver();
    void attachObserver();

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    void onObjectTypeToggled(bool);
    void onTypeTreeItemSelectionChanged();
    void onSearchBoxTextChanged(const QString&);
    void onItemExpanded(QTreeWidgetItem * item);
    void onItemSelectionChanged();
    void onItemEntered(QTreeWidgetItem *item);
    void onItemSearch();
    void onTimer();
    void onClicked(QAbstractButton *);

private:
    QTreeWidgetItem *createItem(App::DocumentObject *obj, QTreeWidgetItem *parent);
    QTreeWidgetItem *createTypeItem(Base::Type type);
    void filterObjects();
    void filterItem(QTreeWidgetItem *item);
    bool filterType(QTreeWidgetItem *item);
    QTreeWidgetItem *findItem(App::DocumentObject *obj, const char *subname=nullptr, bool *found=nullptr);
    void itemSearch(const QString &text, bool select);
    QList<App::SubObjectT> getLinkFromItem(QTreeWidgetItem *, bool needSubName=true) const;

private:
    Ui_DlgPropertyLink* ui;
    QTimer *timer;
    QPushButton *resetButton;
    QPushButton *refreshButton;

    QPointer<QWidget> parentView;
    std::vector<App::SubObjectT> savedSelections;

    App::DocumentObjectT objProp;
    std::set<App::DocumentObject*> inList;
    std::map<App::Document*, QTreeWidgetItem*> docItems;
    std::map<App::DocumentObject*, QTreeWidgetItem*> itemMap;
    std::map<QByteArray, QTreeWidgetItem*> typeItems;
    std::set<QTreeWidgetItem*> subSelections;
    QList<QTreeWidgetItem*> selections;
    std::set<QByteArray> selectedTypes;
    QList<App::SubObjectT> oldLinks;
    bool allowSubObject = false;
    bool singleSelect = false;
    bool singleParent = false;
    App::DocumentObject *currentObj = nullptr;
    QTreeWidgetItem *searchItem = nullptr;
    QBrush bgBrush;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_DIALOG_DLGPROPERTYLINK_H

