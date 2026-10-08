# V2 architecture map

## Module ownership

| Area | Bootstrap responsibility | Future responsibility |
|---|---|---|
| `Simulation/Core` | clock, timestep, scenario validation, serialization, action boundary | orchestration, version migration, event ordering |
| `Environment` | room-cell environment state | causal room and cell environmental models |
| `Lighting` | PPFD/DLI/photoperiod state shape | governed lighting and canopy sampling models |
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

The room contains spatial cells. Plants retain their location and sample a cell's environment and lighting state. The bootstrap performs only pass-through sampling; it does not implement CFD or biological response.

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

## Current intentionally inert behavior

- fixed-step advancement changes only clock and historical elapsed state
- plants sample the spatial cell at their location
- player actions are accepted at the interface but have no model effect
- no wall-clock value is read
- no biological equation or random biological event is executed
