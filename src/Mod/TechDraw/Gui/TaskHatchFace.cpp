// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 Pierre-Louis Boyer

#include <algorithm>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <QSvgRenderer>
#include <QSignalBlocker>

#include <App/Document.h>
#include <App/PropertyLinks.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/HatchLine.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "TaskHatchFace.h"
#include "ViewProviderGeomHatch.h"
#include "ViewProviderHatch.h"
#include "ui_TaskHatchFace.h"

namespace TechDrawGui
{

TaskHatchFace::TaskHatchFace(
    TechDraw::DrawViewPart* view,
    const std::vector<std::string>& subNames,
    QWidget* parent
)
    : QWidget(parent)
    , ui(std::make_unique<Ui_TaskHatchFace>())
    , m_document(view->getDocument())
    , m_view(view)
    , m_subNames(subNames)
{
    init();
    loadDefaults();
    auto guiDocument = Gui::Application::Instance->getDocument(m_document.getDocument());
    m_transactionId = guiDocument->openCommand(QT_TRANSLATE_NOOP("Command", "Create Hatch"));
    // Removing old faces and creating the replacement belong to the same undo operation.
    removeExistingHatches();
    m_isLoading = false;
    onPatternChanged(ui->patternComboBox->currentIndex());
}

TaskHatchFace::TaskHatchFace(App::DocumentObject* hatch, QWidget* parent)
    : QWidget(parent)
    , ui(std::make_unique<Ui_TaskHatchFace>())
    , m_document(hatch->getDocument())
    , m_target(hatch)
    , m_originalLabel(hatch->Label.getValue())
    , m_isEditMode(true)
{
    init();
    if (const auto source = hatch->getPropertyByName<App::PropertyLinkSub>("Source")) {
        m_view = source->getValue();
        m_subNames = source->getSubValues();
    }
    if (hatch->isDerivedFrom<TechDraw::DrawHatch>()) {
        m_svgHatch = hatch;
    }
    else {
        m_patHatch = hatch;
    }
    loadFromObject(hatch);
    auto guiDocument = Gui::Application::Instance->getDocument(m_document.getDocument());
    m_transactionId = guiDocument->openCommand(QT_TRANSLATE_NOOP("Command", "Edit Hatch"));
    m_isLoading = false;
    // Opening an existing hatch must not rewrite its embedded pattern or clamp its properties.
    ui->lineWidthSpinBox->setEnabled(m_patHatch.getObject() != nullptr);
}

TaskHatchFace::~TaskHatchFace()
{
    reject();
}

bool TaskHatchFace::hasTransaction() const
{
    auto document = m_document.getDocument();
    // FreeCAD books the transaction before its first change creates an undo record.
    return m_transactionId != 0 && document && document->getBookedTransactionID() == m_transactionId;
}

void TaskHatchFace::init()
{
    ui->setupUi(this);
    setWindowTitle(m_isEditMode ? tr("Edit Face Hatch") : tr("Create Face Hatch"));
    ui->lineWidthSpinBox->setMinimum(0.0);
    ui->lineWidthSpinBox->setSingleStep(0.1);
    ui->lineWidthSpinBox->setValue(TechDraw::LineGroup::getDefaultWidth("Graphic"));

    HatchPatterns::populate(ui->patternComboBox);
    connect(
        ui->patternComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskHatchFace::onPatternChanged
    );
    connect(ui->addCustomPatternButton, &QPushButton::clicked, this, &TaskHatchFace::onAddCustomPattern);
    connect(ui->openPatternsFolder, &QToolButton::clicked, this, &HatchPatterns::openUserDirectory);
    connect(
        ui->scaleSpinBox,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &TaskHatchFace::updateHatchObjectPreview
    );
    connect(
        ui->lineWidthSpinBox,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &TaskHatchFace::updateHatchObjectPreview
    );
    connect(ui->colorButton, &Gui::ColorButton::changed, this, &TaskHatchFace::updateHatchObjectPreview);
    for (auto spinBox : {ui->rotationSpinBox, ui->offsetXSpinBox, ui->offsetYSpinBox}) {
        connect(
            spinBox,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &TaskHatchFace::updateHatchObjectPreview
        );
    }
}

void TaskHatchFace::loadDefaults()
{
    auto entry = HatchPatterns::defaultPattern();
    if (entry.filePath.isEmpty()) {
        entry = HatchPatterns::lastUsedPattern();
    }
    if (!QFileInfo::exists(entry.filePath)) {
        entry = HatchPatterns::lastUsedPattern();
    }
    if (QFileInfo::exists(entry.filePath)) {
        HatchPatterns::select(ui->patternComboBox, entry);
    }
    else {
        ui->patternComboBox->setCurrentIndex(ui->patternComboBox->count() > 0 ? 0 : -1);
        entry = ui->patternComboBox->currentData().value<PatternEntry>();
    }
    ui->scaleSpinBox->setValue(1.0);
    const auto color = entry.type == PatternUIType::SVG
        ? TechDraw::DrawHatch::prefSvgHatchColor()
        : TechDraw::DrawGeomHatch::prefGeomHatchColor();
    ui->colorButton->setColor(color.asValue<QColor>());
}

void TaskHatchFace::updateScaleRange(App::DocumentObject* obj)
{
    auto scale = obj->getPropertyByName<App::PropertyFloatConstraint>("ScalePattern");
    if (!scale) {
        if (auto vp = Gui::Application::Instance->getViewProvider(obj)) {
            scale = vp->getPropertyByName<App::PropertyFloatConstraint>("HatchScale");
        }
    }
    if (scale) {
        const QSignalBlocker blocker(ui->scaleSpinBox);
        ui->scaleSpinBox->setRange(scale->getMinimum(), scale->getMaximum());
        ui->scaleSpinBox->setSingleStep(scale->getStepSize());
    }
}

void TaskHatchFace::loadFromObject(App::DocumentObject* obj)
{
    updateScaleRange(obj);
    PatternEntry entry;
    auto vp = Gui::Application::Instance->getViewProvider(obj);
    if (auto hatch = dynamic_cast<TechDraw::DrawHatch*>(obj)) {
        entry.filePath = QDir::fromNativeSeparators(QString::fromUtf8(hatch->HatchPattern.getValue()));
        if (auto provider = dynamic_cast<ViewProviderHatch*>(vp)) {
            ui->scaleSpinBox->setValue(provider->HatchScale.getValue());
            ui->colorButton->setColor(provider->HatchColor.getValue().asValue<QColor>());
            ui->rotationSpinBox->setValue(provider->HatchRotation.getValue());
            const auto offset = provider->HatchOffset.getValue();
            ui->offsetXSpinBox->setValue(offset.x);
            ui->offsetYSpinBox->setValue(offset.y);
        }
    }
    else if (auto hatch = dynamic_cast<TechDraw::DrawGeomHatch*>(obj)) {
        entry.type = PatternUIType::PAT;
        entry.filePath = QDir::fromNativeSeparators(QString::fromUtf8(hatch->FilePattern.getValue()));
        entry.patNameInternal = QString::fromUtf8(hatch->NamePattern.getValue());
        ui->scaleSpinBox->setValue(hatch->ScalePattern.getValue());
        ui->rotationSpinBox->setValue(hatch->PatternRotation.getValue());
        const auto offset = hatch->PatternOffset.getValue();
        ui->offsetXSpinBox->setValue(offset.x);
        ui->offsetYSpinBox->setValue(offset.y);
        if (auto provider = dynamic_cast<ViewProviderGeomHatch*>(vp)) {
            ui->colorButton->setColor(provider->ColorPattern.getValue().asValue<QColor>());
            ui->lineWidthSpinBox->setValue(provider->WeightPattern.getValue());
        }
    }
    entry.displayName = QFileInfo(entry.filePath).fileName();
    if (entry.type == PatternUIType::PAT) {
        entry.displayName += QStringLiteral(" - ") + entry.patNameInternal;
    }
    HatchPatterns::select(ui->patternComboBox, entry);
}

void TaskHatchFace::removeExistingHatches()
{
    auto view = m_view.getObjectAs<TechDraw::DrawViewPart>();
    auto removeFaces = [this](const auto& hatches) {
        for (auto hatch : hatches) {
            auto faces = hatch->Source.getSubValues();
            const auto removed = std::erase_if(faces, [this](const auto& face) {
                return std::ranges::find(m_subNames, face) != m_subNames.end();
            });
            if (removed == 0) {
                continue;
            }
            if (faces.empty()) {
                hatch->getDocument()->removeObject(hatch->getNameInDocument());
            }
            else {
                hatch->Source.setValue(hatch->Source.getValue(), faces);
            }
        }
    };
    removeFaces(view->getHatches());
    removeFaces(view->getGeomHatches());
}

void TaskHatchFace::onPatternChanged(int index)
{
    if (m_isLoading || index < 0) {
        return;
    }
    const auto entry = ui->patternComboBox->itemData(index).value<PatternEntry>();
    ui->lineWidthSpinBox->setEnabled(entry.type == PatternUIType::PAT);
    updateHatchObjectPreview();
}

void TaskHatchFace::onAddCustomPattern()
{
    const auto fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select Custom Pattern File"),
        QString(),
        tr("Hatch patterns (*.svg *.pat);;SVG files (*.svg);;PAT files (*.pat)")
    );
    if (fileName.isEmpty()) {
        return;
    }
    const QFileInfo fileInfo(fileName);
    const auto suffix = fileInfo.suffix().toLower();
    PatternEntry entry;
    if (suffix == QLatin1String("pat")) {
        auto filePath = fileName.toStdString();
        const auto names = TechDraw::PATLineSpec::getPatternList(filePath);
        if (names.empty()) {
            QMessageBox::warning(this, tr("Invalid File"), tr("The PAT file contains no patterns."));
            return;
        }
        entry.type = PatternUIType::PAT;
        entry.patNameInternal = QString::fromStdString(names.front());
    }
    else if (suffix != QLatin1String("svg") || !QSvgRenderer(fileName).isValid()) {
        QMessageBox::warning(
            this,
            tr("Invalid File"),
            tr("Selected file is not a valid SVG or PAT file.")
        );
        return;
    }

    const auto directory = HatchPatterns::userDirectory(entry.type);
    entry.filePath = QDir(directory).filePath(fileInfo.fileName());
    if (!QDir().mkpath(directory)) {
        QMessageBox::critical(
            this,
            tr("Directory Creation Error"),
            tr("Could not create directory '%1'.").arg(directory)
        );
        return;
    }
    if (QFileInfo(entry.filePath).canonicalFilePath() != fileInfo.canonicalFilePath()) {
        if (QFile::exists(entry.filePath)
            && QMessageBox::question(
                   this,
                   tr("File Exists"),
                   tr("Pattern file '%1' already exists. Overwrite?").arg(fileInfo.fileName()),
                   QMessageBox::Yes | QMessageBox::No
               ) != QMessageBox::Yes) {
            return;
        }
        // Atomic replacement keeps the old pattern if reading or writing the import fails.
        QFile source(fileName);
        QSaveFile target(entry.filePath);
        if (!source.open(QIODevice::ReadOnly) || !target.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(
                this,
                tr("Copy Error"),
                tr("Could not copy file to '%1'.").arg(directory)
            );
            return;
        }
        const auto data = source.readAll();
        if (source.error() != QFile::NoError || target.write(data) != data.size()
            || !target.commit()) {
            QMessageBox::critical(
                this,
                tr("Copy Error"),
                tr("Could not copy file to '%1'.").arg(directory)
            );
            return;
        }
    }
    {
        const QScopedValueRollback<bool> loading(m_isLoading, true);
        HatchPatterns::populate(ui->patternComboBox);
        HatchPatterns::select(ui->patternComboBox, entry);
    }
    onPatternChanged(ui->patternComboBox->currentIndex());
}

void TaskHatchFace::updateHatchObjectPreview()
{
    if (m_isLoading || !hasTransaction() || ui->patternComboBox->currentIndex() < 0) {
        return;
    }
    auto document = m_document.getDocument();
    const auto entry = ui->patternComboBox->currentData().value<PatternEntry>();
    auto& selectedHatch = entry.type == PatternUIType::SVG ? m_svgHatch : m_patHatch;
    auto hatch = selectedHatch.getObject();
    if (!hatch) {
        const bool svg = entry.type == PatternUIType::SVG;
        hatch = document->addObject(
            svg ? "TechDraw::DrawHatch" : "TechDraw::DrawGeomHatch",
            svg ? "Hatch" : "GeomHatch"
        );
        selectedHatch = hatch;
        if (m_target) {
            hatch->Label.setValue(m_target.getObject()->Label.getValue());
        }
    }
    if (auto previous = m_target.getObject(); previous && previous != hatch) {
        // Keep each type until acceptance. Switching back then preserves the original object,
        // its name, embedded file and incoming links; Cancel restores everything in one step.
        if (auto source = previous->getPropertyByName<App::PropertyLinkSub>("Source")) {
            source->setValue(nullptr);
        }
    }
    m_target = hatch;
    updateScaleRange(hatch);
    applyToDocumentObject(hatch, entry);
    refreshView();
}

void TaskHatchFace::applyToDocumentObject(App::DocumentObject* obj, const PatternEntry& entry)
{
    if (auto source = obj->getPropertyByName<App::PropertyLinkSub>("Source")) {
        source->setValue(m_view.getObject(), m_subNames);
    }
    const auto provider = Gui::Application::Instance->getViewProvider(obj);
    Base::Color color;
    color.setValue(ui->colorButton->color());
    const Base::Vector3d offset(ui->offsetXSpinBox->value(), ui->offsetYSpinBox->value(), 0.0);
    if (auto hatch = dynamic_cast<TechDraw::DrawHatch*>(obj)) {
        if (entry.filePath
            != QDir::fromNativeSeparators(QString::fromUtf8(hatch->HatchPattern.getValue()))) {
            hatch->HatchPattern.setValue(entry.filePath.toUtf8().constData());
        }
        if (auto vp = dynamic_cast<ViewProviderHatch*>(provider)) {
            vp->HatchScale.setValue(ui->scaleSpinBox->value().getValue());
            vp->HatchColor.setValue(color);
            vp->HatchRotation.setValue(ui->rotationSpinBox->value());
            vp->HatchOffset.setValue(offset);
        }
    }
    else if (auto hatch = dynamic_cast<TechDraw::DrawGeomHatch*>(obj)) {
        if (entry.filePath
            != QDir::fromNativeSeparators(QString::fromUtf8(hatch->FilePattern.getValue()))) {
            hatch->FilePattern.setValue(entry.filePath.toUtf8().constData());
        }
        hatch->NamePattern.setValue(entry.patNameInternal.toUtf8().constData());
        hatch->ScalePattern.setValue(ui->scaleSpinBox->value().getValue());
        hatch->PatternRotation.setValue(ui->rotationSpinBox->value());
        hatch->PatternOffset.setValue(offset);
        if (auto vp = dynamic_cast<ViewProviderGeomHatch*>(provider)) {
            vp->ColorPattern.setValue(color);
            vp->WeightPattern.setValue(ui->lineWidthSpinBox->value().getValue());
        }
    }
}

void TaskHatchFace::refreshView()
{
    if (auto document = m_document.getDocument()) {
        document->recompute();
    }
    if (auto view = m_view.getObjectAs<TechDraw::DrawViewPart>()) {
        view->requestPaint();
    }
}

bool TaskHatchFace::accept()
{
    if (!hasTransaction() || !m_target) {
        return false;
    }
    if (ui->patternComboBox->currentIndex() < 0) {
        QMessageBox::warning(this, tr("No Pattern Selected"), tr("No hatch pattern is selected."));
        return false;
    }
    auto document = m_document.getDocument();
    auto target = m_target.getObject();
    for (const auto& candidate : {m_svgHatch, m_patHatch}) {
        auto unused = candidate.getObject();
        if (!unused || unused == target) {
            continue;
        }
        // Preserve group membership and other links when accepting a change of hatch type.
        const auto parents = unused->getInList();
        for (auto parent : parents) {
            std::vector<App::Property*> properties;
            parent->getPropertyList(properties);
            for (auto property : properties) {
                if (auto link = dynamic_cast<App::PropertyLinkBase*>(property)) {
                    std::unique_ptr<App::Property> replacement(
                        link->CopyOnLinkReplace(parent, unused, target)
                    );
                    if (replacement) {
                        link->Paste(*replacement);
                    }
                }
            }
        }
        document->removeObject(unused->getNameInDocument());
    }
    if (m_isEditMode) {
        // The original label becomes available after removing the other hatch type.
        target->Label.setValue(m_originalLabel);
    }
    auto guiDocument = Gui::Application::Instance->getDocument(document);
    m_transactionId = 0;
    guiDocument->commitCommand();
    HatchPatterns::saveLastUsedPattern(ui->patternComboBox->currentData().value<PatternEntry>());
    refreshView();
    guiDocument->resetEdit();
    return true;
}

bool TaskHatchFace::reject()
{
    if (hasTransaction()) {
        auto guiDocument = Gui::Application::Instance->getDocument(m_document.getDocument());
        m_transactionId = 0;
        guiDocument->abortCommand();
        refreshView();
        guiDocument->resetEdit();
    }
    return true;
}

void TaskHatchFace::documentClosed()
{
    // The document is already being torn down; its undo state must not be replayed here.
    m_transactionId = 0;
}

void TaskHatchFace::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setWindowTitle(m_isEditMode ? tr("Edit Face Hatch") : tr("Create Face Hatch"));
    }
    QWidget::changeEvent(event);
}

TaskDlgHatchFace::TaskDlgHatchFace(TechDraw::DrawViewPart* view, const std::vector<std::string>& subNames)
    : widget(new TaskHatchFace(view, subNames))
{
    init(view);
}

TaskDlgHatchFace::TaskDlgHatchFace(App::DocumentObject* hatch)
    : widget(new TaskHatchFace(hatch))
{
    init(hatch);
}

void TaskDlgHatchFace::init(App::DocumentObject* object)
{
    addTaskBox(Gui::BitmapFactory().pixmap("TechDraw_TreeHatch"), widget);
    setDocumentName(object->getDocument()->getName());
    setAutoCloseOnDeletedDocument(true);
    setAutoCloseOnTransactionChange(true);
}

bool TaskDlgHatchFace::accept()
{
    return widget->accept();
}

bool TaskDlgHatchFace::reject()
{
    return widget->reject();
}

void TaskDlgHatchFace::autoClosedOnDeletedDocument()
{
    widget->documentClosed();
}

}  // namespace TechDrawGui

#include "moc_TaskHatchFace.cpp"
