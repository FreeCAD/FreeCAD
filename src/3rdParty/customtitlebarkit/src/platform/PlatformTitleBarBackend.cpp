// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PlatformTitleBarBackend.h"
#include "generic/GenericTitleBarBackend.h"
#include "customtitlebarkit/MenuIntegration.h"

#ifdef Q_OS_MACOS
#include "mac/MacGenericBackend.h"
#endif

#ifdef Q_OS_LINUX
#include "linux/LinuxTitleBarBackend.h"
#endif

bool PlatformTitleBarBackend::handleNativeEvent(const QByteArray &, void *, qintptr *)
{
    return false;
}

QWidget *PlatformTitleBarBackend::createNativeControlsWidget(QWidget *)
{
    return nullptr;
}

MenuIntegration *PlatformTitleBarBackend::createDefaultMenuIntegration(QObject *parent)
{
    return new DefaultMenuIntegration(parent);
}

std::unique_ptr<PlatformTitleBarBackend> PlatformTitleBarBackend::create()
{
    QByteArray mode = qgetenv("CUSTOMTITLEBARKIT_MODE");
    if (mode.toLower() == "generic")
        return std::make_unique<GenericTitleBarBackend>();
#ifdef Q_OS_MACOS
    if (mode.toLower() == "mac-generic")
        return std::make_unique<MacGenericBackend>();
#endif
#ifdef Q_OS_LINUX
    if (mode.toLower() == "linux")
        return std::make_unique<LinuxTitleBarBackend>();
#endif
    return createPlatform();
}
