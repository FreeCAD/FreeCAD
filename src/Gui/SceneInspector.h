/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_SCENEINSPECTOR_H
#define GUI_SCENEINSPECTOR_H

#include <QStandardItemModel>
#include <QDialog>
#include <QHash>
#include "InventorBase.h"

class SoNode;

namespace Gui {
class Document;
namespace Dialog {

class Ui_SceneInspector;
class DlgInspector;

/// Stores data representing scenegraph nodes.
class SceneModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SceneModel(QObject* parent);
    virtual ~SceneModel();

    /** returns empty QVariant, unless orientation == Qt::Horizontal,
     *  role == Qt::DisplayRole and section == 0 where it returns
     *  "Inventor Tree"
     */
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    /// header data not used: returns false
    bool setHeaderData (int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole);
    /// insert the first node in tree
    void setNode(SoNode* node);
    /// set names per node
    void setNodeNames(const QHash<SoNode*, QString>& names);

    virtual QModelIndex parent(const QModelIndex & index) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int, const QModelIndex &parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &) const;

private:

    friend class DlgInspector;

    struct Item {
        QModelIndex parent;
        CoinPtr<SoNode> node;
        bool expand = true;
    };
    mutable QMap<QModelIndex, Item> items;
    Item rootItem;

    QHash<SoNode*, QString> nodeNames;
    bool autoExpanding = false;
};

/// Dialog window to display scenegraph model as a tree
class DlgInspector : public QDialog
{
    Q_OBJECT

public:
    DlgInspector(QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgInspector();

    void setDocument(Gui::Document* doc);

private Q_SLOTS:
    void on_refreshButton_clicked();

protected:
    void changeEvent(QEvent *e);
    void setNode(SoNode* node);
    void setNodeNames(Gui::Document*);

private:
    Ui_SceneInspector* ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_SCENEINSPECTOR_H
