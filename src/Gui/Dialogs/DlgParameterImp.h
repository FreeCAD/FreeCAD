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

#include <QDialog>
#include <QPointer>
#include <QTreeWidget>
#include <QTreeWidgetItem>


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
    explicit DlgParameterImp( QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags() );
    ~DlgParameterImp() override;

    void accept() override;
    void reject() override;

    void activateParameterSet(const char*);

protected:
    void setupConnections();
    void onChangeParameterSet(int);
    void onButtonFindClicked();
    void onFindGroupTtextChanged(const QString &SearchStr);
    void onButtonSaveToDiskClicked();

    void onGroupSelected(QTreeWidgetItem *);
    void onCloseButtonClicked();
    void onCheckSortToggled(bool);

protected:
    void changeEvent(QEvent *e) override;
    void showEvent(QShowEvent*) override;
    void closeEvent(QCloseEvent*) override;

protected:
    QTreeWidget* paramGroup;
    QTreeWidget* paramValue;
    Ui_DlgParameter* ui;
    QPointer<DlgParameterFind> finder;

private:
    QFont defaultFont;
    QBrush defaultColor;
    QFont boldFont;
    QList<QTreeWidgetItem*> foundList;
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
    explicit ParameterGroup( QWidget * parent = nullptr );
    ~ParameterGroup() override;

protected:
    /** Shows the context menu. */
    void contextMenuEvent ( QContextMenuEvent* event ) override;
    /** Triggers the "Del" key. */
    void keyPressEvent (QKeyEvent* event) override;

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
    void changeEvent(QEvent *e) override;

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
    explicit ParameterValue( QWidget * parent = nullptr );
    ~ParameterValue() override;

    /** Sets the current parameter group that is displayed. */
    void setCurrentGroup( const Base::Reference<ParameterGrp>& _hcGrp );
    /** Returns the current parameter group that is displayed. */
    Base::Reference<ParameterGrp> currentGroup() const;

protected:
    /** Shows the context menu. */
    void contextMenuEvent ( QContextMenuEvent* event ) override;
    /** Invokes onDeleteSelectedItem() if the "Del" key was pressed. */
    void keyPressEvent (QKeyEvent* event) override;
    void resizeEvent(QResizeEvent*) override;

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
    bool edit ( const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event ) override;

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
    ~ParameterGroupItem() override;

    void setData ( int column, int role, const QVariant & value ) override;
    QVariant data ( int column, int role ) const override;

    void fillUp();
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
    ~ParameterValueItem() override;

    /** If the name of the item has changed replace() is invoked. */
    void setData ( int column, int role, const QVariant & value ) override;
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
    ~ParameterText() override;

    void changeValue() override;
    void appendToGroup() override;
    void removeFromGroup() override;

protected:
    void replace( const QString& oldName, const QString& newName ) override;
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
    ~ParameterInt() override;

    void changeValue() override;
    void appendToGroup() override;
    void removeFromGroup() override;

protected:
    void replace( const QString& oldName, const QString& newName ) override;
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
    ~ParameterUInt() override;

    void changeValue() override;
    void appendToGroup() override;
    void removeFromGroup() override;

protected:
    void replace( const QString& oldName, const QString& newName ) override;
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
    ~ParameterFloat() override;

    void changeValue() override;
    void appendToGroup() override;
    void removeFromGroup() override;

protected:
    void replace( const QString& oldName, const QString& newName ) override;
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
    ~ParameterBool() override;

    void changeValue() override;
    void appendToGroup() override;
    void removeFromGroup() override;

protected:
    void replace( const QString& oldName, const QString& newName ) override;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGPARAMETER_H
