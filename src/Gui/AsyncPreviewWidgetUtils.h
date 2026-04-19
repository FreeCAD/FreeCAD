// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <QBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QWidget>

#include "AsyncPreviewController.h"

namespace Gui
{

/**
 * @brief Common widget bundle used to present async preview state in task panels.
 *
 * The widgets are owned by the task UI. `initializeAsyncPreviewWidgets()` may
 * rearrange them inside `statusWidget`'s `QBoxLayout` to keep the progress bar
 * stable while status text changes. `updateAsyncPreviewWidgets()` then drives
 * visibility, progress mode, status text, tooltip, and cancel enablement from
 * `AsyncPreviewController` state.
 */
struct AsyncPreviewWidgetSet
{
    QWidget* statusWidget {};
    QProgressBar* progressBar {};
    QLabel* statusLabel {};
    QPushButton* cancelButton {};
};

/// Prepare the shared preview status layout and hide it until a visible recompute starts.
inline void initializeAsyncPreviewWidgets(const AsyncPreviewWidgetSet& widgets)
{
    if (widgets.statusWidget) {
        if (auto* layout = qobject_cast<QBoxLayout*>(widgets.statusWidget->layout())) {
            if (!widgets.statusWidget->property("fcAsyncPreviewLayoutInitialized").toBool()
                && widgets.progressBar && widgets.statusLabel && widgets.cancelButton) {
                layout->removeWidget(widgets.progressBar);
                layout->removeWidget(widgets.statusLabel);
                layout->removeWidget(widgets.cancelButton);
                layout->setDirection(QBoxLayout::TopToBottom);

                auto* progressRow = new QHBoxLayout();
                progressRow->setContentsMargins(0, 0, 0, 0);
                progressRow->setSpacing(layout->spacing());
                progressRow->addWidget(widgets.progressBar, 1);
                progressRow->addWidget(widgets.cancelButton);

                layout->addLayout(progressRow);
                layout->addWidget(widgets.statusLabel);
                widgets.statusWidget->setProperty("fcAsyncPreviewLayoutInitialized", true);
            }
        }
        widgets.statusWidget->hide();
    }
    if (widgets.progressBar) {
        widgets.progressBar->hide();
        widgets.progressBar->setTextVisible(false);
        widgets.progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        widgets.progressBar->setMinimumWidth(120);
    }
    if (widgets.statusLabel) {
        widgets.statusLabel->setWordWrap(false);
        widgets.statusLabel->setMinimumWidth(0);
        widgets.statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }
}

/**
 * @brief Refresh task-panel preview widgets from controller state.
 *
 * `showProgressUi` lets `AsyncPreviewSession` delay short-lived progress displays.
 * `deferredClosePending` switches the status text and disables cancel while the
 * task waits for an in-flight recompute to finish canceling.
 */
template<typename TranslateFn>
void updateAsyncPreviewWidgets(
    const AsyncPreviewController* controller,
    bool deferredClosePending,
    bool showProgressUi,
    const AsyncPreviewWidgetSet& widgets,
    TranslateFn&& translate
)
{
    if (!widgets.statusWidget || !widgets.progressBar || !widgets.statusLabel
        || !widgets.cancelButton) {
        return;
    }

    if (!controller || !controller->isInProgress() || !showProgressUi) {
        widgets.statusWidget->hide();
        widgets.progressBar->hide();
        return;
    }

    widgets.statusWidget->show();
    widgets.progressBar->show();
    if (controller->isProgressDeterminate()) {
        widgets.progressBar->setRange(0, 100);
        widgets.progressBar->setValue(controller->progressPercent());
    }
    else {
        widgets.progressBar->setRange(0, 0);
    }

    QString status = controller->progressText().empty()
        ? translate("Updating preview…")
        : QString::fromUtf8(controller->progressText().c_str());
    if (deferredClosePending) {
        status = translate("Canceling preview and closing…");
    }
    else if (controller->isCancelRequested()) {
        status = translate("Canceling preview…");
    }
    else if (controller->isQueued()) {
        status += translate(" (latest change queued)");
    }

    widgets.statusLabel->setText(status);
    widgets.statusLabel->setToolTip(status);
    widgets.cancelButton->setEnabled(!controller->isCancelRequested() && !deferredClosePending);
}

}  // namespace Gui
