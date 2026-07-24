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
#include <QPushButton>
#include <QComboBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QDir>
#include <QFileInfo>
#include <QSvgWidget>
#include <QSvgRenderer>
#include <QStringLiteral>
#include <QLatin1Char>
#include <QDesktopServices>
#include <QUrl>
#endif  // #ifndef _PreComp_

#include "TaskNewPage.h"
#include "ui_TaskNewPage.h"  // Assuming UI file is TaskNewPage.ui

// FreeCAD Base Includes
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <Base/Exception.h>

// FreeCAD App Includes
#include <App/Application.h>
#include <App/Document.h>

// FreeCAD Gui Includes
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Document.h>             // For Gui::ActiveDocument
#include <Gui/TaskView/TaskDialog.h>  // For Gui::TaskView::TaskDialog base
#include <Gui/TaskView/TaskView.h>    // For Gui::TaskView::TaskBox
#include <Gui/BitmapFactory.h>        // For Gui::BitmapFactory

// TechDraw Includes
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/TemplateTranslator.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/Gui/ViewProviderPage.h>


namespace TechDrawGui
{

//===========================================================================
// TaskNewPage (Widget)
//===========================================================================

TaskNewPage::TaskNewPage(QWidget* parent)  // Parent is now typically the TaskBox
    : QWidget(parent)
    , ui(new Ui_TaskNewPage())
    , m_orientationGroup(nullptr)
{
    ui->setupUi(this);

    ui->svgPreviewWidget->setStyleSheet(QString::fromUtf8("QSvgWidget { background-color: white; border: 1px solid black;}"));
    ui->svgPreviewWidget->ensurePolished();

    m_baseTemplateDir = TechDraw::Preferences::defaultTemplateDir();

    if (!m_baseTemplateDir.endsWith(QLatin1Char('/'))
        && !m_baseTemplateDir.endsWith(QLatin1Char('\\'))) {
        m_baseTemplateDir += QLatin1Char('/');
    }

    m_orientationGroup = new QButtonGroup(this);
    m_orientationGroup->addButton(ui->landscapeRadioButton);
    m_orientationGroup->addButton(ui->portraitRadioButton);

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw");
    bool isLandscape = hGrp->GetBool("TemplateLastUsedLandscape", true);
    if (isLandscape) {
        ui->landscapeRadioButton->setChecked(true);
    }
    else {
        ui->portraitRadioButton->setChecked(true);
    }
    connect(ui->addTemplateButton,
            &QPushButton::clicked,
            this,
            &TaskNewPage::onOpenTemplateFolderClicked);
    connect(ui->standardComboBox,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(onStandardChanged(int)));
    connect(ui->sizeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onSizeChanged(int)));
    connect(ui->landscapeRadioButton, SIGNAL(toggled(bool)), this, SLOT(onOrientationChanged()));

    for (int i = 0; TechDraw::LanguageEnums[i] != nullptr; ++i) {
        const char* langName = TechDraw::LanguageEnums[i];
        ui->languageComboBox->addItem(QString::fromUtf8(langName));
    }
    ui->languageComboBox->setCurrentIndex(hGrp->GetInt("TemplateLastUsedLanguage", 0));

    ui->svgPreviewWidget->show();
    populateStandards();

    // Make sure the preview widget is correctly sized on initial show.
    QTimer::singleShot(200, this, [this]() {
        updatePreviewAndPath();
    });
}

void TaskNewPage::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void TaskNewPage::updatePreviewAndPath()
{
    QString currentStandard = ui->standardComboBox->currentText();
    QString currentSize = ui->sizeComboBox->currentText();
    bool isLandscape = ui->landscapeRadioButton->isChecked();

    m_currentTemplateFile = findTemplateFile(currentStandard, currentSize, isLandscape);

    if (!m_currentTemplateFile.isEmpty()) {
        QFileInfo tfi(m_currentTemplateFile);
        if (tfi.isReadable() && tfi.isFile()) {
            ui->svgPreviewWidget->load(m_currentTemplateFile);
            ui->svgPreviewWidget->setToolTip(
                tr("%1")
                    .arg(QDir(m_baseTemplateDir).relativeFilePath(m_currentTemplateFile)));
        }
        else {
            ui->svgPreviewWidget->load(QString());  // Clear if not readable/file
            ui->svgPreviewWidget->setToolTip(
                tr("Invalid or not readable: %1")
                    .arg(QDir(m_baseTemplateDir).relativeFilePath(m_currentTemplateFile)));
            m_currentTemplateFile.clear();
        }
    }
    else {
        ui->svgPreviewWidget->load(QString());  // Clear preview
        ui->svgPreviewWidget->setToolTip(
            tr("Invalid or not readable: %1")
                .arg(QDir(m_baseTemplateDir).relativeFilePath(m_currentTemplateFile)));
        m_currentTemplateFile.clear();
    }


    ui->svgPreviewWidget->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    ui->svgPreviewWidget->updateGeometry();

    // There is a problem with the height of the preview widget. It's much too tall.
    // So we calculate the correct height and manually set it.
    int currentWidgetWidth = ui->svgPreviewWidget->width();
    QSizeF svgSize = ui->svgPreviewWidget->renderer()->defaultSize();
    if (!svgSize.isEmpty() && svgSize.width() > 0) {
        double aspectRatio = svgSize.height() / svgSize.width();
        int calculatedHeight = static_cast<int>(currentWidgetWidth * aspectRatio);
        ui->svgPreviewWidget->setFixedHeight(calculatedHeight);
    }
}

void TaskNewPage::populateStandards()
{
    ui->standardComboBox->blockSignals(true);
    ui->standardComboBox->clear();

    QDir dir(m_baseTemplateDir);
    if (!dir.exists()) {
        ui->svgPreviewWidget->setToolTip(tr("Template directory not found: %1").arg(m_baseTemplateDir));
        ui->standardComboBox->blockSignals(false);
        updatePreviewAndPath();
        return;
    }

    QStringList rawEntries =
        dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware);

    QStringList entries;
    for (const QString& entry : rawEntries) {
        if (!entry.startsWith(QLatin1Char('.'))) {
            entries.append(entry);
        }
    }

    if (!entries.isEmpty()) {
        ui->standardComboBox->addItems(entries);

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/TechDraw");
        int index = hGrp->GetInt("TemplateLastUsedStandard", 4); //4 is ISO5457
        ui->standardComboBox->setCurrentIndex(index);
    }
    else {
        ui->svgPreviewWidget->setToolTip(tr("No standards found in: %1").arg(m_baseTemplateDir));
        // Still call populateSizes and updatePreviewAndPath to clear/update UI state
    }
    ui->standardComboBox->blockSignals(false);

    // Trigger subsequent updates regardless of whether items were added, to handle empty cases
    populateSizes();
}

void TaskNewPage::onStandardChanged(int index)
{
    Q_UNUSED(index);
    populateSizes();
}

void TaskNewPage::populateSizes()
{
    ui->sizeComboBox->blockSignals(true);
    ui->sizeComboBox->clear();

    QString currentStandard = ui->standardComboBox->currentText();
    if (currentStandard.isEmpty()) {
        ui->sizeComboBox->blockSignals(false);
        updatePreviewAndPath();  // Update preview for empty selection
        return;
    }

    QDir standardDir(m_baseTemplateDir + currentStandard);
    if (!standardDir.exists()) {
        ui->svgPreviewWidget->setToolTip(
            tr("Standard directory not found: %1").arg(standardDir.path()));
        ui->sizeComboBox->blockSignals(false);
        updatePreviewAndPath();  // Update preview for error
        return;
    }

    QStringList entries =
        standardDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware);
    if (!entries.isEmpty()) {
        ui->sizeComboBox->addItems(entries);

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/TechDraw");
        int index = hGrp->GetInt("TemplateLastUsedSize", 3); //3 is A3
        ui->sizeComboBox->setCurrentIndex(index);
    }
    else {
        ui->svgPreviewWidget->setToolTip(tr("No sizes found for standard: %1").arg(currentStandard));
        // Still call updatePreviewAndPath to clear/update UI state
    }

    ui->sizeComboBox->blockSignals(false);
    updatePreviewAndPath();  // Always update preview after size changes (or lack thereof)
}

void TaskNewPage::onOpenTemplateFolderClicked()
{
    QString nativePath = QDir::toNativeSeparators(m_baseTemplateDir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
}

void TaskNewPage::onSizeChanged(int index)
{
    Q_UNUSED(index);
    updatePreviewAndPath();
}

void TaskNewPage::onOrientationChanged()
{
    if (ui->landscapeRadioButton->isChecked() || ui->portraitRadioButton->isChecked()) {
        updatePreviewAndPath();
    }
}

QString TaskNewPage::findTemplateFile(const QString& standard, const QString& size, bool landscape) const
{
    if (standard.isEmpty() || size.isEmpty()) {
        return QString();  // Not enough information to find a template
    }

    // Determine the orientation subfolder name
    QString orientationSubfolder =
        landscape ? QStringLiteral("landscape") : QStringLiteral("portrait");

    // Construct the path to the specific orientation directory
    // m_baseTemplateDir already ends with a slash
    QString orientationDirPath = m_baseTemplateDir + standard + QLatin1Char('/') + size
        + QLatin1Char('/') + orientationSubfolder + QLatin1Char('/');

    QDir orientationDir(orientationDirPath);

    if (!orientationDir.exists()) {
        Base::Console().warning("Template orientation directory not found: %s\n",
        orientationDirPath.toStdString().c_str());
        return QString();  // Directory for this specific orientation does not exist
    }

    // List all SVG files in that directory and select first.
    QStringList nameFilters;
    nameFilters << QStringLiteral("*.svg");  // Find any .svg file

    QStringList files =
        orientationDir.entryList(nameFilters, QDir::Files | QDir::NoSymLinks, QDir::Name);

    if (!files.isEmpty()) {
        // Return the full path to the first SVG file found
        return orientationDir.filePath(files.first());
    }

    Base::Console().warning("No SVG file found in: %s\n", orientationDirPath.toStdString().c_str());
    return QString();  // No SVG template file found in the specified orientation directory
}

QString TaskNewPage::getSelectedTemplatePath() const
{
    return m_currentTemplateFile;
}

bool TaskNewPage::isTemplateValid() const
{
    if (m_currentTemplateFile.isEmpty()) {
        return false;
    }
    QFileInfo tfi(m_currentTemplateFile);
    return tfi.exists() && tfi.isReadable() && tfi.isFile();
}

// New methods for TaskDlgNewPage to call
bool TaskNewPage::acceptPageCreation()
{
    if (!isTemplateValid()) {
        Base::Console().warning(tr("A valid template file must be selected to proceed.").toStdString().c_str());
        return false;
    }

    QString templateFileName = getSelectedTemplatePath();

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc || !doc->getDocument()) {
        Base::Console().warning(tr("No active document found.").toStdString().c_str());
        return false;
    }
    App::Document* appDoc = doc->getDocument();

    doc->openCommand(QT_TRANSLATE_NOOP("Command", "Insert New Page"));

    TechDraw::DrawPage* page = appDoc->addObject<TechDraw::DrawPage>("Page");
    if (!page) {
        throw Base::TypeError(
            "TaskNewPage::acceptPageCreation: Failed to create DrawPage object.");
    }
    page->translateLabel("DrawPage", "Page", page->getNameInDocument());

    TechDraw::DrawSVGTemplate* svgTemplate =
        appDoc->addObject<TechDraw::DrawSVGTemplate>("Template");
    if (!svgTemplate) {
        appDoc->removeObject(page->getNameInDocument());
        throw Base::TypeError(
            "TaskNewPage::acceptPageCreation: Failed to create DrawSVGTemplate object.");
    }
    svgTemplate->translateLabel("DrawSVGTemplate",
                                "Template",
                                svgTemplate->getNameInDocument());

    page->Template.setValue(svgTemplate);
    svgTemplate->Language.setValue(ui->languageComboBox->currentIndex());

    const std::string filespecStd =
        TechDraw::DrawUtil::cleanFilespecBackslash(templateFileName.toStdString());
    svgTemplate->Template.setValue(filespecStd);

    doc->commitCommand();

    auto* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(
        Gui::Application::Instance->getViewProvider(page));
    if (dvp) {
        dvp->show();
    }

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw");
    hGrp->SetBool("TemplateLastUsedLandscape", ui->landscapeRadioButton->isChecked());
    hGrp->SetInt("TemplateLastUsedStandard", ui->standardComboBox->currentIndex());
    hGrp->SetInt("TemplateLastUsedSize", ui->sizeComboBox->currentIndex());
    hGrp->SetInt("TemplateLastUsedLanguage", ui->languageComboBox->currentIndex());

    return true;  // Indicate success
}


//===========================================================================
// TaskDlgNewPage (Dialog)
//===========================================================================

TaskDlgNewPage::TaskDlgNewPage() : Gui::TaskView::TaskDialog()
    , m_widget(new TaskNewPage())
{
    addTaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_PageDefault"), m_widget);
}

void TaskDlgNewPage::open()
{
    m_widget->updatePreviewAndPath();  // Ensure UI is current
}

bool TaskDlgNewPage::accept()
{
    return m_widget->acceptPageCreation();  // Delegate and return success/failure
}

bool TaskDlgNewPage::reject()
{
    return true;
}

}  // namespace TechDrawGui

#include "moc_TaskNewPage.cpp"
