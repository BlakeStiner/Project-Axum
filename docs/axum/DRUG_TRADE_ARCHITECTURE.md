# Morrow County — drug trade architecture

_Design draft: 2026-10-14. Owner direction: primarily NPC sales arranged by
text, changing meeting locations, forest/offshore bulk orders, and processing
equipment placed inside purchased player property. Technical choices below
are proposals; no gameplay implementation or multiplayer validation yet._

## Player experience

A player builds contacts, texts a buyer, agrees to an order, and receives a
private appointment. The server arranges a temporary NPC encounter at a
suitable location. Small orders create local trips; bulk contacts offer remote
forest or offshore meetings. Completed orders consume real player-produced
stock and a finite buyer budget, then affect demand and relationships.

Higher-tier production requires a purchased property with installed equipment.
Legal work provides an accessible route to the property, equipment, vehicle,
and operating funds. Property ownership opens production capacity; contact
relationships and available demand determine whether that capacity is useful.

The design reduces the payoff from camping a permanent marker. It does not
guarantee safety: following a seller, witnessing a meeting, or discovering an
operation remains possible through the shared world.

## Complete economy loop

| Stage | Design |
|---|---|
| Source | World cultivation or finite, manifested inputs; exact first product remains open. |
| Acquire | Players obtain and physically move supplies; supplier stock and origin are recorded. |
| Transform | Install equipment on purchased property, supply inputs, run an abstract timed game recipe, collect packaged output. |
| Move | Packages occupy personal, property, or vehicle storage with capacity limits. |
| Exchange | Text a contact, accept a bounded order, travel to a temporary NPC meeting, exchange the agreed goods for payment. |
| Consequence | Demand/budget falls, relationships change, and loss or seizure affects available stock and future work. |

NPCs are the primary customers. Player-to-player supply, employment, transport,
and resale remain possible but are not prerequisites for finding a customer.
NPC orders replace the earlier player-distribution emphasis. They retain the
production-first rule: no infinite source/sink or fixed buy-drive-sell route.
Customer payments come from an explicit, tunable economy allocation, recorded
as external demand, rather than unexplained cash on every spawned NPC.

## Contacts and appointments

The phone is an in-game interface backed by server-owned records. It does not
require real SMS, Discord, an external messaging service, or a complete phone
simulation. A simple contacts/conversation screen is sufficient initially.

1. An eligible contact has a limited product need and spending budget.
2. The player requests an order. The server offers quantity, total payment,
   meeting category, broad travel region, and an appointment window.
3. Acceptance reserves that order's demand and budget. One active appointment
   per character is the initial proposed limit.
4. The server reserves a suitable site and privately sends the final location
   with enough travel time. The location stays fixed after confirmation unless
   a bounded, explained reschedule is required.
5. The encounter manager spawns the buyer ahead of arrival when the site and
   AI budget permit. Spawn success is required before claiming the buyer is
   ready. Visible pop-in is a failed encounter setup, not normal presentation.
6. At the meeting, the server validates the seller, buyer, order, physical
   goods, distance, capacity, deadline, and payment before settlement.
7. Completion consumes the agreed packages and reserved budget once. The NPC
   departs or is cleaned up when no longer observed or involved in a scene.

Contacts persist as identities and relationships; each appointment has its own
temporary NPC entity. Spawning a new entity does not reset demand, reputation,
budget, death consequences, or cooldowns. Payment is not available as ordinary
NPC loot as well as a transaction payout.

## Meeting types

| Type | Location requirements | Purpose | First implementation |
|---|---|---|---|
| Local customer | Traversable ground, valid standing point and approach, several distinct neighborhoods | Small orders and early contact progression | Temporary stationary buyer with a short approach/departure where supported |
| Forest bulk | Tested clearings/service tracks, cargo access appropriate to the order, varied approach routes | Larger loads with transport and isolation costs | Stationary buyer and optional parked vehicle |
| Offshore bulk | Navigable water inside map bounds, tested depth/clearance, practical boat range and interaction access | Marine logistics and larger external orders | Spawned rendezvous vessel with seated buyer or vessel-bound interaction |

Offshore completion must validate both seller and cargo at the vessel; reaching
a map circle alone cannot pay an order. Do not require walking on a moving deck
or AI piloting in the first version. The selected boat must pass replication,
boarding/interaction, cargo, collision, stability, and cleanup tests. If it
cannot support a reliable interaction, offshore orders remain disabled while
land orders work. An NPC must not simply stand on the water.

## Preventing predictable camping

Use a large authored site pool with runtime validation. Arbitrary coordinates
can land a buyer inside geometry, on an unusable slope, or in unsafe water.
Sites are configuration data, so terrain changes do not require trade logic
changes. Each site records region, type, access routes, clearance, supported
load/vehicle class, spawn position, and a wider exclusion/cooldown area.

- Select across regions and approach routes, then among eligible sites. Avoid
  repeating a neighboring site as though it were a genuinely new destination.
- Track recent use per site, region, and seller; reserve sites against
  overlapping appointments. Avoid sending unrelated sellers to one clearing.
- Check for obvious existing occupancy before booking/spawning. After a buyer
  and seller are committed to a scene, do not teleport the buyer away because
  another player approaches.
- Share appointment details only with the seller and explicitly authorized
  participants. Do not broadcast order coordinates or create public markers.
  World entities remain observable under normal replication and visibility.
- Spread marine access across several usable departure areas. One required
  marina or one required forest road recreates a fixed choke point.
- Keep cancellation/rescheduling bounded with a cooldown. Repeatedly cancelling
  cannot generate unlimited fresh prices, demand, or location scouting.
- If no suitable site exists, delay or decline the booking with a neutral
  availability message. Do not fall back to one permanent dealer or reveal
  that a specific site contains other players.

Proximity screening itself can become a scouting tool if players can repeatedly
probe it. Rate limits, coarse availability messages, and no player-count or
police-presence explanations are necessary. Do not automatically reroute every
meeting around police or promise an ambush-proof location.

Monitor appointment concentration, routes, waiting time, successful deliveries,
cancellations, and encounter failures. Tune the actual distribution after
play-tests; randomly selecting from three familiar spots is insufficient.

## Property processing and progression

Start with predefined purchasable properties and approved equipment sockets
inside them. A player buys a machine kit, chooses an allowed position, and
installs it. This provides placement and ownership gameplay without requiring
full player-built housing. Free placement within validated rooms can follow.

| Record or check | Responsibility |
|---|---|
| Property | Persistent ID, owner character/business, bounds/rooms, equipment capacity, access list |
| Machine | Persistent ID, property ID, type/tier, transform, installed state, input/output storage |
| Permission | Owner or explicitly authorized operator; checked at installation, job start, and collection |
| Recipe | Abstract game inputs/output, equipment tier, duration, capacity; configuration rather than hardcoded drug names |
| Job | Persistent ID, reserved inputs, start/end times, recipe version, state, produced output IDs |

Placement validates the entire machine footprint, room/height, obstruction,
property permission, and capacity on the server. Being within a radius of a
house must not permit equipment outside it or on a neighboring floor.

Higher-tier recipes require an eligible owned property and installed equipment.
Starter product/tier rules remain a separate balancing decision. Recommend
allowing owners to employ authorized players so newcomers can learn and earn
before purchasing their own site. Access to someone's machine does not transfer
ownership of their inputs or output.

Inputs move into reserved job storage before processing. Outputs have finite
storage and are created once. Jobs cannot be collected twice, machines cannot
be packed up while work or stock remains, and a property cannot be sold with
unresolved jobs. Revoked access prevents new operations; existing work and
goods remain attributed to their recorded owner until resolved.

Proposed initial offline rule: processing pauses while the server is down;
persist remaining duration and resume after recovery. Restarting cannot finish
a job instantly, recreate consumed inputs, or generate a second output.
Physical noise, deliveries, observation, and evidence can expose a site; buying
a property does not automatically publish its illegal activity to police.

## Module boundaries

Use one Axus-owned gameplay addon with logical modules, not a Workshop addon or
external service for every feature. Names below are proposed responsibilities,
not engine APIs or files already implemented.

| Module | Owns |
|---|---|
| Contacts | Contact identities, text conversation, access and relationships |
| Orders | Offers, accepted terms, deadlines, reservations, completion/failure |
| Rendezvous | Site selection, cooldowns, reservations, travel feasibility |
| Encounters | NPC/vessel spawning, readiness, lifecycle, death/destruction events |
| Properties | Ownership, access, equipment placement and capacity |
| Production | Machines, recipes, job lifecycle, batch provenance |
| Economy | Goods/payment settlement, demand budgets, ledger, retry handling |
| Persistence adapters | Durable records, recovery, mapping records to world entities |

UI submits intent. These server modules authorize and execute it. Drug-specific
contacts, prices, machine tiers, and meeting requirements sit on shared item,
property, production, and transaction services reusable by legal businesses.
Forest and offshore encounters implement the same appointment contract.

Keep EveronLife integration behind adapters. Its local trader is a small barter
configuration, and its processing action removes/adds inventory quantities;
neither is a complete durable order settlement or owned-machine job system.
Existing quantity merging must preserve batch identity or be disabled for
these items. Begin with sealed packages to avoid introducing arbitrary
splitting/merging alongside the first transaction implementation.

## Persistent state and failure behavior

Persist contact relationships; order terms/status; budget reservations; site
reservations/cooldowns; property ownership; machine inventory; processing jobs;
goods provenance; and settlement journal entries. Use stable IDs rather than
session player IDs or transient NPC entity IDs. Store schema/config versions.

An appointment progresses through offered, booked, preparing, ready, and
settling to completed. Cancelled, expired, failed, and recovery-needed are
explicit states. Keep order outcome separate from NPC cleanup: a cleanup retry
cannot repay a completed order.

| Failure | Required result |
|---|---|
| Repeated interaction or concurrent sale attempts | One settlement; subsequent requests return the recorded outcome |
| Wrong caller, remote interaction, changed goods, full payout storage | Reject before transfer; no partial sale presented as completion |
| NPC dies/vessel sinks before settlement | Fail once; seller retains unsold goods, with any physical loss handled separately |
| Seller disconnects | Preserve order/deadline; no payout or cargo duplication; bounded reconnect grace is a tuning option |
| Buyer cannot spawn or site becomes unusable | Bounded delay/reschedule or cancellation; restore unused reservations without resetting the underlying demand budget |
| Restart during preparation | Recover the same order/reservations; reconcile old entities before spawning replacements |
| Crash during settlement | Recover from a durable transaction record; never blindly repeat remove-goods/add-cash |
| Save backend unavailable | Suspend economic mutations with a clear retry message; do not acknowledge an unsaved payment or job |

Goods, cash, and demand cannot be treated as three unrelated successful saves.
The persistence prototype must prove a durable commit boundary or recoverable
journal with stable operation IDs, locks, and deterministic reconciliation.
Pending goods/payment stay inaccessible until the outcome is known. World
inventory and the economy journal must have an explicit recovery precedence;
EPF entity saves alone must not be assumed to provide multi-record atomicity.
Cash versus bank payout is configurable; proposed initial street payment is
physical cash, using the same settlement safeguards and capacity checks.

Seizure/evidence publishes authorized operational facts to CAD. Private contact
messages and live appointments are not automatically a police tracking feed.

## Implementation sequence and acceptance gates

1. **Technical proof:** one private text/contact screen, one accepted order,
   several tested land sites across distinct regions, one temporary buyer,
   sealed test packages, finite demand, and one recorded payment.
2. **Authority and recovery:** two clients, competing interactions, remote
   requests, full inventories, NPC death, disconnects, and restarts/crash
   injection at each settlement step. Reconcile goods, cash, and budgets.
3. **Property proof:** purchase one predefined property, install one machine,
   authorize another player, run one abstract recipe, restart mid-job, and
   collect exactly one output. Verify boundary and revoked-access checks.
4. **Forest bulk:** add cargo-aware orders, more sites/approaches, and capacity
   tests using the same order/settlement system.
5. **Offshore proof:** select an approved boat, validate the stationary
   rendezvous with two clients, destruction/disconnect/restart behavior, and
   several departure routes. Enable only after it passes.
6. **Content and balance:** choose the first product, legal-work/property
   affordability, equipment tiers, demand replenishment, payout, and time
   windows through closed testing before adding more drug families.

Architecture work starts now. These gates retain the existing requirement to
validate the shared inventory/persistence foundation and legal economy before
releasing higher-tier illegal production. This draft does not certify a game
version, select a Workshop dependency, or claim an engine feature is tested.

## Feasibility evidence

Reviewed 2026-10-14. The local compatibility record remains the September test
of Reforger 1.8.0.13; current online documentation may describe newer APIs.
Record installed client/tools/server versions when running the prototype.

- Bohemia's [NPC/entity spawner API](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceSCR__CatalogEntitySpawnerComponent.html)
  exposes entity and AI-group member spawning. This supports the encounter
  approach; it does not implement the contact or economy system.
- Bohemia's [building-provider API](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceSCR__CampaignBuildingProviderComponent.html)
  provides a placement-system reference. Ownership, room boundaries, machine
  jobs, and persistence remain custom work.
- Bohemia's [1.9 modding update](https://reforger.armaplatform.com/news/modding-update-sept-24-2026)
  documents changed AI spawning/lifecycle behavior. Spawn timing and AI budget
  handling must be validated against the installed version.
- The Motorboat author's [changelog](https://reforger.armaplatform.com/workshop/610C8532D3756F2D-Motorboat/changelog)
  records past boarding/collision issues and qualified AI driving support.
  It is evidence to test marine behavior, not an endorsement or approval of
  that dependency or proof it works in our current stack.
- Local references: [economy contract](PRODUCTION_FIRST_ECONOMY.md),
  [EveronLife audit](EVERONLIFE_AUDIT.md), and
  [validation setup](WORKBENCH_VALIDATION_SETUP.md).
