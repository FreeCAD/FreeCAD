// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef APP_GUIAPPLICATIONBOOTSTRAP_H
#define APP_GUIAPPLICATIONBOOTSTRAP_H

namespace App::QtApp
{

//! Ensure a QGuiApplication instance exists for headless rendering paths.
//! Returns true if a usable QGuiApplication is available (either newly created
//! or already supplied by the caller). Returns false when a non-GUI
//! QCoreApplication is already running or Qt could not be initialised.
//! The created QGuiApplication is intentionally kept alive for the process lifetime (no teardown)
//! to avoid Qt/Python shutdown ordering crashes in headless contexts.
bool ensureGuiApplication();

//! Returns true when a QGuiApplication instance is active in the current
//! process. This is a convenience wrapper around QGuiApplication::instance().
bool hasGuiApplication();

}  // namespace App::QtApp

#endif  // APP_GUIAPPLICATIONBOOTSTRAP_H
