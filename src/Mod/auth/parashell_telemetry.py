from __future__ import annotations

import logging
import sys
import threading

SENTRY_DSN = (
    "https://75ceee35969cf72d7329ba03b170f6ca@o4511315983532032.ingest.us.sentry.io/"
    "4511600428580864"
)

TELEMETRY_ENV_VAR = "PARASHELL_TELEMETRY"
TELEMETRY_ENVIRONMENT_VAR = "PARASHELL_TELEMETRY_ENV"

CONSENT_PARAMETER_PATH = "User parameter:BaseApp/Preferences/General"
CONSENT_ENTRY_NAME = "CrashReportingEnabled"

TRACES_SAMPLE_RATE = 1.0
MAX_BREADCRUMBS = 100

_FALSE_VALUES = frozenset({"0", "false", "no", "off"})

_lock = threading.RLock()
_initialized = False
_enabled = False
_components: set[str] = set()
_previous_excepthook = None
_previous_thread_excepthook = None


def _consent_from_environment():
    import os

    raw = os.environ.get(TELEMETRY_ENV_VAR)
    if raw is None:
        return None
    return raw.strip().lower() not in _FALSE_VALUES


def _consent_from_preferences():
    try:
        import FreeCAD

        group = FreeCAD.ParamGet(CONSENT_PARAMETER_PATH)
        return bool(group.GetBool(CONSENT_ENTRY_NAME, True))
    except Exception:
        return True


def telemetry_enabled() -> bool:
    environment_consent = _consent_from_environment()
    if environment_consent is not None:
        return environment_consent
    return _consent_from_preferences()


def _release_name() -> str:
    try:
        import FreeCAD

        version = FreeCAD.Version()
        return "parashell@%s.%s.%s" % (version[0], version[1], version[2])
    except Exception:
        return "parashell-modules@0.1.2"


def _environment_name() -> str:
    import os

    value = os.environ.get(TELEMETRY_ENVIRONMENT_VAR, "").strip()
    return value or "production"


def _apply_component_tags(sentry_sdk) -> None:
    sentry_sdk.set_tag("parashell.surface", "modules")
    sentry_sdk.set_tag("parashell.components", ",".join(sorted(_components)))


def _install_exception_hooks(sentry_sdk) -> None:
    global _previous_excepthook, _previous_thread_excepthook

    if _previous_excepthook is None:
        _previous_excepthook = sys.excepthook

        def _excepthook(exc_type, exc_value, exc_traceback):
            try:
                if not issubclass(exc_type, (KeyboardInterrupt, SystemExit)):
                    sentry_sdk.capture_exception((exc_type, exc_value, exc_traceback))
                    sentry_sdk.flush(timeout=2.0)
            except Exception:
                pass
            _previous_excepthook(exc_type, exc_value, exc_traceback)

        sys.excepthook = _excepthook

    if _previous_thread_excepthook is None and hasattr(threading, "excepthook"):
        _previous_thread_excepthook = threading.excepthook

        def _thread_excepthook(args):
            try:
                exc_type = args.exc_type
                if exc_type is not None and not issubclass(
                    exc_type, (KeyboardInterrupt, SystemExit)
                ):
                    sentry_sdk.capture_exception(
                        (exc_type, args.exc_value, args.exc_traceback)
                    )
            except Exception:
                pass
            _previous_thread_excepthook(args)

        threading.excepthook = _thread_excepthook


def install(component: str) -> bool:
    global _initialized, _enabled

    with _lock:
        _components.add(component)

        if _initialized:
            if _enabled:
                try:
                    import sentry_sdk

                    _apply_component_tags(sentry_sdk)
                    sentry_sdk.add_breadcrumb(
                        category="parashell.module",
                        message="%s initialised" % component,
                        level="info",
                    )
                except Exception:
                    pass
            return _enabled

        _initialized = True
        _enabled = telemetry_enabled()
        if not _enabled:
            return False

        try:
            import sentry_sdk
            from sentry_sdk.integrations.logging import LoggingIntegration
        except Exception:
            _enabled = False
            return False

        try:
            sentry_sdk.init(
                dsn=SENTRY_DSN,
                release=_release_name(),
                environment=_environment_name(),
                integrations=[
                    LoggingIntegration(
                        level=logging.INFO,
                        event_level=logging.ERROR,
                    )
                ],
                traces_sample_rate=TRACES_SAMPLE_RATE,
                max_breadcrumbs=MAX_BREADCRUMBS,
                attach_stacktrace=True,
                send_default_pii=False,
                auto_enabling_integrations=False,
            )
        except Exception:
            _enabled = False
            return False

        _apply_component_tags(sentry_sdk)
        _install_exception_hooks(sentry_sdk)
        sentry_sdk.add_breadcrumb(
            category="parashell.module",
            message="%s initialised" % component,
            level="info",
        )
        return True
