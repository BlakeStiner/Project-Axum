# Project Axum

Custom county roleplay development built on the EveronLife baseline in this
fork. Our gameplay addon is [axum/ProjectAxum.gproj](axum/ProjectAxum.gproj);
EveronLife remains in `src/` with its original project identity and license.

Start with [Axum development setup](docs/axum/WORKBENCH_VALIDATION_SETUP.md)
and the [drug-trade architecture](docs/axum/DRUG_TRADE_ARCHITECTURE.md).
This branch establishes dependencies and project structure; it does not yet
contain playable Axum trading, processing, or property systems.

Implementation has started with the [appointment core](docs/axum/DRUG_TRADE_CORE.md):
finite buyer reservations, region-aware meeting selection, order state, and
settlement boundaries, verified by native Enforce tests in `axum-tests/`.

The [Drugs Economy assessment](docs/axum/DRUGS_ECONOMY_ASSESSMENT.md) evaluates
optional visuals/UI and records isolated compatibility results. Its
[`compat/drugs-economy/`](compat/drugs-economy/README.md) probe does not change
the active EveronLife baseline or adopt its payment code.

The root README and existing framework documentation are retained upstream
material. Axum-specific development instructions live under `docs/axum/`.
