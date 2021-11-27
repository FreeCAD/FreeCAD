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

#include <set>
#include <unordered_map>
#include <boost_signals2.hpp>

#include <Base/Parameter.h>

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QDialog>
#include <QPointer>
#include <QAction>

namespace Gui {

class PrefWidgetStates;

namespace Dialog {

class Ui_DlgParameter;
class DlgParameterFind;
class ParameterGroup;
class ParameterGroupItem;
class ParameterValue;
class ParameterValueItem;

struct ParamKey {
    ParameterGrp::ParamType type;
    QByteArray name;

    ParamKey(ParameterGrp::ParamType t, const QString &n)
        :type(t), name(n.toUtf8())
    {}

    ParamKey(ParameterGrp::ParamType t, const char *n)
        :type(t), name(n)
    {}

    bool operator<(const ParamKey &other) const {
        if (type < other.type)
            return true;
        if (type > other.type)
            return false;
        if (name.size() < other.name.size())
            return true;
        if (name.size() > other.name.size())
            return false;
        return memcmp(name.constData(), other.name.constData(), name.size()) < 0;
    }
};

/**
 * The DlgParameterImp class implements a dialog showing all parameters in a list view.
 * \author Jürgen Riegel
 */
class GuiExport DlgParameterImp : public QWidget
{
    Q_OBJECT

public:
    DlgParameterImp( QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags() );
    ~DlgParameterImp();

    void activateParameterSet(const char*);

protected Q_SLOTS:
    void onChangeParameterSet(int);
    void on_buttonFind_clicked();
    void on_findGroupLE_textChanged(const QString &SearchStr);
    void on_buttonSaveToDisk_clicked();
    void on_btnExport_clicked();
    void on_btnImport_clicked();
    void on_btnAdd_clicked();
    void on_btnCopy_clicked();
    void on_btnRemove_clicked();
    void on_btnRename_clicked();
    void on_btnRefresh_clicked();
    void on_btnToolTip_clicked();
    void on_btnRestart_clicked();
    void on_btnReset_clicked();
    void on_checkBoxPreset_toggled(bool);
    void on_checkBoxMonitor_toggled(bool);
    void onParameterSetNameChanged();

    void onGroupItemChanged(QTreeWidgetItem *item, int col);
    void onValueItemChanged(QTreeWidgetItem *item, int col);

    void onGroupSelected(QTreeWidgetItem *);
    void on_closeButton_clicked();
    void on_checkSort_toggled(bool);

protected:
    void changeEvent(QEvent *e);

    void doImportOrMerge(ParameterGrp *hGrp, bool merge);
    ParameterGrp::handle copyParameters(ParameterManager *manager);
    bool doExport(ParameterGrp *hGrp);
    void updateGroupItemCheckState(QTreeWidgetItem *item);
    void setGroupItemState(ParameterGroupItem *item, Qt::CheckState state);
    void setValueItemState(ParameterValueItem *item, Qt::CheckState state);
    bool checkGroupItemState(ParameterGroupItem *item, Qt::CheckState state);
    void clearGroupItem(ParameterGroupItem *item,
        std::unordered_map<ParameterGrp*, std::set<ParamKey>> &changes);
    void saveState();
    void removeState();

    void slotParamChanged(ParameterGrp *Param,
                          ParameterGrp::ParamType type,
                          const char *Name,
                          const char *Value);

    void populate();

protected:
    friend class ParameterGroup;
    friend class ParameterGroupItem;
    friend class ParameterValue;
    friend class ParameterValueItem;
    ParameterGroup* paramGroup;
    ParameterValue* paramValue;
    Ui_DlgParameter* ui;
    QPointer<DlgParameterFind> finder;

    struct MonitorInfo {
        ParameterGrp::handle handle;
        boost::signals2::scoped_connection conn;
        std::unordered_map<ParameterGrp*, std::set<ParamKey>> changes;
    };
    std::map<ParameterManager*, MonitorInfo> monitors;
    Base::Reference<ParameterManager> curParamManager;

    bool isReadOnly(ParameterManager *mgr = nullptr);

private:
    QFont defaultFont;
    QBrush defaultColor;
    bool hasDefaultColor = false;
    QFont boldFont;
    QList<QTreeWidgetItem*> foundList;
    bool importing = false;
    bool geometryRestored = false;
    int lastIndex = -1;
    QAction actMerge;
    std::set<ParameterGrp*> warned;
    std::unique_ptr<PrefWidgetStates> widgetStates;
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
    ParameterGroup(DlgParameterImp *owner, QWidget *parent);
    virtual ~ParameterGroup();

    ParameterGroupItem *findItem(ParameterGrp *, bool create = false);
    void clear();

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
    void onMergeFromFile();
    /** Changes the name of the leaf of the selected item. */
    void onRenameSelectedItem();

protected:
    void changeEvent(QEvent *e);
    void applyLanguage();

    friend class ParameterGroupItem;
    friend class DlgParameterImp;

private:
    QMenu* menuEdit;
    QAction* expandAct;
    QAction* subGrpAct;
    QAction* removeAct;
    QAction* renameAct;
    QAction* exportAct;
    QAction* importAct;
    QAction* mergeAct;
    std::unordered_map<ParameterGrp*, ParameterGroupItem*> itemMap;
    DlgParameterImp *_owner;
};

// --------------------------------------------------------------------

class ParameterValueItem;

/**
 * The ParameterValue class displays all leaves of a parameter group. A leaf is represented
 * by the ParameterValueItem class.
 * @author Werner Mayer
 */
class ParameterValue : public QTreeWidget
{
    Q_OBJECT

public:
    ParameterValue(DlgParameterImp *owner, QWidget *parent);
    virtual ~ParameterValue();

    /** Sets the current parameter group that is displayed. */
    void setCurrentGroup( const Base::Reference<ParameterGrp>& _hcGrp );
    /** Returns the current parameter group that is displayed. */
    Base::Reference<ParameterGrp> currentGroup() const;

    ParameterValueItem * findItem(const ParamKey &key) const;
    
    void clear();

protected:
    /** Shows the context menu. */
    void contextMenuEvent ( QContextMenuEvent* event );
    /** Invokes onDeleteSelectedItem() if the "Del" key was pressed. */
    void keyPressEvent (QKeyEvent* event);
    void resizeEvent(QResizeEvent*);

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
    void onTouchItem();

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
    QAction* touchAct;
    Base::Reference<ParameterGrp> _hcGrp;

    friend class ParameterValueItem;
    friend class DlgParameterImp;

    std::map<ParamKey, ParameterValueItem*> itemMap;
    DlgParameterImp *_owner;
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
    ParameterGroupItem( ParameterGroup* parent, const Base::Reference<ParameterGrp> &hcGrp);
    ~ParameterGroupItem();

    void setData ( int column, int role, const QVariant & value );
    QVariant data ( int column, int role ) const;

    void fillUp(void);
    Base::Reference<ParameterGrp> _hcGrp;
    ParameterGroup *_owner;
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
    ParameterValueItem ( ParameterValue* parent,
                         ParameterGrp::ParamType type,
                         const QString &label,
                         const Base::Reference<ParameterGrp> &hcGrp);
    virtual ~ParameterValueItem();

    const ParamKey & getKey() const {return _key;}

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
    ParamKey _key;
    ParameterValue *_owner;
};

/**
 * The ParameterText class allows interaction with "text" parameter leaves.
 * @author Werner Mayer
 */
class ParameterText : public ParameterValueItem
{
public:
    /// Constructor
    ParameterText ( ParameterValue * parent, QString label1, const char* value, const Base::Reference<ParameterGrp> &hcGrp);
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
    ParameterInt ( ParameterValue * parent, QString label1, long value, const Base::Reference<ParameterGrp> &hcGrp);
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
    ParameterUInt ( ParameterValue * parent, QString label1, unsigned long value, const Base::Reference<ParameterGrp> &hcGrp);
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
    ParameterFloat ( ParameterValue * parent, QString label1, double value, const Base::Reference<ParameterGrp> &hcGrp);
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
    ParameterBool ( ParameterValue * parent, QString label1, bool value, const Base::Reference<ParameterGrp> &hcGrp);
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
