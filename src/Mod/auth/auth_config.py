from __future__ import annotations

import os
import sys

import hostcontrol

VERSION = "0.1.0"

USER_AGENT_PREFIX = "PS-Auth/"

WOS_SERVICE_ID = "wos"
APP_SERVICE_ID = "app"

CALLBACK_TIMEOUT = 300.0

HTTP_TIMEOUT = 30.0

ACCESS_TOKEN_LEEWAY = 60
ACCESS_TOKEN_FALLBACK_TTL = 240

CONFIG_DIR_NAME = "PARA"
AUTH_FILE_NAME = "auth.json"

STATUS_OBJECT_NAME = "ParashellAuthStatusSlot"

DATA_DIR_NAME = "data"

STATUS_GENERAL = "general"
STATUS_IN_PROGRESS = "in_progress"
STATUS_SUCCESS = "success"
STATUS_FAILURE = "fail"

ICON_GENERAL = "AuthModGeneral.ico"
ICON_IN_PROGRESS = "AuthModInProgress.ico"
ICON_SUCCESS = "AuthModSuccess.ico"
ICON_FAILURE = "AuthModFail.ico"

STATUS_ICONS = {
    STATUS_IN_PROGRESS: ICON_IN_PROGRESS,
    STATUS_SUCCESS: ICON_SUCCESS,
    STATUS_FAILURE: ICON_FAILURE,
    STATUS_GENERAL: ICON_GENERAL,
}

FAVICON_PATH = "/favicon.ico"
STATUS_PATH = "/status"
STATUS_QUERY_KEY = "s"
CALLBACK_GRACE_MS = 4000


def status_icon_name(state: str) -> str:
    return STATUS_ICONS.get(state, ICON_GENERAL)


_resolved_domain: str | None = None
_resolved_app_domain: str | None = None
_resolved_client_id: str | None = None
_service_cache: dict[str, dict] = {}


def user_agent() -> str:
    return USER_AGENT_PREFIX + VERSION


def _fetch_service(service_id: str) -> dict:
    return hostcontrol.fetch_service(service_id, user_agent())


def _service(service_id: str, force: bool = False) -> dict:
    if force or service_id not in _service_cache:
        _service_cache[service_id] = _fetch_service(service_id)
    return _service_cache[service_id]


def _service_host(service_id: str, force: bool = False) -> str:
    service = _service(service_id, force)
    host = service.get("host")
    if not isinstance(host, str) or not host:
        raise ValueError("HostControl service " + service_id + " has no host")
    return host.rstrip("/")


def _service_extra(service_id: str, key: str, force: bool = False) -> str:
    service = _service(service_id, force)
    extra = service.get("extra_data")
    if not isinstance(extra, dict):
        raise ValueError("HostControl service " + service_id + " has no extra_data")
    value = extra.get(key)
    if not isinstance(value, str) or not value:
        raise ValueError(
            "HostControl service " + service_id + " extra_data has no " + key
        )
    return value


def authkit_domain(force: bool = False) -> str:
    global _resolved_domain
    if _resolved_domain and not force:
        return _resolved_domain
    _resolved_domain = _service_host(WOS_SERVICE_ID, force)
    return _resolved_domain


def client_id(force: bool = False) -> str:
    global _resolved_client_id
    if _resolved_client_id and not force:
        return _resolved_client_id
    _resolved_client_id = _service_extra(WOS_SERVICE_ID, "client_id", force)
    return _resolved_client_id


def app_domain(force: bool = False) -> str:
    global _resolved_app_domain
    if _resolved_app_domain and not force:
        return _resolved_app_domain
    _resolved_app_domain = _service_host(APP_SERVICE_ID, force)
    return _resolved_app_domain


def _is_loopback_host(host: str) -> bool:
    name = host.split("/", 1)[0].rsplit(":", 1)[0].strip().strip("[]").lower()
    if name == "localhost":
        return True
    try:
        import ipaddress

        return ipaddress.ip_address(name).is_loopback
    except ValueError:
        return False


def app_base_url() -> str:
    domain = app_domain()
    if domain.startswith("http://") or domain.startswith("https://"):
        return domain.rstrip("/")
    scheme = "http://" if _is_loopback_host(domain) else "https://"
    return scheme + domain


def mecontrol_url() -> str:
    return app_base_url() + _service_extra(APP_SERVICE_ID, "mecontrol_path")


def app_setup_url() -> str:
    return app_base_url() + _service_extra(APP_SERVICE_ID, "setup_path")


def api_base() -> str:
    return _service_extra(WOS_SERVICE_ID, "api_base").rstrip("/")


def authorize_url() -> str:
    return api_base() + _service_extra(WOS_SERVICE_ID, "authorize_path")


def authenticate_url() -> str:
    return api_base() + _service_extra(WOS_SERVICE_ID, "authenticate_path")


def loopback_host() -> str:
    return _service_extra(WOS_SERVICE_ID, "loopback_host")


def callback_path() -> str:
    return _service_extra(WOS_SERVICE_ID, "callback_path")


def config_dir() -> str:
    if sys.platform == "darwin":
        base = os.path.expanduser("~/Library/Application Support")
    elif os.name == "nt":
        base = os.environ.get("APPDATA", "").strip()
        if not base:
            base = os.path.expanduser("~\\AppData\\Roaming")
    else:
        base = os.environ.get("XDG_CONFIG_HOME", "").strip()
        if not base:
            base = os.path.expanduser("~/.config")
    return os.path.join(base, CONFIG_DIR_NAME)


def auth_file_path() -> str:
    return os.path.join(config_dir(), AUTH_FILE_NAME)


def mask_email(email: str) -> str:
    if not email or "@" not in email:
        return _mask_segment(email or "")
    local, _, domain = email.rpartition("@")
    masked_local = _mask_segment(local)
    if "." in domain:
        name, _, tld = domain.rpartition(".")
        masked_domain = _mask_segment(name) + "." + tld
    else:
        masked_domain = _mask_segment(domain)
    return masked_local + "@" + masked_domain


def _mask_segment(segment: str) -> str:
    if not segment:
        return ""
    if len(segment) == 1:
        return segment
    return segment[0] + "*" * (len(segment) - 1)
