from __future__ import annotations

import ctypes
import hmac
import json
import os
import ssl
import sys
import tempfile
import time
import urllib.request
import xmlrpc.client
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
APP_NAME = "Parashell"
DESCRIPTOR_FILENAME = "bridge.json"
DEFAULT_CONNECT_TIMEOUT = 10.0
DEFAULT_DISCOVER_INTERVAL = 0.1
DEFAULT_REQUEST_TIMEOUT = 30.0

BRIDGE_VERSION = "0.1.2"
BRIDGE_USER_AGENT = "PS-Bridge/" + BRIDGE_VERSION

HOSTCONTROL_URL = "https://hostcontrol.parashell.cloud/a.json"
HOSTCONTROL_SCHEMA = 1
HOSTCONTROL_TIMEOUT = 10.0
CORTEX_SERVICE_ID = "ctx"


class DiscoveryError(RuntimeError):
    pass


class BridgeNotFound(DiscoveryError):
    pass


class BridgeUnavailable(DiscoveryError):
    pass


def runtime_dir() -> Path:
    if sys.platform == "win32":
        base = os.environ.get("LOCALAPPDATA") or os.environ.get("APPDATA")
        root = Path(base) if base else Path.home()
        return root / APP_NAME / "bridge"
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Application Support" / APP_NAME / "bridge"
    xdg_runtime = os.environ.get("XDG_RUNTIME_DIR")
    if xdg_runtime and os.path.isdir(xdg_runtime):
        return Path(xdg_runtime) / APP_NAME.lower() / "bridge"
    xdg_state = os.environ.get("XDG_STATE_HOME")
    root = Path(xdg_state) if xdg_state else Path.home() / ".local" / "state"
    return root / APP_NAME.lower() / "bridge"


def descriptor_path() -> Path:
    return runtime_dir() / DESCRIPTOR_FILENAME


def _ensure_runtime_dir() -> Path:
    directory = runtime_dir()
    directory.mkdir(parents=True, exist_ok=True)
    if os.name == "posix":
        try:
            os.chmod(directory, 0o700)
        except OSError:
            pass
    return directory


def publish(
    host: str,
    port: int,
    token: str,
    pid: int | None = None,
    extra: dict[str, Any] | None = None,
) -> dict[str, Any]:
    directory = _ensure_runtime_dir()
    descriptor: dict[str, Any] = {
        "schema": SCHEMA_VERSION,
        "app": APP_NAME,
        "transport": "xmlrpc",
        "scheme": "http",
        "host": host,
        "port": int(port),
        "token": token,
        "pid": int(pid if pid is not None else os.getpid()),
        "created": time.time(),
    }
    if extra:
        descriptor.update(extra)

    payload = json.dumps(descriptor, separators=(",", ":")).encode("utf-8")
    target = directory / DESCRIPTOR_FILENAME

    fd, tmp_name = tempfile.mkstemp(
        prefix=".bridge-", suffix=".json", dir=str(directory)
    )
    try:
        if os.name == "posix":
            os.fchmod(fd, 0o600)
        with os.fdopen(fd, "wb") as handle:
            handle.write(payload)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(tmp_name, target)
    except BaseException:
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise

    if os.name == "posix":
        try:
            os.chmod(target, 0o600)
        except OSError:
            pass
    return descriptor


def clear() -> None:
    try:
        os.unlink(descriptor_path())
    except (FileNotFoundError, OSError):
        pass


def _pid_alive(pid: int) -> bool:
    if pid <= 0:
        return False
    if sys.platform == "win32":
        process_query_limited_information = 0x1000
        still_active = 259
        kernel32 = ctypes.windll.kernel32
        handle = kernel32.OpenProcess(process_query_limited_information, False, pid)
        if not handle:
            return False
        try:
            code = ctypes.c_ulong()
            if not kernel32.GetExitCodeProcess(handle, ctypes.byref(code)):
                return False
            return code.value == still_active
        finally:
            kernel32.CloseHandle(handle)
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def load(verify_pid: bool = True) -> dict[str, Any] | None:
    try:
        raw = descriptor_path().read_text(encoding="utf-8")
    except (FileNotFoundError, OSError):
        return None

    try:
        descriptor = json.loads(raw)
    except (ValueError, TypeError):
        return None

    if not isinstance(descriptor, dict):
        return None
    if descriptor.get("schema") != SCHEMA_VERSION:
        return None
    if not all(key in descriptor for key in ("host", "port", "token")):
        return None
    if not isinstance(descriptor["host"], str) or not isinstance(
        descriptor["token"], str
    ):
        return None
    try:
        descriptor["port"] = int(descriptor["port"])
    except (ValueError, TypeError):
        return None

    if verify_pid:
        pid = descriptor.get("pid")
        if isinstance(pid, int) and pid > 0 and not _pid_alive(pid):
            return None

    return descriptor


def discover(
    timeout: float = DEFAULT_CONNECT_TIMEOUT,
    interval: float = DEFAULT_DISCOVER_INTERVAL,
    verify_pid: bool = True,
) -> dict[str, Any] | None:
    deadline = time.monotonic() + max(timeout, 0.0)
    while True:
        descriptor = load(verify_pid=verify_pid)
        if descriptor is not None:
            return descriptor
        if time.monotonic() >= deadline:
            return None
        time.sleep(interval)


class AuthenticatedTransport(xmlrpc.client.Transport):
    def __init__(
        self,
        token: str,
        timeout: float | None = DEFAULT_REQUEST_TIMEOUT,
        use_builtin_types: bool = False,
    ) -> None:
        super().__init__(use_builtin_types=use_builtin_types)
        self._auth_token = token
        self._timeout = timeout

    def make_connection(self, host):
        connection = super().make_connection(host)
        if self._timeout is not None:
            connection.timeout = self._timeout
        return connection

    def get_host_info(self, host):
        host, extra_headers, x509 = super().get_host_info(host)
        headers = list(extra_headers or [])
        headers.append(("Authorization", "Bearer " + self._auth_token))
        return host, headers, x509


def make_server_proxy(
    host: str,
    port: int,
    token: str,
    allow_none: bool = True,
    timeout: float | None = DEFAULT_REQUEST_TIMEOUT,
) -> xmlrpc.client.ServerProxy:
    transport = AuthenticatedTransport(token, timeout=timeout)
    return xmlrpc.client.ServerProxy(
        f"http://{host}:{int(port)}", transport=transport, allow_none=allow_none
    )


def connect(
    timeout: float = DEFAULT_CONNECT_TIMEOUT,
    ping: bool = True,
) -> xmlrpc.client.ServerProxy:
    descriptor = discover(timeout=timeout)
    if descriptor is None:
        raise BridgeNotFound(
            "No Parashell bridge descriptor found. Make sure Parashell is running."
        )
    proxy = make_server_proxy(
        descriptor["host"], descriptor["port"], descriptor["token"]
    )
    if ping:
        try:
            proxy.ping()
        except Exception as exc:
            raise BridgeUnavailable(
                f"Found a Parashell bridge descriptor but could not reach the server: {exc}"
            ) from exc
    return proxy


def hostcontrol_user_agent() -> str:
    return BRIDGE_USER_AGENT


def verifying_ssl_context() -> ssl.SSLContext:
    try:
        import certifi

        return ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        return ssl.create_default_context()


def _is_loopback_host(hostname: str) -> bool:
    name = hostname.split("/", 1)[0].rsplit(":", 1)[0].strip().strip("[]").lower()
    return name in ("localhost", "::1") or name.startswith("127.")


def _service_url(host: str) -> str:
    value = host.rstrip("/")
    if value.startswith("http://"):
        return value
    if value.startswith("https://"):
        remainder = value[len("https://") :]
        if _is_loopback_host(remainder):
            return "http://" + remainder
        return value
    scheme = "http://" if _is_loopback_host(value) else "https://"
    return scheme + value


def _fetch_hostcontrol_host(service_id: str) -> str:
    request = urllib.request.Request(
        HOSTCONTROL_URL,
        headers={"User-Agent": BRIDGE_USER_AGENT, "Accept": "application/json"},
        method="GET",
    )
    context = verifying_ssl_context()
    with urllib.request.urlopen(
        request, timeout=HOSTCONTROL_TIMEOUT, context=context
    ) as response:
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


def discover_cortex_url() -> str:
    return _service_url(_fetch_hostcontrol_host(CORTEX_SERVICE_ID))
