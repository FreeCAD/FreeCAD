# Visual snapshots

`visual_baselines.py` runs FreeCAD snapshot suites and writes an HTML
comparison report. Suites follow the `Visual*.py` naming convention in
`src/Mod/Test`; pass the module name to `--suite`.

Run from the repository root with a built FreeCAD. **Always use `--headless`**
for recording, updating, and comparing snapshots. It requires Linux and
`Xvfb` on `PATH` and isolates rendering from your desktop settings. Without it, OS
configuration can subtly change the results.

Replace `VisualYourSuite` with the suite to run:

```sh
python3 tools/rendering/visual_baselines.py compare \
  --suite VisualYourSuite --headless
```

For pixi, use `pixi run python3 ...`.

The report is written to a fresh temporary directory, not the source tree.
Open the printed `index.html` path to inspect expected, actual, and difference
images.

Replace `compare` with `record` to capture images without comparing or changing
baselines, or `update` to copy captures into the baseline directory after the
suite succeeds. Review recorded images before updating; success does not
establish visual correctness.

Baselines live under `tests/visual/baselines` in a directory matching the suite
name. The script launches FreeCAD's test runner, passes snapshot settings
through `FC_VISUAL_*` variables, and collects the suite's results into the
report.

## Command reference

```text
python3 tools/rendering/visual_baselines.py {compare,record,update} [options]
```

Place options after the command:

| Option | Meaning |
| --- | --- |
| `--suite MODULE` | Suite module without `.py`; defaults to `VisualCoinNodeSnapshots`. |
| `--headless` | Isolate rendering under Xvfb (Linux only). Use for every run. |
| `--freecad PATH` | FreeCAD executable; otherwise searches `$FREECAD`, local `build` directories, then `PATH`. |
| `--baseline-dir DIR` | Baseline directory; defaults to `tests/visual/baselines/<suite>` in the source tree. |
| `--out-dir DIR` | Report directory; must be empty or nonexistent. Defaults to a fresh temporary directory. Relative paths use the current working directory. |
| `-h`, `--help` | Show help for the script or command. |
