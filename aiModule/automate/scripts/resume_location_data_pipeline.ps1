$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

function Invoke-PixiPython {
    param([string]$Step, [string[]]$Arguments)
    Write-Output "PIPELINE_STEP_START=$Step"
    & pixi run python @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Pipeline step '$Step' failed with exit code $LASTEXITCODE" }
    Write-Output "PIPELINE_STEP_DONE=$Step"
}

$audit = "audit/location_full_299986_cache_valid.jsonl"
$cache = "dataset/cache/brep_graph_paper_full"
$examples = "dataset/training/location_selection_paper_full"
$index = "dataset/training/location_paper_full"

Invoke-PixiPython "verify_graph_cache" @("verify_graph_cache.py", "--cache-dir", $cache, "--mates", $audit)
Invoke-PixiPython "parallel_build_location_examples" @(
    "parallel_build_location_examples.py", "--audit", $audit, "--cache-dir", $cache,
    "--output-dir", $examples, "--max-pairs", "10000", "--workers", "4"
)
Invoke-PixiPython "verify_location_selection_examples" @(
    "verify_location_selection_examples.py", "--examples-dir", $examples
)
Invoke-PixiPython "build_location_training_index" @(
    "build_location_training_index.py", "--examples-dir", $examples,
    "--output-dir", $index, "--seed", "20260725"
)
Invoke-PixiPython "verify_location_training_index" @(
    "verify_location_training_index.py", "--index-dir", $index
)
Write-Output "PIPELINE_COMPLETE=$index/verification.json"
