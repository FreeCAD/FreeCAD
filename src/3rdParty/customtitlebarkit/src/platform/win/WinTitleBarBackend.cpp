// SPDX-License-Identifier: LGPL-2.1-or-later

#include "WinTitleBarBackend.h"
#include "platform/generic/GenericTitleBarBackend.h"

void WinTitleBarBackend::attach(QWidget *) {}
void WinTitleBarBackend::detach() {}
QSize WinTitleBarBackend::nativeControlsAreaSize() { return {0, 0}; }
void WinTitleBarBackend::setTitleBarHeight(int) {}
bool WinTitleBarBackend::needsWindowControls() const { return true; }
QString WinTitleBarBackend::backendName() const { return QStringLiteral("win"); }
int WinTitleBarBackend::minimumTitleBarHeight() const { return 0; }
int WinTitleBarBackend::snapTitleBarHeight(int requested) const { return requested; }

std::unique_ptr<PlatformTitleBarBackend> PlatformTitleBarBackend::createPlatform()
{
    return std::make_unique<GenericTitleBarBackend>();
}
