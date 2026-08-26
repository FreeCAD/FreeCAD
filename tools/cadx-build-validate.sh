#!/bin/sh

set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${1:-"$project_root/build/debug"}
cmake_bin="$project_root/.pixi/envs/default/bin/cmake"
python_bin="$project_root/.pixi/envs/default/bin/python"
ccache_dir=${CADX_CCACHE_DIR:-/private/tmp/cadx-ccache}
ccache_tmp="$ccache_dir/tmp"

if [ ! -x "$cmake_bin" ]; then
    echo "CadX build gate: missing configured CMake at $cmake_bin" >&2
    exit 2
fi

if [ ! -f "$build_dir/CMakeCache.txt" ]; then
    echo "CadX build gate: $build_dir is not a configured CMake build" >&2
    exit 2
fi

mkdir -p "$ccache_tmp"

export CCACHE_DIR="$ccache_dir"
export CCACHE_TEMPDIR="$ccache_tmp"

"$cmake_bin" --build "$build_dir" --parallel 8 --target \
    Stylesheets_data \
    PreferencePacks_data \
    PreferencePackTemplates_data \
    data/examples/Example_data \
    AssemblyScripts \
    AssemblyTests \
    PartScripts \
    PartDesignScripts \
    StartScripts \
    MeasureScripts \
    Part \
    PartGui \
    Assembly \
    AssemblyGui \
    PartDesign \
    PartDesignGui \
    Start \
    StartGui \
    Measure \
    MeasureGui \
    CadXApp \
    CadXGuiApp \
    FreeCAD \
    CadX_tests_run

for runtime_file in \
    "$build_dir/Mod/Assembly/Init.py" \
    "$build_dir/Mod/Assembly/InitGui.py" \
    "$build_dir/Mod/Part/Init.py" \
    "$build_dir/Mod/Part/InitGui.py" \
    "$build_dir/Mod/PartDesign/Init.py" \
    "$build_dir/Mod/PartDesign/InitGui.py" \
    "$build_dir/Mod/Start/Init.py" \
    "$build_dir/Mod/Start/InitGui.py" \
    "$build_dir/Mod/Measure/Init.py" \
    "$build_dir/Mod/Measure/InitGui.py" \
    "$build_dir/share/Gui/PreferencePacks/FreeCAD Dark/FreeCAD Dark.cfg" \
    "$build_dir/Mod/CadX/CadXApp.so" \
    "$build_dir/Mod/CadX/CadXGuiApp.so" \
    "$build_dir/bin/branding.xml" \
    "$build_dir/bin/FreeCAD"
do
    if [ ! -e "$runtime_file" ]; then
        echo "CadX build gate: missing runtime artifact $runtime_file" >&2
        exit 3
    fi
done

if ! grep -q '<Application>CADX</Application>' "$build_dir/bin/branding.xml"; then
    echo "CadX build gate: branding.xml does not identify the application as CADX" >&2
    exit 3
fi

qrc_file="$build_dir/src/Gui/qrc_resource.cpp"
if [ ! -f "$qrc_file" ]; then
    echo "CadX build gate: missing Qt resource output $qrc_file" >&2
    exit 3
fi

qrc_size=$(wc -c < "$qrc_file")
if [ "$qrc_size" -lt 1000000 ]; then
    echo "CadX build gate: Qt resource output is suspiciously small ($qrc_size bytes)" >&2
    exit 3
fi

for resource_name in freecadsplash_2x.png application-exit.svg Std_ToggleBottomPanels.svg
do
    if ! grep -q "$resource_name" "$qrc_file"; then
        echo "CadX build gate: Qt resource output lacks $resource_name" >&2
        exit 3
    fi
done

"$build_dir/tests/CadX_tests_run"
"$python_bin" "$project_root/src/Mod/CadX/cadx_tests/run_all.py"

echo "CadX build gate passed: linked modules, runtime data, Qt resources, and tests are current."
