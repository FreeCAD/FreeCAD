/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTDESIGNGUI_TASKCHAMFER_H
#define PARTDESIGNGUI_TASKCHAMFER_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <QStandardItemModel>
#include <QItemDelegate>
#include <Mod/Part/Gui/ViewProvider.h>

namespace PartDesignGui {

class Ui_TaskChamfer;
class ChamferDistanceDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ChamferDistanceDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor, 
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class ChamferDistanceModel : public QStandardItemModel
{
    Q_OBJECT

public:
    ChamferDistanceModel(QObject * parent = 0);

    Qt::ItemFlags flags (const QModelIndex & index) const;
    bool setData (const QModelIndex & index, const QVariant & value,
                  int role = Qt::EditRole);
Q_SIGNALS:
    void toggleCheckState(const QModelIndex&);
};

class ChamferWidgetP;
class ChamferWidget : public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    ChamferWidget(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~ChamferWidget();
    bool accept();

protected:
    void findShapes();
    void changeEvent(QEvent *e);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void onDeleteObject(const App::DocumentObject&);
    void onDeleteDocument(const App::Document&);

private Q_SLOTS:
    void on_shapeObject_activated(int);
    void on_selectAllButton_clicked();
    void on_selectNoneButton_clicked();
    void on_chamferType_activated(int);
    void on_chamferStartDistance_valueChanged(double);
    void on_chamferEndDistance_valueChanged(double);
    void toggleCheckState(const QModelIndex&);

private:
    std::auto_ptr<Ui_TaskChamfer> ui;
    std::auto_ptr<ChamferWidgetP> d;
};

class TaskChamfer : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskChamfer();
    ~TaskChamfer();

public:
    virtual void open();
    virtual void clicked(int);
    virtual bool accept();
    virtual bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    ChamferWidget* widget;
};

class ViewProviderChamfer : public PartGui::ViewProviderPart
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderChamfer);

public:
    /// constructor
    ViewProviderChamfer();
    /// destructor
    virtual ~ViewProviderChamfer();
};

} // namespace PartDesignGui

#endif // PARTDESIGNGUI_TASKCHAMFER_H
