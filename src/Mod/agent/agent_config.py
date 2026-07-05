from __future__ import annotations

import ipaddress
import urllib.parse

import hostcontrol

LOOPBACK_HOSTNAMES = {"localhost"}

VERSION = "0.1.2"

USER_AGENT_PREFIX = "PS-ELoader/"

ELOADER_SERVICE_ID = "elo"

MIN_SIDEBAR_WIDTH = 460
DEFAULT_SIDEBAR_WIDTH = 440

DOCK_OBJECT_NAME = "ParashellAgentDock"
TOOLBAR_OBJECT_NAME = "ParashellAgentToolbar"

_eloader_url: str | None = None


def user_agent() -> str:
    return USER_AGENT_PREFIX + VERSION


def _hostname(url: str) -> str:
    target = url if "://" in url else "//" + url
    return (urllib.parse.urlsplit(target).hostname or "").lower()


def is_loopback_url(url: str) -> bool:
    host = _hostname(url)
    if not host:
        return False
    if host in LOOPBACK_HOSTNAMES:
        return True
    try:
        return ipaddress.ip_address(host).is_loopback
    except ValueError:
        return False


def _service_url(host: str) -> str:
    if host.startswith("http://") or host.startswith("https://"):
        return host.rstrip("/")
    if is_loopback_url(host):
        return "http://" + host.rstrip("/")
    return "https://" + host.rstrip("/")


def _fetch_service_host(service_id: str) -> str:
    service = hostcontrol.fetch_service(service_id, user_agent())
    host = service.get("host")
    if not isinstance(host, str) or not host:
        raise ValueError("HostControl service " + service_id + " has no host")
    return host


def discover_eloader_url(force: bool = False) -> str:
    global _eloader_url
    if _eloader_url and not force:
        return _eloader_url
    _eloader_url = _service_url(_fetch_service_host(ELOADER_SERVICE_ID))
    return _eloader_url


def agent_url() -> str:
    return discover_eloader_url()
