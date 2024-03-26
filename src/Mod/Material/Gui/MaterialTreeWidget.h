/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MATGUI_MATERIALTREEWIDGET_H
#define MATGUI_MATERIALTREEWIDGET_H

#include <memory>

#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>

#include <FCGlobal.h>

#include <Base/Parameter.h>
#include <Gui/WidgetFactory.h>


#include <Mod/Material/App/MaterialFilter.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>

namespace MatGui
{
class CommandManager;
class WidgetFactoryInst;

/** The Material Tree widget class
 * This widget is intended for use wherever materials are used. It is a light weight
 * alternative to the full Materials editor.
 *
 * The widget itself is the combination of a number of smaller widgets. A simple text
 * field shows any currently selected material. An arrow will expand a tree to show
 * the widget library, allowing the user to select the material they require.
 *
 * When expanded, the user will be presented the option to launch the full material
 * editor. This will allow them to create/copy/modify as required.
 *
 * Additionally, they will be given the option to create a material card based on the
 * current settings.
 *
 * \author David Carter
 */
class MatGuiExport MaterialTreeWidget: public QWidget
{
    Q_OBJECT

public:
    explicit MaterialTreeWidget(std::shared_ptr<Materials::MaterialFilter> filter,
                                QWidget* parent = nullptr);
    explicit MaterialTreeWidget(QWidget* parent = nullptr);
    ~MaterialTreeWidget() override;

    //   void setEntryName( const QByteArray& name );
    //   QByteArray entryName() const;
    //   /** Does the same as setEntryName().
    //    * This function is added for convenience because the ui compiler
    //    * will use this function if the attribute stdset isn't set to 0 in a .ui file.
    //    */
    //   void setPrefEntry(const QByteArray& name);

    //   void setParamGrpPath( const QByteArray& path );
    //   QByteArray paramGrpPath() const;
    //   /** Does the same as setParamGrpPath().
    //    * This function is added for convenience because the ui compiler
    //    * will use this function if the attribute stdset isn't set to 0 in a .ui file.
    //    */
    //   void setPrefPath(const QByteArray& name);

    //   void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;
    //   void onSave();
    //   void onRestore();

    /** Set the material by specifying its UUID
     */
    void setMaterial(const QString& uuid);
    /** get the material UUID
     */
    QString getMaterialUUID() const;
    /** Set the material filter
     */
    void setFilter(std::shared_ptr<Materials::MaterialFilter> filter);

Q_SIGNALS:
    /** Emits this signal when a material has been selected */
    void materialSelected(const std::shared_ptr<Materials::Material>& material);

private Q_SLOTS:
    void expandClicked(bool checked);
    void editorClicked(bool checked);
    void onSelectMaterial(const QItemSelection& selected, const QItemSelection& deselected);
    void onDoubleClick(const QModelIndex& index);

private:
    void setup();

    QLineEdit* m_material;
    QPushButton* m_expand;
    QTreeView* m_materialTree;
    QPushButton* m_editor;
    bool m_expanded;

    QString m_materialDisplay;
    QString m_uuid;

    std::list<QString> _favorites;
    std::list<QString> _recents;
    std::shared_ptr<Materials::MaterialFilter> _filter;
    int _recentMax;

    Materials::MaterialManager _materialManager;

    // friends
    friend class Gui::WidgetFactoryInst;

protected:
    //   bool m_Restored = false;

    Materials::MaterialManager& getMaterialManager()
    {
        return _materialManager;
    }

    void getFavorites();
    void getRecents();

    /** Create the widgets UI objects
     */
    void createLayout();

    bool findInTree(const QStandardItem& node, QModelIndex* index, const QString& uuid);
    QModelIndex findInTree(const QString& uuid);
    void updateMaterial(const QString& uuid);
    void createMaterialTree();
    void fillMaterialTree();
    void updateMaterialTree();
    void addExpanded(QStandardItem* parent, QStandardItem* child);
    void addExpanded(QStandardItemModel* model, QStandardItem* child);
    void addRecents(QStandardItem* parent);
    void addFavorites(QStandardItem* parent);
    void addMaterials(
        QStandardItem& parent,
        const std::shared_ptr<std::map<QString, std::shared_ptr<Materials::MaterialTreeNode>>>&
            modelTree,
        const QIcon& folderIcon,
        const QIcon& icon);

    void openWidgetState(bool open);
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALTREEWIDGET_H