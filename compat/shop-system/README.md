# Optional Shop System NPC probe

Development remains on `codex/drug-trade-foundation`. This addon is isolated
from `axum/ProjectAxum.gproj`; loading the normal Axum project does not load it.
It depends on Shop System **1.0.18** (`5D2D1436D1FA5A13`) and the existing Axum
test addon, preserving EveronLife and the pinned EPF/EDF source stack.

## What is built

- An actual FIA character prefab, `Prefabs/AX_ShopProbeBuyer.et`, inheriting the
  standard loadout as a temporary visual fixture. It is not the final buyer art.
- A derived shop component with one sell-only field-dressing fixture and a
  nominal displayed price. No joint or brick assets/production are implemented.
- A custom sell-only action that avoids the stock buy-catalog opening gate.
- A derived shop menu that blocks checkout before any stock RPC and adds an
  original heading because the installed layout's inherited heading is absent
  on the tested UI templates. Both purchase and sale dispatch on the component
  also return false before stock inventory/payment code, even if called directly.
- A probe-only menu configuration retaining the Axum phone and Shop System
  presets. No input configuration or player-controller shop-manager patch.

There is no phone-to-NPC spawning hookup, authenticated appointment transport,
per-player allowance persistence or real settlement. Do not treat this component
as a production appointment adapter. The future adapter must derive player
identity and position on the server, enforce the reserved order and player
allowance, and use Axum's recoverable settlement.

## Reproduce the native probe

From the repository root, adjust these installed paths as needed:

```powershell
.\tools\Invoke-ShopSystemCompatibility.ps1 `
  -ServerExe 'D:\Steam\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe' `
  -WorkbenchExe 'D:\Steam\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe' `
  -GameDirectory 'D:\Steam\steamapps\common\Arma Reforger' `
  -CandidatePath 'C:\Users\grave\OneDrive\Documents\My Games\ArmaReforger\addons\ShopSystem_5D2D1436D1FA5A13'
```

The runner verifies package identity/version and clean dependency pins, copies
current working sources and the installed package into a new ignored
`.local/shop-system-compat/run-*` directory, and omits existing source RDB caches.
It starts a hidden disposable Workbench against that snapshot, waits for startup
and resource scanning to finish, and stops only its own process. It then runs
three diagnostic-server suites with separate profiles. Original source and
installed package files are not modified. The temporary Workbench does not open
or save the user's world; it does use the tools' normal per-project temp caches.

Pass requires package mounting, expected native JUnit counts, no failures,
errors or skips, and no compiler/VM exception or probe resource/replication
error. Output profiles, source hashes and package hash remain in `evidence.json`.
The JUnits and compact receipt for the verified run are in
`docs/axum/validation/2026-10-15-shop-system/`.

## Verified and pending

**Native 1.8.0.13, pinned stack: 21 passing cases.** Four probe cases verify
resource loading, actual character construction with the sell-only catalog and
registered action/context, shop menu/tab/heading binding and blocked checkout,
and base-typed transfer dispatch rejection. Eleven core and six phone cases
remain passing with the optional package loaded.

The diagnostic main-menu world lacks the graphical item-preview service.
The probe therefore binds tabs but does not populate item rows when that
service is absent. A missing service is not evidence that rendering works.
Trying to instantiate the graphics service in this headless environment caused
a native crash during investigation; that experiment was removed. Dummy-renderer
texture/input warnings and a font resource-leak report remain in the logs.
Shop System also emits a duplicate replication-registration warning on character
construction. These passes do not establish a clean graphical or network run.

**Next graphical check:** open the probe from the runner's disposable addon
snapshot with its `addons` directory available. Use a separate saved copy of a
known playable debug world, with a local character and an ItemPreviewManager.
Do not edit/save into EveronLife's world. Place `AX_ShopProbeBuyer.et`, enter
Play, approach the buyer and use **Preview buyer shop (no transfers)**. Verify:

1. The interaction appears and opens the sell-only fixture with a readable name,
   quantity, price and preview heading.
2. Adding the fixture to the cart and attempting checkout exchanges nothing.
3. Closing the shop restores movement; the F6 phone still opens/closes.
4. Stopping Play with the shop open returns to the editor without an assertion.

Record the exact baseline/package sources used for that graphical run. The
repository's EveronLife debug-world file is an empty placeholder; the owner's
earlier phone check used the installed Workshop world. Do not assume those are
the same runtime stack. Graphical visibility, item rows, player-driven action
execution, AI/navigation, multiplayer ownership/proximity, appointment spawning,
real goods/payments and restart recovery are **pending**.

## Integration decision

Keep this as an optional probe until the graphical and multiplayer checks pass.
The smallest candidate integration is the shop interaction/UI behind an Axum
adapter. Do not import the stock selling/payment flow: static inspection shows
goods collection before payment, payment errors only logged, and eventual
success returned despite a failed payment. Post-sale callbacks cannot repair
that commit boundary. Keep the EveronLife baseline, custom phone contacts,
authored rotating sites, joint/local and brick/bulk lanes, per-player sales
allowances, property processing and durable recovery architecture.

See [third-party provenance](THIRD_PARTY_NOTICE.md).
