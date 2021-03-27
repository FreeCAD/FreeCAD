/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QApplication>
# include <QDoubleSpinBox>
# include <QRegExp>
# include <QGridLayout>
# include <QMessageBox>
# include <memory>
#endif

#include "DlgSettingsNavigation.h"
#include "ui_DlgSettingsNavigation.h"
#include "MainWindow.h"
#include "NavigationStyle.h"
#include "PrefWidgets.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ui_MouseButtons.h"
#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsNavigation */

/**
 *  Constructs a DlgSettingsNavigation which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsNavigation::DlgSettingsNavigation(QWidget* parent)
    : PreferencePage( parent )
    , ui(new Ui_DlgSettingsNavigation)
    , q0(0), q1(0), q2(0), q3(1)
{
    ui->setupUi(this);
    retranslate();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsNavigation::~DlgSettingsNavigation()
{
}

void DlgSettingsNavigation::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    QVariant data = ui->comboNavigationStyle->itemData(ui->comboNavigationStyle->currentIndex(), Qt::UserRole);
    hGrp->SetASCII("NavigationStyle", (const char*)data.toByteArray());

    int index = ui->comboOrbitStyle->currentIndex();
    hGrp->SetInt("OrbitStyle", index);

    index = ui->naviCubeCorner->currentIndex();
    hGrp->SetInt("CornerNaviCube", index);

    index = ui->comboRotationMode->currentIndex();
    hGrp->SetInt("RotationMode", index);

    ui->checkBoxZoomAtCursor->onSave();
    ui->checkBoxInvertZoom->onSave();
    ui->checkBoxDisableTilt->onSave();
    ui->spinBoxZoomStep->onSave();
    ui->CheckBox_UseAutoRotation->onSave();
    ui->qspinNewDocScale->onSave();
    ui->prefStepByTurn->onSave();
    ui->naviCubeToNearest->onSave();
    ui->prefCubeSize->onSave();

    bool showNaviCube = ui->groupBoxNaviCube->isChecked();
    hGrp->SetBool("ShowNaviCube", showNaviCube);

    QVariant camera = ui->comboNewDocView->itemData(ui->comboNewDocView->currentIndex(), Qt::UserRole);
    hGrp->SetASCII("NewDocumentCameraOrientation", (const char*)camera.toByteArray());
    if (camera == QByteArray("Custom")) {
        ParameterGrp::handle hCustom = hGrp->GetGroup("Custom");
        hCustom->SetFloat("Q0", q0);
        hCustom->SetFloat("Q1", q1);
        hCustom->SetFloat("Q2", q2);
        hCustom->SetFloat("Q3", q3);
    }
}

void DlgSettingsNavigation::loadSettings()
{
    ui->checkBoxZoomAtCursor->onRestore();
    ui->checkBoxInvertZoom->onRestore();
    ui->checkBoxDisableTilt->onRestore();
    ui->spinBoxZoomStep->onRestore();
    ui->CheckBox_UseAutoRotation->onRestore();
    ui->qspinNewDocScale->onRestore();
    ui->prefStepByTurn->onRestore();
    ui->naviCubeToNearest->onRestore();
    ui->prefCubeSize->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    std::string model = hGrp->GetASCII("NavigationStyle",CADNavigationStyle::getClassTypeId().getName());
    int index = ui->comboNavigationStyle->findData(QByteArray(model.c_str()));
    if (index > -1) ui->comboNavigationStyle->setCurrentIndex(index);

    index = hGrp->GetInt("OrbitStyle", int(NavigationStyle::Trackball));
    index = Base::clamp(index, 0, ui->comboOrbitStyle->count()-1);
    ui->comboOrbitStyle->setCurrentIndex(index);

    index = hGrp->GetInt("CornerNaviCube", 1);
    ui->naviCubeCorner->setCurrentIndex(index);

    index = hGrp->GetInt("RotationMode", 1);
    ui->comboRotationMode->setCurrentIndex(index);

    bool showNaviCube = hGrp->GetBool("ShowNaviCube", true);
    ui->groupBoxNaviCube->setChecked(showNaviCube);

    ui->comboNewDocView->addItem(tr("Isometric"), QByteArray("Isometric"));
    ui->comboNewDocView->addItem(tr("Dimetric"), QByteArray("Dimetric"));
    ui->comboNewDocView->addItem(tr("Trimetric"), QByteArray("Trimetric"));
    ui->comboNewDocView->addItem(tr("Top"), QByteArray("Top"));
    ui->comboNewDocView->addItem(tr("Front"), QByteArray("Front"));
    ui->comboNewDocView->addItem(tr("Left"), QByteArray("Left"));
    ui->comboNewDocView->addItem(tr("Right"), QByteArray("Right"));
    ui->comboNewDocView->addItem(tr("Rear"), QByteArray("Rear"));
    ui->comboNewDocView->addItem(tr("Bottom"), QByteArray("Bottom"));
    ui->comboNewDocView->addItem(tr("Custom"), QByteArray("Custom"));
    std::string camera = hGrp->GetASCII("NewDocumentCameraOrientation", "Trimetric");
    index = ui->comboNewDocView->findData(QByteArray(camera.c_str()));
    if (index > -1) ui->comboNewDocView->setCurrentIndex(index);
    if (camera == "Custom") {
        ParameterGrp::handle hCustom = hGrp->GetGroup("Custom");
        q0 = hCustom->GetFloat("Q0", q0);
        q1 = hCustom->GetFloat("Q1", q1);
        q2 = hCustom->GetFloat("Q2", q2);
        q3 = hCustom->GetFloat("Q3", q3);
    }

    connect(ui->comboNewDocView, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onNewDocViewChanged(int)));
}

void DlgSettingsNavigation::on_mouseButton_clicked()
{
    QDialog dlg(this);
    Ui_MouseButtons uimb;
    uimb.setupUi(&dlg);

    QVariant data = ui->comboNavigationStyle->itemData(ui->comboNavigationStyle->currentIndex(), Qt::UserRole);
    void* instance = Base::Type::createInstanceByName((const char*)data.toByteArray());
    std::unique_ptr<UserNavigationStyle> ns(static_cast<UserNavigationStyle*>(instance));
    uimb.groupBox->setTitle(uimb.groupBox->title()+QString::fromLatin1(" ")+ui->comboNavigationStyle->currentText());
    QString descr;
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::SELECTION));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    uimb.selectionLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::PANNING));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    uimb.panningLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::DRAGGING));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    uimb.rotationLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::ZOOMING));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    uimb.zoomingLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    dlg.exec();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsNavigation::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int navigation = ui->comboNavigationStyle->currentIndex();
        int orbit = ui->comboOrbitStyle->currentIndex();
        int corner = ui->naviCubeCorner->currentIndex();
        ui->retranslateUi(this);
        retranslate();
        ui->comboNavigationStyle->setCurrentIndex(navigation);
        ui->comboOrbitStyle->setCurrentIndex(orbit);
        ui->naviCubeCorner->setCurrentIndex(corner);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsNavigation::retranslate()
{
    ui->comboNavigationStyle->clear();

    // add submenu at the end to select navigation style
    std::map<Base::Type, std::string> styles = UserNavigationStyle::getUserFriendlyNames();
    for (std::map<Base::Type, std::string>::iterator it = styles.begin(); it != styles.end(); ++it) {
        QByteArray data(it->first.getName());
        QString name = QApplication::translate(it->first.getName(), it->second.c_str());

        ui->comboNavigationStyle->addItem(name, data);
    }
}

void DlgSettingsNavigation::onNewDocViewChanged(int index)
{
    QVariant camera = ui->comboNewDocView->itemData(index, Qt::UserRole);
    if (camera == QByteArray("Custom")) {
        CameraDialog dlg(this);
        dlg.setValues(q0, q1, q2, q3);
        if (dlg.exec()) {
            dlg.getValues(q0, q1, q2, q3);
        }
    }
}

// ----------------------------------------------------------------------------

CameraDialog::CameraDialog(QWidget* parent)
    : QDialog(parent)
{
    this->setWindowTitle(tr("Camera settings"));

    QGridLayout *gridLayout;
    gridLayout = new QGridLayout(this);

    QGroupBox *groupBox;
    groupBox = new QGroupBox(this);
    groupBox->setTitle(tr("Orientation"));
    gridLayout->addWidget(groupBox, 0, 0, 1, 1);

    QDialogButtonBox *buttonBox;
    buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    gridLayout->addWidget(buttonBox, 3, 0, 1, 1);

    QGridLayout *layout;
    layout = new QGridLayout(groupBox);

    // Q0
    QLabel* label0 = new QLabel(groupBox);
    label0->setText(tr("Q0"));
    layout->addWidget(label0, 0, 0, 1, 1);

    sb0 = new QDoubleSpinBox(groupBox);
    sb0->setRange(-1, 1);
    sb0->setSingleStep(0.1);
    layout->addWidget(sb0, 0, 1, 1, 1);

    // Q1
    QLabel* label1 = new QLabel(groupBox);
    label1->setText(tr("Q1"));
    layout->addWidget(label1, 1, 0, 1, 1);

    sb1 = new QDoubleSpinBox(groupBox);
    sb1->setRange(-1, 1);
    sb1->setSingleStep(0.1);
    layout->addWidget(sb1, 1, 1, 1, 1);

    // Q2
    QLabel* label2 = new QLabel(groupBox);
    label2->setText(tr("Q2"));
    layout->addWidget(label2, 2, 0, 1, 1);

    sb2 = new QDoubleSpinBox(groupBox);
    sb2->setRange(-1, 1);
    sb2->setSingleStep(0.1);
    layout->addWidget(sb2, 2, 1, 1, 1);

    // Q3
    QLabel* label3 = new QLabel(groupBox);
    label3->setText(tr("Q3"));
    layout->addWidget(label3, 3, 0, 1, 1);

    sb3 = new QDoubleSpinBox(groupBox);
    sb3->setRange(-1, 1);
    sb3->setSingleStep(0.1);
    layout->addWidget(sb3, 3, 1, 1, 1);

    QPushButton *currentViewButton;
    currentViewButton = new QPushButton(this);
    currentViewButton->setText(tr("Current view"));
    currentViewButton->setObjectName(QString::fromLatin1("currentView"));
    layout->addWidget(currentViewButton, 4, 1, 2, 1);

    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QMetaObject::connectSlotsByName(this);
}

CameraDialog::~CameraDialog()
{
}

void CameraDialog::setValues(double q0, double q1, double q2, double q3)
{
    sb0->setValue(q0);
    sb1->setValue(q1);
    sb2->setValue(q2);
    sb3->setValue(q3);
}

void CameraDialog::getValues(double& q0, double& q1, double& q2, double& q3) const
{
    q0 = sb0->value();
    q1 = sb1->value();
    q2 = sb2->value();
    q3 = sb3->value();
}

void CameraDialog::on_currentView_clicked()
{
    View3DInventor* mdi = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (mdi) {
        SbRotation rot = mdi->getViewer()->getCameraOrientation();
        const float* q = rot.getValue();
        sb0->setValue(q[0]);
        sb1->setValue(q[1]);
        sb2->setValue(q[2]);
        sb3->setValue(q[3]);
    }
}

#include "moc_DlgSettingsNavigation.cpp"

