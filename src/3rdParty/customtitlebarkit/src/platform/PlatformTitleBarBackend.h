// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef PLATFORMTITLEBARBACKEND_H
#define PLATFORMTITLEBARBACKEND_H

#include <QObject>
#include <QSize>
#include <memory>

class QWidget;
class MenuIntegration;

class PlatformTitleBarBackend : public QObject {
    Q_OBJECT
public:
    ~PlatformTitleBarBackend() override = default;

    /// Configure the native window for expanded client area.
    virtual void attach(QWidget *window) = 0;
    virtual void detach() = 0;

    /// Return the size of the native window controls area (e.g. traffic lights on macOS).
    virtual QSize nativeControlsAreaSize() = 0;

    /// Set the native titlebar height (controls NSToolbar style on macOS).
    virtual void setTitleBarHeight(int height) = 0;

    /// Whether the titlebar needs custom window control buttons (min/max/close).
    /// True for frameless backends, false for native titlebar backends.
    virtual bool needsWindowControls() const = 0;

    /// Minimum titlebar height for this platform (e.g. 28px for macOS native).
    virtual int minimumTitleBarHeight() const = 0;

    /// Snap a requested height to the nearest valid titlebar height for this platform.
    virtual int snapTitleBarHeight(int requested) const = 0;

    /// Backend name for QSS property selectors (e.g. "mac", "generic").
    virtual QString backendName() const = 0;

    /// Whether the backend is currently in fullscreen mode.
    virtual bool isFullscreen() const { return false; }

    /// Create a custom widget for the native controls area.
    /// Returns nullptr by default (transparent spacer is used). Caller takes ownership.
    virtual QWidget *createNativeControlsWidget(QWidget *parent);

    /// Create the default menu integration strategy for this platform.
    virtual MenuIntegration *createDefaultMenuIntegration(QObject *parent);

    static std::unique_ptr<PlatformTitleBarBackend> create();

private:
    static std::unique_ptr<PlatformTitleBarBackend> createPlatform();

Q_SIGNALS:
    void fullscreenChanged(bool fullscreen);
};

#endif // PLATFORMTITLEBARBACKEND_H
