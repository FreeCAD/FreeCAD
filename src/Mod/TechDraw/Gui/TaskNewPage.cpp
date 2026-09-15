// SPDX - License - Identifier: LGPL - 2.1 - or -later
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
#include <QEvent>
#include <QPainter>
#endif  // #ifndef _PreComp_

#include "TaskNewPage.h"
#include "ui_TaskNewPage.h"
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/BitmapFactory.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/TemplateTranslator.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/Gui/ViewProviderPage.h>


namespace TechDrawGui
{

TaskNewPage::TaskNewPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskNewPage())
    , m_orientationGroup(nullptr)
{
    ui->setupUi(this);

    ui->svgPreviewWidget->installEventFilter(this);

    m_baseTemplateDir = TechDraw::Preferences::defaultTemplateDir();

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
    connect(ui->browseTemplateButton, &QPushButton::clicked, this, &TaskNewPage::onBrowseTemplate);
    connect(ui->templateFolderButton,
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
}

void TaskNewPage::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        const int manualIndex = ui->standardComboBox->count() - 1;
        ui->standardComboBox->setItemText(manualIndex, tr("Choose template manually"));
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

    m_currentTemplateFile = isManualSelection()
        ? m_browsedTemplateFile
        : findTemplateFile(currentStandard, currentSize, isLandscape);

    if (!m_currentTemplateFile.isEmpty()) {
        QFileInfo tfi(m_currentTemplateFile);
        if (tfi.isReadable() && tfi.isFile()) {
            ui->svgPreviewWidget->load(m_currentTemplateFile);
            ui->svgPreviewWidget->setToolTip(
                QDir(m_baseTemplateDir).relativeFilePath(m_currentTemplateFile));
        }
        else {
            ui->svgPreviewWidget->load(
                QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\"/>"));
            ui->svgPreviewWidget->setToolTip(
                tr("Invalid or not readable: %1")
                    .arg(QDir(m_baseTemplateDir).relativeFilePath(m_currentTemplateFile)));
            m_currentTemplateFile.clear();
        }
    }
    else {
        ui->svgPreviewWidget->load(
            QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\"/>"));
        ui->svgPreviewWidget->setToolTip(QString());
    }


    ui->selectedTemplateLabel->setText(
        m_currentTemplateFile.isEmpty() ? QString()
                                      : QDir(m_baseTemplateDir).relativeFilePath(m_currentTemplateFile));
    ui->selectedTemplateLabel->setVisible(isManualSelection());

    ui->svgPreviewWidget->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    ui->svgPreviewWidget->updateGeometry();
    updatePreviewSize();
    Q_EMIT templateValidityChanged(isTemplateValid());
}

bool TaskNewPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->svgPreviewWidget && event->type() == QEvent::Paint) {
        QPainter painter(ui->svgPreviewWidget);
        // SVG templates describe ink on paper, independently of the application theme.
        painter.fillRect(ui->svgPreviewWidget->rect(), Qt::white);
        ui->svgPreviewWidget->renderer()->render(&painter);
        return true;
    }
    if (watched == ui->svgPreviewWidget
        && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
        updatePreviewSize();
    }
    return QWidget::eventFilter(watched, event);
}

void TaskNewPage::updatePreviewSize()
{
    int currentWidgetWidth = ui->svgPreviewWidget->width();
    QSizeF svgSize = ui->svgPreviewWidget->renderer()->defaultSize();
    if (!svgSize.isEmpty() && svgSize.width() > 0) {
        double aspectRatio = svgSize.height() / svgSize.width();
        int calculatedHeight = static_cast<int>(currentWidgetWidth * aspectRatio);
        if (ui->svgPreviewWidget->height() != calculatedHeight) {
            ui->svgPreviewWidget->setFixedHeight(calculatedHeight);
        }
    }
}

void TaskNewPage::populateStandards()
{
    ui->standardComboBox->blockSignals(true);
    ui->standardComboBox->clear();

    QDir dir(m_baseTemplateDir);
    QStringList rawEntries =
        dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware);

    QStringList entries;
    for (const QString& entry : rawEntries) {
        if (!entry.startsWith(QLatin1Char('.'))) {
            entries.append(entry);
        }
    }

    ui->standardComboBox->addItems(entries);
    ui->standardComboBox->addItem(tr("Choose template manually"), true);

    if (!entries.isEmpty()) {

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/TechDraw");
        const QString standard = QString::fromStdString(
            hGrp->GetASCII("TemplateLastUsedStandardName", "ISO5457"));
        int index = ui->standardComboBox->findText(standard);
        if (index < 0) {
            index = ui->standardComboBox->findText(QStringLiteral("ISO5457"));
        }
        if (index < 0) {
            index = 0;
        }
        ui->standardComboBox->setCurrentIndex(index);
    }
    else {
        ui->svgPreviewWidget->setToolTip(tr("No standards found in: %1").arg(m_baseTemplateDir));
    }
    ui->standardComboBox->blockSignals(false);
    onStandardChanged(ui->standardComboBox->currentIndex());
}

bool TaskNewPage::isManualSelection() const
{
    return ui->standardComboBox->currentData().toBool();
}

void TaskNewPage::onStandardChanged(int index)
{
    Q_UNUSED(index);
    const bool manual = isManualSelection();
    ui->labelSize->setVisible(!manual);
    ui->sizeComboBox->setVisible(!manual);
    ui->labelOrientation->setVisible(!manual);
    ui->landscapeRadioButton->setVisible(!manual);
    ui->portraitRadioButton->setVisible(!manual);
    ui->labelLanguage->setVisible(!manual);
    ui->languageComboBox->setVisible(!manual);
    ui->browseTemplateButton->setVisible(manual);
    ui->selectedTemplateLabel->setVisible(manual);
    if (manual) {
        updatePreviewAndPath();
    }
    else {
        populateSizes();
    }
}

void TaskNewPage::populateSizes()
{
    ui->sizeComboBox->blockSignals(true);
    ui->sizeComboBox->clear();

    QString currentStandard = ui->standardComboBox->currentText();
    if (currentStandard.isEmpty()) {
        ui->sizeComboBox->blockSignals(false);
        updatePreviewAndPath();
        return;
    }

    QDir standardDir(QDir(m_baseTemplateDir).filePath(currentStandard));
    if (!standardDir.exists()) {
        ui->svgPreviewWidget->setToolTip(
            tr("Standard directory not found: %1").arg(standardDir.path()));
        ui->sizeComboBox->blockSignals(false);
        updatePreviewAndPath();
        return;
    }

    QStringList entries =
        standardDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware);
    if (!entries.isEmpty()) {
        ui->sizeComboBox->addItems(entries);

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/TechDraw");
        const QString size = QString::fromStdString(
            hGrp->GetASCII("TemplateLastUsedSizeName", "A3"));
        int index = ui->sizeComboBox->findText(size);
        if (index < 0) {
            index = ui->sizeComboBox->findText(QStringLiteral("A3"));
        }
        if (index < 0) {
            index = 0;
        }
        ui->sizeComboBox->setCurrentIndex(index);
    }
    else {
        ui->svgPreviewWidget->setToolTip(tr("No sizes found for standard: %1").arg(currentStandard));
    }

    ui->sizeComboBox->blockSignals(false);
    updatePreviewAndPath();
}

void TaskNewPage::onBrowseTemplate()
{
    const QString workingDirectory = Gui::FileDialog::getWorkingDirectory();
    const QString filename = Gui::FileDialog::getOpenFileName(
        Gui::getMainWindow(), tr("Select a template file"), m_baseTemplateDir,
        Gui::FileDialog::FilterList{{tr("Template"), {"*.svg"}}});
    Gui::FileDialog::setWorkingDirectory(workingDirectory);
    if (!filename.isEmpty()) {
        m_browsedTemplateFile = filename;
        updatePreviewAndPath();
    }
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
        return QString();
    }
    QString orientationSubfolder =
        landscape ? QStringLiteral("landscape") : QStringLiteral("portrait");
    QString orientationDirPath = QDir(m_baseTemplateDir).filePath(
        QStringLiteral("%1/%2/%3").arg(standard, size, orientationSubfolder));

    QDir orientationDir(orientationDirPath);

    if (!orientationDir.exists()) {
        Base::Console().warning("Template orientation directory not found: %s\n",
        orientationDirPath.toStdString().c_str());
        return QString();
    }
    QStringList nameFilters;
    nameFilters << QStringLiteral("*.svg");

    QStringList files =
        orientationDir.entryList(nameFilters, QDir::Files | QDir::NoSymLinks, QDir::Name);

    if (!files.isEmpty()) {
        return orientationDir.filePath(files.first());
    }

    Base::Console().warning("No SVG file found in: %s\n", orientationDirPath.toStdString().c_str());
    return QString();
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
    return tfi.exists() && tfi.isReadable() && tfi.isFile()
        && ui->svgPreviewWidget->renderer()->isValid();
}

bool TaskNewPage::acceptPageCreation()
{
    if (!isTemplateValid()) {
        Base::Console().warning("%s\n", tr("A valid template file must be selected to proceed.").toStdString().c_str());
        return false;
    }

    QString templateFileName = getSelectedTemplatePath();

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc || !doc->getDocument()) {
        Base::Console().warning("%s\n", tr("No active document found.").toStdString().c_str());
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
    svgTemplate->Language.setValue(isManualSelection() ? 0 : ui->languageComboBox->currentIndex());

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
    if (!isManualSelection()) {
        hGrp->SetBool("TemplateLastUsedLandscape", ui->landscapeRadioButton->isChecked());
        hGrp->SetASCII("TemplateLastUsedStandardName", ui->standardComboBox->currentText().toUtf8().constData());
        hGrp->SetASCII("TemplateLastUsedSizeName", ui->sizeComboBox->currentText().toUtf8().constData());
        hGrp->SetInt("TemplateLastUsedLanguage", ui->languageComboBox->currentIndex());
    }

    return true;
}

TaskDlgNewPage::TaskDlgNewPage() : Gui::TaskView::TaskDialog()
    , m_widget(new TaskNewPage())
{
    addTaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_PageDefault"), m_widget);
}

void TaskDlgNewPage::modifyStandardButtons(QDialogButtonBox* buttons)
{
    if (auto* okButton = buttons->button(QDialogButtonBox::Ok)) {
        okButton->setEnabled(m_widget->isTemplateValid());
        connect(m_widget, &TaskNewPage::templateValidityChanged,
                okButton, &QWidget::setEnabled, Qt::UniqueConnection);
    }
}

void TaskDlgNewPage::open()
{
    m_widget->updatePreviewAndPath();
}

bool TaskDlgNewPage::accept()
{
    return m_widget->acceptPageCreation();
}

bool TaskDlgNewPage::reject()
{
    return true;
}

}  // namespace TechDrawGui

#include "moc_TaskNewPage.cpp"
