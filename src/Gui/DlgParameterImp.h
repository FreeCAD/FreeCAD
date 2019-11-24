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


#ifndef GUI_DIALOG_DLGPARAMETER_H
#define GUI_DIALOG_DLGPARAMETER_H

#include <Base/Parameter.h>

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QDialog>
#include <QPointer>

namespace Gui {
namespace Dialog {

class Ui_DlgParameter;
class DlgParameterFind;

/**
 * The DlgParameterImp class implements a dialog showing all parameters in a list view.
 * \author Jürgen Riegel
 */
class GuiExport DlgParameterImp : public QDialog
{ 
    Q_OBJECT

public:
    DlgParameterImp( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    ~DlgParameterImp();

    void accept();
    void reject();

    void activateParameterSet(const char*);

protected Q_SLOTS:
    void onChangeParameterSet(int);
    void on_buttonFind_clicked();
    void on_buttonSaveToDisk_clicked();

    void onGroupSelected(QTreeWidgetItem *);
    void on_closeButton_clicked();

protected:
    void changeEvent(QEvent *e);
    void showEvent(QShowEvent*);
    void closeEvent(QCloseEvent*);

protected:
    QTreeWidget* paramGroup;
    QTreeWidget* paramValue;
    Ui_DlgParameter* ui;
    QPointer<DlgParameterFind> finder;
};

// --------------------------------------------------------------------

/**
 * The ParameterGroup class represents the tree structure of the parameters.
 * The leaves represented by ParameterValueItem are displayed in ParameterValue.
 * @author Werner Mayer
 */
class ParameterGroup : public QTreeWidget
{
    Q_OBJECT

public:
    ParameterGroup( QWidget * parent = 0 );
    virtual ~ParameterGroup();

protected:
    /** Shows the context menu. */
    void contextMenuEvent ( QContextMenuEvent* event );
    /** Triggers the "Del" key. */
    void keyPressEvent (QKeyEvent* event);

protected Q_SLOTS:
    /** Removes the underlying parameter group and its sub-groups from the
     * parameter tree structure.
     */
    void onDeleteSelectedItem();
    /** Creates a sub-group to the current selected parameter group. */
    void onCreateSubgroup();
    /** Expands or closes the selected item. If it is open it will be closed and the
     * other way round.
     */
    void onToggleSelectedItem();
    /** Exports the current selected parameter with all sub-groups to a file. */
    void onExportToFile();
    /** Imports a file and inserts the parameter to the current selected parameter node. */
    void onImportFromFile();
    /** Changes the name of the leaf of the selected item. */
    void onRenameSelectedItem();

protected:
    void changeEvent(QEvent *e);

private:
    QMenu* menuEdit;
    QAction* expandAct;
    QAction* subGrpAct;
    QAction* removeAct;
    QAction* renameAct;
    QAction* exportAct;
    QAction* importAct;
};

// --------------------------------------------------------------------

/**
 * The ParameterValue class displays all leaves of a parameter group. A leaf is represented
 * by the ParameterValueItem class.
 * @author Werner Mayer
 */
class ParameterValue : public QTreeWidget
{
    Q_OBJECT

public:
    ParameterValue( QWidget * parent = 0 );
    virtual ~ParameterValue();

    /** Sets the current parameter group that is displayed. */
    void setCurrentGroup( const Base::Reference<ParameterGrp>& _hcGrp );
    /** Returns the current parameter group that is displayed. */
    Base::Reference<ParameterGrp> currentGroup() const;

protected:
    /** Shows the context menu. */
    void contextMenuEvent ( QContextMenuEvent* event );
    /** Invokes onDeleteSelectedItem() if the "Del" key was pressed. */
    void keyPressEvent (QKeyEvent* event);

protected Q_SLOTS:
    /** Changes the value of the leaf of the selected item. */
    void onChangeSelectedItem(QTreeWidgetItem*, int);
    void onChangeSelectedItem();
    /** Remove the underlying leaf from the parameter group. The selected item is also
     * removed and destroyed.
     */
    void onDeleteSelectedItem();
    /** Changes the name of the leaf of the selected item. */
    void onRenameSelectedItem();
    /** Creates and appends a new "text" leaf. */
    void onCreateTextItem();
    /** Creates and appends a new "integer" leaf. */
    void onCreateIntItem();
    /** Creates and appends a new "unsigned integer" leaf. */
    void onCreateUIntItem();
    /** Creates and appends a new "float" leaf. */
    void onCreateFloatItem();
    /** Creates and appends a new "boolean" leaf. */
    void onCreateBoolItem();
    /** Defines that the first column is editable. 
     * @note We need to reimplement this method as QTreeWidgetItem::flags()
     * doesn't have an int parameter.
     */
    bool edit ( const QModelIndex & index, EditTrigger trigger, QEvent * event );

private:
    QMenu* menuEdit;
    QMenu* menuNew;
    QAction* changeAct;
    QAction* removeAct;
    QAction* renameAct;
    QAction* newStrAct;
    QAction* newFltAct;
    QAction* newIntAct;
    QAction* newUlgAct;
    QAction* newBlnAct;
    Base::Reference<ParameterGrp> _hcGrp;
};

/** The link between the Tree and the shown Label.
 * Every (shown) Label in the FCDocument class get it 
 * associated FCTreeLabel which controls the visibility 
 * and the functions of the Label.
 *
 * \author Jürgen Riegel
 */
class ParameterGroupItem : public QTreeWidgetItem
{
public:
    /// Constructor
    ParameterGroupItem( ParameterGroupItem * parent, const Base::Reference<ParameterGrp> &hcGrp );
    ParameterGroupItem( QTreeWidget* parent, const Base::Reference<ParameterGrp> &hcGrp);
    ~ParameterGroupItem();

    void setData ( int column, int role, const QVariant & value );
    QVariant data ( int column, int role ) const;

    void fillUp(void);
    Base::Reference<ParameterGrp> _hcGrp;
};

// --------------------------------------------------------------------

/**
 * The ParameterValueItem class represents items that are added to the ParameterValue
 * listview. Each item represents a leaf in a parameter group and allows interaction
 * with this leaf, such as modifying its name, its value or even remove it from the 
 * parameter group.
 * @author Werner Mayer
 */
class ParameterValueItem : public QTreeWidgetItem
{
public:
    /// Constructor
    ParameterValueItem ( QTreeWidget* parent, const Base::Reference<ParameterGrp> &hcGrp);
    virtual ~ParameterValueItem();

    /** If the name of the item has changed replace() is invoked. */
    virtual void setData ( int column, int role, const QVariant & value );
    /** Opens an input dialog to change the value. */
    virtual void changeValue() = 0;
    /** Append this item as leaf to the parameter group. */
    virtual void appendToGroup() = 0;
    /** Remove the leaf from the parameter group. */
    virtual void removeFromGroup() = 0;

protected:
    /** Replaces the name of the leaf from \a oldName to \a newName. */
    virtual void replace( const QString& oldName, const QString& newName ) = 0;

protected:
    Base::Reference<ParameterGrp> _hcGrp;
};

/**
 * The ParameterText class allows interaction with "text" parameter leaves.
 * @author Werner Mayer
 */
class ParameterText : public ParameterValueItem
{
public:
    /// Constructor
    ParameterText ( QTreeWidget * parent, QString label1, const char* value, const Base::Reference<ParameterGrp> &hcGrp);
    ~ParameterText();

    void changeValue();
    void appendToGroup();
    void removeFromGroup();

protected:
    void replace( const QString& oldName, const QString& newName );
};

/**
 * The ParameterInt class allows interaction with "integer" parameter leaves.
 * @author Werner Mayer
 */
class ParameterInt : public ParameterValueItem
{
public:
    /// Constructor
    ParameterInt ( QTreeWidget * parent, QString label1, long value, const Base::Reference<ParameterGrp> &hcGrp);
    ~ParameterInt();

    void changeValue();
    void appendToGroup();
    void removeFromGroup();

protected:
    void replace( const QString& oldName, const QString& newName );
};

/**
 * The ParameterUInt class allows interaction with "unsigned integer" parameter leaves.
 * @author Werner Mayer
 */
class ParameterUInt : public ParameterValueItem
{
public:
    /// Constructor
    ParameterUInt ( QTreeWidget * parent, QString label1, unsigned long value, const Base::Reference<ParameterGrp> &hcGrp);
    ~ParameterUInt();

    void changeValue();
    void appendToGroup();
    void removeFromGroup();

protected:
    void replace( const QString& oldName, const QString& newName );
};

/**
 * The ParameterFloat class allows interaction with "float" parameter leaves.
 * @author Werner Mayer
 */
class ParameterFloat : public ParameterValueItem
{
public:
    /// Constructor
    ParameterFloat ( QTreeWidget * parent, QString label1, double value, const Base::Reference<ParameterGrp> &hcGrp);
    ~ParameterFloat();

    void changeValue();
    void appendToGroup();
    void removeFromGroup();

protected:
    void replace( const QString& oldName, const QString& newName );
};

/**
 * The ParameterBool class allows interaction with "boolean" parameter leaves.
 * @author Werner Mayer
 */
class ParameterBool : public ParameterValueItem
{
public:
    /// Constructor
    ParameterBool ( QTreeWidget * parent, QString label1, bool value, const Base::Reference<ParameterGrp> &hcGrp);
    ~ParameterBool();

    void changeValue();
    void appendToGroup();
    void removeFromGroup();

protected:
    void replace( const QString& oldName, const QString& newName );
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGPARAMETER_H
