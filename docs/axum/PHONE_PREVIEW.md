# Axum phone and Street trade preview

The first phone implementation lives in `axum/` on
`codex/drug-trade-foundation`. It reuses RPPhone's phone-frame and message-icon
art with an original Axum controller, navigation and contact app. The complete
RPPhone addon and its banking, garage, shop and job controllers are not loaded.
EveronLife, EPF and EDF remain the baseline and retain their existing pins.

## Try it in Workbench

1. Open `axum/ProjectAxum.gproj` with the source dependencies registered as
   described in [Workbench setup](WORKBENCH_VALIDATION_SETUP.md). Allow the
   Resource Manager to index the new resources, then compile scripts.
2. Open EveronLife's `Worlds/DebugWorld/DebugWorld.ent`, start Play and take
   control of a character in a single-player session.
3. Press **F6**. Choose **Street trade**, then **Eli** or **Morgan**.
4. Choose a package quantity, select **Request offer**, then **Accept meeting**
   within 60 seconds. The app shows the reserved location and time remaining.
5. Cancel the meeting. After the character request cooldown (60 seconds from
   booking), request and accept another offer; the previous region remains
   excluded by its separate cooldown. An uncancelled meeting expires after
   ten minutes and releases its demand reservation.
6. Use **Home**, **X**, F6 or Escape to navigate/close. Up/down and the normal
   menu-select action also navigate the available buttons. F6 is the current
   keyboard opener; a dedicated gamepad opener and Settings binding remain work.

An already running world must stop and compile the new scripts before testing.
Input/menu configuration changes may require reopening the project. Save editor
changes first. The Enfusion MCP bridge additionally requires a Workbench restart
when its handler classes are first installed.

## What this proves

The app exercises predefined text conversations, expiring quotes, finite buyer
demand/budgets, one active order per character, region reservations, rotating
locations, cancellation and expiry through the existing `AX_TradeService`.
The phone session survives closing/reopening the menu on the same controller.
Messages are bounded to eight entries per contact.

The booking adapter is deliberately limited to **Workbench, the EveronLife
debug world, a local authoritative controller and at most one player**. The
screen can open elsewhere, but the demo contacts are unavailable. Prices,
contacts and authored coordinates are preview fixtures. The preview uses the
EveronLife active character ID when present, otherwise an explicitly temporary
Workbench identity. It is not production authentication or private replication.

There is **no spawned buyer, map marker, inventory handoff, cash payment or
restart persistence** yet. The UI states that no goods or money are exchanged.
The base settlement port rejects all settlements. Closing Workbench or replacing
the controller resets this preview. Do not connect it to real goods or currency.

## Code and licensing

- `Scripts/Game/Axum/Phone/AX_DrugAppSession.c`: contact conversation/quote state
  and adapter to the existing order service.
- `AX_PhoneController.c`: local input and restricted Workbench session.
- `AX_PhoneMenu.c`: reusable phone shell and first app navigation.
- `Configs/System/chimeraInputCommon.conf` and `chimeraMenus.conf`: F6 action
  and menu registration; retain the baseline resource identities for merging.
- `ThirdParty/RPPhone/`: two unmodified textures, an adapted layout, separate
  [APL notice](../../axum/ThirdParty/RPPhone/NOTICE.md) and hash provenance.

RPPhone material is credited to NewbKodii and remains under APL, with its
NonCommercial and ArmaOnly conditions. It is excluded from the repository's MIT
license. See the [source assessment](PHONE_STARTING_POINT.md).

## Verification and next work

On October 15, 2026, the native diagnostic server **1.8.0.13** passed all
**11 appointment-core cases and 6 phone cases**, with no failures/errors/skips.
The phone suite covers quotes, invalid/expired requests, duplicate acceptance,
cancellation/rotation, expiry, widget bindings/wrapping, standalone and actual
menu geometry, and preservation of baseline input bindings in the merged config.

```powershell
.\tools\Invoke-TradeCoreTests.ps1 -ServerExe "D:\Steam\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe" -Suite AX_PhoneSuite
```

Omit `-Suite` to run `AX_TradeCoreSuite`. Evidence and source hashes are under
`validation/2026-10-15-phone-preview/`.

The owner confirmed these live Workbench results:

- The RPPhone frame, message icon and Axum home page render with readable text.
- WASD works before opening the phone and after closing it with F6.
- Street trade → Eli → Request offer → Accept meeting shows the conversation
  and reserved meeting details.
- Stopping Play with the phone open returns to the editor without an assertion.

The live editor resolved EveronLife/EPF/EDF from installed Workshop packages;
the native tests used the pinned source graph. These are separate environments.
There is no multiplayer, durable recovery or real transfer result. An earlier
live script reload triggered an engine resource-leak assertion whose leaked
resource was not named in the log. Explicit menu cleanup was added afterward;
the final normal Stop test passed. Do not edit scripts during Play.

Two regressions are now guarded: missing layout edge offsets placed the phone
outside the viewport, and a separate priority-11 input context masked normal
priority-10 character input while being masked by priority-50 menus. The phone
now appends its action to the existing character/menu contexts, preserving their
bindings and priorities. See Bohemia's [input context rules](https://community.bistudio.com/wiki/Arma_Reforger:Input_Manager#Context).
Both UI debounces use real elapsed time. Workbench may require a saved restart
to discard cached input configuration after changing bindings.

Engine `System.MakeScreenshot` omitted GUI even when the owner's screenshot
showed it. It is not used as visibility proof, and automatic capture/temporary
layout diagnostics were removed. The temporary Enfusion MCP bridge scripts were
also removed from the addon after testing so they do not ship with the mod.

Next: an authoritative shared service host, owner-only RPC transport, and one
temporary land buyer with spawn/cleanup handling. Keep durable transaction
journaling and recovery ahead of real goods/payment integration. Property
processing and forest/offshore bulk encounters remain later milestones.
