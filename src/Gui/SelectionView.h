/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef GUI_DOCKWND_SELECTIONVIEW_H
#define GUI_DOCKWND_SELECTIONVIEW_H

#include "DockWindow.h"
#include "Selection.h"


class QListWidget;
class QListWidgetItem;
class QCheckBox;
class QLabel;

namespace App {
class DocumentObject;
}

namespace Gui {
namespace DockWnd {

/** A test class. A more elaborate class description.
 */
class SelectionView : public Gui::DockWindow,
                      public Gui::SelectionObserver
{
    Q_OBJECT

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    explicit SelectionView(Gui::Document* pcDocument, QWidget *parent=nullptr);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
    */
    ~SelectionView() override;

    /// Observer message from the Selection
    void onSelectionChanged(const SelectionChanges& msg) override;

    void leaveEvent(QEvent*) override;

    bool onMsg(const char* pMsg,const char** ppReturn) override;

    const char *getName() const override {return "SelectionView";}

    /// get called when the document is changed or updated
    void onUpdate() override;

    QListWidget* selectionView;
    QLabel*      countLabel;

    QCheckBox *enablePickList;
    QListWidget *pickList;

public Q_SLOTS:
    /// get called when text is entered in the search box
    void search(const QString& text);
    /// get called when enter is pressed in the search box
    void validateSearch();
    /// get called when the list is right-clicked
    void onItemContextMenu(const QPoint& point);
    /// different actions
    void select(QListWidgetItem* item=nullptr);
    void deselect();
    void zoom();
    void treeSelect();
    void toPython();
    void touch();
    void showPart();
    void onEnablePickList();
    void toggleSelect(QListWidgetItem* item=nullptr);
    void preselect(QListWidgetItem* item=nullptr);

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;

private:
    QString getModule(const char* type) const;
    QString getProperty(App::DocumentObject* obj) const;
    bool supportPart(App::DocumentObject* obj, const QString& part) const;

private:
    float x,y,z;
    std::vector<App::DocumentObject*> searchList;
    bool openedAutomatically;
};

} // namespace DockWnd
} // namespace Gui

#endif // GUI_DOCKWND_SELECTIONVIEW_H
