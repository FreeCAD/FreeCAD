/* SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/
#include "PreCompiled.h"
#ifndef _PreComp_
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QDoubleSpinBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#endif// #ifndef _PreComp_

#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Base/Quantity.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>  // For PropertyLinkSub
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/QuantitySpinBox.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/HatchLine.h>
#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/Gui/ViewProviderHatch.h>
#include <Mod/TechDraw/Gui/ViewProviderGeomHatch.h>
#include <Mod/TechDraw/Gui/ViewProviderViewPart.h>
#include <Mod/TechDraw/Gui/DrawGuiUtil.h>

#include "TaskHatchFace.h"
#include "ui_TaskHatchFace.h"

namespace TechDrawGui
{

TaskHatchFace::TaskHatchFace(TechDraw::DrawViewPart* dvp,
                             const std::vector<std::string>& subNames,
                             QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskHatchFace)
    , m_dvp(dvp)
    , m_subNames(subNames)
    , m_targetHatchObject(nullptr)
    , m_isEditMode(false)
    , m_isLoading(true)  // Start in loading state
{
    setWindowTitle(QObject::tr("Create Face Hatch"));
    ui->setupUi(this);
    init();  // Populates combobox, sets initial UI values, connects signals

    // loadDefaults sets UI elements to default values
    // m_isLoading is still true here, so onPatternChanged won't trigger preview yet
    loadDefaults();

    App::Document* doc = m_dvp->getDocument();
    m_doc = Gui::Application::Instance->getDocument(doc);
    if (!doc || !m_doc) {
        return;
    }

    m_doc->openCommand(QObject::tr("Create Hatch").toStdString().c_str());

    PatternEntry initialPattern;
    if (ui->patternComboBox->currentIndex() >= 0) {
        initialPattern = ui->patternComboBox->currentData().value<PatternEntry>();
    }
    else {
        // Fallback if no patterns (should not happen if populatePatternsComboBox works)
        Base::Console().error(
            "TaskHatchFace: No pattern selected after loadDefaults for new hatch.\n");
        // Default to SVG to avoid crash, though this state is problematic
        initialPattern.type = PatternUIType::SVG;
        initialPattern.filePath = QString::fromStdString(TechDraw::DrawHatch::prefSvgHatch());
        // Try to find it or add it to combobox if really necessary
    }

    std::string objTypeName = (initialPattern.type == PatternUIType::SVG)
        ? "TechDraw::DrawHatch"
        : "TechDraw::DrawGeomHatch";
    std::string baseName = (initialPattern.type == PatternUIType::SVG) ? "Hatch" : "GeomHatch";
    std::string featName = m_dvp->getDocument()->getUniqueObjectName(baseName.c_str());

    // Create the object within the command scope
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().addObject('%s', '%s')",
                            objTypeName.c_str(),
                            featName.c_str());
    m_targetHatchObject = m_dvp->getDocument()->getObject(featName.c_str());

    if (!m_targetHatchObject) {
        Base::Console().error("TaskHatchFace: Failed to create initial hatch object.\n");
        m_doc->abortCommand();  // Abort if creation failed
        m_isLoading = false;           // Allow UI interaction if it's not completely broken
        // Potentially disable UI or show error
        return;
    }

    if (auto dh = dynamic_cast<TechDraw::DrawHatch*>(m_targetHatchObject)) {
        dh->translateLabel("DrawHatch", baseName.c_str(), featName.c_str());
    }
    else if (auto dgh = dynamic_cast<TechDraw::DrawGeomHatch*>(m_targetHatchObject)) {
        dgh->translateLabel("DrawGeomHatch", baseName.c_str(), featName.c_str());
    }

    m_originalType = initialPattern.type;  // Store type of the initially created object
    m_originalObjectName = m_targetHatchObject->getNameInDocument();

    // Apply initial properties based on UI defaults
    applyToDocumentObject(m_targetHatchObject, initialPattern);

    m_isLoading = false;  // Now allow live updates

    // Trigger a preview update if the combobox selection logic in loadDefaults
    // didn't directly call onPatternChanged with isLoading=false
    if (ui->patternComboBox->currentIndex() >= 0) {
        onPatternChanged(ui->patternComboBox->currentIndex());
    }

    // Touch the parent feature so the hatching in tree view appears as a child
    m_dvp->touch();
    m_dvp->recomputeFeature();
}

TaskHatchFace::TaskHatchFace(App::DocumentObject* hatchObjectToEdit, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskHatchFace)
    , m_dvp(nullptr)
    , m_targetHatchObject(hatchObjectToEdit)
    , m_isEditMode(true)
    , m_isLoading(true)  // Start in loading state
{
    setWindowTitle(QObject::tr("Edit Face Hatch"));
    ui->setupUi(this);
    init();  // Populates combobox, connects signals

    if (!m_targetHatchObject) {
        Base::Console().error("TaskHatchFace: Edit mode started with null object.\n");
        m_isLoading = false;
        return;
    }

    // Determine DVP and subNames from the existing hatch object (as in your original)
    App::PropertyLinkSub* pSource = nullptr;
    if (m_targetHatchObject->isDerivedFrom(TechDraw::DrawHatch::getClassTypeId())) {
        pSource = &static_cast<TechDraw::DrawHatch*>(m_targetHatchObject)->Source;
        m_originalType = PatternUIType::SVG;
    }
    else if (m_targetHatchObject->isDerivedFrom(TechDraw::DrawGeomHatch::getClassTypeId())) {
        pSource = &static_cast<TechDraw::DrawGeomHatch*>(m_targetHatchObject)->Source;
        m_originalType = PatternUIType::PAT;
    }

    if (pSource) {
        App::DocumentObject* sourceObj = pSource->getValue();
        // Ensure sourceObj is actually a DrawViewPart
        if (sourceObj && sourceObj->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            m_dvp = static_cast<TechDraw::DrawViewPart*>(sourceObj);
        }
        else if (sourceObj) {
            Base::Console().warning(
                "TaskHatchFace: Source of hatch is not a DrawViewPart but a %s.\n",
                sourceObj->getTypeId().getName());
        }
        m_subNames = pSource->getSubValues();
    }
    else {
        Base::Console().error("TaskHatchFace: Could not determine source DVP for edited hatch.\n");
    }
    m_originalObjectName = m_targetHatchObject->getNameInDocument();

    App::Document* doc = m_dvp->getDocument();
    m_doc = Gui::Application::Instance->getDocument(doc);
    if (!doc || !m_doc) {
        return;
    }

    m_doc->openCommand(QObject::tr("Edit Hatch").toStdString().c_str());

    // loadFromObject loads UI from the existing object properties
    // m_isLoading is still true, so onPatternChanged from setCurrentIndex won't trigger preview yet
    loadFromObject(m_targetHatchObject);
    saveOriginalState();  // Saves full current state of original object for reference

    m_isLoading = false;  // Now allow live updates
    // Trigger initial preview update based on loaded state
    if (ui->patternComboBox->currentIndex() >= 0) {
        onPatternChanged(ui->patternComboBox->currentIndex());
    }
}

TaskHatchFace::~TaskHatchFace() = default;  // Definition for unique_ptr

void TaskHatchFace::init()
{
    // m_isLoading is set to true by constructors before calling init
    qRegisterMetaType<TechDrawGui::PatternEntry>("TechDrawGui::PatternEntry");

    ui->scaleSpinBox->setMinimum(0.01);
    ui->scaleSpinBox->setSingleStep(0.1);
    ui->scaleSpinBox->setValue(1.0);  // Default, will be overridden by loadDefaults/loadFromObject

    ui->lineWidthSpinBox->setMinimum(0.01);
    ui->lineWidthSpinBox->setSingleStep(0.1);
    ui->lineWidthSpinBox->setValue(0.18);  // Default

    connect(ui->patternComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &TaskHatchFace::onPatternChanged);
    connect(ui->addCustomPatternButton,
            &QPushButton::clicked,
            this,
            &TaskHatchFace::onAddCustomPattern);
    connect(ui->openPatternsFolder,
            &QPushButton::clicked,
            this,
            &TaskHatchFace::onOpenPatternsFolder);

    // Connect other controls to onParamsChanged for live preview
    connect(ui->scaleSpinBox,
            QOverload<double>::of(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskHatchFace::onParamsChanged);
    connect(ui->colorButton,
            &Gui::ColorButton::changed,
            this,
            &TaskHatchFace::onParamsChanged);  // Gui::ColorButton
    connect(ui->lineWidthSpinBox,
            QOverload<double>::of(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskHatchFace::onParamsChanged);
    connect(ui->rotationSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &TaskHatchFace::onParamsChanged);
    connect(ui->offsetXSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &TaskHatchFace::onParamsChanged);
    connect(ui->offsetYSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &TaskHatchFace::onParamsChanged);

    populatePatternsComboBox(ui->patternComboBox);
}

void TaskHatchFace::populatePatternsComboBox(QComboBox* box)
{
    box->blockSignals(true);

    box->clear();

    // SVG Patterns
    QDirIterator it(
        QString::fromStdString(App::Application::getResourceDir() + "Mod/TechDraw/Patterns/"),
        QStringList() << QStringLiteral("*.svg"),
        QDir::Files,
        QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        PatternEntry entry = {fileInfo.fileName(),
                              PatternUIType::SVG,
                              filePath,
                              QStringLiteral("")};
        box->addItem(entry.displayName, QVariant::fromValue(entry));
    }

    // PAT Patterns
    QDirIterator it2(
        QString::fromStdString(App::Application::getResourceDir() + "Mod/TechDraw/PAT/"),
        QStringList() << QStringLiteral("*.pat"),
        QDir::Files,
        QDirIterator::Subdirectories);
    while (it2.hasNext()) {
        QString filePath = it2.next();
        QFileInfo fileInfo(filePath);
        std::string filePathStd = filePath.toStdString();
        std::vector<std::string> patNames = TechDraw::PATLineSpec::getPatternList(filePathStd);
        for (const std::string& patName : patNames) {
            PatternEntry entry = {fileInfo.fileName() + QStringLiteral(" - ")
                                      + QString::fromStdString(patName),
                                  PatternUIType::PAT,
                                  filePath,
                                  QString::fromStdString(patName)};
            box->addItem(entry.displayName, QVariant::fromValue(entry));
        }
    }

    box->blockSignals(false);
}

void TaskHatchFace::updateUIControlsForPatternType(PatternUIType type)  // Unchanged
{
    ui->lineWidthSpinBox->setEnabled(type == PatternUIType::PAT);
}

void TaskHatchFace::loadDefaults()
{
    // isLoading is true, so setCurrentIndex won't trigger onPatternChanged ->
    // updateHatchObjectPreview

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw");
    int index = hGrp->GetInt("HatchDefaultPattern", ui->patternComboBox->count());

    if (index > ui->patternComboBox->count() - 1) { // last used
        index = hGrp->GetInt("HatchLastUsedPattern", 0);
    }

    ui->patternComboBox->setCurrentIndex(index);

    ui->scaleSpinBox->setValue(1.0);
    ui->colorButton->setColor(TechDraw::DrawHatch::prefSvgHatchColor().asValue<QColor>());
    ui->lineWidthSpinBox->setValue(TechDraw::LineGroup::getDefaultWidth("Graphic"));
    ui->rotationSpinBox->setValue(0.0);
    ui->offsetXSpinBox->setValue(0.0);
    ui->offsetYSpinBox->setValue(0.0);

    // Update UI controls based on the selected pattern type, without triggering full preview
    if (ui->patternComboBox->currentIndex() >= 0) {
        PatternEntry currentEntry = ui->patternComboBox->currentData().value<PatternEntry>();
        updateUIControlsForPatternType(currentEntry.type);
    }
}

void TaskHatchFace::loadFromObject(App::DocumentObject* obj)
{
    // isLoading is true, so setCurrentIndex won't trigger onPatternChanged ->
    // updateHatchObjectPreview
    PatternEntry currentPattern;
    Gui::ViewProvider* vp =
        Gui::Application::Instance->getDocument(obj->getDocument())->getViewProvider(obj);

    if (auto hatch = dynamic_cast<TechDraw::DrawHatch*>(obj)) {
        currentPattern.type = PatternUIType::SVG;
        currentPattern.filePath = QString::fromStdString(hatch->HatchPattern.getValue());
        currentPattern.displayName =
            QStringLiteral("SVG: ") + QFileInfo(currentPattern.filePath).fileName();
        // m_originalType already set in constructor

        if (auto vph = dynamic_cast<TechDrawGui::ViewProviderHatch*>(vp)) {
            ui->scaleSpinBox->setValue(vph->HatchScale.getValue());
            ui->colorButton->setColor(vph->HatchColor.getValue().asValue<QColor>());  // Corrected
            ui->rotationSpinBox->setValue(vph->HatchRotation.getValue());
            Base::Vector3d offset = vph->HatchOffset.getValue();
            ui->offsetXSpinBox->setValue(offset.x);
            ui->offsetYSpinBox->setValue(offset.y);
        }
    }
    else if (auto geomHatch = dynamic_cast<TechDraw::DrawGeomHatch*>(obj)) {
        currentPattern.type = PatternUIType::PAT;
        currentPattern.filePath = QString::fromStdString(geomHatch->FilePattern.getValue());
        currentPattern.patNameInternal = QString::fromStdString(geomHatch->NamePattern.getValue());
        currentPattern.displayName = QStringLiteral("PAT: ")
            + QFileInfo(currentPattern.filePath).fileName() + QStringLiteral(" - ")
            + currentPattern.patNameInternal;
        // m_originalType already set

        if (auto vpgh = dynamic_cast<TechDrawGui::ViewProviderGeomHatch*>(vp)) {
            ui->scaleSpinBox->setValue(geomHatch->ScalePattern.getValue());
            ui->colorButton->setColor(
                vpgh->ColorPattern.getValue().asValue<QColor>());  // Corrected
            ui->lineWidthSpinBox->setValue(vpgh->WeightPattern.getValue());
            ui->rotationSpinBox->setValue(geomHatch->PatternRotation.getValue());
            Base::Vector3d offset = geomHatch->PatternOffset.getValue();
            ui->offsetXSpinBox->setValue(offset.x);
            ui->offsetYSpinBox->setValue(offset.y);
        }
    }

    int indexToSelect = -1;
    for (int i = 0; i < ui->patternComboBox->count(); ++i) {
        if (ui->patternComboBox->itemData(i).value<PatternEntry>() == currentPattern) {
            indexToSelect = i;
            break;
        }
    }
    if (indexToSelect != -1) {
        ui->patternComboBox->setCurrentIndex(indexToSelect);
    }
    else {
        Base::Console().warning("TaskHatchFace: Current hatch pattern '%s' not found in lists. UI "
                                "may not reflect true state.\n",
                                currentPattern.displayName.toStdString().c_str());
        if (!currentPattern.displayName.isEmpty()) {
            ui->patternComboBox->addItem(currentPattern.displayName,
                                         QVariant::fromValue(currentPattern));
            ui->patternComboBox->setCurrentIndex(ui->patternComboBox->count() - 1);
        }
        else if (ui->patternComboBox->count() > 0) {
            ui->patternComboBox->setCurrentIndex(0);  // Fallback
        }
    }
    // Update UI controls based on the selected pattern type, without triggering full preview
    if (ui->patternComboBox->currentIndex() >= 0) {
        PatternEntry entry = ui->patternComboBox->currentData().value<PatternEntry>();
        updateUIControlsForPatternType(entry.type);
    }
}

void TaskHatchFace::saveOriginalState()  // Uses .getValue() for spinboxes
{
    // This is called after loadFromObject or after initial creation defaults are set.
    // It stores the state that corresponds to the object *as it was when the dialog opened* (for
    // edit) or *as it was initially created* (for new).
    if (ui->patternComboBox->currentIndex() >= 0) {
        m_originalPattern = ui->patternComboBox->currentData().value<PatternEntry>();
    }
    else if (m_targetHatchObject) {  // If pattern not in list (e.g. from loadFromObject, added
                                     // temporarily)
        if (auto hatch = dynamic_cast<TechDraw::DrawHatch*>(m_targetHatchObject)) {
            m_originalPattern.type = PatternUIType::SVG;
            m_originalPattern.filePath = QString::fromStdString(hatch->HatchPattern.getValue());
        }
        else if (auto geomHatch = dynamic_cast<TechDraw::DrawGeomHatch*>(m_targetHatchObject)) {
            m_originalPattern.type = PatternUIType::PAT;
            m_originalPattern.filePath = QString::fromStdString(geomHatch->FilePattern.getValue());
            m_originalPattern.patNameInternal =
                QString::fromStdString(geomHatch->NamePattern.getValue());
        }
    }

    m_originalScale = ui->scaleSpinBox->value().getValue();  // Corrected
    m_originalColor.setValue(ui->colorButton->color());      // Corrected (QColor to Base::Color)
    m_originalLineWidth = ui->lineWidthSpinBox->value().getValue();  // Corrected
    m_originalRotation = ui->rotationSpinBox->value();
    m_originalOffset.x = ui->offsetXSpinBox->value();
    m_originalOffset.y = ui->offsetYSpinBox->value();
    m_originalOffset.z = 0.0;
    // m_originalType and m_originalObjectName are set in constructors.
}


void TaskHatchFace::onPatternChanged(int index)
{
    if (m_isLoading || index < 0) {
        return;
    }
    PatternEntry entry = ui->patternComboBox->itemData(index).value<PatternEntry>();
    updateUIControlsForPatternType(entry.type);  // Update enabled state of line width etc.
    updateHatchObjectPreview();                  // Then update the actual object
}

void TaskHatchFace::onParamsChanged()
{
    if (m_isLoading) {
        return;
    }
    updateHatchObjectPreview();
}

void TaskHatchFace::onAddCustomPattern()
{
    if (m_isLoading) {
        return;
    }

    QString filter =
        QObject::tr("Hatch patterns (*.svg *.pat);;SVG files (*.svg);;PAT files (*.pat)");
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    QObject::tr("Select Custom Pattern File"),
                                                    QString(),  // Default dir
                                                    filter);
    if (fileName.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(fileName);
    std::string targetDirStdPath;
    bool isSvg = fileInfo.suffix().toLower() == QLatin1String("svg");
    bool isPat = fileInfo.suffix().toLower() == QLatin1String("pat");

    std::string resourcePatternBaseDir = App::Application::getResourceDir() + "Mod/TechDraw/";


    if (isSvg) {
        targetDirStdPath = resourcePatternBaseDir + "Patterns/";
    }
    else if (isPat) {
        targetDirStdPath = resourcePatternBaseDir + "PAT/";
    }
    else {
        QMessageBox::warning(this,
                             QObject::tr("Invalid File"),
                             QObject::tr("Selected file is not a valid SVG or PAT file."));
        return;
    }

    QDir dir(QString::fromStdString(targetDirStdPath));
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            QMessageBox::critical(this,
                                  QObject::tr("Directory Creation Error"),
                                  QObject::tr("Could not create directory '%1'.")
                                      .arg(QString::fromStdString(targetDirStdPath)));
            return;
        }
    }

    QString targetFilePath =
        QString::fromStdString(targetDirStdPath) + QDir::separator() + fileInfo.fileName();

    bool copyFile = true;
    if (QFile::exists(targetFilePath)) {
        if (QMessageBox::question(this,
                                  QObject::tr("File Exists"),
                                  QObject::tr("Pattern file '%1' already exists. Overwrite?")
                                      .arg(fileInfo.fileName()),  // Original used resource dir,
                                                                  // adjust message if user dir
                                  QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::No) {
            copyFile = false;
        }
        else {
            if (QFile::exists(targetFilePath)) {
                QFile::remove(targetFilePath);
            }
        }
    }

    if (copyFile) {
        if (!QFile::copy(fileName, targetFilePath)) {  // Your original QFile::copy
            QMessageBox::critical(this,
                                  QObject::tr("Copy Error"),
                                  QObject::tr("Could not copy file to '%1'.")
                                      .arg(QString::fromStdString(targetDirStdPath)));
            return;
        }
    }

    int oldComboBoxIndex = ui->patternComboBox->currentIndex();
    PatternEntry oldSelectedEntry;
    if (oldComboBoxIndex >= 0) {
        oldSelectedEntry = ui->patternComboBox->itemData(oldComboBoxIndex).value<PatternEntry>();
    }

    bool wasLoading = m_isLoading;
    m_isLoading = true;
    populatePatternsComboBox(ui->patternComboBox);  // Repopulate
    m_isLoading = wasLoading;

    int newIndexToSelect = -1;
    // Find the newly added/selected file
    if (isSvg) {
        for (int i = 0; i < ui->patternComboBox->count(); ++i) {
            PatternEntry entry = ui->patternComboBox->itemData(i).value<PatternEntry>();
            if (entry.type == PatternUIType::SVG
                && entry.filePath == targetFilePath) {  // Original comparison
                newIndexToSelect = i;
                break;
            }
        }
    }
    else if (isPat) {
        std::string originalFileNameStd =
            fileName.toStdString();  // PAT names from the *source* file
        std::vector<std::string> patNames =
            TechDraw::PATLineSpec::getPatternList(originalFileNameStd);
        if (!patNames.empty()) {
            QString firstPatName = QString::fromStdString(patNames[0]);
            for (int i = 0; i < ui->patternComboBox->count(); ++i) {
                PatternEntry entry = ui->patternComboBox->itemData(i).value<PatternEntry>();
                // Compare against targetFilePath where it was copied
                if (entry.type == PatternUIType::PAT && entry.filePath == targetFilePath
                    &&  // Original comparison
                    entry.patNameInternal == firstPatName) {
                    newIndexToSelect = i;
                    break;
                }
            }
        }
    }

    if (newIndexToSelect != -1) {
        ui->patternComboBox->setCurrentIndex(
            newIndexToSelect);  // This will trigger onPatternChanged if index actually changes
    }
    else {  // Try to restore old selection or select first
        int fallbackIndex = -1;
        for (int i = 0; i < ui->patternComboBox->count(); ++i) {
            if (ui->patternComboBox->itemData(i).value<PatternEntry>() == oldSelectedEntry) {
                fallbackIndex = i;
                break;
            }
        }
        if (fallbackIndex != -1) {
            ui->patternComboBox->setCurrentIndex(fallbackIndex);
        }
        else if (ui->patternComboBox->count() > 0) {
            ui->patternComboBox->setCurrentIndex(0);
        }
    }
    // If the index didn't change but the list might have, or if it did, ensure preview updates.
    // onPatternChanged handles this if currentIndex changes. If not, call explicitly if necessary.
    // The setCurrentIndex above should trigger onPatternChanged if index changes.
    // If index selection results in same index as before, call onParamsChanged to force update
    if (ui->patternComboBox->currentIndex() == oldComboBoxIndex && newIndexToSelect == -1) {
        updateHatchObjectPreview();  // Explicitly update if selection did not change
    }
}

void TaskHatchFace::onOpenPatternsFolder()
{
    QString patternsPathString =
        QString::fromStdString(App::Application::getResourceDir() + "Mod/TechDraw/Patterns/");

    QDir patternsDir(patternsPathString);
    if (!patternsDir.exists()) {
        Base::Console().warning("TaskHatchFace: SVG patterns folder does not exist at '%s'\n",
                                patternsPathString.toStdString().c_str());
    }

    // Convert the local file path to a URL
    // QUrl::fromLocalFile ensures correct formatting for different OS
    // Use canonicalPath for a clean, absolute path
    QUrl patternsUrl = QUrl::fromLocalFile(patternsDir.canonicalPath());

    // Open the folder in the system's default file explorer
    if (!QDesktopServices::openUrl(patternsUrl)) {
        Base::Console().error("TaskHatchFace: Could not open SVG patterns folder at '%s'\n",
                              patternsUrl.toString().toStdString().c_str());
    }
}

void TaskHatchFace::updateHatchObjectPreview()
{
    if (m_isLoading || !m_targetHatchObject) {
        return;
    }

    App::Document* doc = m_targetHatchObject->getDocument();
    if (!doc) {  // Should not happen if m_targetHatchObject is valid
        Base::Console().error("TaskHatchFace: Target object has no document.\n");
        return;
    }
    if (ui->patternComboBox->currentIndex() < 0) {  // No pattern selected
        Base::Console().warning("TaskHatchFace: No pattern selected in combobox for preview.\n");
        // Potentially clear the hatch or show some placeholder if desired
        return;
    }

    PatternEntry selectedPattern = ui->patternComboBox->currentData().value<PatternEntry>();

    bool currentObjectIsSvg =
        m_targetHatchObject->isDerivedFrom(TechDraw::DrawHatch::getClassTypeId());
    bool selectedPatternIsSvg = (selectedPattern.type == PatternUIType::SVG);

    if (currentObjectIsSvg != selectedPatternIsSvg) {  // Type switch needed
        std::string currentObjectName = m_targetHatchObject->getNameInDocument();

        // Remove old object - this is recorded by the command
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().removeObject('%s')",
                                currentObjectName.c_str());

        std::string newObjTypeName =
            selectedPatternIsSvg ? "TechDraw::DrawHatch" : "TechDraw::DrawGeomHatch";
        std::string newBaseName = selectedPatternIsSvg ? "Hatch" : "GeomHatch";
        std::string newObjectNameToUse;

        // If in edit mode and switching back to the original object's type and name (if it was
        // deleted)
        if (m_isEditMode
            && ((m_originalType == PatternUIType::SVG && selectedPatternIsSvg)
                || (m_originalType == PatternUIType::PAT && !selectedPatternIsSvg))
            && !m_originalObjectName.empty()
            && (currentObjectName == m_originalObjectName
                || !doc->getObject(m_originalObjectName.c_str()))) {
            newObjectNameToUse = m_originalObjectName;
        }
        else {
            newObjectNameToUse = doc->getUniqueObjectName(newBaseName.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().addObject('%s', '%s')",
                                newObjTypeName.c_str(),
                                newObjectNameToUse.c_str());
        m_targetHatchObject = doc->getObject(newObjectNameToUse.c_str());

        if (!m_targetHatchObject) {
            Base::Console().error("TaskHatchFace: Failed to create new hatch object during type "
                                  "switch for preview.\n");
            // Command is still open. If user cancels, this aborted creation should be reverted.
            return;
        }
        // Set Label for new object
        if (auto dh = dynamic_cast<TechDraw::DrawHatch*>(m_targetHatchObject)) {
            dh->translateLabel("DrawHatch", newBaseName.c_str(), newObjectNameToUse.c_str());
        }
        else if (auto dgh = dynamic_cast<TechDraw::DrawGeomHatch*>(m_targetHatchObject)) {
            dgh->translateLabel("DrawGeomHatch", newBaseName.c_str(), newObjectNameToUse.c_str());
        }

        m_dvp->touch();
        m_dvp->recomputeFeature();
    }

    if (applyToDocumentObject(m_targetHatchObject, selectedPattern)) {
        doc->recompute();  // Recompute to show changes
    }

    // Make sure the hatch is claimed by the view.
    Gui::ViewProvider* dvp_vp =
        Gui::Application::Instance->getDocument(doc)->getViewProvider(m_dvp);
    dvp_vp->claimChildren();
}

bool TaskHatchFace::applyToDocumentObject(App::DocumentObject* obj, const PatternEntry& entry)
{
    if (!obj || !m_dvp && !m_isEditMode) {  // For new hatch, dvp is essential. For edit, obj might
                                            // be orphaned.
        if (!m_dvp && !m_isEditMode) {
            Base::Console().error("TaskHatchFace: DVP is null for new hatch.\n");
        }
        if (!obj) {
            Base::Console().error(
                "TaskHatchFace: Target object is null in applyToDocumentObject.\n");
        }
        return false;
    }
    if (!obj->getDocument()) {  // Safety check
        Base::Console().error(
            "TaskHatchFace: Object %s has no document in applyToDocumentObject.\n",
            obj->getNameInDocument());
        return false;
    }


    Gui::ViewProvider* vp =
        Gui::Application::Instance->getDocument(obj->getDocument())->getViewProvider(obj);
    // vp can be null if the object was just created and VP not attached yet,
    // or if headless. Object properties should still be set. VP properties only if vp exists.

    // Set Source property (common to both)
    App::PropertyLinkSub* pSource = nullptr;
    if (auto h = dynamic_cast<TechDraw::DrawHatch*>(obj)) {
        pSource = &h->Source;
    }
    else if (auto gh = dynamic_cast<TechDraw::DrawGeomHatch*>(obj)) {
        pSource = &gh->Source;
    }

    if (pSource) {
        // In edit mode, m_dvp and m_subNames are from the original object.
        // In new mode, they are from the constructor arguments.
        // This ensures Source is set correctly or reflects an orphaned state if m_dvp is null.
        pSource->setValue(m_dvp, m_subNames);
    }

    Base::Color bc;
    bc.setValue(ui->colorButton->color());  // QColor to Base::Color

    if (entry.type == PatternUIType::SVG) {
        auto hatch = dynamic_cast<TechDraw::DrawHatch*>(obj);
        if (!hatch) {
            return false;  // Should be correct type due to updateHatchObjectPreview logic
        }

        hatch->HatchPattern.setValue(entry.filePath.toStdString());
        if (vp) {  // Only set VP properties if VP exists
            auto vph = dynamic_cast<TechDrawGui::ViewProviderHatch*>(vp);
            if (vph) {
                vph->HatchScale.setValue(ui->scaleSpinBox->value().getValue());  // Corrected
                vph->HatchColor.setValue(bc);
                vph->HatchRotation.setValue(ui->rotationSpinBox->value());
                vph->HatchOffset.setValue(
                    Base::Vector3d(ui->offsetXSpinBox->value(), ui->offsetYSpinBox->value(), 0.0));
            }
            else {
                Base::Console().warning("TaskHatchFace: ViewProvider for DrawHatch is not of "
                                        "expected type ViewProviderHatch.\n");
            }
        }
    }
    else {  // PAT
        auto geomHatch = dynamic_cast<TechDraw::DrawGeomHatch*>(obj);
        if (!geomHatch) {
            return false;  // Should be correct type
        }

        geomHatch->FilePattern.setValue(entry.filePath.toStdString());
        geomHatch->NamePattern.setValue(entry.patNameInternal.toStdString());
        geomHatch->ScalePattern.setValue(
            ui->scaleSpinBox->value().getValue());  // Corrected (Scale is on object)
        geomHatch->PatternRotation.setValue(ui->rotationSpinBox->value());  // On object
        geomHatch->PatternOffset.setValue(Base::Vector3d(ui->offsetXSpinBox->value(),
                                                         ui->offsetYSpinBox->value(),
                                                         0.0));  // On object

        if (vp) {  // Only set VP properties if VP exists
            auto vpgh = dynamic_cast<TechDrawGui::ViewProviderGeomHatch*>(vp);
            if (vpgh) {
                vpgh->ColorPattern.setValue(bc);
                vpgh->WeightPattern.setValue(
                    ui->lineWidthSpinBox->value().getValue());  // Corrected
            }
            else {
                Base::Console().warning("TaskHatchFace: ViewProvider for DrawGeomHatch is not of "
                                        "expected type ViewProviderGeomHatch.\n");
            }
        }
    }
    obj->touch();  // Mark object as changed
    return true;
}

bool TaskHatchFace::accept()
{
    if (ui->patternComboBox->currentIndex() < 0) {
        QMessageBox::warning(this,
                             QObject::tr("No Pattern Selected"),
                             QObject::tr("No hatch pattern is selected."));
        return false;
    }
    if (!m_targetHatchObject) {
        QMessageBox::critical(this,
                              QObject::tr("Error"),
                              QObject::tr("No target hatch object. Cannot accept."));
        m_doc->abortCommand();  // Abort anything done so far
        return false;
    }
    if (!m_isEditMode && !m_dvp) {  // DVP is crucial for new hatches
        QMessageBox::critical(this,
                              QObject::tr("Error"),
                              QObject::tr("No valid drawing view for new hatch."));
        m_doc->abortCommand();
        return false;
    }

    m_doc->commitCommand();

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw");
    hGrp->SetInt("HatchLastUsedPattern", ui->patternComboBox->currentIndex());

    m_targetHatchObject->recomputeFeature();
    m_dvp->requestPaint();
    return true;
}

bool TaskHatchFace::reject()
{
    // The magic of Gui::Command: abort reverts all changes made within its scope.
    // This includes object creation, deletion (doCommand removeObject), and property changes.
    if (m_doc) {
        m_doc->abortCommand();
    }

    m_dvp->requestPaint();

    return true;
}


// --------------------- DLG (Unchanged from your original) ----------------------

TaskDlgHatchFace::TaskDlgHatchFace(TechDraw::DrawViewPart* dvp,
                                   const std::vector<std::string>& subNames)
    : Gui::TaskView::TaskDialog()
{
    widget = new TaskHatchFace(dvp, subNames);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_TreeHatch"),
                                         widget->windowTitle(),
                                         true,
                                         0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgHatchFace::TaskDlgHatchFace(App::DocumentObject* hatchObjectToEdit)
    : Gui::TaskView::TaskDialog()
{
    widget = new TaskHatchFace(hatchObjectToEdit);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_TreeHatch"),
                                         widget->windowTitle(),
                                         true,
                                         0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}


void TaskDlgHatchFace::open()
{
    // Called when the dialog is made visible
}

void TaskDlgHatchFace::clicked(int buttonId)
{
    // Called for custom buttons, not standard OK/Cancel
    Q_UNUSED(buttonId);
    // TaskDialog's standard buttons (OK/Cancel) typically call accept()/reject() internally.
    // No need for Gui::Application::Instance->signalStandardButton connection.
}

bool TaskDlgHatchFace::accept()
{
    if (widget->accept()) {  // Delegate to widget's accept
        // TaskDialog's accept() usually handles finish()
        return Gui::TaskView::TaskDialog::accept();  // Call base to ensure proper closing
    }
    return false;
}

bool TaskDlgHatchFace::reject()
{
    if (widget->reject()) {                          // Delegate to widget's reject
        return Gui::TaskView::TaskDialog::reject();  // Call base
    }
    return false;
}

}  // namespace TechDrawGui

#include "moc_TaskHatchFace.cpp"