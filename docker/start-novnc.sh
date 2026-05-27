#!/usr/bin/env bash
set -euo pipefail

export DISPLAY="${DISPLAY:-:1}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export FREECAD_USER_HOME="${FREECAD_USER_HOME:-/workspace/.freecad}"

mkdir -p "${FREECAD_USER_HOME}"

Xvfb "${DISPLAY}" -screen 0 "${VNC_RESOLUTION:-1280x800x24}" -ac +extension GLX +render -noreset &
xvfb_pid=$!

openbox &
openbox_pid=$!

x11vnc -display "${DISPLAY}" -forever -shared -nopw -listen 0.0.0.0 -rfbport 5900 &
x11vnc_pid=$!

websockify --web=/usr/share/novnc/ 0.0.0.0:6080 localhost:5900 &
websockify_pid=$!

cleanup() {
  kill "${websockify_pid}" "${x11vnc_pid}" "${openbox_pid}" "${xvfb_pid}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

if [ -x build/debug/bin/FreeCAD ]; then
  pixi run build/debug/bin/FreeCAD &
else
  xterm -geometry 120x32 -e "echo 'FreeCAD is not built yet.'; echo 'Run: docker compose run --rm configure && docker compose run --rm build'; bash" &
fi

wait "${websockify_pid}"
