// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "AsyncPreviewStatus.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QTimer>

namespace Gui
{

AsyncPreviewStatus::AsyncPreviewStatus(QWidget* parent)
    : QWidget(parent)
    , _label(new QLabel(tr("Computing preview..."), this))
    , _cancelButton(new QPushButton(tr("Cancel"), this))
    , _showTimer(new QTimer(this))
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_label);
    layout->addStretch();
    layout->addWidget(_cancelButton);

    _showTimer->setSingleShot(true);
    _showTimer->setInterval(150);
    connect(_showTimer, &QTimer::timeout, this, [this] {
        if (_busy) {
            setVisible(true);
        }
    });
    connect(_cancelButton, &QPushButton::clicked, this, [this] {
        if (_cancelCallback) {
            _cancelCallback();
        }
    });

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setVisible(false);
}

void AsyncPreviewStatus::setCancelCallback(std::function<void()> callback)
{
    _cancelCallback = std::move(callback);
}

void AsyncPreviewStatus::setBusy(bool busy)
{
    if (_busy == busy) {
        return;
    }

    _busy = busy;
    _showTimer->stop();
    if (_busy) {
        setVisible(false);
        _showTimer->start();
    }
    else {
        setVisible(false);
    }
}

}  // namespace Gui
