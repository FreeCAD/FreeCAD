/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
 *            Ofentse Kgoa <kgoaot@eskom.co.za>                            *
 *   Based on Force constraint by Jan Rheinländer                          *
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
#include <BRepAdaptor_Surface.hxx>
#include <QAction>
#include <QMessageBox>
#include <TopoDS.hxx>
#include <sstream>
#endif

#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintTransform.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintTransform.h"
#include "ui_TaskFemConstraintTransform.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintTransform */

TaskFemConstraintTransform::TaskFemConstraintTransform(
    ViewProviderFemConstraintTransform* ConstraintView,
    QWidget* parent)
    : TaskFemConstraint(ConstraintView, parent, "FEM_ConstraintTransform")
    , ui(new Ui_TaskFemConstraintTransform)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_Rect);
    connect(deleteAction,
            &QAction::triggered,
            this,
            &TaskFemConstraintTransform::onReferenceDeleted);

    // highlight seletcted list items in the model
    connect(ui->lw_Rect,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintTransform::setSelection);
    connect(ui->lw_Rect,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintTransform::setSelection);
    connect(ui->lw_displobj_rect,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintTransform::setSelection);
    connect(ui->lw_displobj_rect,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintTransform::setSelection);

    this->groupLayout()->addWidget(proxy);

    connect(ui->rb_rect, &QRadioButton::clicked, this, &TaskFemConstraintTransform::Rect);
    connect(ui->rb_cylin, &QRadioButton::clicked, this, &TaskFemConstraintTransform::Cyl);

    connect(ui->sp_X,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintTransform::x_Changed);
    connect(ui->sp_Y,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintTransform::y_Changed);
    connect(ui->sp_Z,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintTransform::z_Changed);

    // Get the feature data
    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->sp_X->setValue(pcConstraint->X_rot.getQuantityValue());
    ui->sp_Y->setValue(pcConstraint->Y_rot.getQuantityValue());
    ui->sp_Z->setValue(pcConstraint->Z_rot.getQuantityValue());
    std::string transform_type = pcConstraint->TransformType.getValueAsString();
    if (transform_type == "Rectangular") {
        ui->sw_transform->setCurrentIndex(0);
        ui->rb_rect->setChecked(true);
        ui->rb_cylin->setChecked(false);
    }
    else if (transform_type == "Cylindrical") {
        ui->sw_transform->setCurrentIndex(1);
        ui->rb_rect->setChecked(false);
        ui->rb_cylin->setChecked(true);
    }

    ui->lw_Rect->clear();

    // Transformable surfaces
    Gui::Command::doCommand(
        Gui::Command::Doc,
        TaskFemConstraintTransform::getSurfaceReferences(
            (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
            .c_str());
    std::vector<App::DocumentObject*> ObjDispl = pcConstraint->RefDispl.getValues();
    std::vector<std::string> SubElemDispl = pcConstraint->RefDispl.getSubValues();

    for (std::size_t i = 0; i < ObjDispl.size(); i++) {
        ui->lw_displobj_rect->addItem(makeRefText(ObjDispl[i], SubElemDispl[i]));
        ui->lw_displobj_cylin->addItem(makeRefText(ObjDispl[i], SubElemDispl[i]));
    }

    std::vector<App::DocumentObject*> nDispl = pcConstraint->NameDispl.getValues();
    for (auto i : nDispl) {
        ui->lw_dis_rect->addItem(makeText(i));
        ui->lw_dis_cylin->addItem(makeText(i));
    }

    if (!Objects.empty()) {
        for (std::size_t i = 0; i < Objects.size(); i++) {
            ui->lw_Rect->addItem(makeRefText(Objects[i], SubElements[i]));
        }
    }
    int p = 0;
    for (std::size_t i = 0; i < ObjDispl.size(); i++) {
        for (std::size_t j = 0; j < Objects.size(); j++) {
            if ((makeRefText(ObjDispl[i], SubElemDispl[i]))
                == (makeRefText(Objects[j], SubElements[j]))) {
                p++;
            }
        }
    }
    // Selection buttons
    connect(ui->btnAdd, &QToolButton::clicked, this, &TaskFemConstraintTransform::addToSelection);
    connect(ui->btnRemove,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintTransform::removeFromSelection);

    // Bind input fields to properties
    ui->sp_X->bind(pcConstraint->X_rot);
    ui->sp_Y->bind(pcConstraint->Y_rot);
    ui->sp_Z->bind(pcConstraint->Z_rot);

    updateUI();

    if ((p == 0) && (!Objects.empty())) {
        QMessageBox::warning(this,
                             tr("Analysis feature update error"),
                             tr("The transformable faces have changed. Please add only the "
                                "transformable faces and remove non-transformable faces!"));
        return;
    }
}

TaskFemConstraintTransform::~TaskFemConstraintTransform() = default;

const QString TaskFemConstraintTransform::makeText(const App::DocumentObject* obj) const
{
    return QString::fromUtf8((std::string(obj->getNameInDocument())).c_str());
}

void TaskFemConstraintTransform::updateUI()
{
    if (ui->lw_Rect->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintTransform::x_Changed(int i)
{
    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());
    double x = i;
    pcConstraint->X_rot.setValue(x);
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.X_rot = %f", name.c_str(), x);
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    pcConstraint->References.setValues(Objects, SubElements);
}

void TaskFemConstraintTransform::y_Changed(int i)
{
    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());
    double y = i;
    pcConstraint->Y_rot.setValue(y);
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Y_rot = %f", name.c_str(), y);
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    pcConstraint->References.setValues(Objects, SubElements);
}

void TaskFemConstraintTransform::z_Changed(int i)
{
    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());
    double z = i;
    pcConstraint->Z_rot.setValue(z);
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Z_rot = %f", name.c_str(), z);
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    pcConstraint->References.setValues(Objects, SubElements);
}

void TaskFemConstraintTransform::Rect()
{
    ui->sw_transform->setCurrentIndex(0);
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.TransformType = %s",
                            name.c_str(),
                            get_transform_type().c_str());
    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    if (!Objects.empty()) {
        setSelection(ui->lw_Rect->item(0));
        removeFromSelection();
    }
}

void TaskFemConstraintTransform::Cyl()
{
    ui->sw_transform->setCurrentIndex(1);
    ui->sp_X->setValue(0);
    ui->sp_Y->setValue(0);
    ui->sp_Z->setValue(0);
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.TransformType = %s",
                            name.c_str(),
                            get_transform_type().c_str());
    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    if (!Objects.empty()) {
        setSelection(ui->lw_Rect->item(0));
        removeFromSelection();
    }
}

void TaskFemConstraintTransform::addToSelection()
{
    int rows = ui->lw_Rect->model()->rowCount();
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    if (rows == 1) {
        QMessageBox::warning(this,
                             tr("Selection error"),
                             tr("Only one face for rectangular local coordinate system!"));
        Gui::Selection().clearSelection();
        return;
    }

    if ((rows == 0) && (selection.size() >= 2)) {
        QMessageBox::warning(this,
                             tr("Selection error"),
                             tr("Only one face for rectangular local coordinate system!"));
        Gui::Selection().clearSelection();
        return;
    }

    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    std::vector<App::DocumentObject*> ObjDispl = pcConstraint->RefDispl.getValues();
    std::vector<std::string> SubElemDispl = pcConstraint->RefDispl.getSubValues();
    for (auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        App::DocumentObject* obj = it.getObject();
        if (subNames.size() != 1) {
            QMessageBox::warning(this,
                                 tr("Selection error"),
                                 tr("Only one face for local coordinate system!"));
            Gui::Selection().clearSelection();
            return;
        }
        for (const auto& subName : subNames) {  // for every selected sub element
            bool addMe = true;
            if (subName.substr(0, 4) != "Face") {
                QMessageBox::warning(this, tr("Selection error"), tr("Only faces can be picked"));
                return;
            }
            if (subName.substr(0, 4) == "Face") {
                if (ui->rb_cylin->isChecked()) {
                    Part::Feature* feat = static_cast<Part::Feature*>(obj);
                    TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());
                    BRepAdaptor_Surface surface(TopoDS::Face(ref));
                    if (surface.GetType() != GeomAbs_Cylinder) {
                        QMessageBox::warning(this,
                                             tr("Selection error"),
                                             tr("Only cylindrical faces can be picked"));
                        return;
                    }
                }
            }
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subName);
                 itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subName)) {  // for every sub element in selection that
                                              // matches one in old list
                if (obj
                    == Objects[std::distance(
                        SubElements.begin(),
                        itr)]) {  // if selected sub element's object equals the one in old list
                                  // then it was added before so don't add
                    addMe = false;
                }
            }
            if (addMe) {
                disconnect(ui->lw_Rect,
                           &QListWidget::currentItemChanged,
                           this,
                           &TaskFemConstraintTransform::setSelection);
                for (std::size_t i = 0; i < ObjDispl.size(); i++) {
                    if ((makeRefText(ObjDispl[i], SubElemDispl[i]))
                        == (makeRefText(obj, subName))) {
                        Objects.push_back(obj);
                        SubElements.push_back(subName);
                        ui->lw_Rect->addItem(makeRefText(obj, subName));
                        connect(ui->lw_Rect,
                                &QListWidget::currentItemChanged,
                                this,
                                &TaskFemConstraintTransform::setSelection);
                    }
                }
                if (Objects.empty()) {
                    QMessageBox::warning(
                        this,
                        tr("Selection error"),
                        tr("Only transformable faces can be selected! Apply displacement boundary "
                           "condition to surface first then apply local coordinate system to "
                           "surface"));
                    Gui::Selection().clearSelection();
                    return;
                }
            }
        }
    }
    // Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
    if (ui->rb_rect->isChecked()) {
        Base::Vector3d normal = pcConstraint->NormalDirection.getValue();
        double n = normal.x;
        double m = normal.y;
        double l = normal.z;
        // about Z-axis
        double about_z;
        double mag_norm_z = sqrt(n * n + m * m);  // normal vector mapped onto XY plane
        if (mag_norm_z == 0) {
            about_z = 0;
        }
        else {
            about_z = (-1 * (acos(m / mag_norm_z) * 180 / M_PI) + 180);
        }
        if (n > 0) {
            about_z = about_z * (-1);
        }
        // rotation to ZY plane
        double m_p = n * sin(about_z * M_PI / 180) + m * cos(about_z * M_PI / 180);
        double l_p = l;
        // about X-axis
        double about_x;
        double mag_norm_x = sqrt(m_p * m_p + l_p * l_p);
        if (mag_norm_x == 0) {
            about_x = 0;
        }
        else {
            about_x = -(acos(l_p / mag_norm_x) * 180 / M_PI);  // rotation to the Z axis
        }
        ui->sp_X->setValue(round(about_x));
        ui->sp_Z->setValue(round(about_z));
    }
}

void TaskFemConstraintTransform::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintTransform* pcConstraint =
        static_cast<Fem::ConstraintTransform*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (const auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        const App::DocumentObject* obj = it.getObject();

        for (const auto& subName : subNames) {  // for every selected sub element
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subName);
                 itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subName)) {  // for every sub element in selection that
                                              // matches one in old list
                if (obj
                    == Objects[std::distance(
                        SubElements.begin(),
                        itr)]) {  // if selected sub element's object equals the one in old list
                                  // then it was added before so mark for deletion
                    itemsToDel.push_back(std::distance(SubElements.begin(), itr));
                }
            }
        }
    }
    std::sort(itemsToDel.begin(), itemsToDel.end());
    while (!itemsToDel.empty()) {
        Objects.erase(Objects.begin() + itemsToDel.back());
        SubElements.erase(SubElements.begin() + itemsToDel.back());
        itemsToDel.pop_back();
    }
    // Update UI
    {
        QSignalBlocker block(ui->lw_Rect);
        ui->lw_Rect->clear();
    }
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
    ui->sp_X->setValue(0);
    ui->sp_Y->setValue(0);
    ui->sp_Z->setValue(0);
}

const std::string TaskFemConstraintTransform::getReferences() const
{
    std::vector<std::string> items;
    int rowsRect = ui->lw_Rect->model()->rowCount();
    for (int r = 0; r < rowsRect; r++) {
        items.push_back(ui->lw_Rect->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

void TaskFemConstraintTransform::onReferenceDeleted()
{
    TaskFemConstraintTransform::removeFromSelection();
}

std::string TaskFemConstraintTransform::getSurfaceReferences(std::string showConstr = "")
// https://forum.freecad.org/viewtopic.php?f=18&t=43650
{
    return "\n\
doc = FreeCAD.ActiveDocument\n\
for obj in doc.Objects:\n\
        if obj.isDerivedFrom(\"Fem::FemAnalysis\"):\n\
                if doc."
        + showConstr + " in obj.Group:\n\
                        analysis = obj\n\
A = []\n\
i = 0\n\
ss = []\n\
for member in analysis.Group:\n\
        if ((member.isDerivedFrom(\"Fem::ConstraintDisplacement\")) or (member.isDerivedFrom(\"Fem::ConstraintForce\"))) and len(member.References) > 0:\n\
                m = member.References\n\
                A.append(m)\n\
                if i >0:\n\
                        p = p + m[0][1]\n\
                        x = (A[0][0][0],p)\n\
                        for t in range(len(m[0][1])):\n\
                                ss.append(member)\n\
                else:\n\
                        p = A[i][0][1]\n\
                        x = (A[0][0][0],p)\n\
                        for t in range(len(m[0][1])):\n\
                                ss.append(member)\n\
                i = i+1\n\
if i>0:\n\
        doc."
        + showConstr + ".RefDispl = [x]\n\
        doc."
        + showConstr + ".NameDispl = ss\n\
else:\n\
        doc."
        + showConstr + ".RefDispl = None\n\
        doc."
        + showConstr + ".NameDispl = []\n";
}

std::string TaskFemConstraintTransform::get_X_rot() const
{
    return ui->sp_X->value().getSafeUserString().toStdString();
}
std::string TaskFemConstraintTransform::get_Y_rot() const
{
    return ui->sp_Y->value().getSafeUserString().toStdString();
}
std::string TaskFemConstraintTransform::get_Z_rot() const
{
    return ui->sp_Z->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintTransform::get_transform_type() const
{
    std::string transform;
    if (ui->rb_rect->isChecked()) {
        transform = "\"Rectangular\"";
    }
    else if (ui->rb_cylin->isChecked()) {
        transform = "\"Cylindrical\"";
    }
    return transform;
}

bool TaskFemConstraintTransform::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintTransform::changeEvent(QEvent*)
{}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintTransform::TaskDlgFemConstraintTransform(
    ViewProviderFemConstraintTransform* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintTransform(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintTransform::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Local coordinate system");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            ViewProviderFemConstraint::gethideMeshShowPartStr(
                (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
                .c_str());  // OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintTransform::accept()
{
    /* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintTransform* parameters =
        static_cast<const TaskFemConstraintTransform*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.X_rot = \"%s\"",
                                name.c_str(),
                                parameters->get_X_rot().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Y_rot = \"%s\"",
                                name.c_str(),
                                parameters->get_Y_rot().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Z_rot = \"%s\"",
                                name.c_str(),
                                parameters->get_Z_rot().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.TransformType = %s",
                                name.c_str(),
                                parameters->get_transform_type().c_str());
        std::string scale = parameters->getScale();  // OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Scale = %s",
                                name.c_str(),
                                scale.c_str());  // OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    /* */
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintTransform::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintTransform.cpp"
