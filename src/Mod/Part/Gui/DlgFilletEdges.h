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

#ifndef PARTGUI_DLGFILLETEDGES_H
#define PARTGUI_DLGFILLETEDGES_H

#include <QDialog>
#include <QItemDelegate>
#include <QStandardItemModel>

#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace Base {
    class Quantity;
}
namespace Part {
    class FilletBase;
    class Fillet;
    class Chamfer;
}

namespace PartGui {

class Ui_DlgFilletEdges;
class FilletRadiusDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit FilletRadiusDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class FilletRadiusModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit FilletRadiusModel(QObject * parent = nullptr);

    Qt::ItemFlags flags (const QModelIndex & index) const override;
    bool setData (const QModelIndex & index, const QVariant & value,
                  int role = Qt::EditRole) override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    void updateCheckStates();

Q_SIGNALS:
    void toggleCheckState(const QModelIndex&);
};

class DlgFilletEdges : public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    enum FilletType { FILLET, CHAMFER };

    DlgFilletEdges(FilletType type, Part::FilletBase*, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgFilletEdges() override;
    bool accept();

protected:
    void findShapes();
    void setupFillet(const std::vector<App::DocumentObject*>&);
    void changeEvent(QEvent *e) override;
    virtual const char* getFilletType() const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void onDeleteObject(const App::DocumentObject&);
    void onDeleteDocument(const App::Document&);
    void onSelectEdge(const QString& subelement, int type);
    void onSelectEdgesOfFace(const QString& subelement, int type);

private Q_SLOTS:
    void on_shapeObject_activated(int);
    void on_selectEdges_toggled(bool);
    void on_selectFaces_toggled(bool);
    void on_selectAllButton_clicked();
    void on_selectNoneButton_clicked();
    void on_filletType_activated(int);
    void on_filletStartRadius_valueChanged(const Base::Quantity&);
    void on_filletEndRadius_valueChanged(const Base::Quantity&);
    void toggleCheckState(const QModelIndex&);
    void onHighlightEdges();

private:
    std::unique_ptr<Ui_DlgFilletEdges> ui;
    class Private;
    std::unique_ptr<Private> d;
};

class FilletEdgesDialog : public QDialog
{
    Q_OBJECT

public:
    FilletEdgesDialog(DlgFilletEdges::FilletType type, Part::FilletBase* fillet, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~FilletEdgesDialog() override;
    void accept() override;

private:
    DlgFilletEdges* widget;
};

class DlgChamferEdges : public DlgFilletEdges
{
    Q_OBJECT

public:
    explicit DlgChamferEdges(Part::FilletBase*, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgChamferEdges() override;

protected:
    const char* getFilletType() const override;
};

class TaskFilletEdges : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskFilletEdges(Part::Fillet*);
    ~TaskFilletEdges() override;

public:
    void open() override;
    void clicked(int) override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }
    bool needsFullSpace() const override
    { return true; }

private:
    DlgFilletEdges* widget;
    Gui::TaskView::TaskBox* taskbox;
};

class TaskChamferEdges : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskChamferEdges(Part::Chamfer*);
    ~TaskChamferEdges() override;

public:
    void open() override;
    void clicked(int) override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }
    bool needsFullSpace() const override
    { return true; }

private:
    DlgChamferEdges* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui

#endif // PARTGUI_DLGFILLETEDGES_H
