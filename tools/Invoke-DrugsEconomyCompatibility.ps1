param(
    [Parameter(Mandatory = $true)][string] $ServerExe,
    [Parameter(Mandatory = $true)][string] $CandidatePath,
    [ValidateSet('Pinned', 'Declared')][string] $Stack = 'Pinned',
    [string] $InstalledAddonRoot,
    [ValidateRange(10, 300)][int] $TimeoutSeconds = 90
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$serverPath = (Resolve-Path -LiteralPath $ServerExe).Path
if ([IO.Path]::GetFileName($serverPath) -ne 'ArmaReforgerServerDiag.exe') {
    throw 'Use ArmaReforgerServerDiag.exe; this probe does not launch the player game.'
}
$candidate = (Resolve-Path -LiteralPath $CandidatePath).Path
if (-not $InstalledAddonRoot) { $InstalledAddonRoot = Split-Path $candidate }
$installedRoot = (Resolve-Path -LiteralPath $InstalledAddonRoot).Path
$runRoot = Join-Path $repoRoot ('.local\drugs-economy-compat\' + $Stack.ToLowerInvariant() + '-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 6))
$addonRoot = Join-Path $runRoot 'addons'
New-Item -ItemType Directory -Path $addonRoot | Out-Null
$evidence = [ordered]@{
    stack = $Stack
    utc = [DateTime]::UtcNow.ToString('o')
    engine = (Get-Item -LiteralPath $serverPath).VersionInfo.FileVersion
    branch = (git -C $repoRoot branch --show-current | Out-String).Trim()
    head = (git -C $repoRoot rev-parse HEAD | Out-String).Trim()
    probe = '4E6001DE6EAF45DB'
    packages = @()
    sources = @()
    suites = @()
    result = 'NOT_RUN'
    limitation = 'Headless compile/domain/resource resolution only; no rendered UI, spawned gameplay, network authorization, persistence or settlement fault injection.'
}

function Copy-Package([string] $Source, [string] $Name, [string] $Id, [string] $Version) {
    $metadata = Get-Content -LiteralPath (Join-Path $Source 'ServerData.json') -Raw | ConvertFrom-Json
    if ($metadata.id -ne $Id -or $metadata.revision.version -ne $Version -or $metadata.revision.corrupted) {
        throw "Unexpected or corrupt package: $Source. Expected $Id version $Version."
    }
    $destination = Join-Path $addonRoot $Name
    New-Item -ItemType Directory -Path $destination | Out-Null
    $projects = @(Get-ChildItem -LiteralPath $Source -File -Filter '*.gproj')
    if ($projects.Count -ne 1 -or (Get-Content -LiteralPath $projects[0].FullName -Raw) -notmatch ('GUID\s+"' + $Id + '"')) {
        throw "Expected one project with GUID $Id at $Source."
    }
    Copy-Item -LiteralPath $projects[0].FullName -Destination (Join-Path $destination $projects[0].Name)
    # Never mount installed packages directly: the engine may update RDB caches.
    foreach ($file in @('data.pak', 'resourceDatabase.rdb', 'ServerData.json')) {
        Copy-Item -LiteralPath (Join-Path $Source $file) -Destination (Join-Path $destination $file)
    }
    $sourceHash = (Get-FileHash -LiteralPath (Join-Path $Source 'data.pak') -Algorithm SHA256).Hash
    if ((Get-FileHash -LiteralPath (Join-Path $destination 'data.pak') -Algorithm SHA256).Hash -ne $sourceHash) { throw 'Package copy hash mismatch.' }
    $evidence.packages += [ordered]@{ id = $Id; version = $Version; source = $Source; sha256 = $sourceHash; bytes = (Get-Item -LiteralPath (Join-Path $Source 'data.pak')).Length }
}

try {
    Copy-Package $candidate 'DrugsEconomy' '692677B5062F640E' '0.1.62'
    $sources = [ordered]@{
        ProjectAxum = 'axum'
        ProjectAxumTests = 'axum-tests'
        EveronLife = 'src'
        DrugsEconomyProbe = 'compat\drugs-economy'
    }
    if ($Stack -eq 'Pinned') {
        foreach ($dependency in @(
            @('EnfusionPersistenceFramework', 'cbf255fd8448e1c2235859797c70e1464ebe896f'),
            @('EnfusionDatabaseFramework', '9a92e9a3aad75704f6d35bf543563f373a731a1a')
        )) {
            $dependencyRoot = Join-Path $repoRoot ('dependencies\' + $dependency[0])
            $actual = (git -C $dependencyRoot rev-parse HEAD | Out-String).Trim()
            if ($LASTEXITCODE -ne 0 -or $actual -ne $dependency[1]) { throw "Unexpected pin for $($dependency[0]): $actual" }
            if (git -C $dependencyRoot status --porcelain) { throw "Dependency has local edits: $dependencyRoot. Leave them intact and audit separately." }
            $sources[$dependency[0]] = 'dependencies\' + $dependency[0] + '\src'
        }
    } else {
        Copy-Package (Join-Path $installedRoot 'EnfusionPersistenceFramework_5D6EBC81EB1842EF') 'EnfusionPersistenceFramework' '5D6EBC81EB1842EF' '0.6.18'
        Copy-Package (Join-Path $installedRoot 'EnfusionDatabaseFramework_5D6EA74A94173EDF') 'EnfusionDatabaseFramework' '5D6EA74A94173EDF' '0.6.10'
    }
    foreach ($name in $sources.Keys) {
        $source = Join-Path $repoRoot $sources[$name]
        if (-not (Test-Path -LiteralPath $source)) { throw "Missing source: $source" }
        # Snapshot working files, including any uncommitted edits, without writing back.
        Copy-Item -LiteralPath $source -Destination (Join-Path $addonRoot $name) -Recurse
        foreach ($file in Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object { $_.Extension -in @('.c', '.gproj', '.conf', '.et', '.layout') }) {
            $evidence.sources += [ordered]@{ path = $sources[$name] + '/' + $file.FullName.Substring($source.Length + 1).Replace('\', '/'); sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash }
        }
    }
    foreach ($suite in @(@('AX_TradeCoreSuite', 11), @('AX_DrugsEconomyResourceSuite', 1))) {
        $profile = Join-Path $runRoot $suite[0]
        New-Item -ItemType Directory -Path $profile | Out-Null
        $stdout = Join-Path $profile 'stdout.log'
        $stderr = Join-Path $profile 'stderr.log'
        $arguments = '-addonsDir "' + $addonRoot + '" -addons 4E6001DE6EAF45DB -autotest ' + $suite[0] + ' -profile "' + $profile + '" -logStats 0'
        $process = $null
        $suiteResult = [ordered]@{ name = $suite[0]; expected = $suite[1]; result = 'RUNNING'; profile = $profile }
        $evidence.suites += $suiteResult
        try {
            Write-Output "Running $Stack / $($suite[0]): $profile"
            $process = Start-Process -FilePath $serverPath -ArgumentList $arguments -WorkingDirectory (Split-Path $serverPath) -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
            $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
            while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
                Start-Sleep -Milliseconds 200
                $logs = (Get-Content -LiteralPath $stdout, $stderr -Raw -ErrorAction SilentlyContinue) -join "`n"
                if ($logs -match 'Can.t compile|SCRIPT\s+\(E\): @|Virtual Machine Exception') { throw "Compile/runtime error in $profile" }
            }
            if (-not $process.HasExited) { throw "Timeout in $profile" }
            if ($process.ExitCode -ne 0) { throw "Diagnostic exit $($process.ExitCode) in $profile" }
            $logs = (Get-Content -LiteralPath $stdout, $stderr -Raw) -join "`n"
            if ($logs -match 'Can.t compile|SCRIPT\s+\(E\): @|Virtual Machine Exception') { throw "Compile/runtime error in $profile" }
            if ($logs -notmatch "Adding package '[^']*DrugsEconomy[/\\]'" -or $logs -notmatch 'name=ProjectAxum.DrugsEconomyProbe') { throw 'Optional package/probe did not mount; cannot infer compatibility.' }
            $reports = @(Get-ChildItem -LiteralPath $profile -Recurse -File -Filter junit.xml)
            if ($reports.Count -ne 1) { throw 'Expected exactly one native JUnit report.' }
            [xml] $report = Get-Content -LiteralPath $reports[0].FullName -Raw
            $cases = @($report.SelectNodes('//testsuite[@name="' + $suite[0] + '"]/testcase'))
            $bad = @($report.SelectNodes('//failure|//error|//skipped'))
            $suiteResult.cases = $cases.Count
            $suiteResult.junit = $reports[0].FullName
            if ($cases.Count -ne $suite[1] -or $bad.Count) { throw "Unexpected native results: $($cases.Count) cases, $($bad.Count) failures/errors/skips." }
            $suiteResult.result = 'PASS'
        } catch {
            $suiteResult.result = 'FAIL'
            $suiteResult.error = $_.Exception.Message
            throw
        } finally {
            if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
        }
    }
    $evidence.result = 'PASS'
    Write-Output 'PASS: 11 appointment-core cases and one eight-resource resolution case.'
} catch {
    $evidence.result = 'FAIL'
    $evidence.error = $_.Exception.Message
    throw
} finally {
    $evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $runRoot 'evidence.json') -Encoding UTF8
    Write-Output "Evidence: $runRoot\evidence.json"
}
