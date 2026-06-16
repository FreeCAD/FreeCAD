from __future__ import annotations

import json
import os
import urllib.request

VERSION = "0.1.2"

USER_AGENT_PREFIX = "PS-ELoader/"

ENV_AGENT_URL = "PARASHELL_AGENT_URL"
ENV_BRIDGE_DIR = "PARASHELL_BRIDGE_DIR"

HOSTCONTROL_URL = "https://hostcontrol.parashell.cloud/a.json"
HOSTCONTROL_SCHEMA = 1
HOSTCONTROL_TIMEOUT = 10.0
ELOADER_SERVICE_ID = "elo"

MIN_SIDEBAR_WIDTH = 320
DEFAULT_SIDEBAR_WIDTH = 460

DOCK_OBJECT_NAME = "ParashellAgentDock"
TOOLBAR_OBJECT_NAME = "ParashellAgentToolbar"

_eloader_url: str | None = None


def user_agent() -> str:
    return USER_AGENT_PREFIX + VERSION


def _service_url(host: str) -> str:
    if host.startswith("http://") or host.startswith("https://"):
        return host.rstrip("/")
    return "https://" + host


def _fetch_service_host(service_id: str) -> str:
    request = urllib.request.Request(
        HOSTCONTROL_URL,
        headers={"User-Agent": user_agent(), "Accept": "application/json"},
        method="GET",
    )
    with urllib.request.urlopen(request, timeout=HOSTCONTROL_TIMEOUT) as response:
        document = json.loads(response.read().decode("utf-8"))
    if not isinstance(document, dict) or document.get("schema") != HOSTCONTROL_SCHEMA:
        raise ValueError("unexpected HostControl schema")
    services = document.get("services")
    if not isinstance(services, dict):
        raise ValueError("HostControl document is missing services")
    service = services.get(service_id)
    if not isinstance(service, dict):
        raise ValueError("HostControl has no service " + service_id)
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
    value = os.environ.get(ENV_AGENT_URL, "").strip()
    if value:
        return value
    return discover_eloader_url()
