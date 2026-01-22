// SPDX-License-Identifier: LGPL-2.1-or-later
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

#pragma once

#include "DockWindow.h"
#include "Selection.h"
#include <QMenu>
#include <QPointer>
#include <map>
#include <set>
#include <string>
#include <vector>


class QListWidget;
class QListWidgetItem;
class QCheckBox;
class QLabel;

namespace App
{
class DocumentObject;
}

struct ElementInfo;
struct SubMenuInfo;

namespace Gui
{
namespace DockWnd
{

/** A test class. A more elaborate class description.
 */
class SelectionView: public Gui::DockWindow, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    explicit SelectionView(Gui::Document* pcDocument, QWidget* parent = nullptr);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~SelectionView() override;

    /// Observer message from the Selection
    void onSelectionChanged(const SelectionChanges& msg) override;

    void leaveEvent(QEvent*) override;

    bool onMsg(const char* pMsg, const char** ppReturn) override;

    const char* getName() const override
    {
        return "SelectionView";
    }

    /// get called when the document is changed or updated
    void onUpdate() override;

    QListWidget* selectionView;
    QLabel* countLabel;

    QCheckBox* enablePickList;
    QListWidget* pickList;

public Q_SLOTS:
    /// get called when text is entered in the search box
    void search(const QString& text);
    /// get called when enter is pressed in the search box
    void validateSearch();
    /// get called when the list is right-clicked
    void onItemContextMenu(const QPoint& point);
    /// different actions
    void select(QListWidgetItem* item = nullptr);
    void deselect();
    void zoom();
    void treeSelect();
    void toPython();
    void touch();
    void showPart();
    void onEnablePickList();
    void toggleSelect(QListWidgetItem* item = nullptr);
    void preselect(QListWidgetItem* item = nullptr);

protected:
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

private:
    QString getModule(const char* type) const;
    QString getProperty(App::DocumentObject* obj) const;
    bool supportPart(App::DocumentObject* obj, const QString& part) const;

private:
    float x, y, z;
    std::vector<App::DocumentObject*> searchList;
    bool openedAutomatically;
};

}  // namespace DockWnd

// Simple selection data structure
struct PickData
{
    App::DocumentObject* obj;
    std::string element;
    std::string docName;
    std::string objName;
    std::string subName;
};

// Add SelectionMenu class outside the DockWnd namespace
class GuiExport SelectionMenu: public QMenu
{
    Q_OBJECT
public:
    SelectionMenu(QWidget* parent = nullptr);

    /** Populate and show the menu for picking geometry elements.
     *
     * @param sels: a list of geometry element references
     * @param pos: optional position to show the menu (defaults to current cursor position)
     * @return Return the picked geometry reference
     *
     * The menu will be divided into submenus that are grouped by element type.
     */
    PickData doPick(const std::vector<PickData>& sels, const QPoint& pos = QCursor::pos());

public Q_SLOTS:
    void onHover(QAction*);

protected:
    bool eventFilter(QObject*, QEvent*) override;
    void leaveEvent(QEvent* e) override;
    PickData onPicked(QAction*, const std::vector<PickData>& sels);

private:
    void processSelections(std::vector<PickData>& selections, std::map<std::string, SubMenuInfo>& menus);
    void buildMenuStructure(
        std::map<std::string, SubMenuInfo>& menus,
        const std::vector<PickData>& selections
    );

    App::DocumentObject* getSubObject(const PickData& sel);
    std::string extractElementType(const PickData& sel);
    std::string createObjectKey(const PickData& sel);
    QIcon getOrCreateIcon(App::DocumentObject* sobj, std::map<App::DocumentObject*, QIcon>& icons);
    void addGeoFeatureTypes(
        App::DocumentObject* sobj,
        std::map<std::string, SubMenuInfo>& menus,
        std::set<std::string>& createdTypes
    );
    void addWholeObjectSelection(
        const PickData& sel,
        App::DocumentObject* sobj,
        std::vector<PickData>& selections,
        std::map<std::string, SubMenuInfo>& menus,
        const QIcon& icon
    );
    bool shouldGroupMenu(const SubMenuInfo& info);
    void createFlatMenu(
        ElementInfo& elementInfo,
        QMenu* parentMenu,
        const std::string& label,
        const std::string& elementType,
        const std::vector<PickData>& selections
    );
    void createGroupedMenu(
        ElementInfo& elementInfo,
        QMenu* parentMenu,
        const std::string& label,
        const std::string& elementType,
        const std::vector<PickData>& selections
    );

    QPointer<QMenu> activeMenu;
    QPointer<QAction> activeAction;
    std::vector<PickData> currentSelections;
};

}  // namespace Gui
