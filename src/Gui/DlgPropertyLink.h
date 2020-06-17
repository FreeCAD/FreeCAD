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

#include <functional>

#include <QTime>
#include <QDialog>
#include <QAbstractItemView>
#include <QTimer>
#include <QPushButton>
#include <QPointer>

#include <App/DocumentObserver.h>

class QTreeWidget;

namespace Gui { namespace Dialog {

class Ui_DlgPropertyLink;
class GuiExport DlgPropertyLink : public QWidget
{
    Q_OBJECT

public:
    enum Flag {
        NoButton = 1,
        NoSearchBox = 2,
        NoTypeFilter = 4,
        NoSyncSubObject = 8,
        AlwaysSyncSubObject = 16,
        AllowSubElement = 32,
        NoSubObject = 64,
    };
    DlgPropertyLink(QWidget* parent = 0, int flags=0);
    ~DlgPropertyLink();

    QList<App::SubObjectT> currentLinks() const;
    QList<App::SubObjectT> originalLinks() const;

    void setContext(App::SubObjectT &&objT);

    void setInitObjects(std::vector<App::DocumentObjectT> &&objs);

    template<class T>
    void setObjectFilter(T t) {
        objFilter = t;
    }

    template<class T>
    void setElementFilter(T t) {
        elementFilter = t;
    }

    void setTypeFilter(std::set<QByteArray> &&filter);
    void setTypeFilter(Base::Type type);
    void setTypeFilter(const std::vector<Base::Type> &types);

    void init(const App::DocumentObjectT &prop, bool tryFilter=true);

    static QString linksToPython(QList<App::SubObjectT> links);

    static QList<App::SubObjectT> getLinksFromProperty(const App::PropertyLinkBase *prop);

    static QString formatObject(App::Document *ownerDoc, App::DocumentObject *obj, const char *sub);

    static inline QString formatObject(App::Document *ownerDoc, const App::SubObjectT &sobj) {
        return formatObject(ownerDoc, sobj.getObject(), sobj.getSubName().c_str());
    }

    static QString formatLinks(App::Document *ownerDoc, QList<App::SubObjectT> links);

    void selectionChanged(const Gui::SelectionChanges& msg);
    void detachObserver(Gui::SelectionObserver *);
    void attachObserver(Gui::SelectionObserver *);

    void clearSelection(QTreeWidgetItem *);

    QTreeWidget *treeWidget();

Q_SIGNALS:
    void linkChanged();
    void accepted();
    void rejected();

protected:
    void leaveEvent(QEvent *);
    bool eventFilter(QObject *obj, QEvent *ev);
    void keyPressEvent(QKeyEvent *ev);

private Q_SLOTS:
    void on_checkObjectType_toggled(bool);
    void on_typeTree_itemSelectionChanged();
    void on_searchBox_textChanged(const QString&);
    void onItemExpanded(QTreeWidgetItem * item);
    void onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*);
    void onItemSelectionChanged();
    void onItemEntered(QTreeWidgetItem *item);
    void onItemSearch();
    void onTimer();
    void onClicked(QAbstractButton *);
    void checkItemSelection();

private:
    QTreeWidgetItem *createItem(App::DocumentObject *obj, QTreeWidgetItem *parent);
    QTreeWidgetItem *createTypeItem(Base::Type type);
    void filterObjects();
    void filterItem(QTreeWidgetItem *item);
    bool filterType(QTreeWidgetItem *item);
    QTreeWidgetItem *findItem(App::DocumentObject *obj, const char *subname=0, bool *found=nullptr);
    void itemSearch(const QString &text, bool select);
    QList<App::SubObjectT> getLinkFromItem(QTreeWidgetItem *, bool needElement=true) const;
    void setItemLabel(QTreeWidgetItem *item, std::size_t idx=0);

private:
    Ui_DlgPropertyLink* ui;
    QTimer *timer;
    QPushButton *resetButton = nullptr;
    QPushButton *refreshButton = nullptr;

    QPointer<QWidget> parentView;
    std::vector<App::SubObjectT> savedSelections;

    App::DocumentObjectT objProp;
    std::set<App::DocumentObject*> inList;
    std::map<App::Document*, QTreeWidgetItem*> docItems;
    std::map<App::DocumentObject*, QTreeWidgetItem*> itemMap;
    std::map<QByteArray, QTreeWidgetItem*> typeItems;
    std::set<QTreeWidgetItem*> elementSels;
    QList<QTreeWidgetItem*> selections;
    std::set<QByteArray> selectedTypes;
    QList<App::SubObjectT> oldLinks;
    bool allowSubObject = false;
    bool singleSelect = false;
    bool singleParent = false;
    bool busy = false;
    bool isXLink = false;
    App::DocumentObject *currentObj = nullptr;
    QTreeWidgetItem *searchItem = nullptr;
    QBrush bgBrush;

    QTime enterTime;

    App::SubObjectT selContext;
    std::vector<App::DocumentObjectT> initObjs;

    std::function<bool (const App::SubObjectT &)> objFilter;
    std::function<bool (const App::SubObjectT &, std::string &element)> elementFilter;

    int flags;
};


class PropertyLinkEditor: public QDialog, public Gui::SelectionObserver
{
public:
    PropertyLinkEditor(QWidget *parent);

    ~PropertyLinkEditor();

    void onSelectionChanged(const Gui::SelectionChanges& msg);

    DlgPropertyLink *getProxy() {
        return proxy;
    }
protected:
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void closeEvent (QCloseEvent * e);
private:
    DlgPropertyLink *proxy;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_DIALOG_DLGPROPERTYLINK_H

