from __future__ import annotations

import os

VERSION = "0.1.2"

USER_AGENT_PREFIX = "PS-ELoader/"

DEFAULT_AGENT_URL = "http://localhost:3000"

ENV_AGENT_URL = "PARASHELL_AGENT_URL"
ENV_BRIDGE_DIR = "PARASHELL_BRIDGE_DIR"

MIN_SIDEBAR_WIDTH = 320
DEFAULT_SIDEBAR_WIDTH = 460

DOCK_OBJECT_NAME = "ParashellAgentDock"
TOOLBAR_OBJECT_NAME = "ParashellAgentToolbar"


def user_agent() -> str:
    return USER_AGENT_PREFIX + VERSION


def agent_url() -> str:
    value = os.environ.get(ENV_AGENT_URL, "").strip()
    if value:
        return value
    return DEFAULT_AGENT_URL
