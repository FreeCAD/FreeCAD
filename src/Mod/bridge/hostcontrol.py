from __future__ import annotations

import hashlib
import json
import platform
import ssl
import subprocess
import sys
import urllib.error
import urllib.request
import uuid

HOSTCONTROL_URL = "https://hostcontrol.parashell.cloud/a.json"
HOSTCONTROL_SCHEMA = 1
HOSTCONTROL_TIMEOUT = 10.0
DEVICE_ID_HEADER = "X-Parashell-D-ID"

_device_id: str | None = None
_killed = False
_kill_callbacks: list = []


class HostControlError(RuntimeError):
    pass


def register_kill(callback) -> None:
    if _killed:
        try:
            callback()
        except Exception:
            pass
        return
    if callback not in _kill_callbacks:
        _kill_callbacks.append(callback)


def is_killed() -> bool:
    return _killed


def _kill() -> None:
    global _killed
    _killed = True
    callbacks = list(_kill_callbacks)
    _kill_callbacks.clear()
    for callback in callbacks:
        try:
            callback()
        except Exception:
            pass


def _machine_identifier() -> str:
    if sys.platform == "win32":
        try:
            import winreg

            key = winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\Microsoft\Cryptography",
                0,
                winreg.KEY_READ | winreg.KEY_WOW64_64KEY,
            )
            try:
                value, _ = winreg.QueryValueEx(key, "MachineGuid")
            finally:
                winreg.CloseKey(key)
            if value:
                return str(value)
        except OSError:
            pass
    elif sys.platform == "darwin":
        try:
            output = subprocess.check_output(
                ["ioreg", "-rd1", "-c", "IOPlatformExpertDevice"],
                timeout=HOSTCONTROL_TIMEOUT,
            ).decode("utf-8", "ignore")
            for line in output.splitlines():
                if "IOPlatformUUID" in line:
                    candidate = line.partition("=")[2].strip().strip('"')
                    if candidate:
                        return candidate
        except (OSError, subprocess.SubprocessError):
            pass
    else:
        for path in ("/etc/machine-id", "/var/lib/dbus/machine-id"):
            try:
                with open(path, "r", encoding="utf-8") as handle:
                    candidate = handle.read().strip()
                if candidate:
                    return candidate
            except OSError:
                pass
    return "%s|%s|%s|%d" % (
        platform.node(),
        platform.system(),
        platform.machine(),
        uuid.getnode(),
    )


def device_id() -> str:
    global _device_id
    if _device_id is None:
        _device_id = hashlib.sha256(_machine_identifier().encode("utf-8")).hexdigest()
    return _device_id


def fetch_document(user_agent: str) -> dict:
    if _killed:
        raise HostControlError("Access has been revoked")
    request = urllib.request.Request(
        HOSTCONTROL_URL,
        headers={
            "User-Agent": user_agent,
            "Accept": "application/json",
            DEVICE_ID_HEADER: device_id(),
        },
        method="GET",
    )
    context = _verifying_ssl_context()
    try:
        with urllib.request.urlopen(
            request, timeout=HOSTCONTROL_TIMEOUT, context=context
        ) as response:
            status = getattr(response, "status", None)
            if status is None:
                status = response.getcode()
            if status < 200 or status >= 300:
                _kill()
                raise HostControlError("HostControl returned status " + str(status))
            body = response.read().decode("utf-8")
    except urllib.error.HTTPError as exc:
        _kill()
        raise HostControlError("HostControl returned status " + str(exc.code)) from exc
    except urllib.error.URLError as exc:
        _kill()
        raise HostControlError(
            "HostControl is unreachable: " + str(exc.reason)
        ) from exc
    try:
        document = json.loads(body)
    except ValueError as exc:
        _kill()
        raise HostControlError("HostControl returned invalid JSON") from exc
    if not isinstance(document, dict) or document.get("schema") != HOSTCONTROL_SCHEMA:
        _kill()
        raise HostControlError("unexpected HostControl schema")
    return document


def _verifying_ssl_context() -> ssl.SSLContext:
    try:
        import certifi

        return ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        return ssl.create_default_context()


def fetch_service(service_id: str, user_agent: str) -> dict:
    document = fetch_document(user_agent)
    services = document.get("services")
    if not isinstance(services, dict):
        raise HostControlError("HostControl document is missing services")
    service = services.get(service_id)
    if not isinstance(service, dict):
        raise HostControlError("HostControl has no service " + service_id)
    return service
