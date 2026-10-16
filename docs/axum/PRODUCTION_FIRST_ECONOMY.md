# Production-first economy

_Status: approved design principle — 2026-09-12._

## The rule

**Nothing of value simply appears at a menu and no activity may collapse into
"buy from NPC, drive to NPC, sell."** Every valuable item must have a
believable origin, a recorded transformation or transfer, a physical location,
and a reason for another player to care about it.

The intended feel is a grounded criminal-and-civilian progression: a player
can begin small, learn a trade, build a supply network, take meaningful risks,
and eventually influence the county. It is inspiration for our progression
rhythm, not a copy of another game, its story, or its content.

## The production contract

Every production loop must define all six stages before it is approved.

| Stage | Requirement | Example |
|---|---|---|
| 1. Source | A world resource, player business, visible shipment, or deliberately limited bootstrap stock. | Orchard trees, a mushroom patch, a farm field, a component delivery at a depot. |
| 2. Acquire | A player gathers, buys, contracts for, steals, or transports the input. | A farm crew harvests crates; a driver signs a collection manifest. |
| 3. Transform | Inputs, equipment, time, and a physical work site create a different output. | Apples become pressed juice; a controlled crop is processed and packaged. |
| 4. Move | Goods occupy inventory, vehicle cargo, warehouse stock, or evidence storage. | A refrigerated truck moves stock to a shop; deputies seize a marked load. |
| 5. Exchange | A player business, contract, customer order, or negotiated criminal distribution creates revenue. | A shop fulfills local demand; a crew supplies a player distributor. |
| 6. Consequence | The activity changes shortages, pricing, evidence, reputation, risk, or follow-up work. | A missed delivery creates a shortage; a crash needs EMS/tow; a seizure changes supply. |

If a proposed feature cannot describe all six stages, it is a convenience
interaction, not a production loop, and does not belong in the core economy.

## What may enter or leave the world

Raw nature can regenerate on a stated schedule: crops mature, fruit returns,
and seasonal resources reappear. That regeneration is visible in the world
and tuned as a resource budget, not as finished products spawning in a player
inventory.

Early development may use **bootstrap stock** for necessities that no player
can yet produce. It must be limited, attached to a named physical supplier or
map-edge delivery, priced as a temporary subsidy, and visible in an admin
ledger. A production loop replaces its bootstrap input as soon as it is ready.

Goods may leave through player consumption, a player business sale, a posted
contract, seizure/destruction, or an external order represented by a manifest.
The system records the reason, quantity, and location. An NPC can provide
character or service, but cannot be an infinite invisible source or sink.

## Core launch chains

| Lane | First chain | Work it creates for others |
|---|---|---|
| Legal agriculture | Harvest fruit or mushrooms → crate → process → label/package → player shop or contract delivery | Farm hands, drivers, mechanics/tow, shop owners, Police/Sheriff traffic and theft calls. |
| Logistics | Receive a visible depot manifest → load by cargo class → deliver/store → settle the contract | Dispatch, warehouse workers, fuel/service, recovery, roadside enforcement. |
| Controlled-crop crime | Obtain scarce inputs → cultivate at a maintained site → process/package with property-installed equipment → contact-arranged NPC orders and optional player distribution | Property owners, workers, runners, security, informants, investigators, EMS and evidence/jail work. |
| Small industry | Recover or contract for components → assemble/repair at a workbench → certify/stock → sell to a player business | Salvage crews, transport, business records, theft investigations. |

We launch only one chain per lane. Depth, reliability, and counterplay matter
more than a long catalog of crops or contraband.

The 2026-10-14 owner direction makes NPC customers the primary drug-sales
channel: private text appointments, changing meeting sites, and forest/offshore
bulk orders. Higher-tier processing requires equipment inside purchased player
property. NPC demand remains finite and recorded under the production contract.
See [the architecture draft](DRUG_TRADE_ARCHITECTURE.md) for implementation
proposals and validation gates.

## The county rhythm

The server should feel like it has a schedule without forcing players through
a scripted campaign. Publish a county calendar and let the current world state
choose the stories.

| Window | County activity | Resulting RP |
|---|---|---|
| Opening shift | Fields mature, business stock reports, municipal/county contracts post. | Crews form, drivers accept work, agencies see planned high-value movements. |
| Trade shift | Processing and deliveries run; local demand and shortages update. | Negotiation, business rivalry, traffic stops, theft, accidents, service calls. |
| Night shift | Riskier distribution, recovery jobs, inspections, and limited special contracts become viable. | Investigations, covert deals, welfare checks, patrol decisions, EMS scenes. |
| Weekly county beat | Market day, inspection cycle, auction, seasonal harvest, or public event. | Community RP with a practical effect on supply, pricing, licenses, or reputation. |

Windows should flex to actual player population. A low-population night must
not lock a solo player out of progression; it can reduce volume and risk rather
than shut the economy off.

## Event standard: a VIP convoy example

A convoy is a story with logistics, not a loot pin.

1. A visible cargo manifest arrives at an edge depot and a departure window is
   announced through dispatch, businesses, and rumors.
2. A player or staff-run convoy stages, departs from that depot, and follows a
   real route to a destination that needs the cargo.
3. Players may lawfully escort, observe, report, negotiate, or intervene under
   server rules; agencies receive jurisdictionally appropriate calls.
4. The result changes county stock. A completed delivery supports an industry;
   an intercepted or wrecked load produces recoverable cargo, evidence,
   insurance, medical/recovery work, and an investigation.

No crate spawns in a field solely because an event timer fired. If something is
found, players can trace how it arrived and why it matters.

## Technical rules that protect the fantasy

- Server authority validates every production, transfer, sale, seizure, and
  destruction; clients never grant items or money.
- Material batches carry a lightweight provenance record: source type, time,
  transformations, current holder/location, and relevant license/evidence
  state. This supports both anti-duplication and investigations.
- Every business, storage point, vehicle cargo space, and evidence locker has
  capacity. Inventory is gameplay, not an abstract number.
- Prices respond to stock, distance, risk, quality, and fulfilled contracts;
  they do not become an infinite money faucet.
- The CAD receives operational facts (manifest, seizure, impound, incident),
  not private medical information by default.

## Vertical-slice acceptance test

Before adding a second chain, prove one complete legal chain and one complete
controlled-crop chain with two players:

1. source appears according to the stated world rule;
2. one player acquires and another transports/processes it;
3. the server records the quantity and provenance through each transfer;
4. a player business or contract settles the finished good;
5. a loss, seizure, or failed delivery changes the next opportunity rather
   than silently replacing the goods; and
6. a server restart preserves the truthful state without duplication.

This becomes Phase 1's core vertical slice once EveronLife's multiplayer and
persistence gates pass.
