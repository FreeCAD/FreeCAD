// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <QBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "AsyncPreviewController.h"

namespace Gui
{

class AsyncPreviewBusySpinner: public QWidget
{
public:
    explicit AsyncPreviewBusySpinner(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("fcAsyncPreviewSpinner"));
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(&timer, &QTimer::timeout, this, [this]() {
            phase = (phase + 1) % SegmentCount;
            update();
        });
        timer.setInterval(80);
    }

    QSize sizeHint() const override
    {
        return {16, 16};
    }

    QSize minimumSizeHint() const override
    {
        return sizeHint();
    }

    void setAnimated(bool animatedNow)
    {
        animated = animatedNow;
        if (animated && isVisible()) {
            if (!timer.isActive()) {
                timer.start();
            }
        }
        else {
            timer.stop();
        }
        update();
    }

protected:
    void showEvent(QShowEvent* event) override
    {
        if (animated && !timer.isActive()) {
            timer.start();
        }
        QWidget::showEvent(event);
    }

    void hideEvent(QHideEvent* event) override
    {
        timer.stop();
        QWidget::hideEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.translate(rect().center());

        const QColor baseColor = palette().color(QPalette::WindowText);
        const qreal outerRadius = std::min(width(), height()) / 2.0 - 1.0;
        const qreal innerRadius = outerRadius * 0.45;
        const qreal spokeWidth = std::max<qreal>(1.6, outerRadius * 0.18);
        const qreal spokeLength = outerRadius - innerRadius;

        for (int index = 0; index < SegmentCount; ++index) {
            painter.save();
            painter.rotate((360.0 / SegmentCount) * index);
            QColor color = baseColor;
            const int distance = (index - phase + SegmentCount) % SegmentCount;
            const qreal opacity = 0.18
                + (static_cast<qreal>(SegmentCount - distance) / SegmentCount) * 0.72;
            color.setAlphaF(std::clamp(opacity, 0.0, 1.0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRoundedRect(
                QRectF(-spokeWidth / 2.0, -outerRadius, spokeWidth, spokeLength),
                spokeWidth / 2.0,
                spokeWidth / 2.0
            );
            painter.restore();
        }
    }

private:
    static constexpr int SegmentCount = 12;

    QTimer timer;
    int phase = 0;
    bool animated = false;
};

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
    const auto retainSizeWhenHidden = [](QWidget* widget) {
        if (!widget) {
            return;
        }

        QSizePolicy policy = widget->sizePolicy();
        policy.setRetainSizeWhenHidden(true);
        widget->setSizePolicy(policy);
    };

    if (widgets.statusWidget) {
        if (auto* layout = qobject_cast<QBoxLayout*>(widgets.statusWidget->layout())) {
            if (!widgets.statusWidget->property("fcAsyncPreviewLayoutInitialized").toBool()
                && widgets.progressBar && widgets.statusLabel && widgets.cancelButton) {
                auto* spinner = new AsyncPreviewBusySpinner(widgets.statusWidget);
                layout->removeWidget(widgets.progressBar);
                layout->insertWidget(0, spinner);
                widgets.statusWidget->setProperty("fcAsyncPreviewLayoutInitialized", true);
            }
        }
        retainSizeWhenHidden(widgets.statusWidget);
        widgets.statusWidget->hide();
    }
    if (widgets.progressBar) {
        widgets.progressBar->hide();
        widgets.progressBar->setTextVisible(false);
        widgets.progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        widgets.progressBar->setMinimumWidth(120);
        retainSizeWhenHidden(widgets.progressBar);
    }
    if (widgets.statusLabel) {
        widgets.statusLabel->setWordWrap(false);
        widgets.statusLabel->setMinimumWidth(0);
        widgets.statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        retainSizeWhenHidden(widgets.statusLabel);
    }
    if (widgets.cancelButton) {
        retainSizeWhenHidden(widgets.cancelButton);
    }
    if (widgets.statusWidget) {
        if (auto* spinner
            = widgets.statusWidget->findChild<QWidget*>(QStringLiteral("fcAsyncPreviewSpinner"))) {
            retainSizeWhenHidden(spinner);
        }
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

    auto* spinner = static_cast<AsyncPreviewBusySpinner*>(
        widgets.statusWidget->findChild<QWidget*>(QStringLiteral("fcAsyncPreviewSpinner"))
    );

    if (!controller || !controller->isInProgress()) {
        if (spinner) {
            spinner->setAnimated(false);
            spinner->hide();
        }
        widgets.statusWidget->hide();
        widgets.progressBar->hide();
        widgets.statusLabel->hide();
        return;
    }

    widgets.cancelButton->setEnabled(!controller->isCancelRequested() && !deferredClosePending);

    if (!showProgressUi) {
        widgets.statusWidget->show();
        if (spinner) {
            spinner->setAnimated(false);
            spinner->hide();
        }
        widgets.progressBar->hide();
        widgets.statusLabel->hide();
        return;
    }

    widgets.statusWidget->show();
    if (spinner) {
        spinner->show();
        spinner->setAnimated(true);
    }
    widgets.statusLabel->show();
    widgets.progressBar->hide();

    QString status = translate("Updating preview…");
    if (deferredClosePending) {
        status = translate("Canceling preview and closing…");
    }
    else if (controller->isCancelRequested()) {
        status = translate("Canceling preview…");
    }

    widgets.statusLabel->setText(status);
    widgets.statusLabel->setToolTip(status);
}

}  // namespace Gui
