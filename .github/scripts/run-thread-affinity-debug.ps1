# SPDX-License-Identifier: LGPL-2.1-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,

    [Parameter(Mandatory = $true)]
    [string]$TestRegex,

    [Parameter(Mandatory = $true)]
    [string]$TestProcessName,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    [int]$TimeoutSeconds = 90
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

function Write-DiagnosticLine {
    param([string]$Message)

    $timestamp = [DateTimeOffset]::UtcNow.ToString("O")
    Write-Host "[thread-affinity-runner][$timestamp] $Message"
}

function Write-ProcessSnapshot {
    param(
        [System.Diagnostics.Process]$Process,
        [string]$Path
    )

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("Captured: $([DateTimeOffset]::UtcNow.ToString('O'))")
    $lines.Add("PID: $($Process.Id)")

    try {
        $Process.Refresh()
        $lines.Add("Name: $($Process.ProcessName)")
        $lines.Add("Responding: $($Process.Responding)")
        $lines.Add("StartTime: $($Process.StartTime.ToUniversalTime().ToString('O'))")
        $lines.Add("TotalProcessorTime: $($Process.TotalProcessorTime)")
        $lines.Add("WorkingSet64: $($Process.WorkingSet64)")
        $lines.Add("PrivateMemorySize64: $($Process.PrivateMemorySize64)")
        $lines.Add("HandleCount: $($Process.HandleCount)")
        $lines.Add("ThreadCount: $($Process.Threads.Count)")
    }
    catch {
        $lines.Add("Get-Process details failed: $($_.Exception)")
    }

    $lines.Add("")
    $lines.Add("Win32_Process:")
    try {
        $processInfo = Get-CimInstance Win32_Process -Filter "ProcessId = $($Process.Id)"
        $lines.Add(($processInfo |
            Select-Object ProcessId, ParentProcessId, Name, ExecutablePath, CommandLine |
            Format-List |
            Out-String))
    }
    catch {
        $lines.Add("Win32_Process query failed: $($_.Exception)")
    }

    $lines.Add("")
    $lines.Add("Win32_Thread:")
    try {
        $threadInfo = Get-CimInstance Win32_Thread |
            Where-Object { $_.ProcessHandle -eq [string]$Process.Id } |
            Select-Object Handle, ThreadState, ThreadWaitReason, Priority,
                PriorityBase, KernelModeTime, UserModeTime, StartAddress
        $lines.Add(($threadInfo | Format-Table -AutoSize | Out-String))
    }
    catch {
        $lines.Add("Win32_Thread query failed: $($_.Exception)")
    }

    $lines | Set-Content -Path $Path -Encoding UTF8
}

function Write-MiniDump {
    param(
        [System.Diagnostics.Process]$Process,
        [string]$Path
    )

    if (-not ("ThreadAffinityDiagnostics.MiniDump" -as [type])) {
        Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

namespace ThreadAffinityDiagnostics
{
    public static class MiniDump
    {
        [Flags]
        public enum DumpType : uint
        {
            Normal = 0x00000000,
            WithHandleData = 0x00000004,
            WithUnloadedModules = 0x00000020,
            WithIndirectlyReferencedMemory = 0x00000040,
            WithProcessThreadData = 0x00000100,
            WithFullMemoryInfo = 0x00000800,
            WithThreadInfo = 0x00001000
        }

        [DllImport("Dbghelp.dll", SetLastError = true)]
        public static extern bool MiniDumpWriteDump(
            IntPtr processHandle,
            uint processId,
            IntPtr fileHandle,
            DumpType dumpType,
            IntPtr exceptionParam,
            IntPtr userStreamParam,
            IntPtr callbackParam
        );
    }
}
"@
    }

    $dumpType =
        [ThreadAffinityDiagnostics.MiniDump+DumpType]::WithHandleData -bor
        [ThreadAffinityDiagnostics.MiniDump+DumpType]::WithUnloadedModules -bor
        [ThreadAffinityDiagnostics.MiniDump+DumpType]::WithIndirectlyReferencedMemory -bor
        [ThreadAffinityDiagnostics.MiniDump+DumpType]::WithProcessThreadData -bor
        [ThreadAffinityDiagnostics.MiniDump+DumpType]::WithFullMemoryInfo -bor
        [ThreadAffinityDiagnostics.MiniDump+DumpType]::WithThreadInfo

    $stream = [System.IO.File]::Open(
        $Path,
        [System.IO.FileMode]::Create,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None
    )

    try {
        $Process.Refresh()
        $success = [ThreadAffinityDiagnostics.MiniDump]::MiniDumpWriteDump(
            $Process.Handle,
            [uint32]$Process.Id,
            $stream.SafeFileHandle.DangerousGetHandle(),
            $dumpType,
            [IntPtr]::Zero,
            [IntPtr]::Zero,
            [IntPtr]::Zero
        )

        if (-not $success) {
            $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            throw "MiniDumpWriteDump failed with Win32 error $errorCode"
        }
    }
    finally {
        $stream.Dispose()
    }
}

function Publish-TestOutput {
    param(
        [string]$Label,
        [string]$Path
    )

    Write-DiagnosticLine "===== ${Label}: $Path ====="
    if (Test-Path $Path) {
        Get-Content -Path $Path | ForEach-Object { Write-Host $_ }
    }
    else {
        Write-DiagnosticLine "$Label file was not created"
    }
}

function Find-DescendantProcess {
    param(
        [int]$RootProcessId,
        [string]$ProcessName
    )

    $processes = @(Get-CimInstance Win32_Process |
        Select-Object ProcessId, ParentProcessId, Name, CreationDate)

    $pending = [System.Collections.Generic.Queue[uint32]]::new()
    $descendantIds = [System.Collections.Generic.HashSet[uint32]]::new()
    $pending.Enqueue([uint32]$RootProcessId)
    $descendantIds.Add([uint32]$RootProcessId) | Out-Null

    while ($pending.Count -gt 0) {
        $parentId = $pending.Dequeue()
        foreach ($child in $processes | Where-Object { $_.ParentProcessId -eq $parentId }) {
            if ($descendantIds.Add([uint32]$child.ProcessId)) {
                $pending.Enqueue([uint32]$child.ProcessId)
            }
        }
    }

    $match = $processes |
        Where-Object {
            $_.ProcessId -ne $RootProcessId -and
            $descendantIds.Contains([uint32]$_.ProcessId) -and
            $_.Name -ieq $ProcessName
        } |
        Sort-Object CreationDate -Descending |
        Select-Object -First 1

    if (-not $match) {
        return $null
    }

    return Get-Process -Id $match.ProcessId -ErrorAction SilentlyContinue
}

function Write-ProcessTree {
    param(
        [int]$RootProcessId,
        [string]$Path
    )

    try {
        $processes = @(Get-CimInstance Win32_Process |
            Select-Object ProcessId, ParentProcessId, Name, ExecutablePath,
                CommandLine, CreationDate)
        $descendantIds = [System.Collections.Generic.HashSet[uint32]]::new()
        $pending = [System.Collections.Generic.Queue[uint32]]::new()
        $pending.Enqueue([uint32]$RootProcessId)
        $descendantIds.Add([uint32]$RootProcessId) | Out-Null

        while ($pending.Count -gt 0) {
            $parentId = $pending.Dequeue()
            foreach ($child in $processes | Where-Object { $_.ParentProcessId -eq $parentId }) {
                if ($descendantIds.Add([uint32]$child.ProcessId)) {
                    $pending.Enqueue([uint32]$child.ProcessId)
                }
            }
        }

        $tree = $processes |
            Where-Object { $descendantIds.Contains([uint32]$_.ProcessId) } |
            Sort-Object CreationDate

        @(
            "Captured: $([DateTimeOffset]::UtcNow.ToString('O'))"
            "Root PID: $RootProcessId"
            ""
            ($tree | Format-Table -Wrap -AutoSize | Out-String)
        ) | Set-Content -Path $Path -Encoding UTF8
    }
    catch {
        "Process-tree capture failed: $($_.Exception)" |
            Set-Content -Path $Path -Encoding UTF8
    }
}

function Stop-ProcessTree {
    param([int]$RootProcessId)

    & taskkill.exe /PID $RootProcessId /T /F 2>&1 |
        ForEach-Object { Write-DiagnosticLine "taskkill: $_" }
}

$resolvedBuildDir = (Resolve-Path $BuildDir).Path
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
$resolvedOutputDir = (Resolve-Path $OutputDir).Path

$ctestCommand = Get-Command ctest.exe -ErrorAction Stop
$env:QT_FORCE_STDERR_LOGGING = "1"
$env:FREECAD_MAIN_THREAD_TRACE = "1"

$stdoutPath = Join-Path $resolvedOutputDir "ThreadAffinity.ctest.stdout.log"
$stderrPath = Join-Path $resolvedOutputDir "ThreadAffinity.ctest.stderr.log"
$snapshotPath = Join-Path $resolvedOutputDir "ThreadAffinity.process.txt"
$treePath = Join-Path $resolvedOutputDir "ThreadAffinity.process-tree.txt"
$dumpPath = Join-Path $resolvedOutputDir "ThreadAffinity.hang.dmp"
$runnerPath = Join-Path $resolvedOutputDir "ThreadAffinity.runner.txt"

$ctestArguments = @(
    "--test-dir"
    $resolvedBuildDir
    "-R"
    $TestRegex
    "-V"
    "--output-on-failure"
)

@(
    "Started: $([DateTimeOffset]::UtcNow.ToString('O'))"
    "CTest: $($ctestCommand.Source)"
    "Arguments: $($ctestArguments -join ' ')"
    "BuildDir: $resolvedBuildDir"
    "TestRegex: $TestRegex"
    "TestProcessName: $TestProcessName"
    "OutputDir: $resolvedOutputDir"
    "TimeoutSeconds: $TimeoutSeconds"
    "FREECAD_MAIN_THREAD_TRACE: $env:FREECAD_MAIN_THREAD_TRACE"
) | Set-Content -Path $runnerPath -Encoding UTF8

Write-DiagnosticLine "Starting CTest for $TestRegex"
$ctestProcess = Start-Process `
    -FilePath $ctestCommand.Source `
    -ArgumentList $ctestArguments `
    -PassThru `
    -NoNewWindow `
    -RedirectStandardOutput $stdoutPath `
    -RedirectStandardError $stderrPath

Write-DiagnosticLine "CTest PID=$($ctestProcess.Id), watchdog=${TimeoutSeconds}s"
$deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
$nextHeartbeat = [DateTimeOffset]::UtcNow
$nextChildLookup = [DateTimeOffset]::UtcNow
$testProcess = $null
$timedOut = $false

while (-not $ctestProcess.HasExited) {
    $now = [DateTimeOffset]::UtcNow
    if ($now -ge $deadline) {
        $timedOut = $true
        break
    }

    if (
        $now -ge $nextChildLookup -and
        (-not $testProcess -or $testProcess.HasExited)
    ) {
        $testProcess = Find-DescendantProcess `
            -RootProcessId $ctestProcess.Id `
            -ProcessName $TestProcessName

        if ($testProcess) {
            Write-DiagnosticLine (
                "Located test process PID=$($testProcess.Id) name=$($testProcess.ProcessName)"
            )
        }
        $nextChildLookup = $now.AddSeconds(1)
    }

    if ($now -ge $nextHeartbeat) {
        try {
            $ctestProcess.Refresh()
            $testStatus = "not-found"
            if ($testProcess -and -not $testProcess.HasExited) {
                $testProcess.Refresh()
                $testStatus = (
                    "pid=$($testProcess.Id) cpu=$($testProcess.TotalProcessorTime) " +
                    "threads=$($testProcess.Threads.Count) workingSet=$($testProcess.WorkingSet64)"
                )
            }

            Write-DiagnosticLine (
                "alive ctestPid=$($ctestProcess.Id) ctestCpu=$($ctestProcess.TotalProcessorTime) " +
                "test=[$testStatus]"
            )
        }
        catch {
            Write-DiagnosticLine "heartbeat refresh failed: $($_.Exception.Message)"
        }
        $nextHeartbeat = $now.AddSeconds(10)
    }

    Start-Sleep -Milliseconds 250
}

if ($timedOut) {
    Write-Host "::error::$TestRegex exceeded the ${TimeoutSeconds}s diagnostic watchdog"

    if (-not $testProcess -or $testProcess.HasExited) {
        $testProcess = Find-DescendantProcess `
            -RootProcessId $ctestProcess.Id `
            -ProcessName $TestProcessName
    }

    Write-ProcessTree -RootProcessId $ctestProcess.Id -Path $treePath

    $dumpProcess = $testProcess
    if (-not $dumpProcess -or $dumpProcess.HasExited) {
        Write-DiagnosticLine (
            "Test process was not available; capturing the CTest process instead"
        )
        $dumpProcess = $ctestProcess
    }

    Write-ProcessSnapshot -Process $dumpProcess -Path $snapshotPath
    try {
        Write-MiniDump -Process $dumpProcess -Path $dumpPath
        Write-DiagnosticLine (
            "Wrote minidump for PID=$($dumpProcess.Id): $dumpPath"
        )
    }
    catch {
        Write-DiagnosticLine "Minidump capture failed: $($_.Exception)"
        $_ | Out-String | Add-Content -Path $snapshotPath
    }

    Stop-ProcessTree -RootProcessId $ctestProcess.Id
    Wait-Process -Id $ctestProcess.Id -Timeout 10 -ErrorAction SilentlyContinue

    Publish-TestOutput -Label "CTEST STDOUT" -Path $stdoutPath
    Publish-TestOutput -Label "CTEST STDERR" -Path $stderrPath
    Publish-TestOutput -Label "PROCESS SNAPSHOT" -Path $snapshotPath
    Publish-TestOutput -Label "PROCESS TREE" -Path $treePath
    exit 124
}

$ctestProcess.WaitForExit()
Publish-TestOutput -Label "CTEST STDOUT" -Path $stdoutPath
Publish-TestOutput -Label "CTEST STDERR" -Path $stderrPath

Write-DiagnosticLine "CTest exited with code $($ctestProcess.ExitCode)"
exit $ctestProcess.ExitCode
