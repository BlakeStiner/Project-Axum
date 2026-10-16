# Project Axum addon

`ProjectAxum.gproj` is the custom gameplay project. It depends on the
EveronLife project in `../src/`; EveronLife in turn depends on EPF and EDF.
Register all projects in Workbench using the [setup guide](../docs/axum/WORKBENCH_VALIDATION_SETUP.md).

Future Axum scripts belong under `Scripts/Game/Axum/` with the `AX_` class
prefix. Custom prefabs, configurations, UI, and worlds belong inside this
addon. Add those files when their first feature is implemented rather than
committing empty placeholder classes.

Keep contacts, orders, rendezvous selection, encounters, property permissions,
production, and transaction services separate within this addon. Use adapters
for EveronLife inventory/account behavior. Do not implement custom drug trading
by changing the upstream barter or quantity classes in place.

The first appointment-domain scripts now live under `Scripts/Game/Axum/DrugTrade/`.
See [implementation status](../docs/axum/DRUG_TRADE_CORE.md) and the separate
`axum-tests/` addon. Native headless compilation/tests have passed on Reforger
1.8.0.13. The [phone preview](../docs/axum/PHONE_PREVIEW.md) adds a reusable
phone shell and Street trade app for the single-player Workbench debug world.
Press F6 after loading/compiling the resources. RPPhone art and the adapted
layout retain their separate APL notice under `ThirdParty/RPPhone/`.
There is no live settlement adapter or Workshop release yet; multiplayer and
persistence validation remain pending.
