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


#include "PreCompiled.h"
#include <QSignalMapper>
#include <QDockWidget>
#include <QMessageBox>
#include <QClipboard>
#include <QMetaObject>

#include "Placement.h"
#include "ui_Placement.h"
#include <Gui/DockWindowManager.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/Window.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

using namespace Gui::Dialog;

namespace Gui { namespace Dialog {
class find_placement
{
public:
    find_placement(const std::string& name) : propertyname(name)
    {
    }
    bool operator () (const std::pair<std::string, App::Property*>& elem) const
    {
        if (elem.first == propertyname) {
            //  flag set that property is read-only or hidden
            if (elem.second->testStatus(App::Property::ReadOnly) || elem.second->testStatus(App::Property::Hidden))
                return false;
            App::PropertyContainer* parent = elem.second->getContainer();
            if (parent) {
                //  flag set that property is read-only or hidden
                if (parent->isReadOnly(elem.second) ||
                    parent->isHidden(elem.second))
                    return false;
            }
            return elem.second->isDerivedFrom
                (Base::Type::fromName("App::PropertyPlacement"));
        }

        return false;
    }

    std::string propertyname;
};

}
}

/* TRANSLATOR Gui::Dialog::Placement */

Placement::Placement(QWidget* parent, Qt::WindowFlags fl)
  : Gui::LocationDialog(parent, fl)
  , changeProperty(false)
{
    selectionObjects = Gui::Selection().getSelectionEx();

    propertyName = "Placement"; // default name
    ui = new Ui_PlacementComp(this);

    ui->xPos->setUnit(Base::Unit::Length);
    ui->yPos->setUnit(Base::Unit::Length);
    ui->zPos->setUnit(Base::Unit::Length);
    ui->axialPos->setUnit(Base::Unit::Length);
    ui->xCnt->setValue(Base::Quantity(0, Base::Unit::Length));
    ui->yCnt->setValue(Base::Quantity(0, Base::Unit::Length));
    ui->zCnt->setValue(Base::Quantity(0, Base::Unit::Length));
    ui->angle->setUnit(Base::Unit::Angle);
    ui->yawAngle->setUnit(Base::Unit::Angle);
    ui->pitchAngle->setUnit(Base::Unit::Angle);
    ui->rollAngle->setUnit(Base::Unit::Angle);

    // create a signal mapper in order to have one slot to perform the change
    signalMapper = new QSignalMapper(this);
    connect(this, SIGNAL(directionChanged()), signalMapper, SLOT(map()));
    signalMapper->setMapping(this, 0);

    int id = 1;
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (QList<Gui::QuantitySpinBox*>::iterator it = sb.begin(); it != sb.end(); ++it) {
        connect(*it, SIGNAL(valueChanged(double)), signalMapper, SLOT(map()));
        signalMapper->setMapping(*it, id++);
    }

    connect(signalMapper, SIGNAL(mapped(int)),
            this, SLOT(onPlacementChanged(int)));
    connectAct = Application::Instance->signalActiveDocument.connect
        (boost::bind(&Placement::slotActiveDocument, this, _1));
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (activeDoc) documents.insert(activeDoc->getName());

    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Placement");
    long index = hGrp->GetInt("RotationMethod");
    ui->rotationInput->setCurrentIndex(index);
    ui->stackedWidget->setCurrentIndex(index);
}

Placement::~Placement()
{
    connectAct.disconnect();
    delete ui;
}

void Placement::showDefaultButtons(bool ok)
{
    ui->oKButton->setVisible(ok);
    ui->closeButton->setVisible(ok);
    ui->applyButton->setVisible(ok);
}

void Placement::open()
{
    if (propertyName != "Placement") {
        changeProperty = true;
        openTransaction();
    }
}

void Placement::slotActiveDocument(const Gui::Document& doc)
{
    documents.insert(doc.getDocument()->getName());

    if (changeProperty) {
        QMetaObject::invokeMethod(this, "openTransaction", Qt::QueuedConnection);
    }
}

void Placement::openTransaction()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (activeDoc)
        activeDoc->openTransaction("Placement");
}

QWidget* Placement::getInvalidInput() const
{
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (QList<Gui::QuantitySpinBox*>::iterator it = sb.begin(); it != sb.end(); ++it) {
        if (!(*it)->hasValidInput())
            return (*it);
    }
    return 0;
}

void Placement::revertTransformation()
{
    for (std::set<std::string>::iterator it = documents.begin(); it != documents.end(); ++it) {
        Gui::Document* document = Application::Instance->getDocument(it->c_str());
        if (!document) continue;

        if (!changeProperty) {
            std::vector<App::DocumentObject*> obj = document->getDocument()->
                getObjectsOfType(App::DocumentObject::getClassTypeId());
            if (!obj.empty()) {
                for (std::vector<App::DocumentObject*>::iterator it=obj.begin();it!=obj.end();++it) {
                    std::map<std::string,App::Property*> props;
                    (*it)->getPropertyMap(props);
                    // search for the placement property
                    std::map<std::string,App::Property*>::iterator jt;
                    jt = std::find_if(props.begin(), props.end(), find_placement(this->propertyName));
                    if (jt != props.end()) {
                        App::PropertyPlacement* property = static_cast<App::PropertyPlacement*>(jt->second);
                        Base::Placement cur = property->getValue();
                        Gui::ViewProvider* vp = document->getViewProvider(*it);
                        if (vp) vp->setTransformation(cur.toMatrix());
                    }
                }
            }
        }
        else {
            document->abortCommand();
        }
    }
}

void Placement::applyPlacement(const Base::Placement& p, bool incremental)
{
    Gui::Document* document = Application::Instance->activeDocument();
    if (!document) return;

    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
        (App::DocumentObject::getClassTypeId(), document->getDocument()->getName());
    if (!sel.empty()) {
        // apply transformation only on view matrix not on placement property
        for (std::vector<App::DocumentObject*>::iterator it=sel.begin();it!=sel.end();++it) {
            std::map<std::string,App::Property*> props;
            (*it)->getPropertyMap(props);
            // search for the placement property
            std::map<std::string,App::Property*>::iterator jt;
            jt = std::find_if(props.begin(), props.end(), find_placement(this->propertyName));
            if (jt != props.end()) {
                App::PropertyPlacement* property = static_cast<App::PropertyPlacement*>(jt->second);
                Base::Placement cur = property->getValue();
                if (incremental)
                    cur = p * cur;
                else
                    cur = p;

                if (!changeProperty) {
                    Gui::ViewProvider* vp = document->getViewProvider(*it);
                    if (vp) vp->setTransformation(cur.toMatrix());
                }
                else {
                    property->setValue(cur);
                }
            }
        }
    }
    else {
        Base::Console().Warning("No object selected.\n");
    }
}

void Placement::applyPlacement(const QString& data, bool incremental)
{
    Gui::Document* document = Application::Instance->activeDocument();
    if (!document) return;

    // When directly changing the property we now only have to commit the transaction,
    // do a recompute and open a new transaction
    if (changeProperty) {
        document->commitCommand();
        try {
            document->getDocument()->recompute();
        }
        catch (...) {
        }
        document->openCommand("Placement");
    }
    else {
        std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
            (App::DocumentObject::getClassTypeId(), document->getDocument()->getName());
        if (!sel.empty()) {
            document->openCommand("Placement");
            for (std::vector<App::DocumentObject*>::iterator it=sel.begin();it!=sel.end();++it) {
                std::map<std::string,App::Property*> props;
                (*it)->getPropertyMap(props);
                // search for the placement property
                std::map<std::string,App::Property*>::iterator jt;
                jt = std::find_if(props.begin(), props.end(), find_placement(this->propertyName));
                if (jt != props.end()) {
                    QString cmd;
                    if (incremental)
                        cmd = QString::fromLatin1(
                            "App.getDocument(\"%1\").%2.%3=%4.multiply(App.getDocument(\"%1\").%2.%3)")
                            .arg(QLatin1String((*it)->getDocument()->getName()))
                            .arg(QLatin1String((*it)->getNameInDocument()))
                            .arg(QLatin1String(this->propertyName.c_str()))
                            .arg(data);
                    else {
                        cmd = QString::fromLatin1(
                            "App.getDocument(\"%1\").%2.%3=%4")
                            .arg(QLatin1String((*it)->getDocument()->getName()))
                            .arg(QLatin1String((*it)->getNameInDocument()))
                            .arg(QLatin1String(this->propertyName.c_str()))
                            .arg(data);
                    }

                    Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
                }
            }

            document->commitCommand();
            try {
                document->getDocument()->recompute();
            }
            catch (...) {
            }
        }
        else {
            Base::Console().Warning("No object selected.\n");
        }
    }
}

void Placement::onPlacementChanged(int)
{
    // If there are listeners to the 'placementChanged' signal we rely
    // on that the listener updates any placement. If no listeners
    // are connected the placement is applied to all selected objects
    // automatically.
    bool incr = ui->applyIncrementalPlacement->isChecked();
    Base::Placement plm = this->getPlacement();
    applyPlacement(plm, incr);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    /*emit*/ placementChanged(data, incr, false);
}

void Placement::on_centerOfMass_toggled(bool on)
{
    ui->xCnt->setDisabled(on);
    ui->yCnt->setDisabled(on);
    ui->zCnt->setDisabled(on);

    if (on) {
        cntOfMass.Set(0,0,0);
        std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
            (App::GeoFeature::getClassTypeId());
        if (!sel.empty()) {
            for (auto it : sel) {
                const App::PropertyComplexGeoData* propgeo = static_cast<App::GeoFeature*>(it)->getPropertyOfGeometry();
                const Data::ComplexGeoData* geodata = propgeo ? propgeo->getComplexData() : nullptr;
                if (geodata && geodata->getCenterOfGravity(cntOfMass)) {
                    break;
                }
            }
        }

        ui->xCnt->setValue(cntOfMass.x);
        ui->yCnt->setValue(cntOfMass.y);
        ui->zCnt->setValue(cntOfMass.z);
    }
}

void Placement::on_selectedVertex_clicked()
{
    cntOfMass.Set(0,0,0);
    ui->centerOfMass->setChecked(false);

    bool success=false;
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    std::vector<Base::Vector3d> picked;
    //combine all pickedpoints into single vector
    //even if points are from separate objects
    Base::Vector3d firstSelected; //first selected will be central point when 3 points picked
    for (std::vector<Gui::SelectionObject>::iterator it=selection.begin(); it!=selection.end(); ++it){
        std::vector<Base::Vector3d> points = it->getPickedPoints();
        if (it==selection.begin() && points.size()>0){
            firstSelected=points[0];
        }
        picked.insert(picked.begin(),points.begin(),points.end());
    }
    //we have to clear selection and reselect original object(s)
    //else later on the rotation is applied twice because there will
    //be 2 (vertex) objects in the selection, and even if both are subobjects
    //of the same object the rotation still gets applied twice
    Gui::Selection().clearSelection();
    //reselect original object that was selected when placement dlg first opened
    for (auto it : selectionObjects)
        Gui::Selection().addSelection(it);

    if (picked.size() == 1) {
        ui->xCnt->setValue(picked[0].x);
        ui->yCnt->setValue(picked[0].y);
        ui->zCnt->setValue(picked[0].z);
        cntOfMass.x=picked[0].x;
        cntOfMass.y=picked[0].y;
        cntOfMass.z=picked[0].z;
        success=true;
    }
    else if (picked.size() == 2) {
        //average the coords to get center of rotation
        ui->xCnt->setValue((picked[0].x+picked[1].x)/2.0);
        ui->yCnt->setValue((picked[0].y+picked[1].y)/2.0);
        ui->zCnt->setValue((picked[0].z+picked[1].z)/2.0);
        cntOfMass.x=(picked[0].x+picked[1].x)/2.0;
        cntOfMass.y=(picked[0].y+picked[1].y)/2.0;
        cntOfMass.z=(picked[0].z+picked[1].z)/2.0;
        //setup a customized axis since the user selected 2 points
        //keep any existing angle, but setup our own axis
        Base::Placement plm = getPlacement();
        Base::Rotation rot = plm.getRotation();
        Base::Vector3d tmp;
        double angle;
        rot.getRawValue(tmp, angle);
        Base::Vector3d axis;
        if (firstSelected==picked[0]){
            axis = Base::Vector3d(picked[1]-picked[0]);
        }
        else {
            axis = Base::Vector3d(picked[0]-picked[1]);
        }
        double length = axis.Length();
        Base::Console().Message("Distance: %.8f\n",length);
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier){ //copy to clipboard on Shift+click
            QLocale loc;
            QApplication::clipboard()->setText(loc.toString(length,'g',8));
        }
        else {
            Base::Console().Message("(Shift + click Selected points button to copy distance to clipboard)\n");
        }
        axis.Normalize();
        rot.setValue(axis, angle);
        plm.setRotation(rot);
        setPlacementData(plm); //creates custom axis, if needed
        ui->rotationInput->setCurrentIndex(0); //use rotation with axis instead of euler
        ui->stackedWidget->setCurrentIndex(0);
        success=true;
    }
    else if (picked.size() == 3){
        /* User selected 3 points, so we find the plane defined by those
         * and use the normal vector that contains the first point picked
         * as the axis of rotation.
         */

        Base::Vector3d a, b(firstSelected), c; //b is on central axis
        if (picked[0] == firstSelected){
            a = picked[1];
            c = picked[2];
        }
        else if (picked[1]==firstSelected){
            a = picked[0];
            c = picked[2];
        }
        else if (picked[2] == firstSelected){
            a = picked[0];
            c = picked[1];
        }

        Base::Vector3d norm((a-b).Cross(c-b));
        norm.Normalize();
        ui->xCnt->setValue(b.x);
        ui->yCnt->setValue(b.y);
        ui->zCnt->setValue(b.z);
        cntOfMass.x=b.x;
        cntOfMass.y=b.y;
        cntOfMass.z=b.z;
        //setup a customized axis normal to the plane
        //keep any existing angle, but setup our own axis
        Base::Placement plm = getPlacement();
        Base::Rotation rot = plm.getRotation();
        Base::Vector3d tmp;
        double angle;
        rot.getRawValue(tmp, angle);
        double length = (a-c).Length();
        Base::Console().Message("Distance: %.8f\n",length);
        Base::Vector3d v1(a-b);
        Base::Vector3d v2(c-b);
        v1.Normalize();
        v2.Normalize();
        double targetAngle = Base::toDegrees(v2.GetAngle(v1));
        Base::Console().Message("Target angle: %.8f degrees, complementary: %.8f degrees\n",targetAngle, 90.0-targetAngle);
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier){ //copy to clipboard on Shift+click
            QLocale loc;
            QApplication::clipboard()->setText(loc.toString(targetAngle,'g',8));
            Base::Console().Message("(Angle copied to clipboard, but you might need to use a negative (-) angle sometimes.)\n");
        }
        else {
            Base::Console().Message("(Shift + click Selected points button to copy angle to clipboard)\n");
        }
        rot.setValue(norm, angle);
        plm.setRotation(rot);
        setPlacementData(plm); //creates custom axis, if needed
        ui->rotationInput->setCurrentIndex(0); //use rotation with axis instead of euler
        ui->stackedWidget->setCurrentIndex(0);
        success=true;
    }

    if (!success){
        Base::Console().Warning("Placement selection error.  Select either 1 or 2 points.\n");
        QMessageBox msgBox;
        msgBox.setText(tr("Please select 1, 2, or 3 points before clicking this button.  A point may be on a vertex, \
face, or edge.  If on a face or edge the point used will be the point at the mouse position along \
face or edge.  If 1 point is selected it will be used as the center of rotation.  If 2 points are \
selected the midpoint between them will be the center of rotation and a new custom axis will be \
created, if needed.  If 3 points are selected the first point becomes the center of rotation and \
lies on the vector that is normal to the plane defined by the 3 points.  Some distance and angle \
information is provided in the report view, which can be useful when aligning objects.  For your \
convenience when Shift + click is used the appropriate distance or angle is copied to the clipboard."));
        msgBox.exec();
        ui->xCnt->setValue(0);
        ui->yCnt->setValue(0);
        ui->zCnt->setValue(0);
        return;
    }
}

void Placement::on_applyAxial_clicked()
{
    signalMapper->blockSignals(true);
    double axPos = ui->axialPos->value().getValue();
    Base::Placement p = getPlacementData();
    double angle;
    Base::Vector3d axis;
    p.getRotation().getValue(axis, angle);
    Base::Vector3d curPos (p.getPosition());
    Base::Vector3d newPos;
    Qt::KeyboardModifiers km = QApplication::keyboardModifiers();
    if (km == Qt::ShiftModifier){ //go opposite direction on Shift+click
        newPos = Base::Vector3d(curPos.x-(axis.x*axPos),curPos.y-(axis.y*axPos),curPos.z-(axis.z*axPos));
    }
    else {
        newPos = Base::Vector3d(curPos.x+(axis.x*axPos),curPos.y+(axis.y*axPos),curPos.z+(axis.z*axPos));
    }
    ui->xPos->setValue(Base::Quantity(newPos.x,Base::Unit::Length));
    ui->yPos->setValue(Base::Quantity(newPos.y,Base::Unit::Length));
    ui->zPos->setValue(Base::Quantity(newPos.z,Base::Unit::Length));
    signalMapper->blockSignals(false);
    onPlacementChanged(0);
}

void Placement::on_applyIncrementalPlacement_toggled(bool on)
{
    if (on) {
        this->ref = getPlacementData();
        on_resetButton_clicked();
    }
    else {
        Base::Placement p = getPlacementData();
        p = p * this->ref;
        setPlacementData(p);
        onPlacementChanged(0);
    }
}

void Placement::keyPressEvent(QKeyEvent* ke)
{
    // The placement dialog is embedded into a task panel
    // which is a parent widget and will handle the event
    ke->ignore();
}

void Placement::reject()
{
    Base::Placement plm;
    applyPlacement(plm, true);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    /*emit*/ placementChanged(data, true, false);

    revertTransformation();
    QDialog::reject();
}

void Placement::accept()
{
    if (onApply()) {
        revertTransformation();
        QDialog::accept();
    }
}

void Placement::on_applyButton_clicked()
{
    onApply();
}

bool Placement::onApply()
{
    //only process things when we have valid inputs!
    QWidget* input = getInvalidInput();
    if (input) {
        input->setFocus();
        QMessageBox msg(this);
        msg.setWindowTitle(tr("Incorrect quantity"));
        msg.setIcon(QMessageBox::Critical);
        msg.setText(tr("There are input fields with incorrect input, please ensure valid placement values!"));
        msg.exec();
        return false;
    }

    // If there are listeners to the 'placementChanged' signal we rely
    // on that the listener updates any placement. If no listeners
    // are connected the placement is applied to all selected objects
    // automatically.
    bool incr = ui->applyIncrementalPlacement->isChecked();
    Base::Placement plm = this->getPlacement();
    applyPlacement(getPlacementString(), incr);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    /*emit*/ placementChanged(data, incr, true);

    if (ui->applyIncrementalPlacement->isChecked()) {
        QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
        for (QList<Gui::QuantitySpinBox*>::iterator it = sb.begin(); it != sb.end(); ++it) {
            (*it)->blockSignals(true);
            (*it)->setValue(0);
            (*it)->blockSignals(false);
        }
    }

    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Placement");
    hGrp->SetInt("RotationMethod", ui->rotationInput->currentIndex());

    return true;
}

void Placement::on_resetButton_clicked()
{
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (QList<Gui::QuantitySpinBox*>::iterator it = sb.begin(); it != sb.end(); ++it) {
        (*it)->blockSignals(true);
        (*it)->setValue(0);
        (*it)->blockSignals(false);
    }

    onPlacementChanged(0);
}

void Placement::directionActivated(int index)
{
    if (ui->directionActivated(this, index)) {
        /*emit*/ directionChanged();
    }
}

Base::Vector3d Placement::getDirection() const
{
    return ui->getDirection();
}

void Placement::setPlacement(const Base::Placement& p)
{
    setPlacementData(p);
}

void Placement::setPlacementData(const Base::Placement& p)
{
    signalMapper->blockSignals(true);
    ui->xPos->setValue(Base::Quantity(p.getPosition().x, Base::Unit::Length));
    ui->yPos->setValue(Base::Quantity(p.getPosition().y, Base::Unit::Length));
    ui->zPos->setValue(Base::Quantity(p.getPosition().z, Base::Unit::Length));

    double Y,P,R;
    p.getRotation().getYawPitchRoll(Y,P,R);
    ui->yawAngle->setValue(Base::Quantity(Y, Base::Unit::Angle));
    ui->pitchAngle->setValue(Base::Quantity(P, Base::Unit::Angle));
    ui->rollAngle->setValue(Base::Quantity(R, Base::Unit::Angle));

    // check if the user-defined direction is already there
    bool newitem = true;
    double angle;
    Base::Vector3d axis;
    p.getRotation().getValue(axis, angle);
    ui->angle->setValue(Base::Quantity(angle*180.0/D_PI, Base::Unit::Angle));
    Base::Vector3d dir(axis.x,axis.y,axis.z);
    for (int i=0; i<ui->direction->count()-1; i++) {
        QVariant data = ui->direction->itemData (i);
        if (data.canConvert<Base::Vector3d>()) {
            const Base::Vector3d val = data.value<Base::Vector3d>();
            if (val == dir) {
                ui->direction->setCurrentIndex(i);
                newitem = false;
                break;
            }
        }
    }

    if (newitem) {
        // add a new item before the very last item
        QString display = QString::fromLatin1("(%1,%2,%3)")
            .arg(dir.x)
            .arg(dir.y)
            .arg(dir.z);
        ui->direction->insertItem(ui->direction->count()-1, display,
            QVariant::fromValue<Base::Vector3d>(dir));
        ui->direction->setCurrentIndex(ui->direction->count()-2);
    }
    signalMapper->blockSignals(false);
}

Base::Placement Placement::getPlacement() const
{
    Base::Placement p = getPlacementData();
    return p;
}

Base::Vector3d Placement::getCenterData() const
{
    if (ui->centerOfMass->isChecked())
        return this->cntOfMass;
    return Base::Vector3d(ui->xCnt->value().getValue(),
                          ui->yCnt->value().getValue(),
                          ui->zCnt->value().getValue());
}

Base::Placement Placement::getPlacementData() const
{
    int index = ui->rotationInput->currentIndex();
    Base::Rotation rot;
    Base::Vector3d pos;
    Base::Vector3d cnt;

    pos = Base::Vector3d(ui->xPos->value().getValue(),ui->yPos->value().getValue(),ui->zPos->value().getValue());
    cnt = getCenterData();

    if (index == 0) {
        Base::Vector3d dir = getDirection();
        rot.setValue(Base::Vector3d(dir.x,dir.y,dir.z),Base::toRadians(ui->angle->value().getValue()));
    }
    else if (index == 1) { // Euler angles (XY'Z'')
        rot.setYawPitchRoll(
            ui->yawAngle->value().getValue(),
            ui->pitchAngle->value().getValue(),
            ui->rollAngle->value().getValue());
    }

    Base::Placement p(pos, rot, cnt);
    return p;
}

QString Placement::getPlacementString() const
{
    QString cmd;
    int index = ui->rotationInput->currentIndex();
    Base::Vector3d cnt = getCenterData();

    if (index == 0) {
        Base::Vector3d dir = getDirection();
        cmd = QString::fromLatin1(
            "App.Placement(App.Vector(%1,%2,%3), App.Rotation(App.Vector(%4,%5,%6),%7), App.Vector(%8,%9,%10))")
            .arg(ui->xPos->value().getValue())
            .arg(ui->yPos->value().getValue())
            .arg(ui->zPos->value().getValue())
            .arg(dir.x)
            .arg(dir.y)
            .arg(dir.z)
            .arg(ui->angle->value().getValue())
            .arg(cnt.x)
            .arg(cnt.y)
            .arg(cnt.z);
    }
    else if (index == 1) {
        cmd = QString::fromLatin1(
            "App.Placement(App.Vector(%1,%2,%3), App.Rotation(%4,%5,%6), App.Vector(%7,%8,%9))")
            .arg(ui->xPos->value().getValue())
            .arg(ui->yPos->value().getValue())
            .arg(ui->zPos->value().getValue())
            .arg(ui->yawAngle->value().getValue())
            .arg(ui->pitchAngle->value().getValue())
            .arg(ui->rollAngle->value().getValue())
            .arg(cnt.x)
            .arg(cnt.y)
            .arg(cnt.z);
    }

    return cmd;
}

void Placement::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslate(this);
    }
    else {
        QDialog::changeEvent(e);
    }
}

// ----------------------------------------------

/* TRANSLATOR Gui::Dialog::DockablePlacement */

DockablePlacement::DockablePlacement(QWidget* parent, Qt::WindowFlags fl) : Placement(parent, fl)
{
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    QDockWidget* dw = pDockMgr->addDockWindow(QT_TR_NOOP("Placement"),
        this, Qt::BottomDockWidgetArea);
    dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    dw->show();
}

DockablePlacement::~DockablePlacement()
{
}

void DockablePlacement::accept()
{
    // closes the dock window
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    pDockMgr->removeDockWindow(this);
    Placement::accept();
}

void DockablePlacement::reject()
{
    // closes the dock window
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    pDockMgr->removeDockWindow(this);
    Placement::reject();
}

// ----------------------------------------------

/* TRANSLATOR Gui::Dialog::TaskPlacement */

TaskPlacement::TaskPlacement()
{
    this->setButtonPosition(TaskPlacement::South);
    widget = new Placement();
    widget->showDefaultButtons(false);
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(),true, 0);
    taskbox->groupLayout()->addWidget(widget);

    Content.push_back(taskbox);
    connect(widget, SIGNAL(placementChanged(const QVariant &, bool, bool)),
            this, SLOT(slotPlacementChanged(const QVariant &, bool, bool)));
}

TaskPlacement::~TaskPlacement()
{
    // automatically deleted in the sub-class
}

void TaskPlacement::open()
{
    widget->open();
}

void TaskPlacement::setPropertyName(const QString& name)
{
    widget->propertyName = (const char*)name.toLatin1();
}

QDialogButtonBox::StandardButtons TaskPlacement::getStandardButtons() const
{ 
    return QDialogButtonBox::Ok|
           QDialogButtonBox::Cancel|
           QDialogButtonBox::Apply;
}

void TaskPlacement::setPlacement(const Base::Placement& p)
{
    widget->setPlacement(p);
}

void TaskPlacement::slotPlacementChanged(const QVariant & p, bool incr, bool data)
{
    /*emit*/ placementChanged(p, incr, data);
}

bool TaskPlacement::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

bool TaskPlacement::reject()
{
    widget->reject();
    return (widget->result() == QDialog::Rejected);
}

void TaskPlacement::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->on_applyButton_clicked();
    }
}

#include "moc_Placement.cpp"
