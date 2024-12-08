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

    connect(ui->spb_rot_axis_x,
            qOverload<double>(&DoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintTransform::xAxisChanged);
    connect(ui->spb_rot_axis_y,
            qOverload<double>(&DoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintTransform::yAxisChanged);
    connect(ui->spb_rot_axis_z,
            qOverload<double>(&DoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintTransform::zAxisChanged);
    connect(ui->qsb_rot_angle,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintTransform::angleChanged);

    // Get the feature data
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    Base::Vector3d axis;
    double angle;
    pcConstraint->Rotation.getValue().getValue(axis, angle);
    ui->spb_rot_axis_x->setValue(axis.x);
    ui->spb_rot_axis_y->setValue(axis.y);
    ui->spb_rot_axis_z->setValue(axis.z);
    Base::Quantity rotAngle(angle, QString::fromUtf8("rad"));
    ui->qsb_rot_angle->setValue(rotAngle.getValueAs(Base::Quantity::Degree));

    ui->spb_rot_axis_x->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Axis.x")));
    ui->spb_rot_axis_y->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Axis.y")));
    ui->spb_rot_axis_z->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Axis.z")));
    ui->qsb_rot_angle->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Angle")));

    ui->spb_rot_axis_x->setMinimum(-FLOAT_MAX);
    ui->spb_rot_axis_x->setMaximum(FLOAT_MAX);
    ui->spb_rot_axis_y->setMinimum(-FLOAT_MAX);
    ui->spb_rot_axis_y->setMaximum(FLOAT_MAX);
    ui->spb_rot_axis_z->setMinimum(-FLOAT_MAX);
    ui->spb_rot_axis_z->setMaximum(FLOAT_MAX);
    ui->qsb_rot_angle->setMinimum(-FLOAT_MAX);
    ui->qsb_rot_angle->setMaximum(FLOAT_MAX);

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
    Gui::Command::doCommand(Gui::Command::Doc,
                            TaskFemConstraintTransform::getSurfaceReferences(
                                (ConstraintView->getObject<Fem::Constraint>())->getNameInDocument())
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

void TaskFemConstraintTransform::xAxisChanged(double x)
{
    (void)x;
    Base::Rotation rot = getRotation();
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
    pcConstraint->Rotation.setValue(rot);
}

void TaskFemConstraintTransform::yAxisChanged(double y)
{
    (void)y;
    Base::Rotation rot = getRotation();
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
    pcConstraint->Rotation.setValue(rot);
}

void TaskFemConstraintTransform::zAxisChanged(double z)
{
    (void)z;
    Base::Rotation rot = getRotation();
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
    pcConstraint->Rotation.setValue(rot);
}

void TaskFemConstraintTransform::angleChanged(double a)
{
    (void)a;
    Base::Rotation rot = getRotation();
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
    pcConstraint->Rotation.setValue(rot);
}

void TaskFemConstraintTransform::Rect()
{
    ui->sw_transform->setCurrentIndex(0);
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.TransformType = %s",
                            name.c_str(),
                            get_transform_type().c_str());
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    if (!Objects.empty()) {
        setSelection(ui->lw_Rect->item(0));
        removeFromSelection();
    }
}

void TaskFemConstraintTransform::Cyl()
{
    ui->sw_transform->setCurrentIndex(1);
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.TransformType = %s",
                            name.c_str(),
                            get_transform_type().c_str());
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
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

    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
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
        Base::Rotation rot(Base::Vector3d(0, 0, 1), normal);
        Base::Vector3d axis;
        double angle;
        rot.getValue(axis, angle);
        ui->spb_rot_axis_x->setValue(axis.x);
        ui->spb_rot_axis_y->setValue(axis.y);
        ui->spb_rot_axis_z->setValue(axis.z);
        Base::Quantity rotAngle(angle, QString::fromUtf8("rad"));
        ui->qsb_rot_angle->setValue(rotAngle.getValueAs(Base::Quantity::Degree));
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
    Fem::ConstraintTransform* pcConstraint = ConstraintView->getObject<Fem::ConstraintTransform>();
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
    ui->spb_rot_axis_x->setValue(0);
    ui->spb_rot_axis_y->setValue(0);
    ui->spb_rot_axis_z->setValue(1);
    ui->qsb_rot_angle->setValue(0);
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

Base::Rotation TaskFemConstraintTransform::getRotation() const
{
    double x = ui->spb_rot_axis_x->value();
    double y = ui->spb_rot_axis_y->value();
    double z = ui->spb_rot_axis_z->value();
    double angle = ui->qsb_rot_angle->value().getValueAs(Base::Quantity::Radian);

    return Base::Rotation(Base::Vector3d(x, y, z), angle);
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

bool TaskDlgFemConstraintTransform::accept()
{
    /* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintTransform* parameters =
        static_cast<const TaskFemConstraintTransform*>(parameter);

    try {
        Base::Rotation rot = parameters->getRotation();
        Base::Vector3d axis;
        double angle;
        rot.getValue(axis, angle);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.ActiveDocument.%s.Rotation = App.Rotation(App.Vector(%f,% f, %f), Radian=%f)",
            name.c_str(),
            axis.x,
            axis.y,
            axis.z,
            angle);

        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.TransformType = %s",
                                name.c_str(),
                                parameters->get_transform_type().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    /* */
    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintTransform.cpp"
