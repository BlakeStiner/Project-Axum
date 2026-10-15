# Project Axum — development baseline

_2026-10-14. Branch: `codex/drug-trade-foundation`._

## Source layout

| Path | Role |
|---|---|
| `src/EveronLife.gproj` | EveronLife baseline retained in this fork |
| `axum/ProjectAxum.gproj` | Custom addon; depends on EveronLife and vanilla Reforger |
| `dependencies/EnfusionDatabaseFramework/` | EDF, pinned Git submodule |
| `dependencies/EnfusionPersistenceFramework/` | EPF, pinned Git submodule |
| `docs/axum/` | Axum design, historical audit, and this setup guide |
| `tests/` | Existing upstream test project, not an Axum regression suite |

This repository is already an EveronLife fork. Do not add a second copy of
EveronLife as a submodule. Its baseline is the inherited commit
`d430473ccb7824064d1a871fbe5407f52d7017be`. Keep custom gameplay in `axum/`
so future upstream fixes can be reviewed independently.

## Reproduce the checkout

```powershell
git clone --branch codex/drug-trade-foundation --recurse-submodules https://github.com/BlakeStiner/Project-Axum.git
cd Project-Axum
git submodule status
```

For an existing checkout of this branch:

```powershell
git submodule update --init --recursive
```

Expected pins:

| Dependency | Commit | Project GUID |
|---|---|---|
| EDF | `9a92e9a3aad75704f6d35bf543563f373a731a1a` | `5D6EA74A94173EDF` |
| EPF | `cbf255fd8448e1c2235859797c70e1464ebe896f` | `5D6EBC81EB1842EF` |
| EveronLife baseline | `d430473ccb7824064d1a871fbe5407f52d7017be` | `59636E668EA37AD7` |
| Project Axum addon | This repository's `axum/` | `4E6001DE6EAF45D9` |

Submodules normally use a detached HEAD at their pinned commits. Do not use
`git submodule update --remote` for routine setup: it changes the dependency
versions under test. Dependency updates need their own validation and commit.
Git history and gitlinks are the version lock; Workbench GUID dependencies
identify projects but do not pin their source versions.

## Register projects in Workbench

With Arma Reforger, Reforger Tools, and the vanilla game project available,
use **Add Existing** in this order, relative to the repository root:

1. `dependencies/EnfusionDatabaseFramework/src/EnfusionDatabaseFramework.gproj`
2. `dependencies/EnfusionPersistenceFramework/src/EnfusionPersistenceFramework.gproj`
3. `src/EveronLife.gproj`
4. `axum/ProjectAxum.gproj`

Another registered checkout or Workshop copy of any of these projects may use
the same GUID. Resolve each GUID to this checkout's pinned source during this
test; do not load two different EveronLife builds together. Leave the original
audit checkout on disk intact. Registering a source checkout does not publish
a Workshop dependency or provision a dedicated server.

Open Project Axum and compile. Record exact client, Tools, server, and dependency
versions plus diagnostics. Then load EveronLife's
`Worlds/DebugWorld/DebugWorld.ent` from the Resource Browser and run the existing
gather/process flow. The custom addon currently has no mission of its own.

## Validation status and next deliverable

Historical evidence from a separate local checkout reports successful compile
and single-player gather/process testing on Reforger 1.8.0.13, 2026-09-12.
This branch starts from that same EveronLife commit and dependency pins. It
has not yet been compiled or run in Workbench; matching source alone does not
certify the new addon or current installed game version. See the
[historical audit](EVERONLIFE_AUDIT.md).

Before building the sale against these primitives, use upstream
[PeerTool setup](../development_setup.md#setting-up-the-peer-tool) to test two
clients with the actual dependency source paths supplied for the test setup.
Keep test profiles disposable and separate from live player data.

- [ ] Compile the complete addon/dependency graph on the installed toolchain.
- [ ] Two clients spawn, gather, process, and transfer items consistently.
- [ ] Disconnect/reconnect preserves the intended character and inventory.
- [ ] Restart restores storage, cash, and transferred quantities once.
- [ ] Simultaneous transfers, full storage, and disconnect during transfer
      cannot duplicate goods or cash.

The first Axum feature after the foundation checks is one contact-arranged land
sale: basic conversation UI, finite order, private changing meeting site,
temporary buyer, sealed test package, and one durable payment. Follow the
[architecture's acceptance gates](DRUG_TRADE_ARCHITECTURE.md#implementation-sequence-and-acceptance-gates)
before adding property jobs, forest bulk orders, or boats.

## Attribution and upstream changes

EveronLife's MIT notice remains at [LICENSE.txt](../../LICENSE.txt). EDF and
EPF retain their own licenses inside their submodule checkouts. Preserve those
notices when distributing copies. Keep EveronLife project/resource GUIDs and
original assets intact; Project Axum has its own project GUID. This branch
does not publish the upstream example scenario as Axum content.

EveronLife's [custom-server guide](../custom_server.md) describes the separate
addon/dependency approach. Its Workshop publishing and server examples are
upstream references; this development baseline uses the pinned source graph
above and is not a production deployment configuration.
