from __future__ import annotations

import os
import sys
import threading

from PySide6 import QtCore, QtWidgets
from PySide6.QtCore import QObject, Signal, Slot
from PySide6.QtWebChannel import QWebChannel
from PySide6.QtWebEngineCore import (
    QWebEnginePage,
    QWebEngineProfile,
    QWebEngineScript,
    QWebEngineSettings,
)
from PySide6.QtWebEngineWidgets import QWebEngineView

import mcp.types as mcp_types
from mcp.shared.message import SessionMessage

import agent_config

_existing_chromium_flags = os.environ.get("QTWEBENGINE_CHROMIUM_FLAGS", "")
if "--log-level" not in _existing_chromium_flags:
    os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = (
        _existing_chromium_flags + " --log-level=3 --disable-gpu-process-crash-limit"
    ).strip()


class _QuietWebEnginePage(QWebEnginePage):
    _SUPPRESSED = (
        "ResizeObserver loop completed with undelivered notifications",
        "ResizeObserver loop limit exceeded",
    )

    def javaScriptConsoleMessage(self, level, message, line_number, source_id):
        for fragment in self._SUPPRESSED:
            if fragment in message:
                return
        super().javaScriptConsoleMessage(level, message, line_number, source_id)


def _read_qwebchannel_js() -> str:
    resource = QtCore.QFile(":/qtwebchannel/qwebchannel.js")
    mode = QtCore.QIODevice.OpenModeFlag.ReadOnly | QtCore.QIODevice.OpenModeFlag.Text
    if not resource.open(mode):
        raise RuntimeError(
            "Unable to load qwebchannel.js from the Qt resource system. "
            "QtWebChannel is not available in this environment."
        )
    try:
        data = bytes(resource.readAll().data()).decode("utf-8")
    finally:
        resource.close()
    if not data.strip():
        raise RuntimeError("qwebchannel.js resource was empty.")
    return data


def _channel_bootstrap_js() -> str:
    return """
(function () {
  function install() {
    if (typeof QWebChannel === "undefined" || !window.qt || !qt.webChannelTransport) {
      window.setTimeout(install, 25);
      return;
    }
    new QWebChannel(qt.webChannelTransport, function (channel) {
      var auth = channel.objects.auth_native;
      if (auth) {
        var lastAuth = null;
        function pollAuth() {
          auth.current(function (payload) {
            if (payload !== lastAuth) {
              lastAuth = payload;
              window.parashellAuthPayload = payload;
              window.dispatchEvent(
                new CustomEvent("parashell-auth-change", { detail: payload })
              );
            }
          });
        }
        window.parashellAuth = {
          get: function (cb) {
            auth.current(cb);
          },
          subscribe: function (cb) {
            window.addEventListener("parashell-auth-change", function (event) {
              cb(event.detail);
            });
          },
        };
        pollAuth();
        window.setInterval(pollAuth, 2000);
        window.dispatchEvent(new Event("parashell-auth-ready"));
      }
      var native = channel.objects.mcp_native;
      if (native) {
        window.mcp = {
          send: function (payload) {
            native.send(payload);
          },
          stop: function () {
            native.stop();
          },
          stdout: native.stdout,
          stderr: native.stderr,
          exited: native.exited,
          ready: native.ready,
        };
        window.dispatchEvent(new Event("mcp-ready"));
        try {
          native.notifyReady();
        } catch (err) {}
      }
    });
  }
  install();
})();
"""


def _scrollbar_hidden_css_js() -> str:
    return """
(function () {
  var STYLE_ID = "parashell-agent-hide-scrollbars";
  var CSS = [
    "html, body, *, *::before, *::after {",
    "  scrollbar-width: none !important;",
    "  -ms-overflow-style: none !important;",
    "}",
    "::-webkit-scrollbar, *::-webkit-scrollbar {",
    "  width: 0 !important;",
    "  height: 0 !important;",
    "  display: none !important;",
    "  background: transparent !important;",
    "}",
    "::-webkit-scrollbar-thumb, *::-webkit-scrollbar-thumb {",
    "  background: transparent !important;",
    "}"
  ].join("\\n");
  function ensure() {
    var root = document.documentElement;
    if (!root) {
      window.setTimeout(ensure, 25);
      return;
    }
    var style = document.getElementById(STYLE_ID);
    if (!style) {
      style = document.createElement("style");
      style.id = STYLE_ID;
      style.type = "text/css";
      style.textContent = CSS;
      root.appendChild(style);
    }
  }
  ensure();
  document.addEventListener("DOMContentLoaded", ensure);
})();
"""


class McpInProcessBridge(QObject):
    stdout = Signal(str)
    stderr = Signal(str)
    exited = Signal(int)
    ready = Signal()

    _relay_stdout = Signal(str)
    _relay_stderr = Signal(str)
    _relay_exited = Signal(int)

    def __init__(self, bridge_dir: str, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self._bridge_dir = bridge_dir
        self._relay_stdout.connect(
            self.stdout, QtCore.Qt.ConnectionType.QueuedConnection
        )
        self._relay_stderr.connect(
            self.stderr, QtCore.Qt.ConnectionType.QueuedConnection
        )
        self._relay_exited.connect(
            self.exited, QtCore.Qt.ConnectionType.QueuedConnection
        )
        self._portal = None
        self._loop = None
        self._read_writer = None
        self._cancel_scope = None
        self._pending: list[SessionMessage] = []
        self._pending_lock = threading.Lock()
        self._flushed = False
        self._stopped = False
        self._thread = threading.Thread(
            target=self._thread_main, name="mcp-bridge", daemon=True
        )
        self._thread.start()

    def _import_bridge(self):
        if self._bridge_dir not in sys.path:
            sys.path.insert(0, self._bridge_dir)
        import bridge

        return bridge

    def _thread_main(self) -> None:
        try:
            from anyio.from_thread import start_blocking_portal
        except Exception as exc:
            self._relay_stderr.emit("Parashell Agent: anyio unavailable: " + str(exc))
            self._relay_exited.emit(1)
            return

        try:
            bridge = self._import_bridge()
        except Exception as exc:
            self._relay_stderr.emit(
                "Parashell Agent: failed to import MCP bridge: " + str(exc)
            )
            self._relay_exited.emit(1)
            return

        try:
            import acs

            acs.register()
        except Exception as exc:
            self._relay_stderr.emit(
                "Parashell Agent: bridge prompt registration failed: " + str(exc)
            )

        try:
            with start_blocking_portal() as portal:
                self._portal = portal
                future = portal.start_task_soon(self._serve, bridge)
                future.result()
        except Exception as exc:
            self._relay_stderr.emit("Parashell Agent: MCP bridge stopped: " + str(exc))
        finally:
            if not self._stopped:
                self._relay_exited.emit(0)

    async def _serve(self, bridge) -> None:
        import anyio
        import asyncio

        self._loop = asyncio.get_running_loop()
        read_writer, read_reader = anyio.create_memory_object_stream(float("inf"))
        write_sender, write_reader = anyio.create_memory_object_stream(100)
        self._read_writer = read_writer
        self._flush_pending(read_writer)
        server = bridge.mcp._mcp_server
        init_options = server.create_initialization_options()

        async def pump_output() -> None:
            chunk_size = 131072
            async with write_reader:
                async for session_message in write_reader:
                    line = session_message.message.model_dump_json(
                        by_alias=True, exclude_none=True
                    )
                    payload = line + "\n"
                    if len(payload) <= chunk_size:
                        self._relay_stdout.emit(payload)
                        continue
                    for start in range(0, len(payload), chunk_size):
                        self._relay_stdout.emit(payload[start : start + chunk_size])
                        await anyio.sleep(0.005)

        with anyio.CancelScope() as cancel_scope:
            self._cancel_scope = cancel_scope
            pump_task = asyncio.ensure_future(pump_output())
            try:
                async with write_sender:
                    await server.run(read_reader, write_sender, init_options)
                await pump_task
            finally:
                if not pump_task.done():
                    pump_task.cancel()
                    with anyio.CancelScope(shield=True):
                        try:
                            await pump_task
                        except asyncio.CancelledError:
                            pass

    def _flush_pending(self, writer) -> None:
        while True:
            with self._pending_lock:
                if not self._pending:
                    self._flushed = True
                    return
                batch = self._pending
                self._pending = []
            for session_message in batch:
                try:
                    writer.send_nowait(session_message)
                except Exception as exc:
                    self._relay_stderr.emit(
                        "Parashell Agent: MCP flush failed: " + str(exc)
                    )

    def _deliver(self, session_message) -> None:
        writer = self._read_writer
        if writer is None or self._stopped:
            return
        try:
            writer.send_nowait(session_message)
        except Exception as exc:
            self._relay_stderr.emit("Parashell Agent: MCP send failed: " + str(exc))

    @Slot()
    def notifyReady(self) -> None:
        self.ready.emit()

    @Slot(str)
    def send(self, payload: str) -> None:
        if self._stopped:
            return
        try:
            message = mcp_types.JSONRPCMessage.model_validate_json(payload)
        except Exception as exc:
            self._relay_stderr.emit(
                "Parashell Agent: discarding invalid MCP payload: " + str(exc)
            )
            return
        session_message = SessionMessage(message)
        with self._pending_lock:
            if not self._flushed:
                self._pending.append(session_message)
                return
            loop = self._loop
        if loop is None or self._stopped:
            return
        try:
            loop.call_soon_threadsafe(self._deliver, session_message)
        except Exception as exc:
            self._relay_stderr.emit("Parashell Agent: MCP send failed: " + str(exc))

    @Slot()
    def stop(self) -> None:
        self.shutdown()

    def _cancel_scope_cancel(self) -> None:
        if self._cancel_scope is not None:
            self._cancel_scope.cancel()

    def shutdown(self) -> None:
        if self._stopped:
            return
        self._stopped = True
        loop = self._loop
        if loop is not None:
            try:
                loop.call_soon_threadsafe(self._cancel_scope_cancel)
            except Exception:
                pass


class AgentWidget(QtWidgets.QWidget):
    def __init__(
        self,
        agent_url: str,
        ua_version: str,
        bridge_dir: str,
        parent: QtWidgets.QWidget | None = None,
        auth_provider=None,
    ) -> None:
        super().__init__(parent)
        self.setMinimumWidth(agent_config.MIN_SIDEBAR_WIDTH)

        self._agent_url = agent_url
        self._loaded = False
        self._bridge = None
        if bridge_dir:
            self._bridge = McpInProcessBridge(os.path.abspath(bridge_dir), self)

        self._profile = QWebEngineProfile("parashell-agent", self)
        self._profile.setHttpUserAgent(agent_config.USER_AGENT_PREFIX + ua_version)

        self._channel = QWebChannel(self)

        self._page = _QuietWebEnginePage(self._profile, self)
        self._page.setWebChannel(self._channel)

        settings = self._page.settings()
        settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalStorageEnabled, True)
        settings.setAttribute(
            QWebEngineSettings.WebAttribute.ScrollAnimatorEnabled, True
        )

        hide_scrollbars = QWebEngineScript()
        hide_scrollbars.setName("parashell-hide-scrollbars")
        hide_scrollbars.setInjectionPoint(
            QWebEngineScript.InjectionPoint.DocumentCreation
        )
        hide_scrollbars.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        hide_scrollbars.setRunsOnSubFrames(True)
        hide_scrollbars.setSourceCode(_scrollbar_hidden_css_js())
        self._page.scripts().insert(hide_scrollbars)

        self._auth = None
        self._auth_start = None
        self._auth_stop = None
        if auth_provider is not None:
            try:
                import ags

                bridge, start, stop = ags.create_agent_auth_bridge(auth_provider)
                self._auth = bridge
                self._auth_start = start
                self._auth_stop = stop
            except Exception:
                self._auth = None
                self._auth_start = None
                self._auth_stop = None
        if self._auth is not None:
            self._channel.registerObject("auth_native", self._auth)

        if self._bridge is not None:
            self._channel.registerObject("mcp_native", self._bridge)

        bootstrap = QWebEngineScript()
        bootstrap.setName("parashell-channel-bridge")
        bootstrap.setInjectionPoint(QWebEngineScript.InjectionPoint.DocumentCreation)
        bootstrap.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        bootstrap.setRunsOnSubFrames(False)
        bootstrap.setSourceCode(_read_qwebchannel_js() + "\n" + _channel_bootstrap_js())
        self._page.scripts().insert(bootstrap)

        self._view = QWebEngineView(self)
        self._view.setPage(self._page)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._view)

    def load(self) -> None:
        if self._loaded:
            return
        self._loaded = True
        if self._auth_start is not None:
            self._auth_start()
        self._page.load(QtCore.QUrl(self._agent_url))

    def bridge(self) -> McpInProcessBridge | None:
        return self._bridge

    def shutdown(self) -> None:
        if self._auth_stop is not None:
            self._auth_stop()
        if self._bridge is not None:
            self._bridge.shutdown()

    def closeEvent(self, event) -> None:
        self.shutdown()
        super().closeEvent(event)
