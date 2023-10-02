/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGTOOLBARS_IMP_H
#define GUI_DIALOG_DLGTOOLBARS_IMP_H

#include <boost/signals2/connection.hpp>
#include "PropertyPage.h"
#include <memory>

class QTreeWidgetItem;

namespace Gui {
namespace Dialog {
class Ui_DlgCustomToolbars;

/** This class implements the creation of user defined toolbars.
 * In the left panel are shown all command groups with their command objects.
 * If any changeable toolbar was created in the left panel are shown all commands
 * of the currently edited toolbar, otherwise it is empty.
 * All changes to a toolbar is done immediately.
 *
 * \author Werner Mayer
 */
class DlgCustomToolbars : public CustomizeActionPage
{
    Q_OBJECT

protected:
    enum Type { Toolbar, Toolboxbar };

    explicit DlgCustomToolbars(Type, QWidget* parent = nullptr);
    ~DlgCustomToolbars() override;

protected:
    void setupConnections();
    void onWorkbenchBoxActivated(int index);
    void onMoveActionRightButtonClicked();
    void onMoveActionLeftButtonClicked();
    void onMoveActionUpButtonClicked();
    void onMoveActionDownButtonClicked();
    void onNewButtonClicked();
    void onRenameButtonClicked();
    void onDeleteButtonClicked();

protected Q_SLOTS:
    void onAddMacroAction(const QByteArray&) override;
    void onRemoveMacroAction(const QByteArray&) override;
    void onModifyMacroAction(const QByteArray&) override;

protected:
    void changeEvent(QEvent *e) override;
    void hideEvent(QHideEvent * event) override;
    virtual void addCustomToolbar(const QString&);
    virtual void removeCustomToolbar(const QString&);
    virtual void renameCustomToolbar(const QString&, const QString&);
    virtual void addCustomCommand(const QString&, const QByteArray&);
    virtual void removeCustomCommand(const QString&, const QByteArray&);
    virtual void moveUpCustomCommand(const QString&, const QByteArray&);
    virtual void moveDownCustomCommand(const QString&, const QByteArray&);
    void onActivateCategoryBox();

private:
    void importCustomToolbars(const QByteArray&);
    void exportCustomToolbars(const QByteArray&);

protected:
    std::unique_ptr<Ui_DlgCustomToolbars> ui;
private:
    Type type;
    boost::signals2::scoped_connection conn;
};

/** This class implements the creation of user defined toolbars.
 * @see DlgCustomToolbars
 * @see DlgCustomCmdbarsImp
 * \author Werner Mayer
 */
class DlgCustomToolbarsImp : public DlgCustomToolbars
{
    Q_OBJECT

public:
    explicit DlgCustomToolbarsImp(QWidget* parent = nullptr);
    ~DlgCustomToolbarsImp() override;

protected:
    void showEvent(QShowEvent* e) override;
    void changeEvent(QEvent *e) override;
    void addCustomToolbar(const QString&) override;
    void removeCustomToolbar(const QString&) override;
    void renameCustomToolbar(const QString&, const QString&) override;
    void addCustomCommand(const QString&, const QByteArray&) override;
    void removeCustomCommand(const QString&, const QByteArray&) override;
    void moveUpCustomCommand(const QString&, const QByteArray&) override;
    void moveDownCustomCommand(const QString&, const QByteArray&) override;

private:
    QList<QAction*> getActionGroup(QAction*);
    void setActionGroup(QAction*, const QList<QAction*>& group);
    bool firstShow = true;
};

/** This class implements the creation of user defined toolbox bars.
 * A toolbox bar is the same as a toolbar - a collection of several
 * action objects - unless a toolbox bar is placed in a toolbox,
 * while a toolbar is placed in the dock areas of the main window.
 * So toolbox bars are predestinated to save place on your desktop.
 * @see DlgCustomToolbars
 * @see DlgCustomToolbarsImp
 * \author Werner Mayer
 */
class DlgCustomToolBoxbarsImp : public DlgCustomToolbars
{
    Q_OBJECT

public:
    explicit DlgCustomToolBoxbarsImp(QWidget* parent = nullptr);
    ~DlgCustomToolBoxbarsImp() override;

protected:
    void changeEvent(QEvent *e) override;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGTOOLBARS_IMP_H
