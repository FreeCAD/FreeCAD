# CadX build and live-validation runbook

This checkout's CadX panel is local-Ollama only. OpenAI sign-in, API-key, or
Responses controls indicate that an older artifact was launched; stop and
rebuild before testing.

The user-facing application name is `CADX`. Internal executable, C++ library,
Python module, bundle identifier, and document-format names remain `FreeCAD`
where compatibility requires them. A current runtime must contain
`bin/branding.xml` with `<Application>CADX</Application>`.

## Build gates

1. Inspect `git status --short` and preserve all unrelated work.
2. Use the authoritative checkout at `/Users/akalari/filesystem/src/cad-x`.
   Disposable mirrors are not source-of-truth unless their exact synchronization
   has been proved.
3. Prefer `tools/cadx-build-validate.sh [build-dir]`. It uses the checkout's
   configured pixi CMake/Python directly and does not depend on an ephemeral
   `/tmp/opencode/cadx-env.sh` wrapper surviving across sessions.
4. Qt translation generation must use a real LinguistTools command and target.
   For Qt 6.8, require `qt6_add_translation` and `Qt6::lrelease`; never install a
   no-op translation wrapper.
5. Before trusting a GUI build, verify `moc`, `uic`, `rcc`, and `lrelease` are
   real executables. Verify `build/debug/src/Gui/qrc_resource.cpp` is substantial
   and contains known FreeCAD icons.
6. Query the configured target list after configuration instead of guessing
   names. In the current GUI build, the native module targets are `CadXApp` and
   `CadXGuiApp`; `CadXGui` is not a target. A successful direct compile is
   useful diagnostics but is not linked-build evidence.
7. A GUI acceptance build must also stage runtime files. Build the script/data
   targets `AssemblyScripts`, `AssemblyTests`, `PartScripts`,
   `PartDesignScripts`, `StartScripts`, `MeasureScripts`, `Stylesheets_data`,
   `PreferencePacks_data`, `PreferencePackTemplates_data`, and
   `data/examples/Example_data`, plus the Part, Assembly, Part Design, Start,
   and Measure app/GUI targets. A build containing only
   `.so` files can launch with an empty Workbench menu because `Init.py` and
   `InitGui.py` were never copied.
8. Require the built `Mod/*/Init.py`, `Mod/*/InitGui.py`, and
   `share/Gui/PreferencePacks/FreeCAD Dark/FreeCAD Dark.cfg` files before
   launch. Missing preference-pack data can trap startup in a repeated
   `No such Preference Pack: FreeCAD Dark` exception loop.
9. Always set `CCACHE_DIR` and `CCACHE_TEMPDIR` to a writable directory under
   `/private/tmp`. Elevation alone does not repair an unwritable or incorrectly
   owned user ccache directory.
10. Native Qt binaries that falsely report a missing `neon` CPU feature inside
   the restricted sandbox must be rerun outside that sandbox. Do not interpret
   that message as a test failure until the same binary fails on the host.
11. Run the current linked native tests and the complete Python suite. Record
   exact counts. Run `git diff --check` last.

## Launch provenance

Launch only the artifact just built from `build/debug/bin/FreeCAD`, with a fresh
test user directory under `/private/tmp`. Keep its terminal process alive. Before
using the UI, verify the running process command points at this exact executable
and inspect the launch log for module-load or Qt-resource errors.

On macOS, Computer Use cannot target a raw FreeCAD Mach-O process by filesystem
path. Launch the verified executable through a disposable `.app` wrapper under
`/private/tmp` with a unique bundle identifier. Before every launch, resolve the
wrapper's `Contents/MacOS/FreeCAD` symlink and require it to equal the current
build's exact executable path. A wrapper pointing at another build is stale and
must not be used.

Do not use an already-running FreeCAD window as evidence. Close only the
explicit test process you launched; never broadly kill FreeCAD processes.

## Computer Use rules

Use the Computer Use `node_repl` integration with `@oai/sky` for every UI
action. Do not substitute AppleScript, shell keystroke injection, or stale
element indexes.

1. The API argument is `app`, never `app_target`, for example
   `sky.get_app_state({app: "org.cadx.live.freecad"})`.
2. Fetch fresh FreeCAD app state before the first action.
3. After every click, key press, paste, or view change, fetch app state again
   and derive new accessibility indexes.
4. Prefer accessibility-index actions. Use screenshot coordinates only when
   the current accessibility tree is insufficient, and inspect a fresh
   screenshot first.
5. Verify the CadX panel before testing tools. It must show an Ollama/local-model
   selector and must not show OpenAI, ChatGPT, sign-in, subscription, API-key,
   or Responses controls.
6. Use FreeCAD's visible Python console for deterministic fixture setup and
   native-tool calls. Paste one `exec(...)` expression when multiline Python is
   needed so embedded newlines do not trigger partial execution.
7. Capture state after each mutation and verify both sides: the visible document
   tree/model and the returned graph revision/evidence.
8. Exercise create, insert, ground, and joint in sequence; verify stale-revision
   rejection, revision advancement, CAD/graph object counts, connector topology,
   and undo/redo invalidation.
9. Save screenshots and logs under `/private/tmp`, not the repository.
10. Treat `timeoutReached`, `noWindowsAvailable`, and ScreenCaptureKit failures
    as Computer Use transport failures, not application failures. Retry one
    fresh state read, then try the bundle identifier from `list_apps()`. If a
    populated workbench still cannot be read, stop UI mutation and preserve the
    launch log; do not act from stale indexes or switch to AppleScript.

If the current artifact cannot link or launch, stop before Computer Use and
report the first build/runtime blocker. Never demonstrate an older UI as a
substitute.
