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

#include "ui_DlgToolbars.h"
#include "PropertyPage.h"

namespace Gui {
namespace Dialog {

/** This class implements the creation of user defined toolbars.
 * In the left panel are shown all command groups with their command objects.
 * If any changeable toolbar was created in the left panel are shown all commands 
 * of the currently edited toolbar, otherwise it is emtpy.
 * All changes to a toolbar is done immediately.
 * 
 * \author Werner Mayer
 */
class DlgCustomToolbars : public CustomizeActionPage, public Ui_DlgCustomToolbars
{ 
    Q_OBJECT

protected:
    enum Type { Toolbar, Toolboxbar };
    
    DlgCustomToolbars(Type, QWidget* parent = 0);
    virtual ~DlgCustomToolbars();

protected Q_SLOTS:
    void on_categoryBox_activated(int index);
    void on_workbenchBox_activated(int index);
    void on_moveActionRightButton_clicked();
    void on_moveActionLeftButton_clicked();
    void on_moveActionUpButton_clicked();
    void on_moveActionDownButton_clicked();
    void on_newButton_clicked();
    void on_renameButton_clicked();
    void on_deleteButton_clicked();
    void onAddMacroAction(const QByteArray&);
    void onRemoveMacroAction(const QByteArray&);
    void onModifyMacroAction(const QByteArray&);

protected:
    void changeEvent(QEvent *e);
    void hideEvent(QHideEvent * event);
    virtual void addCustomToolbar(const QString&);
    virtual void removeCustomToolbar(const QString&);
    virtual void renameCustomToolbar(const QString&, const QString&);
    virtual void addCustomCommand(const QString&, const QByteArray&);
    virtual void removeCustomCommand(const QString&, const QByteArray&);
    virtual void moveUpCustomCommand(const QString&, const QByteArray&);
    virtual void moveDownCustomCommand(const QString&, const QByteArray&);

private:
    void importCustomToolbars(const QByteArray&);
    void exportCustomToolbars(const QByteArray&);

private:
    Type type;
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
    DlgCustomToolbarsImp(QWidget* parent = 0);
    ~DlgCustomToolbarsImp();

protected:
    void changeEvent(QEvent *e);
    virtual void addCustomToolbar(const QString&);
    virtual void removeCustomToolbar(const QString&);
    virtual void renameCustomToolbar(const QString&, const QString&);
    virtual void addCustomCommand(const QString&, const QByteArray&);
    virtual void removeCustomCommand(const QString&, const QByteArray&);
    virtual void moveUpCustomCommand(const QString&, const QByteArray&);
    virtual void moveDownCustomCommand(const QString&, const QByteArray&);
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
    DlgCustomToolBoxbarsImp(QWidget* parent = 0);
    ~DlgCustomToolBoxbarsImp();

protected:
    void changeEvent(QEvent *e);
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGTOOLBARS_IMP_H
