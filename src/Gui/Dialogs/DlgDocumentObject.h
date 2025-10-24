// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef GUI_DIALOG_DLG_DOCUMENT_OBJECT_H
#define GUI_DIALOG_DLG_DOCUMENT_OBJECT_H

#include <QDialog>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include "Selection.h"

#define FC_XLINK_VALUE_INDEX 5

class QTreeWidgetItem;

 namespace Gui::Dialog {

class Ui_DlgDocumentObject;
class DlgDocumentObject : public QDialog, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    static const int ObjectNameRole = Qt::UserRole;
    static const int DocNameRole = Qt::UserRole + 1;
    static const int TypeNameRole = Qt::UserRole + 2;
    static const int ProxyTypeRole = Qt::UserRole + 3;

    explicit DlgDocumentObject(QWidget* parent = nullptr);
    ~DlgDocumentObject() override;

    void accept() override;

    QList<App::SubObjectT> currentSubObjects() const;

    void init(App::DocumentObject* owner,
              bool singleSelect=true,
              bool filterOwner=true,
              bool filterTypeOwner=true,
              bool expandTypeOwner=true);

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
    virtual void onClicked(QAbstractButton *);

protected:

    Ui_DlgDocumentObject* ui;

    App::DocumentObject* owner;

    std::set<App::DocumentObject*> inList;
    std::map<App::Document*, QTreeWidgetItem*> docItems;
    std::map<App::DocumentObject*, QTreeWidgetItem*> itemMap;
    std::map<QByteArray, QTreeWidgetItem*> typeItems;
    std::set<QTreeWidgetItem*> subSelections;
    QList<QTreeWidgetItem*> selections;
    std::set<QByteArray> selectedTypes;

    App::DocumentObject *currentObj = nullptr;
    QTreeWidgetItem *searchItem = nullptr;

    bool allowSubObject = false;
    bool singleSelect = false;
    bool singleParent = false;

    QPushButton *resetButton;
    QPushButton *refreshButton;

private:
    void onObjectTypeToggled(bool);
    void onTypeTreeItemSelectionChanged();
    void onSearchBoxTextChanged(const QString&);
    void onItemExpanded(QTreeWidgetItem * item);
    void onItemSelectionChanged();
    void onItemEntered(QTreeWidgetItem *item);
    void onItemSearch();
    void onTimer();

private:
    QTreeWidgetItem *createItem(App::DocumentObject *obj, QTreeWidgetItem *parent);
    QTreeWidgetItem *createTypeItem(Base::Type type);
    void filterObjects();
    void filterItem(QTreeWidgetItem *item);
    bool filterType(QTreeWidgetItem *item);
    QTreeWidgetItem *findItem(App::DocumentObject *obj, const char *subname=nullptr, bool *found=nullptr);
    void itemSearch(const QString &text, bool select);
    QList<App::SubObjectT> getSubObjectFromItem(QTreeWidgetItem *, bool needSubName=true) const;

private:
    QTimer *timer;

    QPointer<QWidget> parentView;
    std::vector<App::SubObjectT> savedSelections;

    bool filterOwner;
    QBrush bgBrush;
};

} // namespace Gui::Dialog

#endif // GUI_DIALOG_DLG_DOCUMENT_OBJECT_H

