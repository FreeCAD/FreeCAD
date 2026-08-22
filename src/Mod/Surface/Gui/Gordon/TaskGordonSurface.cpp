// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Andrew Shkolik <shkolik@gmail.com>                  *
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
# include <QAction>
# include <QMenu>
# include <QMessageBox>
# include <QTimer>
# include <QDoubleSpinBox>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Widgets.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "Gordon/TaskGordonSurface.h"

#include "Gordon/ui_TaskGordonSurface.h"

using namespace SurfaceGui;

namespace SurfaceGui
{

class GordonSurfacePanel::ShapeSelection: public Gui::SelectionFilterGate
{
public:
    ShapeSelection(
        GordonSurfacePanel::SelectionMode& mode,
        GordonSurfacePanel::SelectionType& selectionType,
        Surface::GordonSurface* editedObject
    )
        : Gui::SelectionFilterGate(nullPointer())
        , mode(mode)
        , editedObject(editedObject)
        , selectionType(selectionType)
    {}
    ~ShapeSelection() override
    {
        mode = GordonSurfacePanel::None;
    }
    /**
     * Allow the user to pick only edges.
     */
    bool allow(App::Document*, App::DocumentObject* pObj, const char* sSubName) override
    {
        // don't allow references to itself
        if (pObj == editedObject) {
            return false;
        }
        if (!pObj->isDerivedFrom<Part::Feature>()) {
            return false;
        }

        if (Base::Tools::isNullOrEmpty(sSubName)) {
            return false;
        }

        switch (mode) {
            case GordonSurfacePanel::AppendEdge:
                return allowEdge(true, editedObject->GuideEdges, pObj, sSubName)
                    && allowEdge(true, editedObject->ProfileEdges, pObj, sSubName);
            case GordonSurfacePanel::RemoveEdge:
                return selectionType == Guide
                    ? allowEdge(false, editedObject->GuideEdges, pObj, sSubName)
                    : allowEdge(false, editedObject->ProfileEdges, pObj, sSubName);
            default:
                return false;
        }
    }

private:
    bool allowEdge(
        bool appendEdges,
        const App::PropertyLinkSubList& edges,
        const App::DocumentObject* pObj,
        const char* sSubName
    ) const
    {
        if (std::string element(sSubName); !element.starts_with("Edge")) {
            return false;
        }

        auto links = edges.getSubListValues();

        for (const auto& [objPtr, subNames] : links) {
            if (objPtr == pObj) {
                for (const auto& jt : subNames) {
                    if (jt == sSubName) {
                        return !appendEdges;
                    }
                }
            }
        }

        return appendEdges;
    }

private:
    GordonSurfacePanel::SelectionMode& mode;
    GordonSurfacePanel::SelectionType& selectionType;
    Surface::GordonSurface* editedObject;
};

// ----------------------------------------------------------------------------

GordonSurfacePanel::GordonSurfacePanel(ViewProviderGordonSurface* vp, Surface::GordonSurface* obj)
    : editedObject(obj)
{
    ui = new Ui_TaskGordonSurface();
    ui->setupUi(this);
    setupConnections();
    // ui->statusLabel->clear();

    selectionType = Profile;
    selectionMode = None;
    this->vp = vp;
    checkCommand = true;
    setEditedObject(obj);

    // Create context menu
    auto* reverseProfile = new QAction(tr("Reverse"), this);
    reverseProfile->setShortcut(QStringLiteral("R"));
    reverseProfile->setShortcutContext(Qt::WidgetShortcut);
    ui->listProfiles->addAction(reverseProfile);
    connect(reverseProfile, &QAction::triggered, this, &GordonSurfacePanel::onReverseProfile);

    auto* deleteProfile = new QAction(tr("Remove"), this);
    deleteProfile->setShortcut(QStringLiteral("Del"));
    deleteProfile->setShortcutContext(Qt::WidgetShortcut);
    ui->listProfiles->addAction(deleteProfile);
    connect(deleteProfile, &QAction::triggered, this, &GordonSurfacePanel::onDeleteProfile);

    ui->listProfiles->setContextMenuPolicy(Qt::ActionsContextMenu);

    auto* reverseGuide = new QAction(tr("Reverse"), this);
    reverseGuide->setShortcut(QStringLiteral("R"));
    reverseGuide->setShortcutContext(Qt::WidgetShortcut);
    ui->listGuides->addAction(reverseGuide);
    connect(reverseGuide, &QAction::triggered, this, &GordonSurfacePanel::onReverseGuide);

    auto* deleteGuide = new QAction(tr("Remove"), this);
    deleteGuide->setShortcut(QStringLiteral("Del"));
    deleteGuide->setShortcutContext(Qt::WidgetShortcut);
    ui->listGuides->addAction(deleteGuide);
    connect(deleteGuide, &QAction::triggered, this, &GordonSurfacePanel::onDeleteGuide);

    ui->listGuides->setContextMenuPolicy(Qt::ActionsContextMenu);
}

/*
 *  Destroys the object and frees any allocated resources
 */
GordonSurfacePanel::~GordonSurfacePanel()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void GordonSurfacePanel::setupConnections()
{
    // clang-format off

    connect(ui->buttonProfileAdd, &QToolButton::toggled,
            this, &GordonSurfacePanel::onButtonProfileAddToggled);
    connect(ui->buttonProfileRemove, &QToolButton::toggled,
            this, &GordonSurfacePanel::onButtonProfileRemoveToggled);

    connect(ui->buttonGuideAdd, &QToolButton::toggled,
            this, &GordonSurfacePanel::onButtonGuideAddToggled);
    connect(ui->buttonGuideRemove, &QToolButton::toggled,
            this, &GordonSurfacePanel::onButtonGuideRemoveToggled);

    connect(ui->toleranceSpinBox, &QDoubleSpinBox::valueChanged,
            this, &GordonSurfacePanel::onToleranceChanged);
    connect(ui->checkUseNative, &QCheckBox::toggled,
            this, &GordonSurfacePanel::onUseNativeToggled);
    connect(ui->checkParallelMode, &QCheckBox::toggled,
            this, &GordonSurfacePanel::onParallelModeToggled);
    connect(ui->comboApproxMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GordonSurfacePanel::onApproxModeChanged);

    // clang-format on
}

void GordonSurfacePanel::appendButtons(Gui::ButtonGroup* buttonGroup)
{
    buttonGroup->addButton(ui->buttonProfileAdd, int(SelectionMode::AppendEdge));
    buttonGroup->addButton(ui->buttonProfileRemove, int(SelectionMode::RemoveEdge));
    buttonGroup->addButton(ui->buttonGuideAdd, int(SelectionMode::AppendEdge));
    buttonGroup->addButton(ui->buttonGuideRemove, int(SelectionMode::RemoveEdge));
}

void GordonSurfacePanel::setEditedObject(Surface::GordonSurface* fea)
{
    editedObject = fea;
    App::Document const* doc = editedObject->getDocument();

    // get the profiles
    auto profilesObjects = editedObject->ProfileEdges.getValues();
    auto profileEdges = editedObject->ProfileEdges.getSubValues();
    auto profileDirections = editedObject->ProfileDirections.getValues();

    for (std::size_t i = 0; i < profilesObjects.size(); i++) {
        App::DocumentObject const* obj = profilesObjects[i];
        std::string edge = profileEdges[i];
        bool direction = profileDirections[i];

        auto* item = new QListWidgetItem(ui->listProfiles);
        ui->listProfiles->addItem(item);

        QString text = QStringLiteral("%1.%2%3").arg(
            QString::fromUtf8(obj->Label.getValue()),
            QString::fromStdString(edge),
            QString::fromUtf8(direction ? " (Reversed)" : "")
        );
        item->setText(text);

        // The user data field of a list widget item
        // is a list of 5 elements:
        // 1. document name
        // 2. object name
        // 3. sub-element name of the edge
        // 4. direction of the edge
        // 5. object label
        QList<QVariant> data;
        data << QByteArray(doc->getName());
        data << QByteArray(obj->getNameInDocument());
        data << QByteArray(edge.c_str());
        data << QVariant(direction);
        data << QByteArray(obj->Label.getValue());
        item->setData(Qt::UserRole, data);
    }

    // get the guides
    auto guidesObjects = editedObject->GuideEdges.getValues();
    auto guidesEdges = editedObject->GuideEdges.getSubValues();
    auto guideDirections = editedObject->GuideDirections.getValues();

    for (std::size_t i = 0; i < guidesObjects.size(); i++) {
        App::DocumentObject const* obj = guidesObjects[i];
        std::string edge = guidesEdges[i];
        bool direction = guideDirections[i];
        auto* item = new QListWidgetItem(ui->listGuides);
        ui->listGuides->addItem(item);

        QString text = QStringLiteral("%1.%2%3").arg(
            QString::fromUtf8(obj->Label.getValue()),
            QString::fromStdString(edge),
            QString::fromUtf8(direction ? " (Reversed)" : "")
        );
        item->setText(text);

        // The user data field of a list widget item
        // is a list of 5 elements:
        // 1. document name
        // 2. object name
        // 3. sub-element name of the edge
        // 4. direction of the edge
        // 5. object label
        QList<QVariant> data;
        data << QByteArray(doc->getName());
        data << QByteArray(obj->getNameInDocument());
        data << QByteArray(edge.c_str());
        data << QVariant(guideDirections[i]);
        data << QByteArray(obj->Label.getValue());
        item->setData(Qt::UserRole, data);
    }

    // attach this document observer
    attachDocument(Gui::Application::Instance->getDocument(doc));

    // populate algorithm controls
    double tol = editedObject->Tolerance.getValue();
    ui->toleranceSpinBox->setValue(tol < Precision::Confusion() ? Precision::Confusion() : tol);

    ui->checkUseNative->setChecked(editedObject->UseNativeAlgorithm.getValue());
    bool nativeEnabled = editedObject->UseNativeAlgorithm.getValue();
    ui->checkParallelMode->setEnabled(nativeEnabled);
    ui->comboApproxMode->setEnabled(nativeEnabled);
    ui->checkParallelMode->setChecked(editedObject->ParallelMode.getValue());

    const char* approxStr = editedObject->ApproximationMode.getValueAsString();
    if (strcmp(approxStr, "Allow approximation") == 0) {
        ui->comboApproxMode->setCurrentIndex(1);
    }
    else {
        ui->comboApproxMode->setCurrentIndex(0);
    }
}

void GordonSurfacePanel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void GordonSurfacePanel::open()
{
    checkOpenCommand();

    // highlight all edges
    this->vp->highlightReferences(
        editedObject->ProfileEdges.getSubListValues(),
        editedObject->GuideEdges.getSubListValues(),
        true
    );
    clearSelection();

    // if the surface is not yet created then automatically start "AppendEdge" mode
    if (editedObject->Shape.getShape().isNull()) {
        ui->buttonProfileAdd->setChecked(true);
    }
}

void GordonSurfacePanel::clearSelection()
{
    Gui::Selection().clearSelection();
}

void GordonSurfacePanel::checkOpenCommand()
{
    if (checkCommand && !editedObject->getDocument()->hasPendingTransaction()) {
        std::string Msg("Edit ");
        Msg += editedObject->Label.getValue();
        editedObject->getDocument()->openTransaction(Msg.c_str());
        checkCommand = false;
    }
}

void GordonSurfacePanel::slotUndoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void GordonSurfacePanel::slotRedoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void GordonSurfacePanel::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    // If this view provider is being deleted then reset the colors of
    // referenced part objects. The dialog will be deleted later.
    if (this->vp == &Obj) {
        this->vp->highlightReferences(
            editedObject->ProfileEdges.getSubListValues(),
            editedObject->GuideEdges.getSubListValues(),
            false
        );
    }
}

bool GordonSurfacePanel::accept()
{
    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    if (editedObject->mustExecute()) {
        editedObject->recomputeFeature();
    }
    if (!editedObject->isValid()) {
        QMessageBox::warning(
            this,
            tr("Invalid object"),
            QString::fromLatin1(editedObject->getStatusString())
        );
        return false;
    }

    this->vp->highlightReferences(
        editedObject->ProfileEdges.getSubListValues(),
        editedObject->GuideEdges.getSubListValues(),
        false
    );

    // Close the edit session and commit the transaction
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    editedObject->getDocument()->commitTransaction();
    Gui::Command::updateActive();
    return true;
}

bool GordonSurfacePanel::reject()
{
    if (!editedObject.expired()) {
        this->vp->highlightReferences(
            editedObject->ProfileEdges.getSubListValues(),
            editedObject->GuideEdges.getSubListValues(),
            false
        );
    }

    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    // Abort the transaction and close the edit session
    editedObject->getDocument()->abortTransaction();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();
    return true;
}

void GordonSurfacePanel::onButtonProfileAddToggled(bool checked)
{
    if (checked) {
        selectionType = Profile;
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(
            new ShapeSelection(selectionMode, selectionType, editedObject.get())
        );
        selectionMode = AppendEdge;
    }
    else if (selectionType == Profile && selectionMode == AppendEdge) {
        exitSelectionMode();
    }
}

void GordonSurfacePanel::onButtonProfileRemoveToggled(bool checked)
{
    if (checked) {
        selectionType = Profile;
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(
            new ShapeSelection(selectionMode, selectionType, editedObject.get())
        );
        selectionMode = RemoveEdge;
    }
    else if (selectionType == Profile && selectionMode == RemoveEdge) {
        exitSelectionMode();
    }
}

void GordonSurfacePanel::onButtonGuideAddToggled(bool checked)
{
    if (checked) {
        selectionType = Guide;
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(
            new ShapeSelection(selectionMode, selectionType, editedObject.get())
        );
        selectionMode = AppendEdge;
    }
    else if (selectionType == Guide && selectionMode == AppendEdge) {
        exitSelectionMode();
    }
}

void GordonSurfacePanel::onButtonGuideRemoveToggled(bool checked)
{
    if (checked) {
        selectionType = Guide;
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(
            new ShapeSelection(selectionMode, selectionType, editedObject.get())
        );
        selectionMode = RemoveEdge;
    }
    else if (selectionType == Guide && selectionMode == RemoveEdge) {
        exitSelectionMode();
    }
}

void GordonSurfacePanel::appendEdges(
    const QList<QVariant> data,
    QListWidget* list,
    App::PropertyLinkSubList& edges,
    App::PropertyBoolList& directions
)
{
    auto item = new QListWidgetItem(list);
    list->addItem(item);

    auto docName = data[0].toByteArray();
    auto objectName = data[1].toByteArray();
    auto subName = data[2].toByteArray();

    auto doc = App::GetApplication().getDocument(docName);
    auto obj = doc ? doc->getObject(objectName) : nullptr;

    if (!obj) {
        return;
    }

    item->setText(QStringLiteral("%1.%2")
                      .arg(QString::fromUtf8(obj->Label.getValue()), QString::fromUtf8(subName)));

    item->setData(
        Qt::UserRole,
        QVariantList() << data[0] << data[1] << data[2] << QVariant(false)
                       << QByteArray(obj->Label.getValue())
    );

    auto objects = edges.getValues();
    objects.push_back(obj);
    auto element = edges.getSubValues();
    element.emplace_back(subName);
    edges.setValues(objects, element);
    auto dirs = directions.getValues();
    dirs.push_back(false);  // default direction
    directions.setValues(dirs);
}

void remove_bit_at(boost::dynamic_bitset<>& db, size_t pos)
{
    if (pos >= db.size()) {
        return;  // check bounds
    }

    // Shift all bits after 'pos' one position to the left (down)
    for (size_t i = pos; i < db.size() - 1; ++i) {
        db[i] = db[i + 1];
    }

    // Resize the bitset to the new, smaller size
    db.resize(db.size() - 1);
}

void GordonSurfacePanel::removeEdge(
    const QList<QVariant> data,
    QListWidget* list,
    App::PropertyLinkSubList& edges,
    App::PropertyBoolList& directions
)
{
    auto docName = data[0].toByteArray();
    auto objectName = data[1].toByteArray();
    auto subName = data[2].toByteArray();

    auto doc = App::GetApplication().getDocument(docName);
    auto obj = doc ? doc->getObject(objectName) : nullptr;

    if (!obj) {
        return;
    }

    for (int i = 0; i < list->count(); i++) {
        QListWidgetItem* item = list->item(i);
        QList<QVariant> userdata = item->data(Qt::UserRole).toList();
        // only the three first elements must match
        if (userdata.mid(0, 3) == data.mid(0, 3)) {
            list->takeItem(i);
            delete item;
            break;
        }
    }

    auto objects = edges.getValues();
    auto element = edges.getSubValues();
    auto dirs = directions.getValues();

    for (std::size_t idx = 0; idx < objects.size() && idx < element.size() && idx < dirs.size();
         ++idx) {
        if (objects[idx] == obj && element[idx] == subName.toStdString()) {
            objects.erase(objects.begin() + idx);
            element.erase(element.begin() + idx);
            remove_bit_at(dirs, idx);

            edges.setValues(objects, element);
            directions.setValues(dirs);
            break;
        }
    }
}

void GordonSurfacePanel::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        auto doc = App::GetApplication().getDocument(msg.pDocName);
        auto obj = doc ? doc->getObject(msg.pObjectName) : nullptr;

        if (!obj) {
            return;
        }

        // The user data field of a list widget item
        // is a list of 5 elements, but only 3 used here:
        // 1. document name
        // 2. object name
        // 3. sub-element name of the edge
        QList<QVariant> data;
        data << QByteArray(doc->getName());
        data << QByteArray(obj->getNameInDocument());
        data << QByteArray(msg.pSubName);

        checkOpenCommand();
        if (selectionMode == AppendEdge) {
            if (selectionType == Profile) {
                appendEdges(
                    data,
                    ui->listProfiles,
                    editedObject->ProfileEdges,
                    editedObject->ProfileDirections
                );
            }
            else {
                appendEdges(data, ui->listGuides, editedObject->GuideEdges, editedObject->GuideDirections);
            }
        }
        else if (selectionMode == RemoveEdge) {
            // this->vp->highlightReferences(editedObject->ProfileEdges.getSubListValues(),
            // editedObject->GuideEdges.getSubListValues(), false);

            if (selectionType == Profile) {
                removeEdge(
                    data,
                    ui->listProfiles,
                    editedObject->ProfileEdges,
                    editedObject->ProfileDirections
                );
            }
            else {
                removeEdge(data, ui->listGuides, editedObject->GuideEdges, editedObject->GuideDirections);
            }
        }

        QTimer::singleShot(50, this, &GordonSurfacePanel::clearSelection);
        this->vp->highlightReferences(
            editedObject->ProfileEdges.getSubListValues(),
            editedObject->GuideEdges.getSubListValues(),
            true
        );
    }
}

void GordonSurfacePanel::onDeleteProfile()
{
    int row = ui->listProfiles->currentRow();
    const QListWidgetItem* item = ui->listProfiles->item(row);

    if (item) {
        checkOpenCommand();
        QList<QVariant> data = item->data(Qt::UserRole).toList();

        this->vp->highlightReferences(
            editedObject->ProfileEdges.getSubListValues(),
            editedObject->GuideEdges.getSubListValues(),
            false
        );

        removeEdge(data, ui->listProfiles, editedObject->ProfileEdges, editedObject->ProfileDirections);

        QTimer::singleShot(50, this, &GordonSurfacePanel::clearSelection);
        this->vp->highlightReferences(
            editedObject->ProfileEdges.getSubListValues(),
            editedObject->GuideEdges.getSubListValues(),
            true
        );

        // editedObject->recomputeFeature();
    }
}

void GordonSurfacePanel::onDeleteGuide()
{
    int row = ui->listGuides->currentRow();
    const QListWidgetItem* item = ui->listGuides->item(row);
    if (item) {
        checkOpenCommand();
        QList<QVariant> data = item->data(Qt::UserRole).toList();

        this->vp->highlightReferences(
            editedObject->ProfileEdges.getSubListValues(),
            editedObject->GuideEdges.getSubListValues(),
            false
        );

        removeEdge(data, ui->listGuides, editedObject->GuideEdges, editedObject->GuideDirections);

        QTimer::singleShot(50, this, &GordonSurfacePanel::clearSelection);
        this->vp->highlightReferences(
            editedObject->ProfileEdges.getSubListValues(),
            editedObject->GuideEdges.getSubListValues(),
            true
        );

        // editedObject->recomputeFeature();
    }
}

void GordonSurfacePanel::onReverseGuide()
{
    int row = ui->listGuides->currentRow();
    QListWidgetItem* item = ui->listGuides->item(row);
    if (item) {
        checkOpenCommand();
        const QList<QVariant> data = item->data(Qt::UserRole).toList();

        auto docName = data[0].toByteArray();
        auto objectName = data[1].toByteArray();
        auto subName = data[2].toByteArray();
        bool direction = data[3].toBool();

        auto doc = App::GetApplication().getDocument(docName);
        auto obj = doc ? doc->getObject(objectName) : nullptr;

        if (!obj) {
            return;
        }

        QString text = QStringLiteral("%1.%2%3").arg(
            QString::fromUtf8(obj->Label.getValue()),
            QString::fromUtf8(subName),
            QString::fromUtf8(!direction ? " (Reversed)" : "")
        );

        item->setData(
            Qt::UserRole,
            QVariantList() << data[0] << data[1] << data[2] << QVariant(!direction)
        );

        item->setText(text);
        auto objects = editedObject->GuideEdges.getValues();
        auto element = editedObject->GuideEdges.getSubValues();
        auto dirs = editedObject->GuideDirections.getValues();

        for (std::size_t idx = 0; idx < objects.size() && idx < element.size() && idx < dirs.size();
             ++idx) {
            if (objects[idx] == obj && element[idx] == subName.toStdString()) {
                dirs[idx] = !dirs[idx];
                editedObject->GuideDirections.setValues(dirs);
                break;
            }
        }
        // editedObject->recomputeFeature();
    }
}

void GordonSurfacePanel::onReverseProfile()
{
    int row = ui->listProfiles->currentRow();
    QListWidgetItem* item = ui->listProfiles->item(row);
    if (item) {
        checkOpenCommand();
        const QList<QVariant> data = item->data(Qt::UserRole).toList();

        auto docName = data[0].toByteArray();
        auto objectName = data[1].toByteArray();
        auto subName = data[2].toByteArray();
        bool direction = data[3].toBool();

        auto doc = App::GetApplication().getDocument(docName);
        auto obj = doc ? doc->getObject(objectName) : nullptr;

        if (!obj) {
            return;
        }

        QString text = QStringLiteral("%1.%2%3").arg(
            QString::fromUtf8(obj->Label.getValue()),
            QString::fromUtf8(subName),
            QString::fromUtf8(!direction ? " (Reversed)" : "")
        );

        item->setData(
            Qt::UserRole,
            QVariantList() << data[0] << data[1] << data[2] << QVariant(!direction)
        );

        item->setText(text);

        auto objects = editedObject->ProfileEdges.getValues();
        auto element = editedObject->ProfileEdges.getSubValues();
        auto dirs = editedObject->ProfileDirections.getValues();

        for (std::size_t idx = 0; idx < objects.size() && idx < element.size() && idx < dirs.size();
             ++idx) {
            if (objects[idx] == obj && element[idx] == subName.toStdString()) {
                dirs[idx] = !dirs[idx];
                editedObject->ProfileDirections.setValues(dirs);
                break;
            }
        }

        // editedObject->recomputeFeature();
    }
}

void GordonSurfacePanel::onToleranceChanged(double value)
{
    editedObject->Tolerance.setValue(value);
}

void GordonSurfacePanel::onUseNativeToggled(bool checked)
{
    checkOpenCommand();
    editedObject->UseNativeAlgorithm.setValue(checked);
    ui->checkParallelMode->setEnabled(checked);
    ui->comboApproxMode->setEnabled(checked);
}

void GordonSurfacePanel::onParallelModeToggled(bool checked)
{
    checkOpenCommand();
    editedObject->ParallelMode.setValue(checked);
}

void GordonSurfacePanel::onApproxModeChanged(int index)
{
    checkOpenCommand();
    editedObject->ApproximationMode.setValue(index == 1 ? "Allow approximation" : "Exact construction");
}


void GordonSurfacePanel::exitSelectionMode()
{
    selectionMode = None;
    // 'selectionMode' is passed by reference to the filter and changed when the filter is deleted
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

// ----------------------------------------------------------------------------

TaskGordonSurface::TaskGordonSurface(ViewProviderGordonSurface* vp, Surface::GordonSurface* obj)
{
    // Set up button group
    buttonGroup = new Gui::ButtonGroup(this);
    buttonGroup->setExclusive(true);

    // first task box
    widget = new GordonSurfacePanel(vp, obj);
    widget->appendButtons(buttonGroup);
    addTaskBox(Gui::BitmapFactory().pixmap("Surface_GordonSurface"), widget);
}

void TaskGordonSurface::setEditedObject(Surface::GordonSurface* obj)
{
    widget->setEditedObject(obj);
}

void TaskGordonSurface::open()
{
    widget->open();
}

bool TaskGordonSurface::accept()
{
    return widget->accept();
}

bool TaskGordonSurface::reject()
{
    return widget->reject();
}

}  // namespace SurfaceGui
