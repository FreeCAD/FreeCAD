// SPDX-License-Identifier: LGPL-2.1-or-later

#include "LinuxTitleBarBackend.h"
#include "platform/generic/GenericTitleBarBackend.h"

void LinuxTitleBarBackend::attach(QWidget *) {}
void LinuxTitleBarBackend::detach() {}
QSize LinuxTitleBarBackend::nativeControlsAreaSize() { return {0, 0}; }
void LinuxTitleBarBackend::setTitleBarHeight(int) {}
bool LinuxTitleBarBackend::needsWindowControls() const { return true; }
QString LinuxTitleBarBackend::backendName() const { return QStringLiteral("linux"); }
int LinuxTitleBarBackend::minimumTitleBarHeight() const { return 0; }
int LinuxTitleBarBackend::snapTitleBarHeight(int requested) const { return requested; }

std::unique_ptr<PlatformTitleBarBackend> PlatformTitleBarBackend::createPlatform()
{
    return std::make_unique<GenericTitleBarBackend>();
}
