# SPDX-License-Identifier: LGPL-2.1-or-later

"""Static configuration for the cad-x assistant.

The assistant talks to a local Ollama server through its OpenAI-compatible
chat API.  All URLs and defaults live here so the rest of the module never
hard-codes endpoints.
"""

from __future__ import annotations

from dataclasses import dataclass


OLLAMA_BASE_URL = "http://localhost:11434"
CHAT_PATH = "/v1/chat/completions"
MODELS_PATH = "/api/tags"

# Empty by design: when no preference is set, the client selects the first
# model reported by the local Ollama server. No model family is hard-coded.

DEFAULT_MODEL = ""

SYSTEM_PROMPT = (
    "You are the cad-x assistant embedded in FreeCAD. Answer clearly and "
    "concisely. For Assembly context, use the graph snapshot and query tools "
    "with exact returned graph identities and revisions. Refresh a stale "
    "graph before relying on dependent facts."
)

# Seconds before a stalled chat stream is abandoned.

REQUEST_TIMEOUT_SECONDS = 600.0
MODEL_LIST_TIMEOUT_SECONDS = 5.0

PREFERENCES_GROUP = "User parameter:BaseApp/Preferences/CadX"
MODEL_PREFERENCE_KEY = "Model"


@dataclass(frozen=True)
class CadXConfig:
    """Immutable snapshot of everything remote calls need."""

    base_url: str = OLLAMA_BASE_URL
    chat_path: str = CHAT_PATH
    models_path: str = MODELS_PATH
    model: str = DEFAULT_MODEL
    system_prompt: str = SYSTEM_PROMPT


def configured_model() -> str:
    """Return the model selected in Preferences, or ``""`` when unset.

    An empty result lets the client fall back to the first model the local
    server reports.  FreeCAD is imported lazily so unit tests can exercise
    every other module without a running application.
    """

    try:
        import FreeCAD as App
    except ImportError:
        return ""
    return App.ParamGet(PREFERENCES_GROUP).GetString(MODEL_PREFERENCE_KEY, "").strip()


def set_configured_model(model: str) -> None:
    """Persist the selected local Ollama model when FreeCAD is available."""

    try:
        import FreeCAD as App
    except ImportError:
        return
    App.ParamGet(PREFERENCES_GROUP).SetString(
        MODEL_PREFERENCE_KEY, str(model or "").strip()
    )
