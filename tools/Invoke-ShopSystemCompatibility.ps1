param(
    [Parameter(Mandatory = $true)][string] $ServerExe,
    [Parameter(Mandatory = $true)][string] $CandidatePath,
    [Parameter(Mandatory = $true)][string] $WorkbenchExe,
    [Parameter(Mandatory = $true)][string] $GameDirectory,
    [ValidateRange(10, 300)][int] $TimeoutSeconds = 90
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$serverPath = (Resolve-Path -LiteralPath $ServerExe).Path
$workbenchPath = (Resolve-Path -LiteralPath $WorkbenchExe).Path
$gameRoot = (Resolve-Path -LiteralPath $GameDirectory).Path
if ([IO.Path]::GetFileName($workbenchPath) -ne 'ArmaReforgerWorkbenchSteamDiag.exe' -or -not (Test-Path -LiteralPath (Join-Path $gameRoot 'addons/data/ArmaReforger.gproj'))) {
    throw 'Use the diagnostic Workbench executable and installed game directory containing addons/data/ArmaReforger.gproj.'
}
if ([IO.Path]::GetFileName($serverPath) -ne 'ArmaReforgerServerDiag.exe') {
    throw 'Use ArmaReforgerServerDiag.exe; this probe does not launch the player game.'
}
$candidate = (Resolve-Path -LiteralPath $CandidatePath).Path
$metadata = Get-Content -LiteralPath (Join-Path $candidate 'ServerData.json') -Raw | ConvertFrom-Json
if ($metadata.id -ne '5D2D1436D1FA5A13' -or $metadata.revision.version -ne '1.0.18' -or $metadata.revision.corrupted) {
    throw 'Expected uncorrupted Shop System 1.0.18. Audit a different version before testing it.'
}
$projects = @(Get-ChildItem -LiteralPath $candidate -File -Filter '*.gproj')
if ($projects.Count -ne 1 -or (Get-Content -LiteralPath $projects[0].FullName -Raw) -notmatch 'GUID\s+"5D2D1436D1FA5A13"') {
    throw 'Expected exactly one Shop System project.'
}
foreach ($dependency in @(
    @('EnfusionPersistenceFramework', 'cbf255fd8448e1c2235859797c70e1464ebe896f'),
    @('EnfusionDatabaseFramework', '9a92e9a3aad75704f6d35bf543563f373a731a1a')
)) {
    $dependencyRoot = Join-Path $repoRoot ('dependencies\' + $dependency[0])
    $actual = (git -C $dependencyRoot rev-parse HEAD | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $actual -ne $dependency[1]) { throw "Unexpected pin for $($dependency[0]): $actual" }
    if (git -C $dependencyRoot status --porcelain) { throw "Dependency has local edits: $dependencyRoot. Leave them intact and audit separately." }
}
$runRoot = Join-Path $repoRoot ('.local\shop-system-compat\run-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 6))
$addonRoot = Join-Path $runRoot 'addons'
New-Item -ItemType Directory -Path $addonRoot | Out-Null
$evidence = [ordered]@{
    utc = [DateTime]::UtcNow.ToString('o')
    engine = (Get-Item -LiteralPath $serverPath).VersionInfo.FileVersion
    workbenchEngine = (Get-Item -LiteralPath $workbenchPath).VersionInfo.FileVersion
    branch = (git -C $repoRoot branch --show-current | Out-String).Trim()
    head = (git -C $repoRoot rev-parse HEAD | Out-String).Trim()
    probe = '4E6001DE6EAF45DC'
    package = [ordered]@{ id = $metadata.id; version = $metadata.revision.version; source = $candidate }
    sources = @()
    suites = @()
    result = 'NOT_RUN'
    limitation = 'Headless character construction, UI binding and disabled-transfer dispatch only. No graphical visibility, player interaction, AI/navigation, multiplayer authorization, real item/payment settlement or durable per-player allowances proven.'
}
try {
    # Copy installed package: native tools may update RDB caches on mounted addons.
    $packageRoot = Join-Path $addonRoot 'ShopSystem'
    New-Item -ItemType Directory -Path $packageRoot | Out-Null
    foreach ($file in @($projects[0].Name, 'data.pak', 'resourceDatabase.rdb', 'ServerData.json', 'meta')) {
        Copy-Item -LiteralPath (Join-Path $candidate $file) -Destination (Join-Path $packageRoot $file)
    }
    $packageHash = (Get-FileHash -LiteralPath (Join-Path $candidate 'data.pak') -Algorithm SHA256).Hash
    if ((Get-FileHash -LiteralPath (Join-Path $packageRoot 'data.pak') -Algorithm SHA256).Hash -ne $packageHash) { throw 'Package copy hash mismatch.' }
    $evidence.package.sha256 = $packageHash
    $sources = [ordered]@{
        ProjectAxum = 'axum'
        ProjectAxumTests = 'axum-tests'
        EveronLife = 'src'
        EnfusionPersistenceFramework = 'dependencies\EnfusionPersistenceFramework\src'
        EnfusionDatabaseFramework = 'dependencies\EnfusionDatabaseFramework\src'
        ShopSystemProbe = 'compat\shop-system'
    }
    foreach ($name in $sources.Keys) {
        $source = Join-Path $repoRoot $sources[$name]
        if (-not (Test-Path -LiteralPath $source)) { throw "Missing source: $source" }
        # Snapshot all current working files without writing back to the checkout.
        Copy-Item -LiteralPath $source -Destination (Join-Path $addonRoot $name) -Recurse -Exclude '*.rdb'
        foreach ($file in Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object { $_.Extension -in @('.c', '.gproj', '.conf', '.et', '.layout', '.meta') }) {
            $evidence.sources += [ordered]@{ path = $sources[$name] + '/' + $file.FullName.Substring($source.Length + 1).Replace('\', '/'); sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash }
        }
    }
    # Native server does not register new metafiles. Workbench scans metadata
    # at startup. Bootstrap only the disposable snapshot's resource caches.
    $buildProfile = Join-Path $runRoot 'resource-registration'
    New-Item -ItemType Directory -Path $buildProfile | Out-Null
    $probeProject = Join-Path $addonRoot 'ShopSystemProbe/ProjectAxum.ShopSystemProbe.gproj'
    $arguments = '-gproj "' + $probeProject + '" -addonsDir "' + $addonRoot + '" -profile "' + $buildProfile + '"'
    $process = $null
    try {
        Write-Output "Registering probe resources: $buildProfile"
        $process = Start-Process -FilePath $workbenchPath -ArgumentList $arguments -WorkingDirectory $gameRoot -WindowStyle Hidden -RedirectStandardOutput (Join-Path $buildProfile 'stdout.log') -RedirectStandardError (Join-Path $buildProfile 'stderr.log') -PassThru
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $scanComplete = $false
        while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 200
            $consoleFiles = @(Get-ChildItem -LiteralPath $buildProfile -Recurse -File -Filter console.log)
            $buildLogs = ($consoleFiles | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw -ErrorAction SilentlyContinue }) -join "`n"
            if ($buildLogs -match 'Can.t compile|SCRIPT\s+\(E\): @|Virtual Machine Exception|Game addon.*not found') { throw "Workbench startup failed. See $buildProfile." }
            if ($buildLogs -match 'Workbench startup took:') { $scanComplete = $true; break }
        }
        if (-not $scanComplete) { throw "Workbench resource scan did not finish. See $buildProfile." }
        if (-not (Test-Path -LiteralPath (Join-Path $addonRoot 'ShopSystemProbe/resourceDatabase.rdb'))) { throw 'No probe resource database produced.' }
        $evidence.resourceRegistration = 'STARTUP_SCAN_COMPLETE; native suites verify actual GUID resolution'
    } finally {
        if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
    }
    foreach ($suite in @(@('AX_ShopSystemProbeSuite', 4), @('AX_TradeCoreSuite', 11), @('AX_PhoneSuite', 6))) {
        $profile = Join-Path $runRoot $suite[0]
        New-Item -ItemType Directory -Path $profile | Out-Null
        $stdout = Join-Path $profile 'stdout.log'
        $stderr = Join-Path $profile 'stderr.log'
        $arguments = '-addonsDir "' + $addonRoot + '" -addons 4E6001DE6EAF45DC -autotest ' + $suite[0] + ' -profile "' + $profile + '" -logStats 0'
        $process = $null
        $suiteResult = [ordered]@{ name = $suite[0]; expected = $suite[1]; result = 'RUNNING'; profile = $profile }
        $evidence.suites += $suiteResult
        try {
            Write-Output "Running $($suite[0]): $profile"
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
            if ($logs -match 'Can.t compile|SCRIPT\s+\(E\): @|Virtual Machine Exception|RPL\s+\(E\)|component.*cannot be combined|Wrong GUID/name.*4E6001DE6EAF47') { throw "Compile/runtime/resource error in $profile" }
            if ($logs -notmatch "Adding package '[^']*ShopSystem[/\\]'" -or $logs -notmatch 'name=ProjectAxum.ShopSystemProbe') { throw 'Optional package/probe did not mount; cannot infer compatibility.' }
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
    Write-Output 'PASS: 4 optional shop cases, 11 appointment cases and 6 phone cases.'
} catch {
    $evidence.result = 'FAIL'
    $evidence.error = $_.Exception.Message
    throw
} finally {
    $evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $runRoot 'evidence.json') -Encoding UTF8
    Write-Output "Evidence: $runRoot\evidence.json"
}
