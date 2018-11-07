/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_SEGMENTATIONBESTFIT_H
#define MESHGUI_SEGMENTATIONBESTFIT_H

#include <QWidget>
#include <list>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include "MeshSelection.h"

class QDoubleSpinBox;

// forward declarations
namespace Mesh { class Feature; }

namespace MeshGui {
class Ui_SegmentationBestFit;

class FitParameter
{
public:
    typedef std::vector<Base::Vector3f> Points;
    virtual ~FitParameter() {}
    virtual std::vector<float> getParameter(Points) const = 0;
};

typedef std::list<std::pair<QString, float> > ParameterList;
class ParametersDialog : public QDialog
{
    Q_OBJECT

public:
    ParametersDialog(std::vector<float>&, FitParameter*,
                     ParameterList, Mesh::Feature* mesh,
                     QWidget* parent=0);
    ~ParametersDialog();
    void accept();
    void reject();

private Q_SLOTS:
    void on_region_clicked();
    void on_single_clicked();
    void on_clear_clicked();
    void on_compute_clicked();

private:
    std::vector<float>& values;
    FitParameter* fitParameter;
    ParameterList parameter;
    Mesh::Feature* myMesh;
    MeshSelection meshSel;
    std::vector<QDoubleSpinBox*> spinBoxes;
};

class MeshGuiExport SegmentationBestFit : public QWidget
{
    Q_OBJECT

public:
    SegmentationBestFit(Mesh::Feature* mesh, QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~SegmentationBestFit();
    void accept();

protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void on_planeParameters_clicked();
    void on_cylinderParameters_clicked();
    void on_sphereParameters_clicked();

private:
    std::vector<float> planeParameter;
    std::vector<float> cylinderParameter;
    std::vector<float> sphereParameter;
    Ui_SegmentationBestFit* ui;
    Mesh::Feature* myMesh;
    MeshSelection meshSel;
};

/**
 * Embed the panel into a task dialog.
 */
class TaskSegmentationBestFit : public Gui::TaskView::TaskDialog
{
public:
    TaskSegmentationBestFit(Mesh::Feature* mesh);
    ~TaskSegmentationBestFit();

public:
    bool accept();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

private:
    SegmentationBestFit* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}

#endif // MESHGUI_SEGMENTATIONBESTFIT_H
