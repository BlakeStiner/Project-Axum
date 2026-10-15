# EveronLife source audit

_Audit date: 2026-09-12. Scope: static source, project metadata, licensing,
dependencies, and local toolchain availability. This is not a runtime or
security certification._

## Recommendation

**Use EveronLife as a source-controlled baseline fork for RP primitives—not
as a drop-in Workshop dependency or as the server's finished identity.**

Its architecture matches our philosophy: it separates reusable RP mechanics
from server-specific content, has a permissive MIT license, and expects server
owners to create their own custom scenario mod. However, it is a pre-alpha
framework with an old public compatibility target, no meaningful automated
test suite, and many overrides of base inventory/UI classes. We should keep
our custom server module independent and migrate a module only after it
compiles and passes a dedicated-server test against the current Reforger tools.

## Audit status

| Check | Result | Evidence |
|---|---|---|
| Source isolated from project code | Pass | `third_party/EveronLife` clone; no files merged into our own module. |
| License allows fork/alteration | Pass | `LICENSE.txt` is MIT. Preserve the copyright/license notice in every substantial copy. |
| Project shape understood | Pass | 419 source files, 57 game scripts, a debug world, a roleplay mode, source and test projects. |
| Dependencies understood | Pass | Vanilla Reforger plus Enfusion Persistence Framework; EPF itself depends on Enfusion Database Framework. |
| Current Workbench compile | Pass | Compiled successfully in Workbench against Reforger 1.8.0.13 on 2026-09-12; no EveronLife compile errors. |
| Local DebugWorld smoke test | Pass | Spawned, moved, and completed the apple-to-juice gather/process chain in the unmodified DebugWorld on 2026-09-12. |
| Dedicated-server persistence/duplication test | Pending | Requires a clean compile plus a disposable dedicated-server test setup. |
| Automated regression suite | Insufficient | Two test scripts provide a test runner/result wrapper; no actual feature test cases were found. |

## What the framework already provides

| Area | Static evidence | Fit for our project |
|---|---|---|
| Roleplay game mode and spawn flow | `EL_GameModeRoleplay`, `EL_SpawnLogic`, roleplay prefabs and mission header | Useful architecture reference; our scenario/map should remain independent. |
| Persistent player account/character selection | `EL_PlayerAccount`, `EL_PlayerAccountManager`, EPF save data | Good conceptual starting point for our one-primary-character alpha policy. It does not implement our full identity, permissions, CAD, or staff model. |
| Cash item and virtual quantities | `EL_MoneyUtils`, `EL_QuantityComponent`, persistence adapter | Potentially useful, but cash is inventory-based and has no ledger/banking/anti-fraud system. Prefer our own transaction-boundary design. |
| Gathering and processing | `EL_GatherAction`, `EL_ProcessAction`, debug-world resource chains | Good reference for configurable world actions. Our version must add player demand, routing, risk, audits, and anti-grind design. |
| Trader/barter interaction | `EL_TraderManagerComponent` and inventory override | Only a simple item-for-item trade. It is not a player economy, shop, price, stock, or contract system. |
| Vehicle license plates | Plate manager/generator/entity scripts | Possible future standalone feature after compatibility and replication testing. |
| Inventory/hand-carry UI | Quantity, split, hand-carry, and UI patches | Highest compatibility risk; it modifies core classes and should not be adopted early. |

## Compatibility and design risks

1. **Version gap.** The latest public EveronLife release targets Reforger 1.1,
   while current Workshop candidates are built for 1.8. Its public repository
   itself labels the mode heavy-development/not ready to play.
2. **Override footprint.** The source contains 15 `modded class` declarations
   and approximately 75 override declarations. Most center on inventory,
   storage, UI, character controls, and callbacks—areas likely to change
   between game versions and conflict with other mods.
3. **Testing gap.** The included test project contains only the generic
   `TestHarness` runner and result wrapper; no account, quantity, trade,
   persistence, or multiplayer regression test cases were located.
4. **Economy gap.** Cash is a stackable inventory item; the trader is simple
   barter. There is no bank ledger, business accounting, tax/fine model,
   supply/demand system, job contracts, or CAD integration.
5. **Authority must be proven, not assumed.** Resource actions and quantity
   transfers rely on replication and inherited EPF helpers. The quantity RPC
   handlers visibly apply a proximity guard, but we need multiplayer tests for
   ownership, remote item manipulation, duplicated transfers, reconnects,
   disconnects during transactions, and restart recovery.
6. **Framework branding/content.** The included pre-alpha mission targets
   `MainWorld`/Everon and contains EveronLife assets and labels. We must not
   ship its scenario/branding as our server identity.

## Adoption boundary

### Eligible to evaluate first

- Account/character persistence pattern.
- Spawn customization pattern.
- Configurable gathering and processing actions.
- License-plate system, later and separately.

### Keep custom from day one

- Economy ledger, bank/cash boundaries, taxes/fines, businesses, contracts,
  legal work, crime chains, and anti-duplication controls.
- Police/Sheriff/EMS systems, CAD backend, website, Discord integration,
  permissions, audit logging, rules, department workflows, and all county
  world content/branding.

### Defer until late compatibility testing

- Quantity stacking/splitting, inventory UI modifications, hand carry, and
  trader override. These alter shared engine/UI behavior and can conflict with
  other mods.

## Runtime validation plan

Once Arma Reforger Tools is installed, run this exact sequence in a disposable
test environment:

1. Add `EveronLife.gproj`, EPF, and EDF as Workbench projects; compile with
   the current stable game/tools version. Record every error before editing.
2. Launch the supplied debug world with two local peers; verify spawn,
   character persistence, gathering, processing, basic trade, cash quantity,
   and inventory split/merge.
3. Run a dedicated server and repeat the same workflow with two remote
   clients. Test disconnect/reconnect and a controlled restart at each step.
4. Run adversarial cases: simultaneous item transfer, full-inventory trade,
   split/transfer at range, reconnect during action, server restart during
   save, and mod version update/rollback.
5. Fork or extract only modules that pass. Start every forked source file with
   the retained MIT notice and list EveronLife attribution in our repository
   and Workshop description.

### Preparation update — 2026-09-12

Arma Reforger Tools 1.8.0.13 is now installed. The audit clones for EDF, EPF,
and EveronLife are present under `third_party/`; see
`WORKBENCH_VALIDATION_SETUP.md` for the exact project-registration order. The
compile was run manually because the desktop automation bridge cannot attach
to the running Workbench window. No source changes were needed.

### Compile result — 2026-09-12

The unmodified EveronLife source compiled successfully in Workbench on
Reforger **1.8.0.13**. The log created the game instance successfully. The
only warning attributed to EveronLife was an unused `skipMove` variable in
`Scripts/Game/Feature/Quantity/SCR_InventoryMenuUI.c`; this is non-blocking.
The remaining reported obsolete API warnings were from vanilla Reforger
sources. This resolves the compile-compatibility gate only; multiplayer,
persistence, and authority tests remain required.

### DebugWorld smoke-test result — 2026-09-12

The unmodified EveronLife `DebugWorld.ent` launched locally on Reforger
1.8.0.13. A test player spawned, moved, and completed the provided
apple-to-juice gathering/processing chain. This validates the basic project
registration, game mode, spawn flow, inventory interaction, resource action,
and processing action in a single local session. It does **not** validate
multiplayer authority, persistence across restart, account recovery, or
duplication resistance.

## Decision

**D-009 decision: approved baseline, gated.** EveronLife is the selected
source-controlled baseline for the server's RP primitives. It may be forked
into an Axus-owned project only after the multiplayer, persistence, transfer,
disconnect, restart, and duplication validation plan passes on the current
toolchain. Axus remains independently responsible for gameplay loops,
economy, CAD, branding, assets, and scenario design. If a module fails those
tests, replace or remove that module rather than inherit the failure.

## Relevant source paths

- `src/EveronLife.gproj` — project dependencies.
- `src/Scripts/Game/Feature/Account/` — persistent account/character pattern.
- `src/Scripts/Game/Feature/Quantity/` — quantity/transfer/stacking changes.
- `src/Scripts/Game/Feature/Gathering/` and `Processing/` — configurable
  resource actions.
- `src/Scripts/Game/Components/InventorySystem/` — trader and inventory
  overrides.
- `tests/Scripts/Game/` — current test harness only.
