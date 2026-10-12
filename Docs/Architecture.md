# V2 architecture map

## Module ownership

| Area | Bootstrap responsibility | Future responsibility |
|---|---|---|
| `Simulation/Core` | clock, timestep, scenario validation, serialization, action boundary | orchestration, version migration, event ordering |
| `Environment` | room-cell measurements plus sourced vapor-pressure/VPD derivations | later causal room and cell environmental models |
| `Lighting` | explicit PPFD schedules, DLI integration, photoperiod bookkeeping | fixture/control models and later canopy sampling models |
| `Plants` | plant identity, location, state separation | plant development and phenotype models |
| `RootZone` | moisture, root-zone temperature, pH, EC state shape | substrate, root, water, oxygen, and root-zone models |
| `Nutrition` | solution/profile identity and model status | nutrient availability and uptake models |
| `Genetics` | cultivar/genotype identity shape | genotype, inheritance, and trait response models |
| `Pests` | population-model identity shape | population and life-cycle models |
| `Disease` | pathogen/latent-state identity shape | pathogen, infection, and environmental-pressure models |
| `Treatments` | treatment identity and model status | efficacy, coverage, resistance, and biological-control models |
| `Harvest` | harvest state shape | biomass, harvest timing, and quality models |
| `Drying` | drying state shape | post-harvest drying model |
| `Curing` | curing state shape | curing and quality-retention model |

## State categories

Each plant contains explicitly separated categories:

- `latent`: eventual biological source of truth
- `observable`: player-facing state projection
- `derived`: UI-oriented metrics and labels
- configuration: scenario and simulation configuration
- `stochastic`: explicit seed and draw accounting
- `history`: elapsed and cumulative state

The room contains spatial cells. Plants retain their location and sample a cell's environment and lighting state. P1A derives physical vapor-pressure measurements per cell and integrates supplied PPFD schedules per cell. It does not implement CFD, fixture optics, canopy attenuation, or biological response.

The original regular-grid lookup remains the default. Explicit scenario cell layouts use deterministic nearest-cell selection so irregular test fixtures can be represented without interpolation. No environmental interpolation is performed.

Horticultural light state is separate from rendering light state. PPFD is supplied as a physical measurement or scenario input; it is never derived from Unreal or renderer brightness.

Leaf temperature is optional. Missing leaf temperature produces unavailable leaf VPD; air temperature is never silently substituted.

Simulation days are explicit 86,400-second intervals from simulation time. DLI and exposure-duration accumulators reset at those boundaries; wall-clock midnight is not consulted.

## Deterministic execution contract

For a supported platform, the tuple below is the reproducibility input:

```text
scenario bytes
configuration
simulation version
seed
fixed timestep
requested duration
```

The current state serializer uses ordered JSON object keys and stable array order. The CLI emits a time-series CSV and a final state hash for operational comparison. Floating-point portability beyond the supported compiler/platform contract must be established before cross-platform bitwise claims are made.

## Root-zone conservation output

The root-zone CSV exposes one cumulative ledger. `conservation_initial_storage_m3` is the persistent baseline, and `current_storage_m3` is the current inventory. Cumulative inputs and outputs are emitted once each; requested plant withdrawal is reconstructed as `cumulative_realized_withdrawal_m3 + cumulative_unmet_demand_m3` when needed. `conservation_residual_m3` is independently recomputable from the emitted columns.

Transpiration output is valid only when its integrated status is available. A converged gas-exchange solve does not authorize a transpiration flux when boundary-layer conductance is unavailable; unavailable transpiration fields are blank, while an available calculated zero remains numeric.

## P1B.5 leaf energy boundary

P1B.4.1 hydraulic state is resolved once at the beginning of each fixed-forcing
segment. P1B.5 then performs a steady-state, single-leaf energy solve using that
fixed beta value:

```text
beginning root/hydraulic state
  -> trial leaf temperature
  -> leaf VPD
  -> existing FvCB/Medlyn gas exchange
  -> shared heat/vapor boundary layer
  -> existing transpiration flux
  -> latent + sensible + longwave + explicit shortwave balance
  -> deterministic bisection
  -> final gas exchange/transpiration
  -> existing water request/allocation
```

All P1B.5 per-area heat and water-vapor fluxes use the common
`projected_one_sided_leaf_area` basis. `surface_factor` applies only to configured
radiation and sensible-heat exchange; it never multiplies the P1B.4.1 transpiration
flux or whole-plant effective leaf area.

Leaf temperature has three explicit modes: `EXTERNAL_LEAF_TEMPERATURE` preserves a
supplied measurement and bypasses the solver, `SOLVED_LEAF_ENERGY_BALANCE` requires
explicit incident shortwave energy and radiative-surroundings temperature, and
`UNAVAILABLE` produces no authoritative leaf VPD/transpiration/water withdrawal.
PPFD is a separate horticultural measurement and is never converted to shortwave
energy by default.

The solver is steady-state only (`S = 0`), bounded by numerical policy, and fails
explicitly for unavailable inputs, unsupported boundary-layer flow, no bracket,
non-finite residuals, or iteration exhaustion. It does not mutate root-zone state
during trial evaluations. Exact water/light/day boundaries remain the only segment
boundaries. Segment duration is diagnostic context; a steady-state result is not a
transient leaf-temperature prediction. The coupled root search also does not trial
below the ambient dew-point boundary because the accepted Medlyn/FvCB model has no
authoritative negative-VPD stomatal solution; the direct P1B.4.1 transpiration path
still preserves valid condensation as negative latent heat and never as negative
plant withdrawal.

P1B.5 is the last planned foundational physical-physiology subsystem. After its
single closure review, the roadmap pivots to P2 — Plant Growth and Gameplay
Consequence Layer. No growth, stress, yield, quality, pest, disease, or treatment
model is part of this boundary.

## Current intentionally inert behavior

- fixed-step advancement changes only clock and historical elapsed state
- plants sample the spatial cell at their location
- player actions are accepted at the interface but have no model effect
- no wall-clock value is read
- no biological equation or random biological event is executed
