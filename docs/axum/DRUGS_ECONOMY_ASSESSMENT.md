# Drugs Economy optional dependency assessment

Assessed October 14, 2026 (native evidence timestamps cross into October 15 UTC).
Decision: retain EveronLife and both framework pins. Keep Drugs Economy optional.
The smallest useful next integration is a separate visual adapter for one harvested
inventory product and one mature plant, backed by Axum's own gameplay components.
Do not connect its existing sales, payment, change, or harvest actions to Axum's
production economy unchanged. No production dependency or third-party asset was
added by this assessment.

## Evidence and package identity

This assessment follows the side-conversation audit's asset/UI-only scope and
independently checks its transaction failure paths against the installed archive.

- Installed source: `C:/Users/grave/OneDrive/Documents/My Games/ArmaReforger/addons/DrugsEconomy_692677B5062F640E`.
- [Workshop source, Rxndytv](https://reforger.armaplatform.com/workshop/692677B5062F640E).
- Local `ServerData.json`: Drugs Economy `0.1.62`, game target `1.7.0.41`;
  EPF `0.6.18` and EDF `0.6.10` requirements. The project itself lists vanilla
  and EPF by GUID; EDF is also pulled transitively through EPF.
- Archive: 635,729,249 bytes, 205 entries. The complete installed package is
  approximately 608 MiB; referencing two resources does not reduce that download.
- Archive SHA-256 and input source hashes are in the linked runtime evidence.
  [Package inventory](validation/2026-10-14-drugs-economy/package-inventory.json)
  records paths and archive entry sizes, without redistributing source or assets.

## What could save development time — static findings

Paths below are relative to the candidate's `data.pak`, not this repository.
Binary presence and prefab wiring establish candidates; appearance, animation,
collision, LODs and performance have not been visually verified.

| Area | Concrete resources | Useful reuse and limits |
| --- | --- | --- |
| Inventory product | `Prefabs/Items/Drugs/CannabisExtract.et`; `Assets/Props/Cannabis Extract/Source/Cannabis.xob` | Ready mesh reference, display/preview attributes, inventory size/weight, replication and placement example. Its item component extends `SCR_PlaceableInventoryItemComponent`; its EPF save-data configuration is not a transaction journal. Replace the gameplay binding with an Axum product/batch identity. Spanish display strings need localization. |
| Alternate product visuals | `Assets/Props/Cannabis Plant/Source/Small Weed Bag.xob`, `WeedBag2.xob`, `Weed Brick2.xob`; `Assets/Props/Decoration/Source/Drug Pack.xob` | Bag/brick/bulk-package candidates could avoid initial modeling. These are model resources, not ready Axum inventory prefabs. New prefab, material dependency, scale, collision, preview and provenance checks remain. No other drug production chain is established by these filenames. |
| Plants | `Prefabs/World/Plants/CannabisPlant_Mature.et`; `Assets/Props/Cannabis Plant/Source/PlantWorld_texture.xob` | Mature plant visual and harvest-interaction example. The mature prefab includes `DE_CannabisHarvestAction`; do not inherit that action into Axum production. No seed-to-growth-to-processing system is supplied by this prefab. |
| Plant placement examples | `CannabisPlantSpawnPoint.et`, `CannabisPlantSpawnZone.et` under `Prefabs/World/Plants/`; `DE_CannabisPlantSpawnPointComponent.c`, `DE_CannabisZoneSpawnerComponent.c` | Server-side placement, terrain alignment, spacing attempts, initial counts and periodic refill are useful references. They do not enforce Axum property permissions, cultivation budgets, growth state, batch provenance or restart recovery. |
| Currency | `Prefabs/Items/Currency/Bill_100.et`, `Bill_200.et`, `Bill_500.et`, `Bill_1000.et`; corresponding `Assets/Props/Currency/` meshes/materials/textures | Four physical-bill examples with values, placement, previews and persistence components. Labels are MXN despite some model filenames saying Dollar. Actual appearance is unverified. Defer these until Axum has one authoritative money representation; importing the visuals must not introduce a second wallet or cash conversion path. |
| Shop UI | `UI/Layouts/Markets/Layouts/BM_Main.layout`, `BM_ItemRow.layout`, `BM_Header.layout`, `BM_Categories.layout` | Catalog panel, item preview, quantity buttons, cart total, wallet/status and focus patterns can save UI layout work. Static layout inspection only; no claim about readability, controller navigation or runtime rendering. This is a shop layout, not a contacts/texting/appointment UI. |
| Shop controller | `Scripts/Game/UI/NEW_BlackMarketMenu.c`, `DE_BlackMarketContext.c`, catalog and purchase-line/result classes | Snapshot-to-catalog mapping, row bindings, preview setup and pending/status feedback are examples. Controller directly calls `DE_BlackMarketComponent`, calculates a bill-based wallet and uses weapon categories. Rebuild the binding to Axum quotes/order IDs; do not route confirmation into the existing payment RPC. A ten-second UI timeout is not durable recovery. |
| World/examples | `Prefabs/Examples/BlackMarket_Example.et`, `MilitaryShop_Example.et`, `CannabisSellPoint_Example.et`, `GovermentConfiscatePoint_Example.et`, `FactionLocker_Example.et`; `Missions/DE_Economy_Test.conf` | Useful component/editor wiring and a four-player smoke-test scenario. Fixed sell/confiscation points do not fit rotating appointments. Military catalogs add little to this milestone. Faction locker illustrates EPF inventory saving, not property ownership or atomic settlement. |
| Documentation/props | Three `Documentation/DE_*System_Instructions.c` files; locker, chest, crate, table and pallet resources | Useful setup references and dressing candidates. Some prefabs wrap vanilla props, so their marginal saving is small. No purchased-property processing machine or property permission system was identified. |

Some binary resources are large: the mature plant mesh entry is 45,249,018 bytes;
an alternate `bud_u1_v1.edds` entry is 89,478,772 bytes. These are archive-entry
sizes, not measured GPU memory, texture resolution, draw calls or a dependency
closure. Do not bundle every source variant to obtain one item.

Only one `modded` declaration was found among the candidate's script files:
`modded enum ChimeraMenuPreset`. That reduces the apparent global patch surface,
but the package also supplies `Configs/System/chimeraMenus.conf`. Full-package
loading still loads all its scripts and configuration, even when no shop is placed.

## Transaction audit — static, not reproduced exploits

Line references refer to unmodified installed `0.1.62` script text.

| Code | Verified control flow | Axum decision |
| --- | --- | --- |
| `Scripts/Game/UserActions/DE_SellCannabisAllAction.c:399–435` | Deletes goods sequentially. A later deletion failure returns without restoring earlier deleted goods. Payment is attempted afterwards; its returned boolean is logged without goods restoration or a durable pending payout. `SpawnPaymentBills:222–236` can complete one denomination before another fails. | Exclude this settlement action. Preflight inventory checks do not reserve capacity or repair partial mutations. |
| `Scripts/Game/UserActions/DE_BlackMarketComponent.c:736–771` | Purchase delivery has a spawned-item rollback path, but payment failure rolls back purchased goods without restoring cash already removed. `DeductPaymentBills:1109–1128` and `DeleteBillsByPrefab:1131–1176` can return after earlier successful deletions. | Do not reuse as a payment adapter unchanged. |
| Same file, `1023–1052` | `GiveChangeBills` ignores individual spawn results; its caller returns `SUCCESS` at line 771 after attempting change. | No reliable all-or-nothing cash/change receipt. |
| Same file, authorization and request handling | One authorized-player slot and a client-supplied player ID require sender-binding/concurrency validation. Static checks alone do not demonstrate impersonation or an exploitable race. | Audit authenticated RPC ownership with two clients before any UI controller reuse. |
| `Scripts/Game/Locker/DE_FactionLockerComponent.c:215–249` | Calls `EPF_PersistenceComponent.Save(readResult)`. | Persisting a locker does not atomically commit goods plus money or resolve a restart between steps. |

The harvest action does check server authority, inventory insertion and a local
per-plant guard before deleting the plant. Those are useful references, not proof
of durable exactly-once harvest/provenance across disconnects or restarts.

## Framework comparison

| Framework | Axum authoritative Git pin | Installed declared package | Static script comparison |
| --- | --- | --- | --- |
| EPF, `5D6EBC81EB1842EF` | `cbf255fd8448e1c2235859797c70e1464ebe896f` | `0.6.18` | All 64 `.c` files identical after UTF-8 BOM removal and CRLF-to-LF normalization; none added/removed. |
| EDF, `5D6EA74A94173EDF` | `9a92e9a3aad75704f6d35bf543563f373a731a1a` | `0.6.10` | All 31 `.c` files identical under the same normalization; none added/removed. |

The closest locally available tags describe the pins as `0.6.15-8-gcbf255f` and
`0.6.8-3-g9a92e9a`; these are Git ancestry descriptions, not installed Workshop
release versions. A nominal requirement difference is not evidence of an API
conflict. In particular, the inspected persistence/save-data APIs compile in both
test configurations. This comparison does not assert byte identity for every
config/resource or certify how the Workshop resolver will select versions.

[Comparison manifest](validation/2026-10-14-drugs-economy/framework-comparison.json)
contains per-script normalized hashes and packaged archive hashes. No framework
upgrade, submodule ref change or replacement baseline is justified by these results.

## Verified native results

Diagnostic server **1.8.0.13**, dummy renderer, no scenario world, fresh profiles.
The probe explicitly loads EveronLife, Axum, Axum tests and Drugs Economy together.

| Matrix | Result | Recorded evidence |
| --- | --- | --- |
| Candidate `0.1.62` + pinned EPF/EDF sources | PASS: all scripts co-loaded; 11 appointment-core cases; one case resolved eight candidate prefab/layout references | [Inputs](validation/2026-10-14-drugs-economy/pinned-evidence.json), [core JUnit](validation/2026-10-14-drugs-economy/pinned-AX_TradeCoreSuite-junit.xml), [resource JUnit](validation/2026-10-14-drugs-economy/pinned-AX_DrugsEconomyResourceSuite-junit.xml), [log excerpts](validation/2026-10-14-drugs-economy/pinned-runtime-excerpts.txt) |
| Candidate `0.1.62` + copied EPF `0.6.18` / EDF `0.6.10` Workshop packages | PASS: same 11 core cases and eight resource references | [Inputs](validation/2026-10-14-drugs-economy/declared-evidence.json), [core JUnit](validation/2026-10-14-drugs-economy/declared-AX_TradeCoreSuite-junit.xml), [resource JUnit](validation/2026-10-14-drugs-economy/declared-AX_DrugsEconomyResourceSuite-junit.xml), [log excerpts](validation/2026-10-14-drugs-economy/declared-runtime-excerpts.txt) |

References resolved: extract, mature plant, four bill prefabs, item-row layout and
the menu preset's `{D38DDB4EC9E4EED3}UI/Layouts/Markets/New2/BM_Main.layout`.
The archive lists the main layout under `Layouts`, but its supplied GUID reference
resolved in both runs. A stale path string alone therefore is not a confirmed bug.

An initial declared-stack attempt stopped during staging because the first runner
assumed every Workshop project was named `addon.gproj`. Framework projects use
their own names. The corrected runner validates the single project's GUID; the
successful rerun above is the native compatibility result. That staging error is
not counted as a framework or game failure.

These tests do **not** spawn or render the models, create widgets, execute a shop
sale, connect clients, validate a database migration, or fault-inject the candidate's
payment implementation. Axum core tests use fake settlement ports. Co-loading and
resource resolution do not prove production economy safety. Headless startup UI
warnings remain possible; pass requires native JUnit and no detected script
compile/VM failure, not a clean renderer log.

## Smallest integration and architecture boundary

1. Keep the full candidate behind the optional probe only. Active
   `axum/ProjectAxum.gproj`, EveronLife sources and submodule pins stay unchanged.
2. Build an optional Axum visual adapter mapping canonical Axum product IDs to
   one inventory mesh and one plant mesh. Prefer a separately packaged minimal
   resource closure when ready to distribute, rather than a 608 MiB gameplay mod
   dependency for two visuals. Check material/texture references, origin credits,
   scale, collision, LODs and rendered previews first. No extraction/admission is
   performed in this change.
3. Use new Axum-owned prefabs/components. Avoid inheriting DE sell/harvest actions,
   bill-valued wallet behavior, automatic world replenishment or property-agnostic
   persistence. Currency art and extra model variants remain deferred.
4. If adapting shop layouts later, bind an Axum view model to server-issued quote,
   order and transaction IDs. The server owns prices, finite demand and budget,
   authenticated character identity, proximity and state transitions. The existing
   menu controller must not dispatch the DE purchase/payment path.
5. Retain custom contacts/texting, temporary rotating land/forest/offshore NPC
   appointments, regional cooldowns and owned-property processing. A processing
   job needs machine placement permission, inputs, duration and durable output
   provenance; an item or plant prefab does not implement these rules.
6. Implement durable transaction intents/receipts and inventory/money reservation
   or escrow before connecting real mutations. Resume/reconcile an uncertain
   operation by transaction ID; never blindly pay or remove goods again. Existing
   session `RECOVERY_REQUIRED` locking remains a boundary, not restart recovery.

## Remaining isolated checks before adoption

- In Workbench, open the probe with the exact recorded stack. Render extract,
  plant, bags/brick and bills; inspect materials, LODs, collision, inventory previews,
  dropped scale and placement on property furniture rather than terrain only.
- Create the shop layout in a client world; check localization, item previews,
  focus/navigation, wallet text, status, close/reopen and both keyboard/controller.
  Keep its purchase button disconnected from live payment while evaluating UI.
- Dedicated server plus two clients: simultaneous harvest/appointment requests,
  forged player IDs, out-of-range requests, reconnects, inventory capacity changes
  and replicated NPC cleanup. Use disposable data and no live player economy.
- With the eventual Axum transaction adapter, inject failure after each item/cash
  mutation, delivery/change insertion failure, duplicate RPCs, timeout, disconnect
  and restart. Verify conservation and one reconciled receipt, including multiple
  bills and partial deletions. No deployment claim until these pass.

See [probe instructions](../../compat/drugs-economy/README.md) and the
[third-party notice](../../compat/drugs-economy/THIRD_PARTY_NOTICE.md).
