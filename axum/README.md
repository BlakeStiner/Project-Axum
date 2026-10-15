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

No Axum game mode, mission, map, scripts, or Workshop release is created by
this scaffold. Workbench compilation and runtime validation remain pending.
