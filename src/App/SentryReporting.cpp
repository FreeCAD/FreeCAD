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

#include "PreCompiled.h"

#include <string>

#include <QLocale>
#include <QString>
#include <QSysInfo>
#include <QThread>
#include <QtGlobal>

#include <Base/Console.h>
#include <Base/Parameter.h>

#include "Application.h"
#include "SentryReporting.h"

#ifdef FC_HAVE_SENTRY
#include <sentry.h>
#endif

using namespace App;

namespace
{
std::string trimTrailingWhitespace(const std::string& text)
{
    std::string::size_type end = text.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) {
        return {};
    }
    return text.substr(0, end + 1);
}

#ifdef FC_HAVE_SENTRY
constexpr const char* SentryDsn =
    "https://75ceee35969cf72d7329ba03b170f6ca@o4511315983532032.ingest.us.sentry.io/"
    "4511600428580864";

std::string buildReleaseName()
{
    auto& config = Application::Config();
    std::string release = "parashell@";
    release += config["BuildVersionMajor"];
    release += '.';
    release += config["BuildVersionMinor"];
    release += '.';
    release += config["BuildVersionPoint"];
    release += config["BuildVersionSuffix"];
    return release;
}

std::string buildEnvironmentName()
{
    const std::string suffix = Application::Config()["BuildVersionSuffix"];
    if (suffix.empty()) {
        return "production";
    }
    return "development";
}
#endif
}  // namespace

class SentryReporting::LogSink: public Base::ILogger
{
public:
    explicit LogSink(SentryReporting& ownerReporting)
        : owner(ownerReporting)
    {
        bMsg = false;
        bLog = false;
        bWrn = true;
        bErr = true;
        bCritical = true;
        bNotification = false;
    }

    void sendLog(
        const std::string& notifiername,
        const std::string& msg,
        Base::LogStyle level,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        if (!owner.consentGiven || !owner.backendRunning) {
            return;
        }

        const std::string text = trimTrailingWhitespace(msg);
        if (text.empty()) {
            return;
        }

        const std::string source = notifiername.empty() ? std::string("Parashell") : notifiername;

#ifdef FC_HAVE_SENTRY
        switch (level) {
            case Base::LogStyle::Error:
            case Base::LogStyle::Critical: {
                sentry_level_t sentryLevel = level == Base::LogStyle::Critical
                    ? SENTRY_LEVEL_FATAL
                    : SENTRY_LEVEL_ERROR;
                sentry_value_t event =
                    sentry_value_new_message_event(sentryLevel, source.c_str(), text.c_str());
                sentry_value_set_by_key(
                    event,
                    "logger",
                    sentry_value_new_string(source.c_str())
                );
                sentry_capture_event(event);
                break;
            }
            case Base::LogStyle::Warning: {
                sentry_value_t crumb = sentry_value_new_breadcrumb("default", text.c_str());
                sentry_value_set_by_key(crumb, "level", sentry_value_new_string("warning"));
                sentry_value_set_by_key(crumb, "category", sentry_value_new_string(source.c_str()));
                sentry_add_breadcrumb(crumb);
                break;
            }
            default:
                break;
        }
#else
        (void)level;
        (void)source;
#endif
    }

    const char* name() override
    {
        return "Sentry";
    }

    LogSink(const LogSink&) = delete;
    LogSink(LogSink&&) = delete;
    LogSink& operator=(const LogSink&) = delete;
    LogSink& operator=(LogSink&&) = delete;
    ~LogSink() override = default;

private:
    SentryReporting& owner;
};

SentryReporting::SentryReporting() = default;

SentryReporting::~SentryReporting()
{
    shutdown();
}

SentryReporting& SentryReporting::instance()
{
    static SentryReporting singleton;
    return singleton;
}

const char* SentryReporting::consentParameterPath()
{
    return "User parameter:BaseApp/Preferences/General";
}

const char* SentryReporting::consentEntryName()
{
    return "CrashReportingEnabled";
}

const char* SentryReporting::consentAskedEntryName()
{
    return "CrashReportingConsentAsked";
}

bool SentryReporting::isAvailable() const
{
#ifdef FC_HAVE_SENTRY
    return true;
#else
    return false;
#endif
}

bool SentryReporting::isConsentGiven() const
{
    ParameterGrp::handle hGrp =
        GetApplication().GetParameterGroupByPath(consentParameterPath());
    return hGrp->GetBool(consentEntryName(), true);
}

bool SentryReporting::hasAskedForConsent() const
{
    ParameterGrp::handle hGrp =
        GetApplication().GetParameterGroupByPath(consentParameterPath());
    return hGrp->GetBool(consentAskedEntryName(), false);
}

void SentryReporting::markConsentAsked()
{
    ParameterGrp::handle hGrp =
        GetApplication().GetParameterGroupByPath(consentParameterPath());
    hGrp->SetBool(consentAskedEntryName(), true);
}

void SentryReporting::setConsentGiven(bool enabled)
{
    ParameterGrp::handle hGrp =
        GetApplication().GetParameterGroupByPath(consentParameterPath());
    hGrp->SetBool(consentEntryName(), enabled);
    consentGiven = enabled;
    applyConsentToBackend();
}

void SentryReporting::initialize()
{
    if (backendRunning) {
        return;
    }

    consentGiven = isConsentGiven();

    startBackend();

    if (backendRunning) {
        applyDeviceContext();
        applyConsentToBackend();

        logSink = new LogSink(*this);
        Base::Console().attachObserver(logSink);

        Base::Console().log("Crash and error reporting initialized\n");
    }
    else {
        Base::Console().log(
            "Crash and error reporting unavailable in this build\n"
        );
    }
}

void SentryReporting::shutdown()
{
    if (logSink) {
        Base::Console().detachObserver(logSink);
        delete logSink;
        logSink = nullptr;
    }

    stopBackend();
}

void SentryReporting::startBackend()
{
#ifdef FC_HAVE_SENTRY
    sentry_options_t* options = sentry_options_new();
    sentry_options_set_dsn(options, SentryDsn);

    std::string databasePath = Application::getUserAppDataDir();
    databasePath += ".sentry-native";
    sentry_options_set_database_path(options, databasePath.c_str());

    const std::string release = buildReleaseName();
    sentry_options_set_release(options, release.c_str());

    const std::string environment = buildEnvironmentName();
    sentry_options_set_environment(options, environment.c_str());

    sentry_options_set_debug(options, 0);
    sentry_options_set_require_user_consent(options, 1);
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_auto_session_tracking(options, 1);

    backendRunning = sentry_init(options) == 0;
#else
    backendRunning = false;
#endif
}

void SentryReporting::stopBackend()
{
#ifdef FC_HAVE_SENTRY
    if (backendRunning) {
        sentry_close();
        backendRunning = false;
    }
#else
    backendRunning = false;
#endif
}

void SentryReporting::applyConsentToBackend()
{
#ifdef FC_HAVE_SENTRY
    if (!backendRunning) {
        return;
    }
    if (consentGiven) {
        sentry_user_consent_give();
    }
    else {
        sentry_user_consent_revoke();
    }
#endif
}

void SentryReporting::applyDeviceContext()
{
#ifdef FC_HAVE_SENTRY
    if (!backendRunning) {
        return;
    }

    auto toUtf8 = [](const QString& value) {
        return value.toUtf8();
    };

    sentry_value_t osContext = sentry_value_new_object();
    sentry_value_set_by_key(
        osContext,
        "name",
        sentry_value_new_string(toUtf8(QSysInfo::productType()).constData())
    );
    sentry_value_set_by_key(
        osContext,
        "version",
        sentry_value_new_string(toUtf8(QSysInfo::productVersion()).constData())
    );
    sentry_value_set_by_key(
        osContext,
        "kernel_version",
        sentry_value_new_string(toUtf8(QSysInfo::kernelVersion()).constData())
    );
    sentry_value_set_by_key(
        osContext,
        "build",
        sentry_value_new_string(toUtf8(QSysInfo::prettyProductName()).constData())
    );
    sentry_set_context("os", osContext);

    sentry_value_t deviceContext = sentry_value_new_object();
    sentry_value_set_by_key(
        deviceContext,
        "name",
        sentry_value_new_string(toUtf8(QSysInfo::machineHostName()).constData())
    );
    sentry_value_set_by_key(
        deviceContext,
        "arch",
        sentry_value_new_string(toUtf8(QSysInfo::currentCpuArchitecture()).constData())
    );
    sentry_value_set_by_key(
        deviceContext,
        "processor_count",
        sentry_value_new_int32(QThread::idealThreadCount())
    );
    sentry_value_set_by_key(
        deviceContext,
        "machine_id",
        sentry_value_new_string(QSysInfo::machineUniqueId().constData())
    );
    sentry_set_context("device", deviceContext);

    sentry_value_t appContext = sentry_value_new_object();
    sentry_value_set_by_key(
        appContext,
        "app_name",
        sentry_value_new_string(Application::Config()["ExeName"].c_str())
    );
    sentry_value_set_by_key(
        appContext,
        "app_version",
        sentry_value_new_string(buildReleaseName().c_str())
    );
    sentry_value_set_by_key(
        appContext,
        "build_revision",
        sentry_value_new_string(Application::Config()["BuildRevision"].c_str())
    );
    sentry_set_context("app", appContext);

    sentry_set_tag("os.arch", toUtf8(QSysInfo::currentCpuArchitecture()).constData());
    sentry_set_tag("os.pretty", toUtf8(QSysInfo::prettyProductName()).constData());
    sentry_set_tag("locale", toUtf8(QLocale::system().name()).constData());
    sentry_set_tag("qt.version", qVersion());

    const std::string& revisionHash = Application::Config()["BuildRevisionHash"];
    if (!revisionHash.empty()) {
        sentry_set_tag("build.hash", revisionHash.c_str());
    }
#endif
}
