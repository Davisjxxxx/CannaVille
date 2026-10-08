# CannaVille V2

CannaVille V2 is the engine-independent bootstrap for a controlled-environment horticulture simulation. The simulation core is modern C++20 and is designed to run headlessly before any Unreal Engine integration exists.

This repository deliberately contains no plant physiology, pest biology, disease equations, nutrient equations, or cultivation formulas. It establishes boundaries, state categories, units, deterministic stepping, scenario loading, serialization, and test infrastructure.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## CLI smoke run

```bash
./build/cannaville-sim run \
  --scenario Scenarios/smoke_test.json \
  --duration 24h \
  --seed 42 \
  --output results.csv
```

The CLI produces a deterministic CSV time series and reports the final serialized-state FNV-1a value. The hash is an operational reproducibility aid, not a cryptographic identity claim.

## Boundaries

- `Simulation/` is engine-independent and must remain free of Unreal headers and cloud SDKs.
- `Tools/SimCLI/` proves headless execution and serialization.
- `Game/` is reserved for future game-facing adapters and presentation contracts.
- `Cloud/` documents future authority boundaries only; it contains no cloud implementation.
- `Docs/` contains governance, units, schema, and integration decisions.
- The audited legacy CannaVille material is outside this repository and is read-only historical reference.
