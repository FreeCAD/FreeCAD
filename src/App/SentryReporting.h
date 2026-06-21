// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Parashell                                           *
 *                                                                          *
 *   This file is part of Parashell.                                        *
 *                                                                          *
 *   Parashell is free software: you can redistribute it and/or modify it   *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   Parashell is distributed in the hope that it will be useful, but       *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Parashell. If not, see                              *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef APP_SENTRYREPORTING_H
#define APP_SENTRYREPORTING_H

#include <FCGlobal.h>

namespace App
{

/**
 * Owns the lifecycle of the Sentry Native SDK and the bridge between the
 * Parashell console logging system and Sentry.
 *
 * The reporter installs a console observer that forwards errors and critical
 * messages to Sentry as events (carrying full device context) and warnings as
 * breadcrumbs. Reporting is gated behind explicit user consent that defaults to
 * enabled and is persisted in the user preferences.
 */
class AppExport SentryReporting
{
public:
    static SentryReporting& instance();

    void initialize();
    void shutdown();

    bool isConsentGiven() const;
    void setConsentGiven(bool enabled);

    bool hasAskedForConsent() const;
    void markConsentAsked();

    bool isAvailable() const;

    static const char* consentParameterPath();
    static const char* consentEntryName();
    static const char* consentAskedEntryName();

    SentryReporting(const SentryReporting&) = delete;
    SentryReporting(SentryReporting&&) = delete;
    SentryReporting& operator=(const SentryReporting&) = delete;
    SentryReporting& operator=(SentryReporting&&) = delete;

private:
    SentryReporting();
    ~SentryReporting();

    void startBackend();
    void stopBackend();
    void applyDeviceContext();
    void applyConsentToBackend();

    class LogSink;

    LogSink* logSink {nullptr};
    bool backendRunning {false};
    bool consentGiven {true};
};

}  // namespace App

#endif  // APP_SENTRYREPORTING_H
