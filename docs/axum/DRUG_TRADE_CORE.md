# Drug Trade Framework — first code milestone

_2026-10-14. Development stays on `codex/drug-trade-foundation`._

## Implemented

The custom addon now contains an isolated Enforce Script appointment service.
It is a session-only domain layer, not a playable drug-sales mission.

| Area | Current behavior |
|---|---|
| Buyer definitions | Product ID, fixed quoted unit price, max order size, finite demand and budget |
| Booking | One active order per character; quantity/price validation; demand and cash-budget reservation |
| Meeting selection | Authored sites validated by an environment adapter; region selected before site to avoid density bias |
| Camping resistance | Whole-region reservation, region cooldown after completion/cancellation/expiry, character request cooldown |
| Readiness | Settlement is blocked until an encounter adapter explicitly marks the buyer ready |
| Access | Owner-only appointment reads; character and distance checks before settlement |
| Order lifecycle | Booked, ready, settling, completed, cancelled, expired, recovery required |
| Definite rejection | Retains appointment/reservations and permits retry after the cause is corrected |
| Ambiguous settlement | Locks the order and reservations; cannot auto-refund, expire, or retry |
| Session idempotency | Completed-order retries return the existing outcome without another settlement call |
| Defensive bounds | Overflow-safe quotes, monotonic time, copied caller-facing terms, 4096 retained order records per service |

No global service, RPC, inventory patch, cash grant, automatic replenishment,
map marker, or production startup hook is installed. Base environment checks
return false and base settlement returns rejected; game adapters must opt in.
The [phone preview](PHONE_PREVIEW.md) adds a restricted Workbench environment
adapter for test appointments; it retains the rejecting settlement port.
Simulated successful/ambiguous settlements exist only in the test addon.

## Code boundaries

- `axum/Scripts/Game/Axum/DrugTrade/AX_TradeTypes.c`: definitions, order copy,
  state/result enums, environment and settlement ports.
- `axum/Scripts/Game/Axum/DrugTrade/AX_TradeService.c`: reservation and appointment
  lifecycle, site selection, expiry and completion guards.
- `axum-tests/`: test-only addon and native `AX_TradeCoreSuite`.
- `tools/Invoke-TradeCoreTests.ps1`: isolated diagnostic-server compile/test run.

`AX_TradeService` is intended to be owned exclusively by an authoritative server
adapter. Its API accepts trusted character IDs, positions, RNG rolls, and
monotonic server seconds so domain tests can be deterministic. Those values are
**not** safe RPC inputs. The future transport must derive identity/position
from the authenticated controlled entity and rate-limit requests server-side.
Public results intentionally use coarse availability failures rather than
exposing occupancy, police locations, or other sellers' appointments.

The settlement port receives a copied order with a stable operation ID. A real
adapter must validate actual goods, provenance, inventory capacity, buyer
readiness, distance, and deadline at commit time; reserve/lock its assets; and
durably resolve the operation exactly once. It may return `COMMITTED` only
after durable confirmation, `REJECTED` only when no economic mutation occurred,
or `UNKNOWN` when the outcome cannot safely be inferred. Async inventory work
must not be disguised as a synchronous successful commit.

The service does not generate a production-safe session ID. Its caller must
supply a non-reused instance ID before using operation IDs outside tests.
Receipts, cooldowns, and demand are currently in memory. Starting a new service
resets them; live settlement remains disabled until durable storage and recovery
replace that behavior. There is no administrative recovery operation yet.

## Native validation

The full source graph compiled on **Arma Reforger diagnostic server 1.8.0.13**.
The native suite ran **11 cases with no failures, errors, or skips**:

1. One successful settlement despite repeated completion requests.
2. Cancellation refund once, character cooldown, and rotation to another region.
3. Ownership, private appointment reads, and distance checks.
4. Expiry refund once; no late payment.
5. Unknown settlement blocks retry, cancellation, and automatic refund.
6. No usable site declines without consuming budget/demand.
7. Concurrent reservations cannot oversubscribe a buyer or share a region.
8. Definite settlement rejection permits a corrected retry.
9. Invalid quantities, backwards time, and edits to returned order copies.
10. Region selection does not give a densely authored region extra slots.
11. Pre-readiness completion and reentrant completion during settlement fail.

Evidence is under `validation/2026-10-14-trade-core/`: native JUnit output and
source hashes. The tests use in-memory fake environment/settlement ports. They
do not prove real transfers, world navigation, multiplayer, crash recovery,
or durability. Native engine UI/resource warnings occurred in the headless
menu; the passing final run had no script compilation or VM exception.

The installed test API uses `[Test(suite: AX_TradeCoreSuite)]`,
`[TestStep(TestStage.Main)]`, and `SetFailure`; old string-suite/Step examples
from framework documentation are incompatible with this installed build.

## Run the same suite

Initialize the pinned submodules, then from the repository root:

```powershell
git submodule update --init --recursive
.\tools\Invoke-TradeCoreTests.ps1 -ServerExe "D:\Steam\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe"
```

Use your own installed diagnostic-server path. The runner makes directory
junctions to the five source addons under ignored `.local/trade-tests/addons`,
then starts a hidden, disposable diagnostic process/profile. It leaves the
source checkout and normal game/profile unchanged and stops only its own
process on error/timeout. The profile retains stdout, stderr, native logs, and
JUnit output. A zero exit code alone is insufficient: the runner requires the
expected suite, exactly 11 cases, and no failure/error/skip nodes.

## Next implementation steps

1. Durable order/journal repository, replay/recovery, and restart tests before
   enabling any real item/cash mutations.
2. An authoritative game-mode host that resolves stable character identity,
   time/RNG, proximity, request limits, and encounter-to-order binding.
3. One land encounter adapter with tested site coordinates and a spawned NPC;
   prove death, cleanup, unavailable spawn, and late-arrival behavior.
4. Minimal contact/conversation UI, private appointment replication, and
   sealed test-package inventory/cash adapter behind the validation gate.
5. Dedicated-server/two-client end-to-end testing, then property processing,
   forest bulk, and offshore prototypes according to the architecture.

This milestone implements the appointment domain and its safeguards. It does
not claim completion of the first playable text-to-NPC-sale vertical slice.
