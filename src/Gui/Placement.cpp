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

#include "Placement.h"
#include "ui_Placement.h"
#include <Gui/DockWindowManager.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <App/Document.h>
#include <App/PropertyGeo.h>
#include <Base/Console.h>

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
            if (elem.second->StatusBits.test(2) || elem.second->StatusBits.test(3))
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

Placement::Placement(QWidget* parent, Qt::WFlags fl)
  : Gui::LocationDialog(parent, fl)
{
    propertyName = "Placement"; // default name
    ui = new Ui_PlacementComp(this);
    ui->applyPlacementChange->hide();

    ui->angle->setSuffix(QString::fromUtf8(" \xc2\xb0"));
    ui->yawAngle->setSuffix(QString::fromUtf8(" \xc2\xb0"));
    ui->pitchAngle->setSuffix(QString::fromUtf8(" \xc2\xb0"));
    ui->rollAngle->setSuffix(QString::fromUtf8(" \xc2\xb0"));

    // create a signal mapper in order to have one slot to perform the change
    signalMapper = new QSignalMapper(this);
    connect(this, SIGNAL(directionChanged()), signalMapper, SLOT(map()));
    signalMapper->setMapping(this, 0);

    int id = 1;
    QList<QDoubleSpinBox*> sb = this->findChildren<QDoubleSpinBox*>();
    for (QList<QDoubleSpinBox*>::iterator it = sb.begin(); it != sb.end(); ++it) {
        connect(*it, SIGNAL(valueChanged(double)), signalMapper, SLOT(map()));
        signalMapper->setMapping(*it, id++);
    }

    connect(signalMapper, SIGNAL(mapped(int)),
            this, SLOT(onPlacementChanged(int)));
    connectAct = Application::Instance->signalActiveDocument.connect
        (boost::bind(&Placement::slotActiveDocument, this, _1));
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (activeDoc) documents.insert(activeDoc->getName());
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

void Placement::slotActiveDocument(const Gui::Document& doc)
{
    documents.insert(doc.getDocument()->getName());
}

void Placement::revertTransformation()
{
    for (std::set<std::string>::iterator it = documents.begin(); it != documents.end(); ++it) {
        Gui::Document* document = Application::Instance->getDocument(it->c_str());
        if (!document) continue;

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
                    Base::Placement cur = static_cast<App::PropertyPlacement*>(jt->second)->getValue();
                    Gui::ViewProvider* vp = document->getViewProvider(*it);
                    if (vp) vp->setTransformation(cur.toMatrix());
                }
            }
        }
    }
}

void Placement::applyPlacement(const Base::Placement& p, bool incremental, bool data)
{
    Gui::Document* document = Application::Instance->activeDocument();
    if (!document) return;

    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
        (App::DocumentObject::getClassTypeId(), document->getDocument()->getName());
    if (!sel.empty()) {
        if (data) {
            document->openCommand("Placement");
            for (std::vector<App::DocumentObject*>::iterator it=sel.begin();it!=sel.end();++it) {
                std::map<std::string,App::Property*> props;
                (*it)->getPropertyMap(props);
                // search for the placement property
                std::map<std::string,App::Property*>::iterator jt;
                jt = std::find_if(props.begin(), props.end(), find_placement(this->propertyName));
                if (jt != props.end()) {
                    Base::Placement cur = static_cast<App::PropertyPlacement*>(jt->second)->getValue();
                    if (incremental)
                        cur = p * cur;
                    else
                        cur = p;

                    Base::Vector3d pos = cur.getPosition();
                    const Base::Rotation& rt = cur.getRotation();
                    QString cmd = QString::fromAscii(
                        "App.getDocument(\"%1\").%2.Placement="
                        "App.Placement("
                        "App.Vector(%3,%4,%5),"
                        "App.Rotation(%6,%7,%8,%9))\n")
                        .arg(QLatin1String((*it)->getDocument()->getName()))
                        .arg(QLatin1String((*it)->getNameInDocument()))
                        .arg(pos.x,0,'g',6)
                        .arg(pos.y,0,'g',6)
                        .arg(pos.z,0,'g',6)
                        .arg(rt[0],0,'g',6)
                        .arg(rt[1],0,'g',6)
                        .arg(rt[2],0,'g',6)
                        .arg(rt[3],0,'g',6);
                    Application::Instance->runPythonCode((const char*)cmd.toAscii());
                }
            }

            document->commitCommand();
            try {
                document->getDocument()->recompute();
            }
            catch (...) {
            }
        }
        // apply transformation only on view matrix not on placement property
        else {
            for (std::vector<App::DocumentObject*>::iterator it=sel.begin();it!=sel.end();++it) {
                std::map<std::string,App::Property*> props;
                (*it)->getPropertyMap(props);
                // search for the placement property
                std::map<std::string,App::Property*>::iterator jt;
                jt = std::find_if(props.begin(), props.end(), find_placement(this->propertyName));
                if (jt != props.end()) {
                    Base::Placement cur = static_cast<App::PropertyPlacement*>(jt->second)->getValue();
                    if (incremental)
                        cur = p * cur;
                    else
                        cur = p;

                    Gui::ViewProvider* vp = document->getViewProvider(*it);
                    if (vp) vp->setTransformation(cur.toMatrix());
                }
            }
        }
    }
    else {
        Base::Console().Warning("No object selected.\n");
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
    applyPlacement(plm, incr, false);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    /*emit*/ placementChanged(data, incr, false);
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

void Placement::reject()
{
    Base::Placement plm;
    applyPlacement(plm, true, false);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    /*emit*/ placementChanged(data, true, false);

    revertTransformation();
    QDialog::reject();
}

void Placement::accept()
{
    on_applyButton_clicked();

    revertTransformation();
    QDialog::accept();
}

void Placement::on_applyButton_clicked()
{
    // If there are listeners to the 'placementChanged' signal we rely
    // on that the listener updates any placement. If no listeners
    // are connected the placement is applied to all selected objects
    // automatically.
    bool incr = ui->applyIncrementalPlacement->isChecked();
    Base::Placement plm = this->getPlacement();
    applyPlacement(plm, incr, true);

    QVariant data = QVariant::fromValue<Base::Placement>(plm);
    /*emit*/ placementChanged(data, incr, true);

    if (ui->applyIncrementalPlacement->isChecked()) {
        QList<QDoubleSpinBox*> sb = this->findChildren<QDoubleSpinBox*>();
        for (QList<QDoubleSpinBox*>::iterator it = sb.begin(); it != sb.end(); ++it) {
            (*it)->blockSignals(true);
            (*it)->setValue(0.0);
            (*it)->blockSignals(false);
        }
    }
}

void Placement::on_resetButton_clicked()
{
    QList<QDoubleSpinBox*> sb = this->findChildren<QDoubleSpinBox*>();
    for (QList<QDoubleSpinBox*>::iterator it = sb.begin(); it != sb.end(); ++it) {
        (*it)->blockSignals(true);
        (*it)->setValue(0.0);
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
    ui->xPos->setValue(p.getPosition().x);
    ui->yPos->setValue(p.getPosition().y);
    ui->zPos->setValue(p.getPosition().z);

    double Y,P,R;
    p.getRotation().getYawPitchRoll(Y,P,R);
    ui->yawAngle->setValue(Y);
    ui->pitchAngle->setValue(P);
    ui->rollAngle->setValue(R);

    // check if the user-defined direction is already there
    bool newitem = true;
    double angle;
    Base::Vector3d axis;
    p.getRotation().getValue(axis, angle);
    ui->angle->setValue(angle*180.0/D_PI);
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
        QString display = QString::fromAscii("(%1,%2,%3)")
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

Base::Placement Placement::getPlacementData() const
{
    int index = ui->rotationInput->currentIndex();
    Base::Rotation rot;
    Base::Vector3d pos;
    Base::Vector3d cnt;

    pos = Base::Vector3d(ui->xPos->value(),ui->yPos->value(),ui->zPos->value());
    cnt = Base::Vector3d(ui->xCnt->value(),ui->yCnt->value(),ui->zCnt->value());

    if (index == 0) {
        Base::Vector3d dir = getDirection();
        rot.setValue(Base::Vector3d(dir.x,dir.y,dir.z),ui->angle->value()*D_PI/180.0);
    }
    else if (index == 1) {
        rot.setYawPitchRoll(
            ui->yawAngle->value(),
            ui->pitchAngle->value(),
            ui->rollAngle->value());
    }

    Base::Placement p(pos, rot, cnt);
    return p;
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

DockablePlacement::DockablePlacement(QWidget* parent, Qt::WFlags fl) : Placement(parent, fl)
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
