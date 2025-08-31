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
#ifndef _PreComp_
#include <QClipboard>
#include <QDockWidget>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QSignalMapper>
#endif

#include <App/ComplexGeoData.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/PlacementPy.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/PythonWrapper.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/Window.h>

#include "Placement.h"
#include "ui_Placement.h"


using namespace Gui::Dialog;
namespace sp = std::placeholders;

namespace Gui { namespace Dialog {
class find_placement
{
public:
    explicit find_placement(const std::string& name) : propertyname(name)
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

    static App::PropertyPlacement* getProperty(const App::DocumentObject* obj,
                                               const std::string& propertyName)
    {
        std::map<std::string,App::Property*> props;
        obj->getPropertyMap(props);

        // search for the placement property
        std::map<std::string,App::Property*>::iterator jt;
        jt = std::find_if(props.begin(), props.end(), find_placement(propertyName));
        if (jt != props.end()) {
            return dynamic_cast<App::PropertyPlacement*>(jt->second);
        }

        return nullptr;
    }

    std::string propertyname;
};

}
}

PlacementHandler::PlacementHandler()
  : propertyName{"Placement"}
  , changeProperty{false}
  , ignoreTransaction{false}
{
    setupDocument();
}

void PlacementHandler::openTransactionIfNeeded()
{
    if (changeProperty) {
        openTransaction();
    }
}

void PlacementHandler::setPropertyName(const std::string& name)
{
    propertyName = name;
    // Only with the Placement property it's possible to directly change the Inventor representation.
    // For other placement properties with a different name the standard property handling must be used.
    changeProperty = (propertyName != "Placement");
}

void PlacementHandler::setIgnoreTransactions(bool value)
{
    ignoreTransaction = value;
}

void PlacementHandler::setSelection(const std::vector<Gui::SelectionObject>& selection)
{
    selectionObjects = selection;
}

void PlacementHandler::reselectObjects()
{
    //we have to clear selection and reselect original object(s)
    //else later on the rotation is applied twice because there will
    //be 2 (vertex) objects in the selection, and even if both are subobjects
    //of the same object the rotation still gets applied twice
    Gui::Selection().clearSelection();
    //reselect original object that was selected when placement dlg first opened
    for (const auto& it : selectionObjects) {
        Gui::Selection().addSelection(it);
    }
}

const App::DocumentObject* PlacementHandler::getFirstOfSelection() const
{
    if (!selectionObjects.empty()) {
        return selectionObjects.front().getObject();
    }

    return nullptr;
}

const std::string& PlacementHandler::getPropertyName() const
{
    return propertyName;
}

void PlacementHandler::appendDocument(const std::string& name)
{
    documents.insert(name);
}

void PlacementHandler::activatedDocument(const std::string& name)
{
    appendDocument(name);

    if (changeProperty) {
        QMetaObject::invokeMethod(this, "openTransaction", Qt::QueuedConnection);
    }
}

void PlacementHandler::openTransaction()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (activeDoc)
        activeDoc->openTransaction("Placement");
}

void PlacementHandler::revertTransformation()
{
    for (const auto & it : documents) {
        Gui::Document* document = Application::Instance->getDocument(it.c_str());
        if (document) {
            if (!changeProperty) {
                revertTransformationOfViewProviders(document);
            }
            else {
                abortCommandIfActive(document);
            }
        }
    }
}

std::vector<const App::DocumentObject*> PlacementHandler::getObjects(const Gui::Document* document) const
{
    auto objs = document->getDocument()->getObjectsOfType(App::DocumentObject::getClassTypeId());
    std::vector<const App::DocumentObject*> list;
    list.insert(list.begin(), objs.begin(), objs.end());
    return list;
}

std::vector<const App::DocumentObject*> PlacementHandler::getSelectedObjects(const Gui::Document* document) const
{
    App::Document* doc = document->getDocument();
    std::vector<const App::DocumentObject*> list;
    list.reserve(selectionObjects.size());
    for (const auto& it : selectionObjects) {
        const App::DocumentObject* obj = it.getObject();
        if (obj && obj->getDocument() == doc) {
            list.push_back(obj);
        }
    }

    if (list.empty()) {
        auto objs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId(), doc->getName());
        list.insert(list.begin(), objs.begin(), objs.end());
    }

    return list;
}

void PlacementHandler::revertTransformationOfViewProviders(Gui::Document* document)
{
    std::vector<const App::DocumentObject*> obj = getObjects(document);
    for (const auto & it : obj) {
        auto property = find_placement::getProperty(it, this->propertyName);
        if (property) {
            Base::Placement cur = property->getValue();
            Gui::ViewProvider* vp = document->getViewProvider(it);
            if (vp) {
                vp->setTransformation(cur.toMatrix());
            }
        }
    }
}

void PlacementHandler::setRefPlacement(const Base::Placement& plm)
{
    ref = plm;
}

const Base::Placement& PlacementHandler::getRefPlacement() const
{
    return ref;
}

void PlacementHandler::applyPlacement(const Base::Placement& p, bool incremental)
{
    Gui::Document* document = Application::Instance->activeDocument();
    if (!document)
        return;

    std::vector<const App::DocumentObject*> sel = getSelectedObjects(document);
    if (!sel.empty()) {
        for (const auto & it : sel) {
            applyPlacement(document, it, p, incremental);
        }
    }
    else {
        Base::Console().warning("No object selected.\n");
    }
}

void PlacementHandler::applyPlacement(const Gui::Document* document, const App::DocumentObject* obj,
                                      const Base::Placement& p, bool incremental)
{
    auto property = find_placement::getProperty(obj, this->propertyName);
    if (property) {
        Base::Placement cur = property->getValue();
        if (incremental)
            cur = p * cur;
        else
            cur = p;

        if (!changeProperty) {
            Gui::ViewProvider* vp = document->getViewProvider(obj);
            if (vp) {
                vp->setTransformation(cur.toMatrix());
            }
        }
        else {
            property->setValue(cur);
        }
    }
}

void PlacementHandler::applyPlacement(const QString& data, bool incremental)
{
    Gui::Document* document = Application::Instance->activeDocument();
    if (!document)
        return;

    // When directly changing the property we now only have to commit the transaction,
    // do a recompute and open a new transaction
    if (changeProperty) {
        commitCommandIfActive(document);
        tryRecompute(document);
        openCommandIfActive(document);
    }
    else {
        std::vector<const App::DocumentObject*> sel = getSelectedObjects(document);
        if (!sel.empty()) {
            openCommandIfActive(document);
            for (const auto & it : sel) {
                applyPlacement(it, data, incremental);
            }
            commitCommandIfActive(document);
            tryRecompute(document);
        }
        else {
            Base::Console().warning("No object selected.\n");
        }
    }
}

void PlacementHandler::applyPlacement(const App::DocumentObject* obj, const QString& data, bool incremental)
{
    auto property = find_placement::getProperty(obj, this->propertyName);
    if (property) {
        QString cmd;
        if (incremental) {
            cmd = getIncrementalPlacement(obj, data);
        }
        else {
            cmd = getSimplePlacement(obj, data);
        }

        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
    }
}

QString PlacementHandler::getIncrementalPlacement(const App::DocumentObject* obj, const QString& data) const
{
    return QStringLiteral(
        R"(App.getDocument("%1").%2.%3=%4.multiply(App.getDocument("%1").%2.%3))")
        .arg(QString::fromLatin1(obj->getDocument()->getName()),
             QString::fromLatin1(obj->getNameInDocument()),
             QString::fromLatin1(this->propertyName.c_str()),
             data);
}

QString PlacementHandler::getSimplePlacement(const App::DocumentObject* obj, const QString& data) const
{
    return QStringLiteral(
        "App.getDocument(\"%1\").%2.%3=%4")
        .arg(QString::fromLatin1(obj->getDocument()->getName()),
             QString::fromLatin1(obj->getNameInDocument()),
             QString::fromLatin1(this->propertyName.c_str()),
             data);
}

Base::Vector3d PlacementHandler::computeCenterOfMass() const
{
    Base::Vector3d centerOfMass;
    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
        (App::GeoFeature::getClassTypeId());
    if (!sel.empty()) {
        for (auto it : sel) {
            const App::PropertyComplexGeoData* propgeo = static_cast<App::GeoFeature*>(it)->getPropertyOfGeometry();
            const Data::ComplexGeoData* geodata = propgeo ? propgeo->getComplexData() : nullptr;
            if (geodata && geodata->getCenterOfGravity(centerOfMass)) {
                break;
            }
        }
    }
    return centerOfMass;
}

void PlacementHandler::setCenterOfMass(const Base::Vector3d& pnt)
{
    cntOfMass = pnt;
}

Base::Vector3d PlacementHandler::getCenterOfMass() const
{
    return cntOfMass;
}

std::tuple<Base::Vector3d, std::vector<Base::Vector3d>> PlacementHandler::getSelectedPoints() const
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    std::vector<Base::Vector3d> picked;
    //combine all pickedpoints into single vector
    //even if points are from separate objects
    Base::Vector3d firstSelected; //first selected will be central point when 3 points picked
    for (auto it = selection.begin(); it != selection.end(); ++it) {
        std::vector<Base::Vector3d> points = it->getPickedPoints();
        if (it == selection.begin() && !points.empty()) {
            firstSelected = points[0];
        }

        picked.insert(picked.begin(), points.begin(), points.end());
    }

    return std::make_tuple(firstSelected, picked);
}

void PlacementHandler::tryRecompute(Gui::Document* document)
{
    try {
        document->getDocument()->recompute();
    }
    catch (...) {
    }
}

void PlacementHandler::setupDocument()
{
    //NOLINTBEGIN
    connectAct = Application::Instance->signalActiveDocument.connect
        (std::bind(&PlacementHandler::slotActiveDocument, this, sp::_1));
    //NOLINTEND
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (activeDoc) {
        appendDocument(activeDoc->getName());
    }
}

void PlacementHandler::slotActiveDocument(const Gui::Document& doc)
{
    activatedDocument(doc.getDocument()->getName());
}

void PlacementHandler::openCommandIfActive(Gui::Document* doc)
{
    if (!ignoreTransaction) {
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Placement"));
    }
}

void PlacementHandler::commitCommandIfActive(Gui::Document* doc)
{
    if (!ignoreTransaction) {
        doc->commitCommand();
    }
}

void PlacementHandler::abortCommandIfActive(Gui::Document* doc)
{
    if (!ignoreTransaction) {
        doc->abortCommand();
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::Placement */

Placement::Placement(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl)
  , ui{nullptr}
{
    setupUi();
    setupConnections();
    setupUnits();
    setupSignalMapper();
    setupRotationMethod();
}

Placement::~Placement()
{
    delete ui;
}

void Placement::setupUi()
{
    ui = new Ui_Placement();
    ui->setupUi(this);
    ui->gridLayout->removeItem(ui->vSpacer);
}

void Placement::setupConnections()
{
    QPushButton* applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    connect(applyButton, &QPushButton::clicked,
            this, &Placement::onApplyButtonClicked);
    connect(ui->applyIncrementalPlacement, &QCheckBox::toggled,
            this, &Placement::onApplyIncrementalPlacementToggled);
    connect(ui->resetButton, &QPushButton::clicked,
            this, &Placement::onResetButtonClicked);
    connect(ui->centerOfMass, &QCheckBox::toggled,
            this, &Placement::onCenterOfMassToggled);
    connect(ui->selectedVertex, &QPushButton::clicked,
            this, &Placement::onSelectedVertexClicked);
    connect(ui->applyAxial, &QPushButton::clicked,
            this, &Placement::onApplyAxialClicked);
}

void Placement::setupUnits()
{
    ui->xPos->setUnit(Base::Unit::Length);
    ui->yPos->setUnit(Base::Unit::Length);
    ui->zPos->setUnit(Base::Unit::Length);
    ui->axialPos->setUnit(Base::Unit::Length);
    ui->xCnt->setValue(Base::Quantity(0, Base::Unit::Length));
    ui->yCnt->setValue(Base::Quantity(0, Base::Unit::Length));
    ui->zCnt->setValue(Base::Quantity(0, Base::Unit::Length));
    ui->angle->setUnit(Base::Unit::Angle);
    ui->yawAngle->setMaximum(180.0F);
    ui->yawAngle->setMinimum(-180.0F);
    ui->yawAngle->setUnit(Base::Unit::Angle);
    ui->yawAngle->checkRangeInExpression(true);
    ui->pitchAngle->setMaximum(90.0F);
    ui->pitchAngle->setMinimum(-90.0F);
    ui->pitchAngle->setUnit(Base::Unit::Angle);
    ui->pitchAngle->checkRangeInExpression(true);
    ui->rollAngle->setMaximum(180.0F);
    ui->rollAngle->setMinimum(-180.0F);
    ui->rollAngle->setUnit(Base::Unit::Angle);
    ui->rollAngle->checkRangeInExpression(true);
}

void Placement::setupSignalMapper()
{
    // create a signal mapper in order to have one slot to perform the change
    signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(this, 0);

    int id = 1;
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (const auto & it : sb) {
        connect(it, qOverload<double>(&QuantitySpinBox::valueChanged), signalMapper, qOverload<>(&QSignalMapper::map));
        signalMapper->setMapping(it, id++);
    }

    connect(signalMapper, &QSignalMapper::mappedInt,
            this, &Placement::onPlacementChanged);
}

void Placement::setupRotationMethod()
{
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Placement");
    long index = hGrp->GetInt("RotationMethod");
    ui->rotationInput->setCurrentIndex(index);
    ui->stackedWidget->setCurrentIndex(index);
}

void Placement::showDefaultButtons(bool ok)
{
    ui->buttonBox->setVisible(ok);
    ui->buttonBoxLayout->invalidate();
    if (ok) {
        ui->buttonBoxLayout->insertSpacerItem(0, ui->buttonBoxSpacer);
    }
    else {
        ui->buttonBoxLayout->removeItem(ui->buttonBoxSpacer);
    }
}

void Placement::open()
{
    handler.openTransactionIfNeeded();
}

QWidget* Placement::getInvalidInput() const
{
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (const auto & it : sb) {
        if (!it->hasValidInput())
            return it;
    }
    return nullptr;
}

void Placement::onPlacementChanged(int)
{
    // If there are listeners to the 'placementChanged' signal we rely
    // on that the listener updates any placement. If no listeners
    // are connected the placement is applied to all selected objects
    // automatically.
    bool incr = ui->applyIncrementalPlacement->isChecked();
    Base::Placement plm = this->getPlacement();
    handler.applyPlacement(plm, incr);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    Q_EMIT placementChanged(data, incr, false);
}

void Placement::onCenterOfMassToggled(bool on)
{
    ui->xCnt->setDisabled(on);
    ui->yCnt->setDisabled(on);
    ui->zCnt->setDisabled(on);

    if (on) {
        Base::Vector3d pnt = handler.computeCenterOfMass();
        handler.setCenterOfMass(pnt);
        ui->xCnt->setValue(pnt.x);
        ui->yCnt->setValue(pnt.y);
        ui->zCnt->setValue(pnt.z);
    }
}

void Placement::onSelectedVertexClicked()
{
    ui->centerOfMass->setChecked(false);

    Base::Vector3d center;
    bool success = false;
    auto [firstSelected, picked] = handler.getSelectedPoints();
    handler.reselectObjects();

    if (picked.size() == 1) {
        center = picked[0];
        success = true;
    }
    else if (picked.size() == 2) {
        //average the coords to get center of rotation
        center = (picked[0] + picked[1]) / 2.0;

        //setup a customized axis since the user selected 2 points
        //keep any existing angle, but setup our own axis
        Base::Placement plm = getPlacement();
        Base::Rotation rot = plm.getRotation();
        Base::Vector3d tmp;
        double angle;
        rot.getRawValue(tmp, angle);
        Base::Vector3d axis;
        if (firstSelected == picked[0]){
            axis = Base::Vector3d(picked[1] - picked[0]);
        }
        else {
            axis = Base::Vector3d(picked[0] - picked[1]);
        }
        double length = axis.Length();
        Base::Console().message("Distance: %.8f\n",length);
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier){ //copy to clipboard on Shift+click
            QLocale loc;
            QApplication::clipboard()->setText(loc.toString(length,'g',8));
        }
        else {
            Base::Console().message("(Shift + click selected points button to copy distance to clipboard)\n");
        }
        axis.Normalize();
        rot.setValue(axis, angle);
        plm.setRotation(rot);
        setPlacementData(plm); //creates custom axis, if needed
        ui->rotationInput->setCurrentIndex(0); //use rotation with axis instead of euler
        ui->stackedWidget->setCurrentIndex(0);
        success = true;
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
        else if (picked[1] == firstSelected){
            a = picked[0];
            c = picked[2];
        }
        else if (picked[2] == firstSelected){
            a = picked[0];
            c = picked[1];
        }

        Base::Vector3d norm((a-b).Cross(c-b));
        norm.Normalize();
        center = b;

        //setup a customized axis normal to the plane
        //keep any existing angle, but setup our own axis
        Base::Placement plm = getPlacement();
        Base::Rotation rot = plm.getRotation();
        Base::Vector3d tmp;
        double angle;
        rot.getRawValue(tmp, angle);
        double length = (a-c).Length();
        Base::Console().message("Distance: %.8f\n",length);
        Base::Vector3d v1(a-b);
        Base::Vector3d v2(c-b);
        v1.Normalize();
        v2.Normalize();
        double targetAngle = Base::toDegrees(v2.GetAngle(v1));
        Base::Console().message("Target angle: %.8f degrees, complementary: %.8f degrees\n",targetAngle, 90.0-targetAngle);
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier){ //copy to clipboard on Shift+click
            QLocale loc;
            QApplication::clipboard()->setText(loc.toString(targetAngle,'g',8));
            Base::Console().message("(Angle copied to clipboard, but you might need to use a negative (-) angle sometimes.)\n");
        }
        else {
            Base::Console().message("(Shift + click selected points button to copy angle to clipboard)\n");
        }
        rot.setValue(norm, angle);
        plm.setRotation(rot);
        setPlacementData(plm); //creates custom axis, if needed
        ui->rotationInput->setCurrentIndex(0); //use rotation with axis instead of euler
        ui->stackedWidget->setCurrentIndex(0);
        success = true;
    }

    handler.setCenterOfMass(center);
    ui->xCnt->setValue(center.x);
    ui->yCnt->setValue(center.y);
    ui->zCnt->setValue(center.z);

    if (!success) {
        Base::Console().warning("Placement selection error.  Select either 1 or 2 points.\n");
        QMessageBox msgBox(this);
        msgBox.setText(tr("Select 1, 2, or 3 points before clicking this button. A point may be on a vertex, \
face, or edge.  If on a face or edge the point used will be the point at the mouse position along \
face or edge.  If 1 point is selected it will be used as the center of rotation.  If 2 points are \
selected the midpoint between them will be the center of rotation and a new custom axis will be \
created, if needed.  If 3 points are selected the first point becomes the center of rotation and \
lies on the vector that is normal to the plane defined by the 3 points.  Some distance and angle \
information is provided in the report view, which can be useful when aligning objects.  For your \
convenience when Shift + click is used the appropriate distance or angle is copied to the clipboard."));
        msgBox.exec();
    }
}

void Placement::onApplyAxialClicked()
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

void Placement::onApplyIncrementalPlacementToggled(bool on)
{
    if (on) {
        handler.setRefPlacement(getPlacementData());
        onResetButtonClicked();
    }
    else {
        Base::Placement p = getPlacementData();
        p = p * handler.getRefPlacement();
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
    handler.applyPlacement(plm, true);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    Q_EMIT placementChanged(data, true, false);

    handler.revertTransformation();

    // One of the quantity spin boxes still can emit a signal when it has the focus
    // but its content is not fully updated.
    // In order to override again the placement the signalMapper is blocked
    // See related forum thread:
    // https://forum.freecad.org/viewtopic.php?f=3&t=44341#p378659
    QSignalBlocker block(signalMapper);
    QDialog::reject();
}

void Placement::accept()
{
    if (onApply()) {
        handler.revertTransformation();
        QDialog::accept();
    }
}

void Placement::onApplyButtonClicked()
{
    onApply();
}

void Placement::showErrorMessage()
{
    QMessageBox msg(this);
    msg.setWindowTitle(tr("Incorrect Quantity"));
    msg.setIcon(QMessageBox::Critical);
    msg.setText(tr("There are input fields with incorrect input. Ensure valid placement values!"));
    msg.exec();
}

bool Placement::onApply()
{
    //only process things when we have valid inputs!
    QWidget* input = getInvalidInput();
    if (input) {
        input->setFocus();
        showErrorMessage();
        return false;
    }

    // If there are listeners to the 'placementChanged' signal we rely
    // on that the listener updates any placement. If no listeners
    // are connected the placement is applied to all selected objects
    // automatically.
    bool incr = ui->applyIncrementalPlacement->isChecked();
    Base::Placement plm = this->getPlacement();
    handler.applyPlacement(getPlacementString(), incr);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    Q_EMIT placementChanged(data, incr, true);

    if (ui->applyIncrementalPlacement->isChecked()) {
        QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
        for (auto & it : sb) {
            it->blockSignals(true);
            it->setValue(0);
            it->blockSignals(false);
        }
    }

    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Placement");
    hGrp->SetInt("RotationMethod", ui->rotationInput->currentIndex());

    return true;
}

void Placement::onResetButtonClicked()
{
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (auto & it : sb) {
        it->blockSignals(true);
        it->setValue(0);
        it->blockSignals(false);
    }

    onPlacementChanged(0);
}

/*!
 * \brief Placement::setSelection
 * Sets the array of selection objects.
 * \param selection
 */
void Placement::setSelection(const std::vector<Gui::SelectionObject>& selection)
{
    handler.setSelection(selection);
}

void Placement::setPropertyName(const std::string& name)
{
    handler.setPropertyName(name);
}

void Placement::setIgnoreTransactions(bool value)
{
    handler.setIgnoreTransactions(value);
}

/*!
 * \brief Placement::bindObject
 * Binds the spin boxes to the placement components of the first object of the selection.
 * This requires the call of \a setSelection() beforehand.
 */
void Placement::bindObject()
{
    if (const App::DocumentObject* obj = handler.getFirstOfSelection()) {
        std::string propertyName = handler.getPropertyName();
        bindProperty(obj, propertyName);
    }
}

/*!
 * \brief Placement::setPlacementAndBindObject
 * Sets the placement, binds the spin boxes to the placement components of the passed object and
 * sets the name of the placement property.
 */
void Placement::setPlacementAndBindObject(const App::DocumentObject* obj, const std::string& propertyName)
{
    if (obj) {
        App::PropertyPlacement* prop = find_placement::getProperty(obj, propertyName);
        if (prop) {
            setPlacement(prop->getValue());
            handler.setPropertyName(propertyName);
            bindProperty(obj, propertyName);
            handler.setSelection({SelectionObject{obj}});
        }
    }
}

void Placement::bindProperty(const App::DocumentObject* obj, const std::string& propertyName)
{
    // clang-format off
    if (obj) {
        App::ObjectIdentifier path = App::ObjectIdentifier::parse(obj, propertyName);
        if (path.getProperty()) {
            ui->xPos->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Base.x")));
            ui->yPos->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Base.y")));
            ui->zPos->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Base.z")));

            ui->xAxis->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Rotation.Axis.x")));
            ui->yAxis->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Rotation.Axis.y")));
            ui->zAxis->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Rotation.Axis.z")));
            ui->angle->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Rotation.Angle")));

            ui->yawAngle  ->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Rotation.Yaw")));
            ui->pitchAngle->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Rotation.Pitch")));
            ui->rollAngle ->bind(App::ObjectIdentifier::parse(obj, propertyName + std::string(".Rotation.Roll")));

            ui->yawAngle->evaluateExpression();
            ui->pitchAngle->evaluateExpression();
            ui->rollAngle->evaluateExpression();
        }
    }
    // clang-format on
}

Base::Vector3d Placement::getDirection() const
{
    double x = ui->xAxis->value().getValue();
    double y = ui->yAxis->value().getValue();
    double z = ui->zAxis->value().getValue();
    return Base::Vector3d(x, y, z);
}

void Placement::setPlacement(const Base::Placement& p)
{
    setPlacementData(p);
}

void Placement::setPlacementData(const Base::Placement& p)
{
    QSignalBlocker block(signalMapper);
    ui->xPos->setValue(Base::Quantity(p.getPosition().x, Base::Unit::Length));
    ui->yPos->setValue(Base::Quantity(p.getPosition().y, Base::Unit::Length));
    ui->zPos->setValue(Base::Quantity(p.getPosition().z, Base::Unit::Length));

    double Y,P,R;
    p.getRotation().getYawPitchRoll(Y,P,R);
    ui->yawAngle->setValue(Base::Quantity(Y, Base::Unit::Angle));
    ui->pitchAngle->setValue(Base::Quantity(P, Base::Unit::Angle));
    ui->rollAngle->setValue(Base::Quantity(R, Base::Unit::Angle));

    double angle;
    Base::Vector3d axis;
    p.getRotation().getRawValue(axis, angle);
    ui->xAxis->setValue(axis.x);
    ui->yAxis->setValue(axis.y);
    ui->zAxis->setValue(axis.z);
    ui->angle->setValue(Base::Quantity(Base::toDegrees(angle), Base::Unit::Angle));
}

Base::Placement Placement::getPlacement() const
{
    Base::Placement p = getPlacementData();
    return p;
}

Base::Rotation Placement::getRotationData() const
{
    Base::Rotation rot;
    int index = ui->rotationInput->currentIndex();
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

    return rot;
}

Base::Vector3d Placement::getPositionData() const
{
    return Base::Vector3d(ui->xPos->value().getValue(),
                          ui->yPos->value().getValue(),
                          ui->zPos->value().getValue());
}

Base::Vector3d Placement::getAnglesData() const
{
    return Base::Vector3d(ui->yawAngle->value().getValue(),
                          ui->pitchAngle->value().getValue(),
                          ui->rollAngle->value().getValue());
}

Base::Vector3d Placement::getCenterData() const
{
    if (ui->centerOfMass->isChecked()) {
        return handler.getCenterOfMass();
    }
    return Base::Vector3d(ui->xCnt->value().getValue(),
                          ui->yCnt->value().getValue(),
                          ui->zCnt->value().getValue());
}

Base::Placement Placement::getPlacementData() const
{
    Base::Rotation rot = getRotationData();
    Base::Vector3d pos = getPositionData();
    Base::Vector3d cnt = getCenterData();

    Base::Placement plm(pos, rot, cnt);
    return plm;
}

QString Placement::getPlacementFromEulerAngles() const
{
    Base::Vector3d pos = getPositionData();
    Base::Vector3d ypr = getAnglesData();
    Base::Vector3d cnt = getCenterData();
    return QStringLiteral(
        "App.Placement(App.Vector(%1,%2,%3), App.Rotation(%4,%5,%6), App.Vector(%7,%8,%9))")
        .arg(pos.x)
        .arg(pos.y)
        .arg(pos.z)
        .arg(ypr.x)
        .arg(ypr.y)
        .arg(ypr.z)
        .arg(cnt.x)
        .arg(cnt.y)
        .arg(cnt.z);
}

QString Placement::getPlacementFromAxisWithAngle() const
{
    Base::Vector3d pos = getPositionData();
    Base::Vector3d cnt = getCenterData();
    Base::Vector3d dir = getDirection();
    double angle = ui->angle->value().getValue();
    return QStringLiteral(
        "App.Placement(App.Vector(%1,%2,%3), App.Rotation(App.Vector(%4,%5,%6),%7), App.Vector(%8,%9,%10))")
        .arg(pos.x)
        .arg(pos.y)
        .arg(pos.z)
        .arg(dir.x)
        .arg(dir.y)
        .arg(dir.z)
        .arg(angle)
        .arg(cnt.x)
        .arg(cnt.y)
        .arg(cnt.z);
}

QString Placement::getPlacementString() const
{
    QString cmd;
    int index = ui->rotationInput->currentIndex();

    if (index == 0) {
        cmd = getPlacementFromAxisWithAngle();
    }
    else if (index == 1) {
        cmd = getPlacementFromEulerAngles();
    }

    return cmd;
}

void Placement::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
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

DockablePlacement::~DockablePlacement() = default;

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
    addTaskBox(widget);
    connect(widget, &Placement::placementChanged, this, &TaskPlacement::slotPlacementChanged);
}

TaskPlacement::~TaskPlacement() = default;


/*!
 * \brief TaskPlacement::setSelection
 * Sets the array of selection objects.
 * \param selection
 */
void TaskPlacement::setSelection(const std::vector<Gui::SelectionObject>& selection)
{
    widget->setSelection(selection);
}

/*!
 * \brief TaskPlacement::clearSelection
 * Clears the array of selection objects.
 */
void TaskPlacement::clearSelection()
{
    widget->setSelection({});
}

/*!
 * \brief TaskPlacement::bindObject
 * Binds the spin boxes to the placement components of the first object of the selection.
 * This requires the call of \a setSelection() beforehand.
 */
void TaskPlacement::bindObject()
{
    widget->bindObject();
}

void TaskPlacement::setPlacementAndBindObject(const App::DocumentObject* obj,
                                              const std::string& propertyName)
{
    widget->setPlacementAndBindObject(obj, propertyName);
}

void TaskPlacement::open()
{
    widget->open();
}

void TaskPlacement::setPropertyName(const QString& name)
{
    widget->setPropertyName(name.toStdString());
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
    Q_EMIT placementChanged(p, incr, data);
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
        widget->onApplyButtonClicked();
    }
}

// ----------------------------------------------

void TaskPlacementPy::init_type()
{
    behaviors().name("TaskPlacement");
    behaviors().doc("TaskPlacement");
    behaviors().set_tp_new(PyMake);
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    // clang-format off
    add_varargs_method("setPropertyName", &TaskPlacementPy::setPropertyName,
                       "setPropertyName(string)");
    add_varargs_method("setPlacement", &TaskPlacementPy::setPlacement,
                       "setPlacement(Placement)");
    add_varargs_method("setSelection", &TaskPlacementPy::setSelection,
                       "setSelection(list)");
    add_varargs_method("bindObject", &TaskPlacementPy::bindObject,
                       "bindObject()");
    add_varargs_method("setPlacementAndBindObject", &TaskPlacementPy::setPlacementAndBindObject,
                       "setPlacementAndBindObject(obj, string)");
    add_varargs_method("setIgnoreTransactions", &TaskPlacementPy::setIgnoreTransactions,
                       "setIgnoreTransactions(bool)");
    add_varargs_method("showDefaultButtons", &TaskPlacementPy::showDefaultButtons,
                       "showDefaultButtons(bool)");
    add_varargs_method("accept", &TaskPlacementPy::accept,
                       "accept()");
    add_varargs_method("reject", &TaskPlacementPy::reject,
                       "reject()");
    add_varargs_method("clicked", &TaskPlacementPy::clicked,
                       "clicked()");
    add_varargs_method("open", &TaskPlacementPy::open,
                       "open()");
    add_varargs_method("isAllowedAlterDocument", &TaskPlacementPy::isAllowedAlterDocument,
                       "isAllowedAlterDocument()");
    add_varargs_method("isAllowedAlterView", &TaskPlacementPy::isAllowedAlterView,
                       "isAllowedAlterView()");
    add_varargs_method("isAllowedAlterSelection", &TaskPlacementPy::isAllowedAlterSelection,
                       "isAllowedAlterSelection()");
    add_varargs_method("getStandardButtons", &TaskPlacementPy::getStandardButtons,
                       "getStandardButtons()");
    // clang-format on
}

PyObject* TaskPlacementPy::PyMake(struct _typeobject * type, PyObject * args, PyObject * kwds)
{
    Q_UNUSED(type)
    Q_UNUSED(kwds)
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    return new TaskPlacementPy();
}

TaskPlacementPy::TaskPlacementPy()
    : widget{new Placement}
{
}

TaskPlacementPy::~TaskPlacementPy() = default;

Py::Object TaskPlacementPy::repr()
{
    return Py::String("TaskPlacement");
}

Py::Object TaskPlacementPy::getattr(const char * name)
{
    if (strcmp(name, "form") == 0) {
        Gui::PythonWrapper wrap;
        wrap.loadWidgetsModule();
        return wrap.fromQWidget(widget, "QDialog");
    }
    return BaseType::getattr(name);
}

int TaskPlacementPy::setattr(const char* name, const Py::Object& attr)
{
    if (strcmp(name, "form") == 0 && attr.isNone()) {
        delete widget;
        widget = nullptr;
        return {};
    }
    return BaseType::setattr(name, attr);
}

Py::Object TaskPlacementPy::setPropertyName(const Py::Tuple& args)
{
    const char* propname {};
    if (!PyArg_ParseTuple(args.ptr(), "s", &propname)) {
        throw Py::Exception();
    }

    if (widget) {
        widget->setPropertyName(propname);
    }
    return Py::None();
}

Py::Object TaskPlacementPy::setPlacement(const Py::Tuple& args)
{
    PyObject* plm {};
    if (!PyArg_ParseTuple(args.ptr(), "O!", &Base::PlacementPy::Type, &plm)) {
        throw Py::Exception();
    }

    if (widget) {
        widget->setPlacement(*static_cast<Base::PlacementPy*>(plm)->getPlacementPtr());
    }
    return Py::None();
}

Py::Object TaskPlacementPy::setSelection(const Py::Tuple& args)
{
    std::vector<Gui::SelectionObject> sel;
    Py::Sequence list(args[0]);

    for (const auto& obj : list) {
        if (PyObject_TypeCheck(obj.ptr(), &App::DocumentObjectPy::Type)) {
            auto doc = static_cast<App::DocumentObjectPy*>(obj.ptr());
            sel.emplace_back(doc->getDocumentObjectPtr());
        }
    }

    if (widget) {
        widget->setSelection(sel);
    }
    return Py::None();
}

Py::Object TaskPlacementPy::bindObject(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    if (widget) {
        widget->bindObject();
    }

    return Py::None();
}

Py::Object TaskPlacementPy::setPlacementAndBindObject(const Py::Tuple& args)
{
    Py::Object object = args[0];
    Py::String name = args[1];
    std::string propName = static_cast<std::string>(name);

    if (PyObject_TypeCheck(object.ptr(), &App::DocumentObjectPy::Type)) {
        auto py = static_cast<App::DocumentObjectPy*>(object.ptr());
        auto obj = py->getDocumentObjectPtr();

        if (widget) {
            widget->setPlacementAndBindObject(obj, propName);
        }
    }

    return Py::None();
}

Py::Object TaskPlacementPy::setIgnoreTransactions(const Py::Tuple& args)
{
    if (widget) {
        widget->setIgnoreTransactions(Py::Boolean(args[0]));
    }
    return Py::None();
}

Py::Object TaskPlacementPy::showDefaultButtons(const Py::Tuple& args)
{
    if (widget) {
        widget->showDefaultButtons(Py::Boolean(args[0]));
    }
    return Py::None();
}

Py::Object TaskPlacementPy::accept(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    bool res = true;
    if (widget) {
        widget->accept();
        res = widget->result() == QDialog::Accepted;
    }
    return Py::Boolean(res);
}

Py::Object TaskPlacementPy::reject(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    bool res = true;
    if (widget) {
        widget->reject();
        res = widget->result() == QDialog::Rejected;
    }
    return Py::Boolean(res);
}

Py::Object TaskPlacementPy::clicked(const Py::Tuple& args)
{
    int index {};
    if (!PyArg_ParseTuple(args.ptr(), "i", &index)) {
        throw Py::Exception();
    }

    if (widget && index == QDialogButtonBox::Apply) {
        widget->onApplyButtonClicked();
    }
    return Py::None();
}

Py::Object TaskPlacementPy::open(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    if (widget) {
        widget->open();
    }
    return Py::None();
}

Py::Object TaskPlacementPy::isAllowedAlterDocument(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    return Py::Boolean(true);
}

Py::Object TaskPlacementPy::isAllowedAlterView(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    return Py::Boolean(true);
}

Py::Object TaskPlacementPy::isAllowedAlterSelection(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    return Py::Boolean(true);
}

Py::Object TaskPlacementPy::getStandardButtons(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    auto buttons = QDialogButtonBox::Ok|
            QDialogButtonBox::Cancel|
            QDialogButtonBox::Apply;
    return Py::Long(static_cast<int>(buttons));
}

#include "moc_Placement.cpp"
