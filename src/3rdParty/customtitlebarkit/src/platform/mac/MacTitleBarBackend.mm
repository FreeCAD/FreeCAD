// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#import "MacTitleBarBackend.h"
#import "platform/generic/GenericTitleBarBackend.h"
#include "customtitlebarkit/CustomTitleBarWindow.h"
#include "customtitlebarkit/MenuIntegration.h"

#import <AppKit/AppKit.h>
#include <QMenuBar>
#include <QWidget>
#include <QWindow>

struct MacTitleBarBackend::Impl {
    NSWindow *nsWindow = nil;
    NSToolbar *toolbar = nil;
    bool fullscreen = false;
    int preFullscreenHeight = 28;

    id willEnterObserver = nil;
    id willExitObserver = nil;
    id didExitObserver = nil;
};

static void RemoveObserver(id __strong &token) {
    if (token) {
        [[NSNotificationCenter defaultCenter] removeObserver:token];
        token = nil;
    }
}

MacTitleBarBackend::MacTitleBarBackend()
    : d(std::make_unique<Impl>())
{
}

MacTitleBarBackend::~MacTitleBarBackend()
{
    detach();
}

void MacTitleBarBackend::attach(QWidget *window)
{
    if (!window) return;

    // Set Qt 6.10 native titlebar flags
    window->setWindowFlags(window->windowFlags()
        | Qt::ExpandedClientAreaHint
        | Qt::NoTitleBarBackgroundHint);
    window->setAttribute(Qt::WA_LayoutOnEntireRect);

    // Get the native NSWindow
    NSView *nsView = (__bridge NSView *)(reinterpret_cast<void *>(window->winId()));
    d->nsWindow = [nsView window];
    if (!d->nsWindow) return;

    d->nsWindow.titlebarAppearsTransparent = YES;
    d->nsWindow.titleVisibility = NSWindowTitleHidden;

    // Remove old observers if re-attaching
    RemoveObserver(d->willEnterObserver);
    RemoveObserver(d->willExitObserver);
    RemoveObserver(d->didExitObserver);

    MacTitleBarBackend *weakSelf = this;

    d->willEnterObserver =
        [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowWillEnterFullScreenNotification
                                                          object:d->nsWindow
                                                           queue:nil
                                                      usingBlock:^(NSNotification *) {
        if (auto self = weakSelf) {
            self->d->fullscreen = true;
            // Remove NSToolbar — macOS removes it during fullscreen anyway
            if (self->d->toolbar) {
                [self->d->nsWindow setToolbar:nil];
                self->d->toolbar = nil;
            }
            Q_EMIT self->fullscreenChanged(true);
        }
    }];

    d->willExitObserver =
        [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowWillExitFullScreenNotification
                                                          object:d->nsWindow
                                                           queue:nil
                                                      usingBlock:^(NSNotification *) {
        if (auto self = weakSelf) {
            // Hide native controls during exit transition — toolbar isn't
            // restored yet so they'd appear at the wrong position.
            self->setNativeControlsHidden(true);
        }
    }];

    d->didExitObserver =
        [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidExitFullScreenNotification
                                                         object:d->nsWindow
                                                          queue:nil
                                                     usingBlock:^(NSNotification *) {
        if (auto self = weakSelf) {
            self->d->fullscreen = false;
            // Restore NSToolbar to pre-fullscreen height
            self->setTitleBarHeight(self->d->preFullscreenHeight);
            self->setNativeControlsHidden(false);
            Q_EMIT self->fullscreenChanged(false);
        }
    }];
}

void MacTitleBarBackend::detach()
{
    if (d->nsWindow && d->toolbar) {
        [d->nsWindow setToolbar:nil];
    }
    d->toolbar = nil;

    RemoveObserver(d->willEnterObserver);
    RemoveObserver(d->willExitObserver);
    RemoveObserver(d->didExitObserver);

    d->nsWindow = nil;
}

QSize MacTitleBarBackend::nativeControlsAreaSize()
{
    if (!d->nsWindow) return {0, 0};

    NSButton *closeButton = [d->nsWindow standardWindowButton:NSWindowCloseButton];
    NSButton *zoomButton = [d->nsWindow standardWindowButton:NSWindowZoomButton];
    if (!closeButton || !zoomButton) return {0, 0};

    // Width: from left edge of close button to right edge of zoom button, plus padding
    CGFloat closeX = closeButton.frame.origin.x;
    CGFloat zoomRight = zoomButton.frame.origin.x + zoomButton.frame.size.width;
    CGFloat buttonH = closeButton.frame.size.height;

    int width = static_cast<int>(zoomRight + closeX); // symmetric padding
    int height = static_cast<int>(buttonH);

    return {width, height};
}

void MacTitleBarBackend::setTitleBarHeight(int height)
{
    if (!d->nsWindow) return;

    if (d->fullscreen) {
        // Remember for restoration after fullscreen
        d->preFullscreenHeight = height;
        return;
    }

    d->preFullscreenHeight = height;

    if (height <= 28) {
        // Standard titlebar height — no toolbar needed
        if (d->toolbar) {
            [d->nsWindow setToolbar:nil];
            d->toolbar = nil;
        }
    } else {
        // Add empty NSToolbar to increase titlebar height
        if (!d->toolbar) {
            d->toolbar = [[NSToolbar alloc] initWithIdentifier:@"customtitlebarkit.titlebar"];
            d->toolbar.showsBaselineSeparator = NO;
            [d->nsWindow setToolbar:d->toolbar];
        }

        if (height >= 46) {
            d->nsWindow.toolbarStyle = NSWindowToolbarStyleUnified;
        } else {
            d->nsWindow.toolbarStyle = NSWindowToolbarStyleUnifiedCompact;
        }
    }
}

bool MacTitleBarBackend::isFullscreen() const
{
    return d->fullscreen;
}

bool MacTitleBarBackend::needsWindowControls() const
{
    return false;
}

QString MacTitleBarBackend::backendName() const
{
    return QStringLiteral("mac");
}

int MacTitleBarBackend::minimumTitleBarHeight() const
{
    return 28;
}

int MacTitleBarBackend::snapTitleBarHeight(int requested) const
{
    if (requested <= 28) return 28;
    if (requested <= 38) return 38;
    if (requested <= 46) return 46;
    return requested;
}

void MacTitleBarBackend::setNativeControlsHidden(bool hidden)
{
    if (!d->nsWindow) return;
    for (NSWindowButton btn : {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton}) {
        NSButton *button = [d->nsWindow standardWindowButton:btn];
        [button setHidden:hidden];
    }
}

/// macOS-specific integration: uses the native global menu bar.
class NativeMenuIntegration : public MenuIntegration {
public:
    using MenuIntegration::MenuIntegration;

    void install(QMenuBar *menuBar, CustomTitleBarWindow *window) override {
        menuBar->setParent(window);
        // Remove actions BEFORE flipping to native — they belong to the old
        // (non-native) platform menu bar.  Removing after setNativeMenuBar(true)
        // triggers "does not belong to QCocoaMenuBar" warnings.
        auto actions = menuBar->actions();
        for (auto *a : actions) menuBar->removeAction(a);
        menuBar->setNativeMenuBar(true);
        // Re-add actions so Qt creates fresh NSMenu items in the new QCocoaMenuBar.
        for (auto *a : actions) menuBar->addAction(a);
    }

    void uninstall(CustomTitleBarWindow *) override {
        // Native menu bar is managed by macOS — nothing to clean up.
    }
};

MenuIntegration *MacTitleBarBackend::createDefaultMenuIntegration(QObject *parent)
{
    return new NativeMenuIntegration(parent);
}

std::unique_ptr<PlatformTitleBarBackend> PlatformTitleBarBackend::createPlatform()
{
    return std::make_unique<MacTitleBarBackend>();
}
