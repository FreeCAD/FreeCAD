/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QSignalMapper>

#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>

#include "Transform.h"
#include "Application.h"
#include "Document.h"
#include "Selection.h"
#include "ui_Placement.h"
#include "ViewProvider.h"
#include "WaitCursor.h"


using namespace Gui::Dialog;

namespace Gui { namespace Dialog {
class find_transform
{
public:
    bool operator () (const std::pair<std::string, App::Property*>& elem) const
    {
        if (elem.first == "Placement") {
            return elem.second->isDerivedFrom
                (Base::Type::fromName("App::PropertyPlacement"));
        }

        return false;
    }
};
}
}

// ----------------------------------------------------------------------------

TransformStrategy::TransformStrategy() = default;

TransformStrategy::~TransformStrategy() = default;

Base::Vector3d TransformStrategy::getRotationCenter() const
{
    // get the global bounding box of all selected objects and use its center as
    // rotation center
    std::set<App::DocumentObject*> objects = transformObjects();
    if (!objects.empty()) {
        Base::BoundBox3d bbox;
        bool first=true;
        for (const auto & object : objects) {
            if (object->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
                // search for a data property
                const App::PropertyGeometry* geo = static_cast<App::GeoFeature*>(object)->getPropertyOfGeometry();
                if (geo) {
                    if (first)
                        bbox = geo->getBoundingBox();
                    else
                        bbox.Add(geo->getBoundingBox());
                    first = false;
                }
            }
        }

        return Base::Vector3d((bbox.MinX+bbox.MaxX)/2,
                              (bbox.MinY+bbox.MaxY)/2,
                              (bbox.MinZ+bbox.MaxZ)/2);
    }

    return Base::Vector3d();
}

void TransformStrategy::commitTransform(const Base::Matrix4D& mat)
{
    std::set<App::DocumentObject*> objects = transformObjects();
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Transform"));
        for (const auto & object : objects) {
            acceptDataTransform(mat, object);
        }
        doc->commitCommand();
    }
}

void TransformStrategy::acceptDataTransform(const Base::Matrix4D& mat, App::DocumentObject* obj)
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(obj->getDocument());
    std::map<std::string,App::Property*> props;
    obj->getPropertyMap(props);
    // search for the placement property
    std::map<std::string,App::Property*>::iterator jt;
    jt = std::find_if(props.begin(), props.end(), find_transform());
    if (jt != props.end()) {
        Base::Placement local = static_cast<App::PropertyPlacement*>(jt->second)->getValue();
        Gui::ViewProvider* vp = doc->getViewProvider(obj);
        if (vp) vp->setTransformation(local.toMatrix());
    }
    else {
        // No placement found
        Gui::ViewProvider* vp = doc->getViewProvider(obj);
        if (vp) vp->setTransformation(Base::Matrix4D());
    }

    // Apply the transformation
    if (obj->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
        // search for a data property
        const App::PropertyGeometry* geo = static_cast<App::GeoFeature*>(obj)->getPropertyOfGeometry();
        if (geo) {
            const_cast<App::PropertyGeometry*>(geo)->transformGeometry(mat);
        }
    }
}

void TransformStrategy::applyTransform(const Base::Placement& plm)
{
    std::set<App::DocumentObject*> objects = transformObjects();
    for (const auto & object : objects) {
        applyViewTransform(plm, object);
    }
}

void TransformStrategy::resetTransform()
{
    std::set<App::DocumentObject*> objects = transformObjects();
    for (const auto & object : objects) {
        resetViewTransform(object);
    }
}

void TransformStrategy::applyViewTransform(const Base::Placement& plm, App::DocumentObject* obj)
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(obj->getDocument());
    std::map<std::string,App::Property*> props;
    obj->getPropertyMap(props);
    // search for the placement property
    std::map<std::string,App::Property*>::iterator jt;
    jt = std::find_if(props.begin(), props.end(), find_transform());
    if (jt != props.end()) {
        Base::Placement local = static_cast<App::PropertyPlacement*>(jt->second)->getValue();
        local *= plm; // in case a placement is already set
        Gui::ViewProvider* vp = doc->getViewProvider(obj);
        if (vp) vp->setTransformation(local.toMatrix());
    }
    else {
        // No placement found, so apply the transformation directly
        Gui::ViewProvider* vp = doc->getViewProvider(obj);
        if (vp) vp->setTransformation(plm.toMatrix());
    }
}

void TransformStrategy::resetViewTransform(App::DocumentObject* obj)
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(obj->getDocument());
    std::map<std::string,App::Property*> props;
    obj->getPropertyMap(props);
    // search for the placement property
    std::map<std::string,App::Property*>::iterator jt;
    jt = std::find_if(props.begin(), props.end(), find_transform());
    if (jt != props.end()) {
        Base::Placement local = static_cast<App::PropertyPlacement*>(jt->second)->getValue();
        Gui::ViewProvider* vp = doc->getViewProvider(obj);
        if (vp) vp->setTransformation(local.toMatrix());
    }
    else {
        // No placement found
        Gui::ViewProvider* vp = doc->getViewProvider(obj);
        if (vp) vp->setTransformation(Base::Matrix4D());
    }
}

// ----------------------------------------------------------------------------

DefaultTransformStrategy::DefaultTransformStrategy(QWidget* w) : widget(w)
{
    Gui::SelectionChanges mod;
    mod.Type = Gui::SelectionChanges::SetSelection;
    onSelectionChanged(mod);
}

DefaultTransformStrategy::~DefaultTransformStrategy() = default;

std::set<App::DocumentObject*> DefaultTransformStrategy::transformObjects() const
{
    return selection;
}

void DefaultTransformStrategy::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == SelectionChanges::SetPreselect ||
        msg.Type == SelectionChanges::RmvPreselect)
        return; // nothing to do
    if (msg.Type == SelectionChanges::ClrSelection) {
        widget->setDisabled(true);
        for (const auto & it : selection)
             resetViewTransform(it);
        selection.clear();
        return;
    }

    std::set<App::DocumentObject*> update_selection;
    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
        (App::DocumentObject::getClassTypeId());
    for (const auto & it : sel) {
        if (it->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            // search for a data property
            const App::PropertyGeometry* geo = static_cast<App::GeoFeature*>(it)->getPropertyOfGeometry();
            if (geo) {
                update_selection.insert(it);
            }
        }
    }

    // now we remove all objects which links to another object
    // of the selected objects because if the source object changes
    // it is touched and thus a recompute later would overwrite the
    // changes here anyway
    std::set<App::DocumentObject*> filter;
    for (auto it = update_selection.begin(); it != update_selection.end(); ++it) {
        std::vector<App::DocumentObject*> deps = (*it)->getOutList();
        std::vector<App::DocumentObject*>::iterator jt;
        for (jt = deps.begin(); jt != deps.end(); ++jt) {
            if (update_selection.find(*jt) != update_selection.end()) {
                filter.insert(*it);
                break;
            }
        }
    }

    if (!filter.empty()) {
        std::set<App::DocumentObject*> diff;
        std::insert_iterator< std::set<App::DocumentObject*> > biit(diff, diff.begin());
        std::set_difference(update_selection.begin(), update_selection.end(),
            filter.begin(), filter.end(), biit);
        update_selection = diff;
    }

    // reset transform for all deselected objects
    std::vector<App::DocumentObject*> diff;
    std::back_insert_iterator< std::vector<App::DocumentObject*> > biit(diff);
    std::set_difference(selection.begin(), selection.end(),
        update_selection.begin(), update_selection.end(), biit);
    for (const auto & it : diff)
         resetViewTransform(it);
    selection = update_selection;

    widget->setDisabled(selection.empty());
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::Transform */

Transform::Transform(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), strategy(nullptr)
{
    ui = new Ui_Placement();
    ui->setupUi(this);
    connect(ui->applyButton, &QPushButton::clicked,
            this, &Transform::onApplyButtonClicked);

    ui->resetButton->hide();
    ui->applyIncrementalPlacement->hide();

    ui->closeButton->setText(tr("Cancel"));
    this->setWindowTitle(tr("Transform"));

    // create a signal mapper in order to have one slot to perform the change
    auto signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(this, 0);

    int id = 1;
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (const auto & it : sb) {
        connect(it, qOverload<double>(&QuantitySpinBox::valueChanged), signalMapper, qOverload<>(&QSignalMapper::map));
        signalMapper->setMapping(it, id++);
    }

#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    connect(signalMapper, qOverload<int>(&QSignalMapper::mapped),
            this, &Transform::onTransformChanged);
#else
    connect(signalMapper, &QSignalMapper::mappedInt,
            this, &Transform::onTransformChanged);
#endif

    setTransformStrategy(new DefaultTransformStrategy(this));
}

Transform::~Transform()
{
    delete ui;
    delete strategy;
}

void Transform::setTransformStrategy(TransformStrategy* ts)
{
    if (!ts || ts == strategy)
        return;
    if (strategy)
        delete strategy;
    strategy = ts;
    Base::Vector3d cnt = strategy->getRotationCenter();
    ui->xCnt->setValue(Base::Quantity(cnt.x, Base::Unit::Length));
    ui->yCnt->setValue(Base::Quantity(cnt.y, Base::Unit::Length));
    ui->zCnt->setValue(Base::Quantity(cnt.z, Base::Unit::Length));
    this->setDisabled(strategy->transformObjects().empty());
}

void Transform::showStandardButtons(bool b)
{
    ui->closeButton->setVisible(b);
    ui->oKButton->setVisible(b);
    ui->applyButton->setVisible(b);
}

void Transform::onTransformChanged(int)
{
    Base::Placement plm = this->getPlacementData();
    strategy->applyTransform(plm);
}

void Transform::accept()
{
    onApplyButtonClicked();
    QDialog::accept();
}

void Transform::reject()
{
    strategy->resetTransform();
    QDialog::reject();
}

void Transform::onApplyButtonClicked()
{
    Gui::WaitCursor wc;
    Base::Placement plm = this->getPlacementData();
    Base::Matrix4D mat = plm.toMatrix();
    strategy->commitTransform(mat);

    // nullify the values
    QList<Gui::QuantitySpinBox*> sb = this->findChildren<Gui::QuantitySpinBox*>();
    for (auto & it : sb) {
        it->blockSignals(true);
        it->setValue(0.0);
        it->blockSignals(false);
    }

    Base::Vector3d cnt = strategy->getRotationCenter();
    ui->xCnt->setValue(Base::Quantity(cnt.x, Base::Unit::Length));
    ui->yCnt->setValue(Base::Quantity(cnt.y, Base::Unit::Length));
    ui->zCnt->setValue(Base::Quantity(cnt.z, Base::Unit::Length));
}

Base::Vector3d Transform::getDirection() const
{
    double x = ui->xAxis->value().getValue();
    double y = ui->yAxis->value().getValue();
    double z = ui->zAxis->value().getValue();
    return Base::Vector3d(x, y, z);
}

Base::Placement Transform::getPlacementData() const
{
    int index = ui->rotationInput->currentIndex();
    Base::Rotation rot;
    Base::Vector3d pos;
    Base::Vector3d cnt;

    pos = Base::Vector3d(ui->xPos->value().getValue(),ui->yPos->value().getValue(),ui->zPos->value().getValue());
    cnt = Base::Vector3d(ui->xCnt->value().getValue(),ui->yCnt->value().getValue(),ui->zCnt->value().getValue());

    if (index == 0) {
        Base::Vector3d dir = getDirection();
        rot.setValue(Base::Vector3d(dir.x,dir.y,dir.z),ui->angle->value().getValue()*D_PI/180.0);
    }
    else if (index == 1) {
        rot.setYawPitchRoll(
            ui->yawAngle->value().getValue(),
            ui->pitchAngle->value().getValue(),
            ui->rollAngle->value().getValue());
    }

    Base::Placement p(pos, rot, cnt);
    return p;
}

void Transform::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        ui->closeButton->setText(tr("Cancel"));
        this->setWindowTitle(tr("Transform"));
    }
    else {
        QDialog::changeEvent(e);
    }
}

// ---------------------------------------

TaskTransform::TaskTransform()
{
    this->setButtonPosition(TaskTransform::South);
    dialog = new Transform();
    dialog->showStandardButtons(false);
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), dialog->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(dialog);
    Content.push_back(taskbox);
}

TaskTransform::~TaskTransform() = default;

void TaskTransform::setTransformStrategy(TransformStrategy* ts)
{
    dialog->setTransformStrategy(ts);
}

bool TaskTransform::accept()
{
    dialog->accept();
    return (dialog->result() == QDialog::Accepted);
}

bool TaskTransform::reject()
{
    dialog->reject();
    return (dialog->result() == QDialog::Rejected);
}

void TaskTransform::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        dialog->onApplyButtonClicked();
    }
}

#include "moc_Transform.cpp"
