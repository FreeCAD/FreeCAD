/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/
#ifndef GUI_DLGOBJECTSELECTION_H
#define GUI_DLGOBJECTSELECTION_H

#include <QDialog>
#include <QTimer>
#include <App/DocumentObserver.h>
#include <Base/Parameter.h>

class QCheckBox;

namespace Gui {

class Ui_DlgObjectSelection;
class GuiExport DlgObjectSelection : public QDialog
{
    Q_OBJECT

public:
    DlgObjectSelection(const std::vector<App::DocumentObject*> &objs,
            QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    DlgObjectSelection(const std::vector<App::DocumentObject*> &objs,
                       const std::vector<App::DocumentObject*> &excludes,
            QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgObjectSelection();

    std::vector<App::DocumentObject*> getSelections(bool invert=false, bool sort=false) const;
    void addCheckBox(QCheckBox *box);
    void setMessage(const QString &);

    void accept();
    void reject();

protected:
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void moveEvent(QMoveEvent *);
    void resizeEvent(QResizeEvent *);
    void closeEvent(QCloseEvent *);

private Q_SLOTS:
    void onDepItemChanged(QTreeWidgetItem * item, int);
    void onObjItemChanged(QTreeWidgetItem * item, int);
    void onItemSelectionChanged();
    void checkItemChanged();
    void onAutoDeps(bool);

private:
    QTreeWidgetItem *getItem(App::DocumentObject *obj);
    QTreeWidgetItem *createDepItem(QTreeWidget *parent, App::DocumentObject *obj);

    void init(const std::vector<App::DocumentObject*> &objs,
              const std::vector<App::DocumentObject*> &excludes);

    void setItemState(std::set<App::DocumentObject*> &set,
                      App::DocumentObject *obj,
                      Qt::CheckState state,
                      bool forced = false);
    bool checkItemState(std::map<App::DocumentObject*, Qt::CheckState> &visited,
                        App::DocumentObject *obj,
                        Qt::CheckState &s);
    void updateAllItemState();
    void saveGeometry();

private:
    Ui_DlgObjectSelection* ui;
    std::vector<App::DocumentObject*> initSels;
    std::vector<App::DocumentObject*> deps;
    std::set<App::DocumentObject*> depSet;
    std::map<App::SubObjectT, QTreeWidgetItem*> itemMap;
    std::map<App::SubObjectT, QTreeWidgetItem*> depMap;
    std::map<App::SubObjectT, QTreeWidgetItem*> inMap;
    std::map<App::SubObjectT, Qt::CheckState> itemChanged;
    QTreeWidgetItem *allItem = nullptr;
    bool geometryRestored = false;
    QSize savedSize;
    QPoint savedPos;

    QTimer timer;
    ParameterGrp::handle hGrp;
};

} // namespace Gui


#endif // GUI_DLGOBJECTSELECTION_H

