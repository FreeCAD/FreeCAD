// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef GENERICTITLEBARBACKEND_H
#define GENERICTITLEBARBACKEND_H

#include "platform/PlatformTitleBarBackend.h"

class QWidget;

/// Frameless window backend that works on all platforms.
/// Removes the native titlebar and provides resize edge handling.
class GenericTitleBarBackend : public PlatformTitleBarBackend {
    Q_OBJECT

public:
    GenericTitleBarBackend();
    ~GenericTitleBarBackend() override;

    void attach(QWidget *window) override;
    void detach() override;
    QSize nativeControlsAreaSize() override;
    void setTitleBarHeight(int height) override;
    bool needsWindowControls() const override;
    QString backendName() const override;
    int minimumTitleBarHeight() const override;
    int snapTitleBarHeight(int requested) const override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QWidget *m_window = nullptr;
    static constexpr int ResizeBorder = 5;

    Qt::Edges edgesAt(const QPoint &pos) const;
    Qt::CursorShape cursorForEdges(Qt::Edges edges) const;
};

#endif // GENERICTITLEBARBACKEND_H
