// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef CUSTOMTITLEBARWINDOW_H
#define CUSTOMTITLEBARWINDOW_H

#include <QMainWindow>
#include <memory>

#include "customtitlebarkit/export.h"

class QMenuBar;
class TitleBarWidget;
class MenuIntegration;

class CUSTOMTITLEBARKIT_EXPORT CustomTitleBarWindow : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(QString backend READ backendName CONSTANT)
    Q_PROPERTY(int titleBarHeight READ titleBarHeight WRITE setTitleBarHeight NOTIFY titleBarHeightChanged)
    Q_PROPERTY(bool titleBarVisible READ isTitleBarVisible WRITE setTitleBarVisible NOTIFY titleBarVisibleChanged)

public:
    enum class Mode { Native, Custom };

    explicit CustomTitleBarWindow(Mode mode = Mode::Custom, QWidget *parent = nullptr);

    Mode mode() const;
    QString backendName() const;
    ~CustomTitleBarWindow() override;

    int titleBarHeight() const;
    void setTitleBarHeight(int height);

    /// Left content area of the titlebar — place toolbars, icons, labels here.
    QWidget *leftArea() const;

    /// Right content area of the titlebar — place toolbars, selectors here.
    QWidget *rightArea() const;

    /// Spacer for native window controls (managed internally, read-only for sizing info).
    QWidget *nativeControlsWidget() const;

    /// Returns the menu bar, creating one lazily if needed.
    /// On macOS (native backend), the menu bar is sent to the global menu.
    /// On other platforms, it's placed inline in the titlebar left area.
    QMenuBar *menuBar();

    /// Replace the menu bar. Takes ownership if parent is this window.
    void setMenuBar(QMenuBar *menuBar);

    /// Set a custom menu integration strategy. Takes ownership.
    /// Re-installs the current menu bar (if any) with the new strategy.
    void setMenuIntegration(MenuIntegration *integration);

    bool isTitleBarVisible() const;
    void setTitleBarVisible(bool visible);

Q_SIGNALS:
    void titleBarHeightChanged(int height);
    void titleBarVisibleChanged(bool visible);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CUSTOMTITLEBARWINDOW_H
