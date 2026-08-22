// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QWidget>

#include <FCGlobal.h>

#include <functional>

class QLabel;
class QPushButton;
class QTimer;

namespace Gui
{

/**
 * @brief Small delayed status row for an asynchronous preview.
 *
 * The row stays hidden for a short period so quick previews do not flicker.
 * It deliberately has no progress model; the Cancel button only requests
 * cooperative cancellation from the owning task.
 */
class GuiExport AsyncPreviewStatus: public QWidget
{
public:
    explicit AsyncPreviewStatus(QWidget* parent = nullptr);

    void setCancelCallback(std::function<void()> callback);
    void setBusy(bool busy);

private:
    QLabel* _label {nullptr};
    QPushButton* _cancelButton {nullptr};
    QTimer* _showTimer {nullptr};
    std::function<void()> _cancelCallback;
    bool _busy {false};
};

}  // namespace Gui
