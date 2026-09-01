// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MaterialSelector.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QVBoxLayout>

#include <exception>
#include <memory>
#include <unordered_set>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Mod/Material/App/MaterialFilter.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/PropertyMaterial.h>

#include "MaterialTreeWidget.h"

using namespace MatGui;

MaterialSelector::MaterialSelector(QWidget* parent)
    : QPushButton(parent)
{
    setText(tr("No material"));
    setEnabled(false);
    connect(this, &QPushButton::clicked, this, &MaterialSelector::chooseMaterial);
}

void MaterialSelector::setObjects(const std::vector<App::DocumentObject*>& objects)
{
    assignedObjects.clear();
    std::unordered_set<App::DocumentObject*> seen;
    for (auto* object : objects) {
        if (object && seen.insert(object).second) {
            assignedObjects.emplace_back(object);
        }
    }
    refresh();
}

void MaterialSelector::clearObjects()
{
    assignedObjects.clear();
    refresh();
}

void MaterialSelector::refresh()
{
    QStringList materialNames;
    bool hasAssignableMaterial = false;

    for (const auto& reference : assignedObjects) {
        auto* object = reference.getObject();
        if (!object) {
            continue;
        }
        auto* materialProperty = freecad_cast<Materials::PropertyMaterial*>(
            object->getPropertyByName("ShapeMaterial")
        );
        if (!materialProperty) {
            continue;
        }
        hasAssignableMaterial = hasAssignableMaterial || !materialProperty->isReadOnly();

        QString name = materialProperty->getValue().getName();
        if (name.isEmpty()) {
            name = tr("No material");
        }
        if (!materialNames.contains(name)) {
            materialNames.append(name);
        }
    }

    setEnabled(hasAssignableMaterial);
    if (materialNames.isEmpty()) {
        setText(tr("No material"));
    }
    else if (materialNames.size() <= 2) {
        setText(materialNames.join(QStringLiteral(", ")));
    }
    else {
        setText(
            materialNames.first() + QStringLiteral(", ") + materialNames.at(1)
            + QStringLiteral(", \u2026")
        );
    }
}

bool MaterialSelector::requirePhysical() const
{
    return physicalRequired;
}

void MaterialSelector::setRequirePhysical(bool required)
{
    physicalRequired = required;
}

void MaterialSelector::chooseMaterial()
{
    std::vector<App::DocumentObject*> targets;
    QString commonUuid;
    bool hasCommonMaterial = true;

    for (const auto& reference : assignedObjects) {
        auto* object = reference.getObject();
        if (!object) {
            continue;
        }
        auto* materialProperty = freecad_cast<Materials::PropertyMaterial*>(
            object->getPropertyByName("ShapeMaterial")
        );
        if (!materialProperty || materialProperty->isReadOnly()) {
            continue;
        }

        targets.push_back(object);
        const QString uuid = materialProperty->getValue().getUUID();
        if (targets.size() == 1) {
            commonUuid = uuid;
        }
        else if (uuid != commonUuid) {
            hasCommonMaterial = false;
        }
    }

    if (targets.empty()) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Assign Material"));
    auto* layout = new QVBoxLayout(&dialog);

    Materials::MaterialFilter filter;
    filter.requirePhysical(physicalRequired);
    auto* picker = new MaterialTreeWidget(filter, &dialog);
    picker->setObjectName(QStringLiteral("materialTreeWidget"));
    picker->setExpanded(true);
    if (hasCommonMaterial && !commonUuid.isEmpty()) {
        picker->setMaterial(commonUuid);
    }
    layout->addWidget(picker);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog
    );
    auto* okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!picker->getMaterialUUID().isEmpty());
    connect(
        picker,
        &MaterialTreeWidget::materialSelected,
        okButton,
        [okButton](const std::shared_ptr<Materials::Material>& material) {
            okButton->setEnabled(static_cast<bool>(material));
        }
    );
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    try {
        const auto material = Materials::MaterialManager::getManager().getMaterial(
            picker->getMaterialUUID()
        );
        if (!material) {
            return;
        }

        std::unordered_set<App::Document*> documents;
        for (auto* target : targets) {
            if (auto* document = target->getDocument()) {
                documents.insert(document);
            }
        }
        std::vector<App::Document*> openedDocuments;
        try {
            for (auto* document : documents) {
                document->openTransaction("Assign Material");
                openedDocuments.push_back(document);
            }
            for (auto* target : targets) {
                auto* materialProperty = freecad_cast<Materials::PropertyMaterial*>(
                    target->getPropertyByName("ShapeMaterial")
                );
                if (materialProperty && !materialProperty->isReadOnly()) {
                    materialProperty->setValue(*material);
                }
            }
            for (auto* document : openedDocuments) {
                document->commitTransaction();
            }
        }
        catch (...) {
            for (auto* document : openedDocuments) {
                if (document->hasPendingTransaction()) {
                    document->abortTransaction();
                }
            }
            throw;
        }

        refresh();
        Q_EMIT materialChanged();
    }
    catch (const Base::Exception& error) {
        QMessageBox::warning(this, tr("Assign Material"), QString::fromUtf8(error.what()));
    }
    catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Assign Material"), QString::fromUtf8(error.what()));
    }
}
