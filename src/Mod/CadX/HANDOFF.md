# HANDOFF: cad-x assistant and graph tools

Date: 2026-08-23
Branch: `freecad-mcp-tools`
State: **Ollama migration is green; native graph foundation is built and
fixture/lifecycle completion remains**

This document hands off the in-progress conversion of the cad-x assistant
from ChatGPT-subscription OAuth to local Ollama, plus the panel-reopen fix.

---

## 1. What this module is

`src/Mod/CadX/` is a hybrid FreeCAD module: a dockable chat panel talking to a
local Ollama server, plus provider-neutral Assembly graph tools backed by
`CadXApp` and `CadXGuiApp`. It is gated behind the `BUILD_CADX` CMake option
like every other module.

Runtime shape:

```
CadXConfig.py     constants: Ollama URLs, default model, system prompt,
                  FreeCAD preference key ("Model" in
                  User parameter:BaseApp/Preferences/CadX)
CadXChatClient.py OllamaClient: chat-completions SSE streaming,
                  model resolution, injectable transport/get_json seams
CadXSession.py    ChatSession: transcript, one turn at a time, worker
                  thread, listener callbacks (pure Python, no Qt)
CadXPanel.py      AssistantPanel QWidget: transcript, composer, status
                  label; bridges session events via Qt signals
CadXGui.py        singletons, dock creation (Qt-native QDockWidget),
                  View-menu entry, CadX_Assistant command
Init.py/InitGui.py FreeCAD bootstraps
cadx_tests/       stdlib unittest suite; `python3 run_all.py`
```

Event contract (unchanged by the migration):
`TurnKind.{STARTED, DELTA, COMPLETED, FAILED, CANCELLED}` as frozen
`TurnEvent` dataclasses. `stream_turn(history, on_event, should_cancel)`
never raises past its seam; failures arrive as FAILED events.

## 2. Changes made in this migration (vs the ChatGPT build)

Deleted:
- `CadXOpenAiAuth.py` — in-process OAuth (PKCE, loopback callback server,
  token exchange/refresh, JWT account-id extraction)
- `CadXTokens.py` — `StoredTokens`, `TokenStore` ABC, `FileTokenStore`
- `cadx_tests/test_cadx_auth.py`, `cadx_tests/test_cadx_tokens.py`

Rewritten:
- `CadXConfig.py` — now Ollama-only: `OLLAMA_BASE_URL` (localhost:11434),
  `CHAT_PATH` (/v1/chat/completions), `MODELS_PATH` (/api/tags),
  an empty model fallback, `SYSTEM_PROMPT`, and timeouts; the first installed
  local model is selected when no preference is set.
  `configured_model()` returns `""` when no preference is set (previously
  returns a default) so the client can fall back to the first installed
  model.
- `CadXChatClient.py` — `ChatGptClient` → `OllamaClient`.
  OpenAI-compatible `/v1/chat/completions` with `stream: true`;
  messages are `{role, content}` plain strings (no content-part arrays);
  system prompt prepended; deltas from `choices[0].delta.content`;
  stream errors via `{"error": ...}` payloads or HTTP error bodies
  (JSON `error.message` extracted for readable messages);
  `resolve_model()` preference chain: explicit `model=` arg →
  Preferences → first model from `/api/tags` → empty fallback
  (cached after first success); `model` property is network-free.
- `CadXPanel.py` — sign-in button/status logic removed; header now shows
  `Ollama · <model>`, refreshed after each turn start (model may have
  been resolved on the turn thread). Send no longer gated on any auth.
- `CadXGui.py` — auth singleton removed; `get_session()` builds
  `ChatSession(OllamaClient())`; `ChatSession.client` public property
  added for panel status; **panel reopen fixed** (see §4).

Unchanged: `CadXSession.py` logic, `Init.py`, `InitGui.py`, test runner.

## 3. Historical migration notes

The migration failures described below were resolved. The current pure-Python
suite has 28 passing tests, and the native C++ graph, store, query, builder,
service, evidence, and audit suite has 15 passing tests. The current
implementation also includes a GUI-thread capture gateway, bounded native
query execution, direct active-Assembly capture, deterministic full graph
evidence round-trips, and an optional JSONL audit trail. Remaining work is
fixture-complete link/nested/joint extraction, observer wiring, full cursor
pagination, and final GUI lifecycle acceptance.

Run: `python3 src/Mod/CadX/cadx_tests/run_all.py`

### 3.1 Implementation bug (fix in `CadXChatClient.py`)
`parse_sse_events` does not actually stop at the `data: [DONE]` sentinel —
it `continue`s and yields payloads after DONE, contradicting its docstring.
Fix: in the loop, when `payload_text == b"[DONE]"`, `return` (end the
generator) instead of `continue`.
Affects: `test_stops_at_done_sentinel_and_skips_malformed`.

### 3.2 Design gap (fix in `CadXChatClient.py`)
`resolve_model()` ignores an explicit `model=` constructor argument: it
consults Preferences, then `/api/tags`, treating the context model only as
final fallback. An explicitly passed model must short-circuit with no
network call.
Suggested fix: record `self._explicit_model = model is not None` (or
`bool(model)`) in `__init__`; in `resolve_model()`, return
`self._context.model` immediately when set.
Affects: `test_explicit_model_wins_without_network`.

### 3.3 Test bugs (fix in `cadx_tests/test_cadx_chat.py`)
- `test_first_installed_model_used_when_no_preference` and
  `test_server_failure_falls_back_to_context_default` use a generic first
  local model and an empty fallback, so no model family is assumed.
- `test_history_roles_use_plain_content`: its fake transport lambda takes
  `(url, headers, body)` but `stream_turn` now passes a 4th
  `timeout_seconds` arg. Add the parameter.

### 3.4 Cosmetic
`CadXChatClient.py` module docstring still references the deleted
`CadXOpenAiAuth`. One-line cleanup.

### 3.5 After tests are green (not yet done)
1. `cmake --build build/debug` (copies .py files into `build/debug/Mod/CadX`)
2. Restart the app: `./build/debug/bin/FreeCAD`
   (no instance is currently running; the previously launched instance was
   the ChatGPT build)
3. Verify: no traceback in the log; panel docks right at startup; header
   shows `Ollama · <model>`; a chat turn streams (requires a running
   Ollama server with at least one model pulled).
4. End-to-end with a real Ollama server has NOT been exercised — only
   fake-transport tests.

## 4. Panel reopen fix (completed, unverified in GUI)

Problem: closing the dock left no obvious way to reopen it.

Fix in `CadXGui.py`:
- The dock's `toggleViewAction()` appears under **View > Panels**
  automatically — this fork's `MainWindow::populateDockWindowMenu`
  (src/Gui/MainWindow.cpp:1718) enumerates `findChildren<QDockWidget*>()`,
  so any Qt-native dock is listed.
- Additionally `_add_view_menu_entry()` appends the toggle action as a
  top-level **View > cad-x Assistant** item (best-effort; skipped if the
  View menu can't be identified on localized UIs).
- The `CadX_Assistant` command (`Gui.runCommand("CadX_Assistant")`) also
  toggles the dock.

## 5. Where I looked in VibeCAD (reference map)

VibeCAD repo: `/Users/akalari/filesystem/src/VibeCAD` (same FreeCAD fork
lineage; heavy AI customization). Relevant files for each area:

| Area | VibeCAD reference | Notes |
|---|---|---|
| **Ollama provider** | `src/Mod/VibeCAD/VibeCADOllama.py` | **Not yet examined — read this first.** VibeCAD's Ollama integration; expected to confirm endpoint/param details (e.g. reasoning-effort handling, num_ctx caveats) |
| Ollama user guidance | `docs/` + `README.md` "Local Models" section | OpenAI-compatible endpoint `http://localhost:11434/v1`, "reasoning effort none" for models that reject reasoning params, 64K context advice |
| Multi-provider auth | `VibeCADAuth.py` | `ProviderSpec` registry, `AuthStatus` (not_configured/configured_unverified/verified/invalid/offline), OS keyring via `keyring` package, `.env` support, credential precedence env → .env → keyring |
| ChatGPT subscription | `VibeCADCodex.py`, `VibeCADCodexResponses.py` | Delegates OAuth+transport to a version-pinned Codex app-server binary; cad-x deliberately replaced this with direct Ollama calls (product decision, no external binary) |
| Turn/session loop | `VibeCADSession.py` (5.9k lines) | Per-document persistence, document-thread dispatch, steering/stop/question callbacks — patterns to borrow when tools arrive |
| Panel UI | `VibeCADGui.py` (5.2k lines) | Composer with attach image/view, conversation selector, authoring-mode header; cad-x intentionally much simpler |
| Docking | `src/Gui/MainWindowPy.cpp` | **VibeCAD added a C++ Python binding** exposing `addDockWindow(widget, name, area='right')` / `registerDockWindow(widget, name)`. cad-x has no such binding — that's why the first docking attempt crashed (`DockWindowManager is unavailable`). cad-x uses Qt-native `QDockWidget` + `addDockWidget` instead, matching this fork's Help/BIM/FEM modules |
| MCP/external control | `VibeCADMCP.py`, `VibeCADMCPStdio.py` | Out of scope for cad-x for now |

## 6. Commands

```
# tests (no FreeCAD needed)
python3 src/Mod/CadX/cadx_tests/run_all.py

# build (module gated by BUILD_CADX, default ON)
cmake --build build/debug

# launch
./build/debug/bin/FreeCAD

# model override (else: first /api/tags model; no model family is hard-coded)
App.ParamGet("User parameter:BaseApp/Preferences/CadX").SetString("Model", "llama4:8b")
```

## 7. Repo hygiene notes for the next engineer

- `build/debug` once contained stale C++ artifacts of an earlier CadX
  attempt (`CadX.so`, `CadXGui.so`, CTest registrations, autogen dirs,
  `BUILD_CADX` cache var). All were removed; Python `.so` shims would
  shadow the `.py` modules at import time. If you see `.so` files under
  `build/debug/Mod/CadX`, delete them.
- `cadx_tests` runs with any Python ≥3.10; FreeCAD imports in
  `CadXConfig.configured_model()` are lazy so tests never need the app.
- cad-x `README.md` now describes the local Ollama adapter and the implemented
  read-only graph path; document mutation tools remain future work.
- The shell environment runs as root; launching GUI apps works but files
  created under `~/Library/Application Support/FreeCAD` may end up
  root-owned.
