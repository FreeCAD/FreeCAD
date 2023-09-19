/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMessageBox>
#include <algorithm>
#endif

#include <App/ComplexGeoData.h>
#include <App/Document.h>
#include <App/Placement.h>
#include <Base/Converter.h>
#include <Base/CoordinateSystem.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>
#include <Mod/Mesh/App/Core/Approximation.h>

#include "FitBSplineSurface.h"
#include "ui_FitBSplineSurface.h"


using namespace ReenGui;

class FitBSplineSurfaceWidget::Private
{
public:
    Ui_FitBSplineSurface ui;
    App::DocumentObjectT obj;
    Private() = default;
    ~Private() = default;
};

/* TRANSLATOR ReenGui::FitBSplineSurfaceWidget */

FitBSplineSurfaceWidget::FitBSplineSurfaceWidget(const App::DocumentObjectT& obj, QWidget* parent)
    : d(new Private())
{
    Q_UNUSED(parent);
    d->ui.setupUi(this);
    connect(d->ui.makePlacement,
            &QPushButton::clicked,
            this,
            &FitBSplineSurfaceWidget::onMakePlacementClicked);
    d->obj = obj;
    restoreSettings();
}

FitBSplineSurfaceWidget::~FitBSplineSurfaceWidget()
{
    saveSettings();
    delete d;
}

void FitBSplineSurfaceWidget::restoreSettings()
{
    d->ui.degreeU->onRestore();
    d->ui.polesU->onRestore();
    d->ui.degreeV->onRestore();
    d->ui.polesV->onRestore();
    d->ui.iterations->onRestore();
    d->ui.sizeFactor->onRestore();
    d->ui.totalWeight->onRestore();
    d->ui.gradient->onRestore();
    d->ui.bending->onRestore();
    d->ui.curvature->onRestore();
    d->ui.uvdir->onRestore();
}

void FitBSplineSurfaceWidget::saveSettings()
{
    d->ui.degreeU->onSave();
    d->ui.polesU->onSave();
    d->ui.degreeV->onSave();
    d->ui.polesV->onSave();
    d->ui.iterations->onSave();
    d->ui.sizeFactor->onSave();
    d->ui.totalWeight->onSave();
    d->ui.gradient->onSave();
    d->ui.bending->onSave();
    d->ui.curvature->onSave();
    d->ui.uvdir->onSave();
}

void FitBSplineSurfaceWidget::onMakePlacementClicked()
{
    try {
        App::GeoFeature* geo = d->obj.getObjectAs<App::GeoFeature>();
        if (geo) {
            const App::PropertyComplexGeoData* geom = geo->getPropertyOfGeometry();
            if (geom) {
                std::vector<Base::Vector3d> points, normals;
                geom->getComplexData()->getPoints(points, normals, 0.001);

                std::vector<Base::Vector3f> data;
                std::transform(points.begin(),
                               points.end(),
                               std::back_inserter(data),
                               [](const Base::Vector3d& v) {
                                   return Base::convertTo<Base::Vector3f>(v);
                               });
                MeshCore::PlaneFit fit;
                fit.AddPoints(data);
                if (fit.Fit() < FLOAT_MAX) {
                    Base::Vector3f base = fit.GetBase();
                    Base::Vector3f dirU = fit.GetDirU();
                    Base::Vector3f norm = fit.GetNormal();

                    Base::CoordinateSystem cs;
                    cs.setPosition(Base::convertTo<Base::Vector3d>(base));
                    cs.setAxes(Base::convertTo<Base::Vector3d>(norm),
                               Base::convertTo<Base::Vector3d>(dirU));
                    Base::Placement pm = Base::CoordinateSystem().displacement(cs);
                    double q0, q1, q2, q3;
                    pm.getRotation().getValue(q0, q1, q2, q3);

                    QString argument = QString::fromLatin1("Base.Placement(Base.Vector(%1, %2, "
                                                           "%3), Base.Rotation(%4, %5, %6, %7))")
                                           .arg(base.x)
                                           .arg(base.y)
                                           .arg(base.z)
                                           .arg(q0)
                                           .arg(q1)
                                           .arg(q2)
                                           .arg(q3);

                    QString document = QString::fromStdString(d->obj.getDocumentPython());
                    QString command =
                        QString::fromLatin1(
                            R"(%1.addObject("App::Placement", "Placement").Placement = %2)")
                            .arg(document, argument);

                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Placement"));
                    Gui::Command::runCommand(Gui::Command::Doc, "from FreeCAD import Base");
                    Gui::Command::runCommand(Gui::Command::Doc, command.toLatin1());
                    Gui::Command::commitCommand();
                    Gui::Command::updateActive();
                }
            }
        }
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        QMessageBox::warning(this, tr("Input error"), QString::fromLatin1(e.what()));
    }
}

bool FitBSplineSurfaceWidget::accept()
{
    try {
        QString document = QString::fromStdString(d->obj.getDocumentPython());
        QString object = QString::fromStdString(d->obj.getObjectPython());

        QString argument =
            QString::fromLatin1("Points=getattr(%1, %1.getPropertyNameOfGeometry()), "
                                "UDegree=%2, VDegree=%3, "
                                "NbUPoles=%4, NbVPoles=%5, "
                                "Smooth=%6, "
                                "Weight=%7, "
                                "Grad=%8, "
                                "Bend=%9, "
                                "Curv=%10, "
                                "Iterations=%11, "
                                "PatchFactor=%12, "
                                "Correction=True")
                .arg(object)
                .arg(d->ui.degreeU->value())
                .arg(d->ui.degreeV->value())
                .arg(d->ui.polesU->value())
                .arg(d->ui.polesV->value())
                .arg(d->ui.groupBoxSmooth->isChecked() ? QLatin1String("True")
                                                       : QLatin1String("False"))
                .arg(d->ui.totalWeight->value())
                .arg(d->ui.gradient->value())
                .arg(d->ui.bending->value())
                .arg(d->ui.curvature->value())
                .arg(d->ui.iterations->value())
                .arg(d->ui.sizeFactor->value());
        if (d->ui.uvdir->isChecked()) {
            std::vector<App::Placement*> selection =
                Gui::Selection().getObjectsOfType<App::Placement>();
            if (selection.size() != 1) {
                QMessageBox::warning(
                    this,
                    tr("Wrong selection"),
                    tr("Please select a single placement object to get local orientation."));
                return false;
            }

            Base::Rotation rot = selection.front()->GeoFeature::Placement.getValue().getRotation();
            Base::Vector3d u(1, 0, 0);
            Base::Vector3d v(0, 1, 0);
            rot.multVec(u, u);
            rot.multVec(v, v);
            argument +=
                QString::fromLatin1(", UVDirs=(FreeCAD.Vector(%1,%2,%3), FreeCAD.Vector(%4,%5,%6))")
                    .arg(u.x)
                    .arg(u.y)
                    .arg(u.z)
                    .arg(v.x)
                    .arg(v.y)
                    .arg(v.z);
        }
        QString command = QString::fromLatin1("%1.addObject(\"Part::Spline\", \"Spline\").Shape = "
                                              "ReverseEngineering.approxSurface(%2).toShape()")
                              .arg(document, argument);

        Gui::WaitCursor wc;
        Gui::Command::addModule(Gui::Command::App, "ReverseEngineering");
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Fit B-Spline"));
        Gui::Command::runCommand(Gui::Command::Doc, command.toLatin1());
        Gui::Command::commitCommand();
        Gui::Command::updateActive();
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        QMessageBox::warning(this, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

void FitBSplineSurfaceWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
}


/* TRANSLATOR ReenGui::TaskFitBSplineSurface */

TaskFitBSplineSurface::TaskFitBSplineSurface(const App::DocumentObjectT& obj)
{
    widget = new FitBSplineSurfaceWidget(obj);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/FitSurface"),
                                         widget->windowTitle(),
                                         true,
                                         nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskFitBSplineSurface::open()
{}

bool TaskFitBSplineSurface::accept()
{
    return widget->accept();
}

#include "moc_FitBSplineSurface.cpp"
