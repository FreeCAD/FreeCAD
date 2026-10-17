# Visual snapshots

`visual_baselines.py` runs FreeCAD snapshot suites and writes an HTML
comparison report. Suites follow the `Visual*.py` naming convention in
`src/Mod/Test`; pass the module name to `--suite`.

Run from the repository root with a built FreeCAD. **Always use `--headless`**
for recording, updating, and comparing snapshots. It requires Linux and
`xvfb-run` and isolates rendering from your desktop settings. Without it, OS
configuration can subtly change the results.

Replace `VisualYourSuite` with the suite to run:

```sh
python3 tools/rendering/visual_baselines.py compare \
  --suite VisualYourSuite --headless \
  --freecad build/relWithDebInfo/bin/FreeCAD \
  --out-dir /tmp/freecad-render-compare
```

The output directory must be empty or nonexistent; omit `--out-dir` for a fresh
temporary directory. Open its `index.html` to inspect expected, actual, and
difference images. `results.json` contains checks and run metadata. Failure
returns a nonzero status.

Replace `compare` with `record` to capture images without comparing or changing
baselines, or `update` to copy captures into the baseline directory after the
suite succeeds. Review recorded images before updating; success does not
establish visual correctness.

Baselines live under `tests/visual/baselines` in a directory matching the suite
name. The script launches FreeCAD's test runner, passes snapshot settings
through `FC_VISUAL_*` variables, and collects the suite's results into the
report.
