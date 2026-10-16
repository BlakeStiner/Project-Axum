# Optional Drugs Economy compatibility probe

This project is intentionally absent from Axum's active dependencies. It depends
on Axum's test addon and Drugs Economy solely for isolated evaluation. The engine
co-loads all candidate scripts; it does not selectively load just the art.

From the repository root in PowerShell:

```powershell
$candidate = 'C:\Users\grave\OneDrive\Documents\My Games\ArmaReforger\addons\DrugsEconomy_692677B5062F640E'
$diagnostic = 'D:\Steam\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe'
.\tools\Invoke-DrugsEconomyCompatibility.ps1 -ServerExe $diagnostic -CandidatePath $candidate -Stack Pinned
.\tools\Invoke-DrugsEconomyCompatibility.ps1 -ServerExe $diagnostic -CandidatePath $candidate -Stack Declared
```

`Pinned` requires the existing clean framework submodules at their recorded SHAs.
`Declared` copies the installed EPF `0.6.18` and EDF `0.6.10` packages from the
candidate's parent directory; use `-InstalledAddonRoot` if these live elsewhere.
No dependency downloads, package rewrites or submodule changes are performed.
Uncommitted Axum/EveronLife working files are copied into the test snapshot, and
remain untouched in the checkout. Source hashes in evidence identify the snapshot.

Each invocation creates a new ignored `.local/drugs-economy-compat/` directory.
All addon sources/packages are copied, not mounted through links to installed
packages. Profiles and engine resource-cache writes stay in that directory.
There is one framework version per GUID per invocation. The runner starts the
diagnostic server hidden and stops only its own child process on timeout/error.
No world, hosted playable scenario, live database configuration or player game
is launched. It never sends Discord messages.

A pass requires both native JUnit reports: 11 cases from `AX_TradeCoreSuite` and
one eight-reference resolution case from `AX_DrugsEconomyResourceSuite`, with no
failures/errors/skips. It also checks that the candidate and probe mounted.
The process exit code alone is insufficient. A failure retains evidence and logs
and exits with an error; inspect staging versus compile/test errors separately.

The resource probe does not spawn entities or instantiate UI widgets. References
that resolve on a dummy renderer still require graphical and multiplayer checks.
Opening the supplied Drugs Economy scenario is a separate manual test, not part
of this runner. Do not point it at production profiles or live persistence data.

Each run copies approximately 608 MiB of candidate data. Evidence JSON contains
local paths and source hashes; review it before publishing outside the project.
See the [assessment](../../docs/axum/DRUGS_ECONOMY_ASSESSMENT.md) for results and
the remaining gameplay/failure-injection matrix.
