# Phone starting point: RPPhone adaptation

Owner direction, October 14, 2026: use **RPPhone as the starting point** for
Axum's reusable phone. Keep EveronLife as the gameplay baseline. This records a
preferred implementation direction and preliminary static findings; it does not
claim a working phone integration.

## Inspected material

- Creator: NewbKodii.
- [Workshop source](https://reforger.armaplatform.com/workshop/65EF7B586691A802-RPPhone),
  ID `65EF7B586691A802`, version `0.0.58`.
- Installed source: `C:/Users/grave/OneDrive/Documents/My Games/ArmaReforger/addons/RPPhone_65EF7B586691A802`.
- `data.pak`: 29,805,839 bytes, 160 entries;
  SHA-256 `514ef8af94b38ebf5f59a55b70bbec3e623cce00ce56d2966ad7b037aea727d2`.
- Workshop currently lists Reforger `1.7.0.54`. Our installed package's
  `ServerData.json` has an empty game-version field and dependency list, while
  `addon.gproj` directly requires BankATM `64D15EA404F23ABD`, GarageRP
  `66685DF17ADE9709` and vanilla. The Workshop additionally lists Shop System,
  Auction System, EDF and EPF. Resolve the actual recursive source dependencies
  before attempting a full-package compatibility test; the empty metadata list
  is not proof of independence.

## Useful starting material — static inspection

| Material | Potential reuse |
| --- | --- |
| `UI/layouts/CellPhone.layout` | Existing phone frame, app/navigation panels, inbox, compose and settings widgets. |
| `Game/Prefabs/UI/TextMessage.layout`, `Player.layout` | Message and recipient rows. |
| `Assets/Textures/phone.edds`, `Mail.edds`, `Msg.edds`, `Settings.edds` and other icons | Phone presentation candidates, subject to rendered inspection and preserved credits. |
| `Scripts/Game/UI/Menu/PHN_CellMain.c` | Menu lifecycle, widget binding, navigation and inbox examples. It directly uses banking, jobs and shop types, so copying the controller alone does not remove those dependencies. |
| `Scripts/Game/Components/PHN_CellComponent.c` | Owner-directed message/notification examples. Also contains bank transfers; it is not an isolated messaging service or Axum transaction adapter. |
| `Cell_CharacterControllerComponent.c`, menu/input configs | Opening and input examples. The existing opener requires `JOB_jobComponent`, uses `GadgetWatch`, and chooses phone/tablet by job. Replace that coupling for Axum. |

The package contains 14 `modded` declarations covering character control, game
mode, inventory, map, shop, vehicle access and menu enums. It also supplies shared
character/vehicle prefab paths and unrelated policing/job systems. Full-package
loading therefore has a larger integration surface than its phone screen.

No native compile, widget creation, rendered preview, RPC ownership test,
multiplayer test or persistence test of RPPhone has been performed here. The
earlier Drugs Economy compatibility results do not apply to RPPhone.

## Adaptation boundary and first proof

1. Preserve RPPhone attribution and modification history in a separate phone
   adaptation package. Inspect the layout/resource closure before copying it;
   do not import the entire gameplay package just to obtain the screen.
2. Reuse suitable phone presentation and navigation, with Axum-owned controller
   and service adapters. Give adapted resources their own project identity and
   resource GUIDs; retain source-to-adaptation provenance.
3. Make the phone shared across Axum. Its first app exposes NPC contacts,
   predefined conversation choices and appointment details. Future apps can
   connect to jobs, properties and other services independently.
4. Derive messaging identity and access from the authenticated character. NPC
   contacts are durable identities, not current player slots. Private messages
   and meeting coordinates must only reach the authorized character.
5. Keep finite demand, quotes, appointments, property processing and recovery in
   Axum's server services. Phone buttons request operations by stable IDs; they
   do not set bank balances, create goods or decide successful settlement.
6. First Workbench proof: open/close an adapted phone screen, select one contact,
   exchange predefined messages, and display a test appointment. Compile and
   render it in an isolated addon before adding it to the active Axum project.
   Then test two clients and reconnect behavior before connecting real goods/cash.

## License and current distribution status

RPPhone is listed under [Arma Public License (APL)](https://www.bohemia.net/en/licenses/arma-public-license).
APL permits adaptation for NonCommercial and ArmaOnly purposes. Retain supplied
creator/copyright/license/disclaimer/source notices, identify modifications and
preserve earlier modification notices. Include the license text or link with
reused material; it does not become MIT merely by entering this repository.
Keep additional supplied asset credits. No RPPhone code, layouts or art are
redistributed by this decision note, and installed files remain unmodified.
