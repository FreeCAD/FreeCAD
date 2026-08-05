param(
    [int]$AuditCount = 300000,
    [int]$Seed = 20260725
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

function Invoke-PixiPython {
    param(
        [Parameter(Mandatory = $true)][string]$Step,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )
    Write-Output "PIPELINE_STEP_START=$Step"
    & pixi run python @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Pipeline step '$Step' failed with exit code $LASTEXITCODE"
    }
    Write-Output "PIPELINE_STEP_DONE=$Step"
}

$audit = "audit/location_full_$AuditCount.jsonl"
$cache = "dataset/cache/brep_graph_paper_full"
$examples = "dataset/training/location_selection_paper_full"
$index = "dataset/training/location_paper_full"

New-Item -ItemType Directory -Force -Path "audit" | Out-Null

Invoke-PixiPython "audit_mates" @(
    "audit_mates.py", "--count", "$AuditCount", "--seed", "$Seed",
    "--output", $audit, "--progress-every", "1000"
)
Invoke-PixiPython "seed_graph_cache" @(
    "seed_graph_cache.py", "--source", "dataset/cache/brep_graph_mate_type_10000",
    "--destination", $cache
)
Invoke-PixiPython "parallel_preprocess_parts" @(
    "parallel_preprocess_parts.py", "--mates", $audit, "--cache-dir", $cache,
    "--workers", "4", "--progress-every", "100"
)
Invoke-PixiPython "verify_graph_cache" @(
    "verify_graph_cache.py", "--cache-dir", $cache, "--mates", $audit
)
Invoke-PixiPython "parallel_build_location_examples" @(
    "parallel_build_location_examples.py", "--audit", $audit,
    "--cache-dir", $cache, "--output-dir", $examples,
    "--max-pairs", "10000", "--workers", "4"
)
Invoke-PixiPython "verify_location_selection_examples" @(
    "verify_location_selection_examples.py", "--examples-dir", $examples
)
Invoke-PixiPython "build_location_training_index" @(
    "build_location_training_index.py", "--examples-dir", $examples,
    "--output-dir", $index, "--seed", "$Seed"
)
Invoke-PixiPython "verify_location_training_index" @(
    "verify_location_training_index.py", "--index-dir", $index
)

Write-Output "PIPELINE_COMPLETE=$index/verification.json"
