param(
    [Parameter(Mandatory = $true)]
    [string] $ServerExe,
    [ValidateSet('AX_TradeCoreSuite', 'AX_PhoneSuite')]
    [string] $Suite = 'AX_TradeCoreSuite',
    [ValidateRange(10, 300)]
    [int] $TimeoutSeconds = 90
)

$ErrorActionPreference = 'Stop'
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$serverPath = (Resolve-Path -LiteralPath $ServerExe).Path
if ([System.IO.Path]::GetFileName($serverPath) -ne 'ArmaReforgerServerDiag.exe') {
    throw 'Use the installed diagnostic server executable ArmaReforgerServerDiag.exe.'
}

$testRoot = Join-Path $repoRoot '.local\trade-tests'
$addonRoot = Join-Path $testRoot 'addons'
$runName = 'run-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 6)
$profileRoot = Join-Path $testRoot $runName
New-Item -ItemType Directory -Path $addonRoot, $profileRoot -Force | Out-Null
$targets = @{
    ProjectAxum = 'axum'
    ProjectAxumTests = 'axum-tests'
    EveronLife = 'src'
    EnfusionDatabaseFramework = 'dependencies\EnfusionDatabaseFramework\src'
    EnfusionPersistenceFramework = 'dependencies\EnfusionPersistenceFramework\src'
}
foreach ($name in $targets.Keys) {
    $sourcePath = Join-Path $repoRoot $targets[$name]
    if (-not (Test-Path -LiteralPath $sourcePath)) { throw "Missing source $sourcePath. Initialize Git submodules first." }
    $linkPath = Join-Path $addonRoot $name
    if (Test-Path -LiteralPath $linkPath) {
        $item = Get-Item -LiteralPath $linkPath
        if ($item.LinkType -ne 'Junction' -or [System.IO.Path]::GetFullPath([string]$item.Target) -ne $sourcePath) {
            throw "Unexpected existing test addon at $linkPath; leave it intact and resolve the path manually."
        }
    } else {
        New-Item -ItemType Junction -Path $linkPath -Value $sourcePath | Out-Null
    }
}

$expectedCases = @{ AX_TradeCoreSuite = 11; AX_PhoneSuite = 6 }[$Suite]
$arguments = '-addonsDir "' + $addonRoot + '" -addons 4E6001DE6EAF45DA -autotest ' + $Suite + ' -profile "' + $profileRoot + '" -logStats 0'
$stdout = Join-Path $profileRoot 'stdout.log'
$stderr = Join-Path $profileRoot 'stderr.log'
$process = $null
try {
    $process = Start-Process -FilePath $serverPath -ArgumentList $arguments -WorkingDirectory (Split-Path $serverPath) -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
    $process.Id | Set-Content -LiteralPath (Join-Path $profileRoot 'process-id.txt')
    Write-Output "Native Enforce tests: $profileRoot"
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 200
        $logs = ((Get-Content -LiteralPath $stdout, $stderr -Raw -ErrorAction SilentlyContinue) -join "`n")
        if ($logs -match 'Can.t compile|SCRIPT\s+\(E\): @|Virtual Machine Exception') {
            throw "Script compile/runtime error. See $profileRoot."
        }
    }
    if (-not $process.HasExited) { throw "Tests timed out. See $profileRoot." }
    if ($process.ExitCode -ne 0) { throw "Diagnostic server exited with code $($process.ExitCode). See $profileRoot." }
    $reports = @(Get-ChildItem -LiteralPath $profileRoot -Recurse -File -Filter junit.xml)
    if ($reports.Count -ne 1) { throw "Expected exactly one native JUnit report. No pass is inferred from exit code alone. See $profileRoot." }
    [xml]$report = Get-Content -LiteralPath $reports[0].FullName -Raw
    $cases = @($report.SelectNodes('//testsuite[@name="' + $Suite + '"]/testcase'))
    $failures = @($report.SelectNodes('//failure|//error|//skipped'))
    if ($cases.Count -ne $expectedCases -or $failures.Count -ne 0) {
        throw "Expected $expectedCases passing cases; found $($cases.Count) cases and $($failures.Count) failed/errored/skipped results. See $($reports[0].FullName)."
    }
    Write-Output "PASS: $($cases.Count) native Enforce cases, no failures/errors/skips."
    Write-Output "Report: $($reports[0].FullName)"
} finally {
    # Stop only the process created by this invocation. Never stop a user's game
    # or another server, and never remove source paths or addon junctions.
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
}
