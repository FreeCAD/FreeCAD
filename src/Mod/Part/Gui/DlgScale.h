/***************************************************************************
 *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
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

#ifndef PARTGUI_DLGSCALE_H
#define PARTGUI_DLGSCALE_H

#include <QDialog>
#include <string>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/FeatureScale.h>

class TopoDS_Shape;

namespace PartGui {

class Ui_DlgScale;
class DlgScale : public QDialog
{
    Q_OBJECT

public:
    DlgScale(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgScale() = default;
    void accept() override;
    void apply();
    void reject() override;

    std::vector<App::DocumentObject*> getShapesToScale() const;

    bool validate();

    void writeParametersToFeature(App::DocumentObject& feature, App::DocumentObject* base) const;

protected:
    void findShapes();
    bool canScale(const TopoDS_Shape&) const;
    void changeEvent(QEvent *e) override;

private:
    void setupConnections();
    void onUniformScaleToggled(bool on);

private:
    ///returns link to any of selected source shapes. Throws if nothing is selected for scaling.
    App::DocumentObject& getShapeToScale() const;

    std::unique_ptr<Ui_DlgScale> ui;
    std::string m_document, m_label;
};

class TaskScale : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskScale();

public:
    bool accept() override;
    bool reject() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Close; }

private:
    DlgScale* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui

#endif // PARTGUI_DLGSCALE_H
