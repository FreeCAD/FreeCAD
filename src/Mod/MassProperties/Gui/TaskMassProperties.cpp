// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "TaskMassProperties.h"
#include "UnitSystem.h"
#include "Mod/MassProperties/App/MassPropertiesResult.h"

#include <QtWidgets>

#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Application.h>

#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>


#include <App/DocumentObject.h>
#include <App/Document.h>
#include <App/Datums.h>
#include <App/Origin.h>

#include <Mod/Part/App/PartFeature.h>

#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>

using namespace MassPropertiesGui;

MassPropertiesData currentInfo;
std::string currentMode = "Global";
App::DocumentObject* currentDatum = nullptr;
std::string unitSystem = "mm, kg, kg·mm²";

TaskMassProperties::TaskMassProperties()
    : Gui::SelectionObserver(true)
    , selectingCustomCoordSystem(false)
{
    this->setButtonPosition(TaskMassProperties::North);

    auto physicalProperties = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("umf-measurement"),
        tr("Physical Properties"),
        true,
        nullptr
    );
    setupShortcuts(physicalProperties);
    QBoxLayout* physicalLayout = physicalProperties->groupLayout();
    physicalLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* objectsLabel = new QLabel(tr("Objects to measure"));
    objectsLabel->setWordWrap(true);
    physicalLayout->addWidget(objectsLabel);

    listWidget = new QListWidget();
    
    listWidget->setMaximumHeight(50);
    physicalLayout->addWidget(listWidget);
    
    QRadioButton *globalRadioButton = new QRadioButton(tr("Global Coordinate system"));
    globalRadioButton->setChecked(true);
    QObject::connect(globalRadioButton, &QRadioButton::toggled, [=](bool checked){
        if (checked) {
            onCoordinateSystemChanged("Global");
        }
    });
    QRadioButton *centerOfGravityRadioButton = new QRadioButton(tr("Center of Gravity"));
    QObject::connect(centerOfGravityRadioButton, &QRadioButton::toggled, [=](bool checked){
        if (checked) {
            onCoordinateSystemChanged("Center of Gravity");
        }
    });
    QRadioButton *customRadioButton = new QRadioButton(tr("Custom"));
    QObject::connect(customRadioButton, &QRadioButton::toggled, [=](bool checked){
        if (checked) {
            onCoordinateSystemChanged("Custom");
        }
    });
    QButtonGroup *coordinateSystemGroup = new QButtonGroup(physicalProperties);
    coordinateSystemGroup->addButton(globalRadioButton);
    coordinateSystemGroup->addButton(centerOfGravityRadioButton);
    coordinateSystemGroup->addButton(customRadioButton);

    QFormLayout* customForm = new QFormLayout();

    customEdit = new QLineEdit();
    customEdit->setReadOnly(true);
    QPushButton* selectButton = new QPushButton(tr("Select..."));
    QObject::connect(selectButton, &QPushButton::pressed, this, &TaskMassProperties::onSelectCustomCoordinateSystem);
    selectButton->setEnabled(true);

    QHBoxLayout* customControls = new QHBoxLayout();
    customControls->setContentsMargins(0, 0, 0, 0);
    customControls->addWidget(customEdit);
    customControls->addWidget(selectButton);

    physicalLayout->addWidget(globalRadioButton);
    physicalLayout->addWidget(centerOfGravityRadioButton);

    customForm->addRow(customRadioButton, customControls);
    physicalLayout->addLayout(customForm);

    QComboBox *UnitsComboBox = new QComboBox(physicalProperties);
    UnitsComboBox->addItem(QStringLiteral("mm, kg, kg·mm²"));
    UnitsComboBox->addItem(QStringLiteral("m, kg, kg·m²"));
    UnitsComboBox->addItem(QStringLiteral("in, lb, lb·in²"));
    UnitsComboBox->addItem(QStringLiteral("ft, lb, lb·ft²"));
    physicalLayout->addWidget(UnitsComboBox);

    unitSystem = UnitSystem::getPreferredSystemName();
    UnitsComboBox->setCurrentIndex(UnitSystem::getSystemIndex(unitSystem));

    QObject::connect(UnitsComboBox, &QComboBox::currentTextChanged, [=](const QString &text){
        unitSystem = text.toStdString();
        update(Gui::SelectionChanges());
    });

    Content.emplace_back(physicalProperties);

    auto basicProperties = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("umf-measurement"),
        tr("Basic Properties"),
        true,
        nullptr
    );

    QBoxLayout* basicLayout = basicProperties->groupLayout();
    basicLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* volumeLabel = new QLabel(tr("Volume: "));
    volumeEdit = new QLineEdit();
    volumeEdit->setReadOnly(true);
    QLabel* massLabel = new QLabel(tr("Mass: "));
    massEdit = new QLineEdit();
    massEdit->setReadOnly(true);
    QLabel* densityLabel = new QLabel(tr("Density: "));
    densityEdit = new QLineEdit();
    densityEdit->setReadOnly(true);
    QLabel* surfaceAreaLabel = new QLabel(tr("Surface Area: "));
    surfaceAreaEdit = new QLineEdit();
    surfaceAreaEdit->setReadOnly(true);

    QFormLayout* basicForm = new QFormLayout();
    basicForm->addRow(volumeLabel, volumeEdit);
    basicForm->addRow(massLabel, massEdit);
    basicForm->addRow(densityLabel, densityEdit);
    basicForm->addRow(surfaceAreaLabel, surfaceAreaEdit);

    basicLayout->addLayout(basicForm);

    Content.emplace_back(basicProperties);
    
    auto centerOfGravity = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("umf-measurement"),
        tr("Center of Gravity"),
        true,
        nullptr
    );

    QBoxLayout* cogLayout = centerOfGravity->groupLayout();
    cogLayout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout* cogFields = new QHBoxLayout();
    cogFields->setContentsMargins(0, 0, 0, 0);

    QLabel* cogXLabel = new QLabel(QStringLiteral("X:"));
    cogXText = new QLineEdit();
    cogXText->setReadOnly(true);
    QLabel* cogYLabel = new QLabel(QStringLiteral("Y:"));
    cogYText = new QLineEdit();
    cogYText->setReadOnly(true);
    QLabel* cogZLabel = new QLabel(QStringLiteral("Z:"));
    cogZText = new QLineEdit();
    cogZText->setReadOnly(true);

    cogFields->addWidget(cogXLabel);
    cogFields->addWidget(cogXText);
    cogFields->addSpacing(8);
    cogFields->addWidget(cogYLabel);
    cogFields->addWidget(cogYText);
    cogFields->addSpacing(8);
    cogFields->addWidget(cogZLabel);
    cogFields->addWidget(cogZText);

    cogLayout->addLayout(cogFields);

    QPushButton* cogDatumButton = new QPushButton(tr("Create Datum Point"));
    QObject::connect(cogDatumButton, &QPushButton::pressed, this, &TaskMassProperties::onCogDatumButtonPressed);
    QHBoxLayout* cogButtonLayout = new QHBoxLayout();
    cogButtonLayout->addStretch();
    cogButtonLayout->addWidget(cogDatumButton);
    cogButtonLayout->addStretch();
    cogLayout->addLayout(cogButtonLayout);
    Content.emplace_back(centerOfGravity);

    auto centerOfVolume = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("umf-measurement"),
        tr("Center of Volume"),
        true,
        nullptr
    );

    QBoxLayout* covLayout = centerOfVolume->groupLayout();
    covLayout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout* covFields = new QHBoxLayout();
    covFields->setContentsMargins(0, 0, 0, 0);

    QLabel* covXLabel = new QLabel(QStringLiteral("X:"));
    covXText = new QLineEdit();
    covXText->setReadOnly(true);
    QLabel* covYLabel = new QLabel(QStringLiteral("Y:"));
    covYText = new QLineEdit();
    covYText->setReadOnly(true);
    QLabel* covZLabel = new QLabel(QStringLiteral("Z:"));
    covZText = new QLineEdit();
    covZText->setReadOnly(true);

    covFields->addWidget(covXLabel);
    covFields->addWidget(covXText);
    covFields->addSpacing(8);
    covFields->addWidget(covYLabel);
    covFields->addWidget(covYText);
    covFields->addSpacing(8);
    covFields->addWidget(covZLabel);
    covFields->addWidget(covZText);

    covLayout->addLayout(covFields);

    QPushButton* covDatumButton = new QPushButton(tr("Create Datum Point"));
    QObject::connect(covDatumButton, &QPushButton::pressed, this, &TaskMassProperties::onCovDatumButtonPressed);
    QHBoxLayout* covButtonLayout = new QHBoxLayout();
    covButtonLayout->addStretch();
    covButtonLayout->addWidget(covDatumButton);
    covButtonLayout->addStretch();
    covLayout->addLayout(covButtonLayout);

    Content.emplace_back(centerOfVolume);

    auto inertiaProperties = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("umf-measurement"),
        tr("Inertia Properties"),
        true,
        nullptr
    );

    QBoxLayout* inertiaLayout = inertiaProperties->groupLayout();
    inertiaLayout->setContentsMargins(10, 10, 10, 10);

    inertiaMatrixWidget = new QWidget();
    QGridLayout* inertiaGrid = new QGridLayout(inertiaMatrixWidget);
    inertiaGrid->setContentsMargins(0, 0, 0, 0);

    QLabel* inertiaJoxLabel = new QLabel(QStringLiteral("Jox:"));
    inertiaJoxText = new QLineEdit(); 
    inertiaJoxText->setReadOnly(true);
    QLabel* inertiaJxyLabel = new QLabel(QStringLiteral("Jxy:"));
    inertiaJxyText = new QLineEdit(); 
    inertiaJxyText->setReadOnly(true);
    QLabel* inertiaJzxLabel = new QLabel(QStringLiteral("Jzx:"));
    inertiaJzxText = new QLineEdit(); 
    inertiaJzxText->setReadOnly(true);

    QLabel* inertiaJoyLabel = new QLabel(QStringLiteral("Joy:"));
    inertiaJoyText = new QLineEdit(); 
    inertiaJoyText->setReadOnly(true);
    QLabel* inertiaJzyLabel = new QLabel(QStringLiteral("Jzy:"));
    inertiaJzyText = new QLineEdit(); 
    inertiaJzyText->setReadOnly(true);

    QLabel* inertiaJozLabel = new QLabel(QStringLiteral("Joz:"));
    inertiaJozText = new QLineEdit(); 
    inertiaJozText->setReadOnly(true);

    inertiaGrid->addWidget(inertiaJoxLabel, 0, 0);
    inertiaGrid->addWidget(inertiaJoxText, 0, 1);
    inertiaGrid->addWidget(inertiaJxyLabel, 0, 2);
    inertiaGrid->addWidget(inertiaJxyText, 0, 3);
    inertiaGrid->addWidget(inertiaJzxLabel, 0, 4);
    inertiaGrid->addWidget(inertiaJzxText, 0, 5);

    inertiaGrid->addWidget(inertiaJoyLabel, 1, 2);
    inertiaGrid->addWidget(inertiaJoyText, 1, 3);
    inertiaGrid->addWidget(inertiaJzyLabel, 1, 4);
    inertiaGrid->addWidget(inertiaJzyText, 1, 5);

    inertiaGrid->addWidget(inertiaJozLabel, 2, 4);
    inertiaGrid->addWidget(inertiaJozText, 2, 5);

    inertiaLayout->addWidget(inertiaMatrixWidget);
    inertiaDiagWidget = new QWidget();
    QHBoxLayout* inertiaDiag = new QHBoxLayout(inertiaDiagWidget);
    inertiaDiag->setContentsMargins(0, 0, 0, 0);
    QLabel* inertiaJxLabel = new QLabel(QStringLiteral("Jx:"));
    inertiaJxText = new QLineEdit();
    inertiaJxText->setReadOnly(true);
    QLabel* inertiaJyLabel = new QLabel(QStringLiteral("Jy:"));
    inertiaJyText = new QLineEdit(); 
    inertiaJyText->setReadOnly(true);
    QLabel* inertiaJzLabel = new QLabel(QStringLiteral("Jz:"));
    inertiaJzText = new QLineEdit(); 
    inertiaJzText->setReadOnly(true);

    inertiaDiag->addWidget(inertiaJxLabel);
    inertiaDiag->addWidget(inertiaJxText);
    inertiaDiag->addSpacing(8);
    inertiaDiag->addWidget(inertiaJyLabel);
    inertiaDiag->addWidget(inertiaJyText);
    inertiaDiag->addSpacing(8);
    inertiaDiag->addWidget(inertiaJzLabel);
    inertiaDiag->addWidget(inertiaJzText);

    inertiaLayout->addWidget(inertiaDiagWidget);

    inertiaLcsWidget = new QWidget();
    QPushButton* inertiaLCSButton = new QPushButton(tr("Create principal axis LCS"));
    QObject::connect(inertiaLCSButton, &QPushButton::pressed, this, &TaskMassProperties::onLcsButtonPressed);
    QHBoxLayout* inertiaLCSButtonLayout = new QHBoxLayout(inertiaLcsWidget);
    inertiaLCSButtonLayout->addStretch();
    inertiaLCSButtonLayout->addWidget(inertiaLCSButton);
    inertiaLCSButtonLayout->addStretch();
    inertiaLayout->addWidget(inertiaLcsWidget);

    axisInertiaWidget = new QWidget();
    QHBoxLayout* axisDiag = new QHBoxLayout(axisInertiaWidget);

    QLabel* axisLabel = new QLabel(tr("Inertia around axis: "));
    axisLabel->setWordWrap(true);
    axisDiag->addWidget(axisLabel);

    axisInertiaText = new QLineEdit();
    axisInertiaText->setReadOnly(true);

    axisDiag->addWidget(axisInertiaText);

    inertiaLayout->addWidget(axisInertiaWidget);

    Content.emplace_back(inertiaProperties);

    currentMode = "Global";
    unitSystem = UnitSystem::getPreferredSystemName();

    QTimer::singleShot(0, this, &TaskMassProperties::invoke);

    updateInertiaVisibility();
    update(Gui::SelectionChanges());
}

TaskMassProperties::~TaskMassProperties()
{
}

void TaskMassProperties::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* ok = box->button(QDialogButtonBox::Ok);
    ok->setVisible(false);

    QPushButton* close = box->button(QDialogButtonBox::Abort);
    close->setText(tr("Close"));
}

void TaskMassProperties::invoke()
{
}

bool TaskMassProperties::accept()
{
    return true;
}

bool TaskMassProperties::reject()
{
    Gui::Control().closeDialog();
    return true;
}

void TaskMassProperties::setupShortcuts(QWidget* parent) 
{
    auto shortcutQuit = new QShortcut(parent);
    shortcutQuit->setKey(QKeySequence(QStringLiteral("ESC")));
    shortcutQuit->setContext(Qt::ApplicationShortcut);
    connect(shortcutQuit, &QShortcut::activated, this, &TaskMassProperties::quit);
}

void TaskMassProperties::quit()
{
    if (this->hasSelection()) {
        this->clearFields();
    }
    else {
        this->reject();
    }
}

bool TaskMassProperties::hasSelection() const
{
    return !Gui::Selection().getSelection().empty();
}

void TaskMassProperties::clearFields()
{
    Gui::Selection().clearSelection();
    tryupdate();
}


void TaskMassProperties::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type != Gui::SelectionChanges::AddSelection
        && msg.Type != Gui::SelectionChanges::RmvSelection
        && msg.Type != Gui::SelectionChanges::SetSelection
        && msg.Type != Gui::SelectionChanges::ClrSelection) {

        return;
    }

    update(msg);
}

void TaskMassProperties::update(const Gui::SelectionChanges& msg)
{
    try {
        tryupdate();
    }
    catch (const std::exception& e) {
        Base::Console().error("Mass Properties update failed: %s", e.what());
    }
}


void TaskMassProperties::tryupdate()
{
    if (isUpdating) return;
    
    auto guiSelection = Gui::Selection().getSelection();
    if (guiSelection.empty()) {
        listWidget->clear();
        volumeEdit->clear();
        massEdit->clear();
        densityEdit->clear();
        surfaceAreaEdit->clear();
        cogXText->clear();
        cogYText->clear();
        cogZText->clear();
        covXText->clear();
        covYText->clear();
        covZText->clear();
        inertiaJoxText->clear();
        inertiaJxyText->clear();
        inertiaJzxText->clear();
        inertiaJoyText->clear();
        inertiaJzyText->clear();
        inertiaJozText->clear();
        inertiaJxText->clear();
        inertiaJyText->clear();
        inertiaJzText->clear();
        axisInertiaText->clear();
        return;
    }

    bool hasSubSelection = false;
    std::vector<App::DocumentObject*> wholeObjects;
    
    for (const auto& sel : guiSelection) {
        if (sel.SubName && strlen(sel.SubName) > 0) {
            hasSubSelection = true;
        }
        if (sel.pObject) {
            if (std::find(wholeObjects.begin(), wholeObjects.end(), sel.pObject) == wholeObjects.end()) {
                wholeObjects.push_back(sel.pObject);
            }
        }
    }
    
    if (hasSubSelection) {
        isUpdating = true;
        Gui::Selection().clearSelection();
        for (auto obj : wholeObjects) {
            if (obj && obj->getDocument()) {
                Gui::Selection().addSelection(obj->getDocument()->getName(),obj->getNameInDocument());
            }
        }
        isUpdating = false;
        tryupdate();
        return;
    }

    std::vector<App::DocumentObject*> objectsToMeasure;
    App::DocumentObject const* referenceDatum = nullptr;
    
    listWidget->clear();
    
    if (selectingCustomCoordSystem) {
        customEdit->clear();
        for (const auto& selObj : guiSelection) {
            if (selObj.pObject) {
                auto datum = dynamic_cast<App::DatumElement*>(selObj.pObject);
                if (datum && datum->getLCS()) {
                    customEdit->setText(QString::fromStdString(selObj.pObject->getFullLabel()));
                    currentDatum = selObj.pObject;
                    selectingCustomCoordSystem = false;
                    break;
                }
                else if (selObj.pObject->isDerivedFrom<App::Line>()) {
                    customEdit->setText(QString::fromStdString(selObj.pObject->getFullLabel()));
                    currentDatum = selObj.pObject;
                    selectingCustomCoordSystem = false;
                    break;
                }

            }
        }
    }
    
    for (const auto& selObj : guiSelection) {
        if (selObj.pObject) {
            std::string typeName = selObj.pObject->getTypeId().getName();
            auto lcs = dynamic_cast<App::DatumElement*>(selObj.pObject);
            if ((lcs && lcs->getLCS()) 
                || typeName == "PartDesign::CoordinateSystem"
                || selObj.pObject->isDerivedFrom<App::Line>())
            {
                if (currentMode == "Custom" && !selectingCustomCoordSystem) {
                    referenceDatum = currentDatum;
                    break;
                }
                continue;
            }
            listWidget->addItem(QString::fromStdString(selObj.pObject->getFullLabel())); 
            objectsToMeasure.push_back(selObj.pObject);
        }
    }

    if (currentMode == "Custom") {
        referenceDatum = currentDatum;
    }
    else {
        customEdit->clear();
    }

    updateInertiaVisibility();
    
    MassPropertiesData info = CalculateMassProperties(objectsToMeasure, currentMode, referenceDatum);

    currentInfo = info;

    

    if (currentMode == "Center of Gravity") {
        info.cogX = 0.0;
        info.cogY = 0.0;
        info.cogZ = 0.0;

        info.covX -= currentInfo.cogX;
        info.covY -= currentInfo.cogY;
        info.covZ -= currentInfo.cogZ;
    }
    else if (currentMode == "Custom" && referenceDatum) {
        auto datum = const_cast<App::DatumElement*>(dynamic_cast<const App::DatumElement*>(referenceDatum));
        if (!datum) {
            return;
        }
        else if (datum->getLCS()) {
            Base::Vector3d originPos = datum->getBasePoint();
            info.cogX -= originPos.x;
            info.cogY -= originPos.y;
            info.cogZ -= originPos.z;
    
            info.covX -= originPos.x;
            info.covY -= originPos.y;
            info.covZ -= originPos.z;
        }
    }



    UnitConversions conv = UnitSystem::getConversions(unitSystem);
    
    auto setText = [](QLineEdit* edit, double value, double factor, int precision, const std::string& unit, const QString& suffix = QString()) {
        QString text;
        QTextStream stream(&text);
        stream << QString::number(value * factor, 'f', precision) << " " << QString::fromUtf8(unit.c_str()) << suffix;
        edit->setText(text);
        edit->setCursorPosition(0);
    };

    setText(volumeEdit, info.volume, conv.volumeFactor, 6, conv.volumeUnit);
    setText(massEdit, info.mass, conv.massFactor, 6, conv.massUnit);
    setText(surfaceAreaEdit, info.surfaceArea, conv.areaFactor, 6, conv.areaUnit);
    setText(densityEdit, info.density, conv.densityFactor, 2, conv.densityUnit, QLatin1String(" (Avg)"));

    setText(cogXText, info.cogX, conv.lengthFactor, 6, conv.lengthUnit);
    setText(cogYText, info.cogY, conv.lengthFactor, 6, conv.lengthUnit);
    setText(cogZText, info.cogZ, conv.lengthFactor, 6, conv.lengthUnit);
    setText(covXText, info.covX, conv.lengthFactor, 6, conv.lengthUnit);
    setText(covYText, info.covY, conv.lengthFactor, 6, conv.lengthUnit);
    setText(covZText, info.covZ, conv.lengthFactor, 6, conv.lengthUnit);

    setText(inertiaJoxText, info.inertiaJox, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(inertiaJoyText, info.inertiaJoy, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(inertiaJozText, info.inertiaJoz, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(inertiaJxyText, info.inertiaJxy, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(inertiaJzxText, info.inertiaJzx, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(inertiaJzyText, info.inertiaJzy, conv.inertiaFactor, 6, conv.inertiaUnit);

    setText(inertiaJxText, info.inertiaJx, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(inertiaJyText, info.inertiaJy, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(inertiaJzText, info.inertiaJz, conv.inertiaFactor, 6, conv.inertiaUnit);
    setText(axisInertiaText, info.axisInertia, conv.inertiaFactor, 6, conv.inertiaUnit);
}

void TaskMassProperties::updateInertiaVisibility()
{
    bool hasAxisSelection = false;
    if (currentMode == "Custom" && currentDatum) {
        hasAxisSelection = currentDatum->isDerivedFrom<App::Line>();
    }

    bool hideInertia = hasAxisSelection;

    inertiaMatrixWidget->setVisible(!hideInertia);
    inertiaDiagWidget->setVisible(!hideInertia);
    inertiaLcsWidget->setVisible(!hideInertia);
    axisInertiaWidget->setVisible(hasAxisSelection);
}

void TaskMassProperties::createDatum(double x, double y, double z, const std::string& name)
{
    try {
        App::Document* doc = App::GetApplication().getActiveDocument();
        doc->openTransaction("Create Datum Point");

        App::DocumentObject* datum = doc->getObject(name.c_str());

        if (!datum) {
            datum = doc->addObject("Part::DatumPoint", name.c_str());
        }
        
        App::Property* baseProp = datum->getPropertyByName("Placement");
        App::PropertyPlacement* prop = dynamic_cast<App::PropertyPlacement*>(baseProp);
        Base::Placement plm;
        plm.setPosition(Base::Vector3d(x, y, z));
        prop->setValue(plm);

        doc->commitTransaction();
        doc->recompute();
    } 
    catch (const std::exception& e) {
        Base::Console().error("Datum Creation failed: %s", e.what());
    }
}

void TaskMassProperties::createLCS()
{
    try {
        App::Document* doc = App::GetApplication().getActiveDocument();
        doc->openTransaction("Create LCS");

        App::DocumentObject* LCS = doc->getObject("Principal_Axes_LCS");

        if (!LCS) {
            LCS = doc->addObject("Part::LocalCoordinateSystem", "Principal_Axes_LCS");
        }
        
        App::Property* baseProp = LCS->getPropertyByName("Placement");
        App::PropertyPlacement* prop = dynamic_cast<App::PropertyPlacement*>(baseProp);
        Base::Placement plm;
        plm.setPosition(Base::Vector3d(
            currentInfo.cogX,
            currentInfo.cogY,
            currentInfo.cogZ
        ));

        Base::Matrix4D mat;
        mat.setToUnity();
        
        mat[0][0] = currentInfo.principalAxisX.x;
        mat[1][0] = currentInfo.principalAxisX.y;
        mat[2][0] = currentInfo.principalAxisX.z;
        
        mat[0][1] = currentInfo.principalAxisY.x;
        mat[1][1] = currentInfo.principalAxisY.y;
        mat[2][1] = currentInfo.principalAxisY.z;
        
        mat[0][2] = currentInfo.principalAxisZ.x;
        mat[1][2] = currentInfo.principalAxisZ.y;
        mat[2][2] = currentInfo.principalAxisZ.z;

        plm.setRotation(mat);

        prop->setValue(plm);
        
        LCS->Visibility.setValue(true);

        doc->commitTransaction();
        doc->recompute();
    } 
    catch (const std::exception& e) {
        Base::Console().error("LCS Creation failed: %s", e.what());
    }
}

void TaskMassProperties::onCogDatumButtonPressed()
{
    createDatum(currentInfo.cogX, currentInfo.cogY, currentInfo.cogZ, "Center_of_Gravity");
}

void TaskMassProperties::onCovDatumButtonPressed()
{
    createDatum(currentInfo.covX, currentInfo.covY, currentInfo.covZ, "Center_of_Volume");
}

void TaskMassProperties::onLcsButtonPressed()
{
    createLCS();
}

void TaskMassProperties::onSelectCustomCoordinateSystem()
{
    selectingCustomCoordSystem = true;
}

void TaskMassProperties::onCoordinateSystemChanged(std::string coordSystem)
{
    currentMode = coordSystem;
    updateInertiaVisibility();
    tryupdate();
}