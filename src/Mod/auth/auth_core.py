from __future__ import annotations

import base64
import hashlib
import json
import os
import secrets
import ssl
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import auth_config


class AuthError(Exception):
    pass


def _b64url(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).decode("ascii").rstrip("=")


def _b64url_decode(segment: str) -> bytes:
    padding = "=" * (-len(segment) % 4)
    return base64.urlsafe_b64decode(segment + padding)


def generate_pkce() -> tuple[str, str]:
    verifier = _b64url(secrets.token_bytes(64))
    digest = hashlib.sha256(verifier.encode("ascii")).digest()
    challenge = _b64url(digest)
    return verifier, challenge


def generate_state() -> str:
    return _b64url(secrets.token_bytes(24))


def decode_jwt_claims(token: str) -> dict:
    if not token:
        return {}
    parts = token.split(".")
    if len(parts) < 2:
        return {}
    try:
        payload = _b64url_decode(parts[1])
        claims = json.loads(payload.decode("utf-8"))
    except (ValueError, json.JSONDecodeError, UnicodeDecodeError):
        return {}
    if not isinstance(claims, dict):
        return {}
    return claims


def _verifying_ssl_context() -> ssl.SSLContext:
    try:
        import certifi

        return ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        return ssl.create_default_context()


_SUCCESS_PAGE = """<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Authentication Successful</title><link rel="preconnect" href="https://fonts.googleapis.com"><link rel="preconnect" href="https://fonts.gstatic.com" crossorigin><link href="https://fonts.googleapis.com/css2?family=Inclusive+Sans:wght@300;700&display=swap" rel="stylesheet"><script type="importmap">{"imports": {"react": "https://esm.sh/react@19.1.0","react/jsx-runtime": "https://esm.sh/react@19.1.0/jsx-runtime","react-dom/client": "https://esm.sh/react-dom@19.1.0/client","@paper-design/shaders-react": "https://esm.sh/@paper-design/shaders-react@0.0.76?external=react"}}</script><style>html,body,#root {margin: 0;width: 100%;min-height: 100%;background: #1a1a1a;font-family: "Inclusive Sans", system-ui, sans-serif;}body {min-height: 100vh;overflow: hidden;}.page {position: relative;min-height: 100vh;display: grid;place-items: center;background: #1a1a1a;overflow: hidden;}.shader {position: fixed;inset: 0;z-index: 0;opacity: 0.8;pointer-events: none;}.shader canvas {width: 100%;height: 100%;display: block;}main {position: relative;z-index: 1;text-align: center;padding: 24px;}h1 {margin: 0;color: #ffffff;font-size: clamp(44px, 7vw, 96px);font-weight: 700;line-height: 1.05;letter-spacing: -0.04em;}p {margin: 18px 0 0;color: #9a9a9a;font-size: clamp(18px, 2vw, 26px);font-weight: 300;line-height: 1.4;}footer {position: fixed;left: 0;right: 0;bottom: 28px;z-index: 1;text-align: center;color: #6f6f6f;font-size: 15px;font-weight: 300;}a {color: #8a8a8a;text-decoration: none;}a:hover {color: #ffffff;}</style></head><body><div id="root"></div><script type="module">import React, { useEffect, useState } from "react";import { createRoot } from "react-dom/client";import { Waves } from "@paper-design/shaders-react";function App() {const [size, setSize] = useState({width: window.innerWidth,height: window.innerHeight});useEffect(() => {function updateSize() {setSize({width: window.innerWidth,height: window.innerHeight});}updateSize();window.addEventListener("resize", updateSize);return () => window.removeEventListener("resize", updateSize);}, []);return React.createElement("div",{ className: "page" },React.createElement("div",{ className: "shader" },React.createElement(Waves, {width: size.width,height: size.height,colorBack: "#1a1a1a",colorFront: "#12402f",frequency: 0.44,amplitude: 0.57,spacing: 1.05,proportion: 0.75,softness: 0,shape: 2.07,scale: 4})),React.createElement("main",null,React.createElement("h1", null, "Authentication Successful!"),React.createElement("p", null, "You may close this tab and return to Parashell.")),React.createElement("footer",null,React.createElement("a", { href: "https://www.parashell.cloud/legal/privacy" }, "Privacy")," | ",React.createElement("a", { href: "https://www.parashell.cloud/legal/terms" }, "Terms")));}createRoot(document.getElementById("root")).render(React.createElement(App));</script></body></html>"""

_FAILURE_PAGE = """<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Authentication Failed</title><link rel="preconnect" href="https://fonts.googleapis.com"><link rel="preconnect" href="https://fonts.gstatic.com" crossorigin><link href="https://fonts.googleapis.com/css2?family=Inclusive+Sans:wght@300;700&display=swap" rel="stylesheet"><script type="importmap">{"imports": {"react": "https://esm.sh/react@19.1.0","react/jsx-runtime": "https://esm.sh/react@19.1.0/jsx-runtime","react-dom/client": "https://esm.sh/react-dom@19.1.0/client","@paper-design/shaders-react": "https://esm.sh/@paper-design/shaders-react@0.0.76?external=react"}}</script><style>html,body,#root {margin: 0;width: 100%;min-height: 100%;background: #1a1a1a;font-family: "Inclusive Sans", system-ui, sans-serif;}body {min-height: 100vh;overflow: hidden;}.page {position: relative;min-height: 100vh;display: grid;place-items: center;background: #1a1a1a;overflow: hidden;}.shader {position: fixed;inset: 0;z-index: 0;opacity: 0.8;pointer-events: none;}.shader canvas {width: 100%;height: 100%;display: block;}main {position: relative;z-index: 1;text-align: center;padding: 24px;}h1 {margin: 0;color: #ffffff;font-size: clamp(44px, 7vw, 96px);font-weight: 700;line-height: 1.05;letter-spacing: -0.04em;}p {margin: 18px 0 0;color: #9a9a9a;font-size: clamp(18px, 2vw, 26px);font-weight: 300;line-height: 1.4;}footer {position: fixed;left: 0;right: 0;bottom: 28px;z-index: 1;text-align: center;color: #6f6f6f;font-size: 15px;font-weight: 300;}a {color: #8a8a8a;text-decoration: none;}a:hover {color: #ffffff;}</style></head><body><div id="root"></div><script type="module">import React, { useEffect, useState } from "react";import { createRoot } from "react-dom/client";import { Waves } from "@paper-design/shaders-react";function App() {const [size, setSize] = useState({width: window.innerWidth,height: window.innerHeight});useEffect(() => {function updateSize() {setSize({width: window.innerWidth,height: window.innerHeight});}updateSize();window.addEventListener("resize", updateSize);return () => window.removeEventListener("resize", updateSize);}, []);return React.createElement("div",{ className: "page" },React.createElement("div",{ className: "shader" },React.createElement(Waves, {width: size.width,height: size.height,colorBack: "#1a1a1a",colorFront: "#a46060",frequency: 0.44,amplitude: 0.57,spacing: 1.05,proportion: 0.75,softness: 0,shape: 2.07,scale: 4})),React.createElement("main",null,React.createElement("h1", null, "Authentication Failed!"),React.createElement("p", null, "Please contact support.")),React.createElement("footer",null,React.createElement("a", { href: "https://www.parashell.cloud/legal/privacy" }, "Privacy")," | ",React.createElement("a", { href: "https://www.parashell.cloud/legal/terms" }, "Terms")));}createRoot(document.getElementById("root")).render(React.createElement(App));</script></body></html>"""

_MODULE_DIR = os.path.dirname(os.path.abspath(__file__))

_icon_bytes_cache: dict[str, bytes] = {}


def _icon_bytes(state: str) -> bytes | None:
    name = auth_config.status_icon_name(state)
    cached = _icon_bytes_cache.get(name)
    if cached is not None:
        return cached
    path = os.path.join(_MODULE_DIR, auth_config.DATA_DIR_NAME, name)
    try:
        with open(path, "rb") as handle:
            data = handle.read()
    except OSError:
        return None
    _icon_bytes_cache[name] = data
    return data


_CALLBACK_SCRIPT = (
    "<script>(function(){"
    "var link=document.getElementById('favicon');"
    "var terminal={'%s':1,'%s':1};"
    "function apply(state){"
    "link.href='%s?%s='+encodeURIComponent(state)+'&t='+Date.now();}"
    "function poll(){"
    "fetch('%s',{cache:'no-store'}).then(function(r){return r.json();})"
    ".then(function(d){var s=d&&d.state?d.state:'%s';apply(s);"
    "if(!terminal[s]){setTimeout(poll,500);}})"
    ".catch(function(){});}"
    "poll();}());</script>"
) % (
    auth_config.STATUS_SUCCESS,
    auth_config.STATUS_FAILURE,
    auth_config.FAVICON_PATH,
    auth_config.STATUS_QUERY_KEY,
    auth_config.STATUS_PATH,
    auth_config.STATUS_GENERAL,
)


def _callback_page(document: str, initial_state: str) -> bytes:
    injection = (
        '<link id="favicon" rel="icon" type="image/x-icon" href="%s?%s=%s">%s'
        % (
            auth_config.FAVICON_PATH,
            auth_config.STATUS_QUERY_KEY,
            initial_state,
            _CALLBACK_SCRIPT,
        )
    )
    document = document.replace("</head>", injection + "</head>", 1)
    return document.encode("utf-8")


class _CallbackHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urllib.parse.urlsplit(self.path)
        if parsed.path == auth_config.FAVICON_PATH:
            self._serve_favicon(parsed.query)
            return
        if parsed.path == auth_config.STATUS_PATH:
            self._serve_status()
            return
        if parsed.path != auth_config.callback_path():
            self.send_response(404)
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        params = urllib.parse.parse_qs(parsed.query)
        result = {
            "code": params.get("code", [None])[0],
            "state": params.get("state", [None])[0],
            "error": params.get("error", [None])[0],
            "error_description": params.get("error_description", [None])[0],
        }
        self.server.auth_result = result
        success = bool(result["code"]) and not result["error"]
        if success:
            initial_state = auth_config.STATUS_IN_PROGRESS
            message = _SUCCESS_PAGE
        else:
            self.server.set_flow_state(auth_config.STATUS_FAILURE)
            initial_state = auth_config.STATUS_FAILURE
            message = _FAILURE_PAGE
        body = _callback_page(message, initial_state)
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _serve_status(self):
        state = self.server.flow_state()
        body = json.dumps({"state": state}).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _serve_favicon(self, query):
        params = urllib.parse.parse_qs(query)
        state = params.get(auth_config.STATUS_QUERY_KEY, [auth_config.STATUS_GENERAL])[
            0
        ]
        body = _icon_bytes(state)
        if body is None:
            body = _icon_bytes(auth_config.STATUS_GENERAL)
        if body is None:
            self.send_response(404)
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        self.send_response(200)
        self.send_header("Content-Type", "image/x-icon")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        return


class _CallbackServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.auth_result = None
        self._flow_state = auth_config.STATUS_IN_PROGRESS
        self._state_lock = threading.Lock()

    def set_flow_state(self, state):
        with self._state_lock:
            self._flow_state = state

    def flow_state(self):
        with self._state_lock:
            return self._flow_state


class LoginFlow:
    def __init__(self):
        self._verifier, self._challenge = generate_pkce()
        self._state = generate_state()
        loopback_host = auth_config.loopback_host()
        callback_path = auth_config.callback_path()
        self._server = _CallbackServer((loopback_host, 0), _CallbackHandler)
        self._thread = threading.Thread(
            target=self._server.serve_forever,
            kwargs={"poll_interval": 0.2},
            daemon=True,
        )
        self._thread.start()
        port = self._server.server_address[1]
        self._redirect_uri = "http://%s:%d%s" % (
            loopback_host,
            port,
            callback_path,
        )

    @property
    def redirect_uri(self) -> str:
        return self._redirect_uri

    def set_state(self, state: str) -> None:
        self._server.set_flow_state(state)

    def authorize_url(self) -> str:
        query = urllib.parse.urlencode(
            {
                "client_id": auth_config.client_id(),
                "redirect_uri": self._redirect_uri,
                "response_type": "code",
                "provider": "authkit",
                "state": self._state,
                "code_challenge": self._challenge,
                "code_challenge_method": "S256",
            }
        )
        return auth_config.authorize_url() + "?" + query

    def _validate(self, result: dict) -> str:
        if result.get("error"):
            raise AuthError(
                result.get("error_description")
                or result.get("error")
                or "sign-in error"
            )
        if result.get("state") != self._state:
            raise AuthError("State mismatch in the authentication callback.")
        code = result.get("code")
        if not code:
            raise AuthError("No authorization code was returned.")
        return code

    def wait_for_code(self, timeout: float | None = None) -> str:
        deadline = time.monotonic() + (
            timeout if timeout is not None else auth_config.CALLBACK_TIMEOUT
        )
        while time.monotonic() < deadline:
            if self._server.auth_result is not None:
                break
            time.sleep(0.05)
        result = self._server.auth_result
        if result is None:
            raise AuthError("Timed out waiting for the sign-in to complete.")
        return self._validate(result)

    def poll_for_code(self) -> str | None:
        result = self._server.auth_result
        if result is None:
            return None
        return self._validate(result)

    def exchange(self, code: str) -> dict:
        response = _authenticate(
            {
                "grant_type": "authorization_code",
                "client_id": auth_config.client_id(),
                "code": code,
                "code_verifier": self._verifier,
            }
        )
        return build_session(response)

    def close(self):
        try:
            self._server.shutdown()
        except Exception:
            pass
        try:
            self._server.server_close()
        except Exception:
            pass


def _authenticate(fields: dict) -> dict:
    data = json.dumps(fields).encode("utf-8")
    request = urllib.request.Request(
        auth_config.authenticate_url(),
        data=data,
        headers={
            "User-Agent": auth_config.user_agent(),
            "Accept": "application/json",
            "Content-Type": "application/json",
        },
        method="POST",
    )
    context = _verifying_ssl_context()
    try:
        with urllib.request.urlopen(
            request, timeout=auth_config.HTTP_TIMEOUT, context=context
        ) as response:
            payload = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        raise AuthError("Authentication failed: " + _read_error(exc))
    except urllib.error.URLError as exc:
        raise AuthError("Could not reach the authentication server: %s" % exc.reason)
    if not isinstance(payload, dict) or not payload.get("access_token"):
        raise AuthError("Authentication response did not contain an access token.")
    return payload


def _read_error(exc) -> str:
    try:
        body = exc.read().decode("utf-8")
        document = json.loads(body)
        if isinstance(document, dict):
            return (
                document.get("error_description")
                or document.get("error")
                or document.get("message")
                or body
            )
        return body
    except Exception:
        return "HTTP %s" % getattr(exc, "code", "error")


def _user_from_workos(user: dict, organization_id: str) -> dict:
    first_name = user.get("first_name") or ""
    last_name = user.get("last_name") or ""
    name = (first_name + " " + last_name).strip()
    return {
        "sub": user.get("id", ""),
        "email": user.get("email", ""),
        "email_verified": bool(user.get("email_verified", False)),
        "name": name,
        "given_name": first_name,
        "family_name": last_name,
        "org_id": organization_id or "",
        "profile_picture_url": user.get("profile_picture_url") or "",
    }


def _expires_at(response: dict, access_token: str) -> int:
    now = int(time.time())
    expires_in = int(response.get("expires_in", 0) or 0)
    if expires_in > 0:
        return now + expires_in
    claims = decode_jwt_claims(access_token)
    exp = claims.get("exp")
    if isinstance(exp, (int, float)) and exp > 0:
        return int(exp)
    return now + auth_config.ACCESS_TOKEN_FALLBACK_TTL


def build_session(response: dict) -> dict:
    now = int(time.time())
    access_token = response.get("access_token", "")
    user_object = response.get("user")
    if not isinstance(user_object, dict):
        user_object = {}
    organization_id = response.get("organization_id", "") or ""
    user = _user_from_workos(user_object, organization_id)
    if not user["sub"]:
        access_claims = decode_jwt_claims(access_token)
        if access_claims.get("sub"):
            user["sub"] = access_claims["sub"]
    return {
        "version": 1,
        "authkit_domain": auth_config.authkit_domain(),
        "client_id": auth_config.client_id(),
        "access_token": access_token,
        "refresh_token": response.get("refresh_token", ""),
        "token_type": response.get("token_type", "Bearer"),
        "access_token_expires_at": _expires_at(response, access_token),
        "obtained_at": now,
        "user": user,
    }


def refresh_session(session: dict) -> dict:
    refresh_token = session.get("refresh_token")
    if not refresh_token:
        raise AuthError("No refresh token is available; sign in again.")
    response = _authenticate(
        {
            "grant_type": "refresh_token",
            "client_id": auth_config.client_id(),
            "refresh_token": refresh_token,
        }
    )
    refreshed = build_session(response)
    if not refreshed.get("refresh_token"):
        refreshed["refresh_token"] = refresh_token
    if not refreshed["user"].get("email") and session.get("user"):
        refreshed["user"] = session["user"]
    return refreshed


class TokenStore:
    def __init__(self, path: str | None = None):
        self._path = path or auth_config.auth_file_path()
        self._lock = threading.Lock()

    @property
    def path(self) -> str:
        return self._path

    def load(self) -> dict | None:
        with self._lock:
            try:
                with open(self._path, "r", encoding="utf-8") as handle:
                    data = json.load(handle)
            except (OSError, ValueError):
                return None
        if not isinstance(data, dict) or not data.get("refresh_token"):
            return None
        return data

    def save(self, session: dict) -> None:
        directory = os.path.dirname(self._path)
        with self._lock:
            os.makedirs(directory, exist_ok=True)
            try:
                os.chmod(directory, 0o700)
            except OSError:
                pass
            temp_path = self._path + ".tmp"
            flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
            descriptor = os.open(temp_path, flags, 0o600)
            try:
                with os.fdopen(descriptor, "w", encoding="utf-8") as handle:
                    json.dump(session, handle, indent=2, sort_keys=True)
                    handle.flush()
                    os.fsync(handle.fileno())
            except Exception:
                try:
                    os.unlink(temp_path)
                except OSError:
                    pass
                raise
            os.replace(temp_path, self._path)
            try:
                os.chmod(self._path, 0o600)
            except OSError:
                pass

    def clear(self) -> None:
        with self._lock:
            try:
                os.remove(self._path)
            except FileNotFoundError:
                pass
            except OSError:
                pass


_store = TokenStore()

_refresh_lock = threading.Lock()


def store() -> TokenStore:
    return _store


def current_session() -> dict | None:
    return _store.load()


def current_user() -> dict | None:
    session = _store.load()
    if session is None:
        return None
    return session.get("user")


def is_signed_in() -> bool:
    return _store.load() is not None


def sign_out() -> None:
    _store.clear()


def _token_if_valid(session: dict) -> str | None:
    if not session:
        return None
    expires_at = int(session.get("access_token_expires_at", 0) or 0)
    now = int(time.time())
    if session.get("access_token") and (
        expires_at == 0 or now < expires_at - auth_config.ACCESS_TOKEN_LEEWAY
    ):
        return session["access_token"]
    return None


def valid_access_token() -> str | None:
    session = _store.load()
    if session is None:
        return None
    token = _token_if_valid(session)
    if token:
        return token
    with _refresh_lock:
        session = _store.load()
        if session is None:
            return None
        token = _token_if_valid(session)
        if token:
            return token
        refreshed = refresh_session(session)
        _store.save(refreshed)
        return refreshed.get("access_token") or None


def fetch_mecontrol(access_token: str) -> dict:
    request = urllib.request.Request(
        auth_config.mecontrol_url(),
        headers={
            "User-Agent": auth_config.user_agent(),
            "Accept": "application/json",
            "Authorization": "Bearer " + access_token,
        },
        method="GET",
    )
    context = _verifying_ssl_context()
    with urllib.request.urlopen(
        request, timeout=auth_config.HTTP_TIMEOUT, context=context
    ) as response:
        payload = json.loads(response.read().decode("utf-8"))
    if not isinstance(payload, dict):
        return {}
    return payload


def update_mecontrol(payload: dict) -> dict | None:
    if not isinstance(payload, dict):
        return None
    session = _store.load()
    if session is None:
        return None
    session["mecontrol"] = payload
    session["mecontrol_updated_at"] = int(time.time())
    account = payload.get("account")
    if isinstance(account, dict):
        name = account.get("name")
        user = session.get("user")
        if name and isinstance(user, dict):
            user["name"] = name
    _store.save(session)
    return session
