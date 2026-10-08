# ADR-0001: Keep the scientific simulation engine independent of Unreal

## Status

Accepted for the V2 bootstrap.

## Decision

The scientific simulation is a standalone modern C++ library with a headless command-line executable. Unreal integration will occur through an adapter boundary after the core contracts are stable.

## Reasons

- deterministic headless testing is easier without render and frame-rate dependencies
- scientific models can be validated independently of mobile UI and actor lifecycles
- cloud/server execution remains possible without shipping Unreal runtime dependencies
- save-state and version migration can be tested from the same authoritative representation
- model governance can remain separate from presentation code
- multiple front ends can consume the same observable state contract

## Consequences

- Unreal will need an explicit adapter and state-mapping layer
- simulation identifiers must not depend on actor pointers or UObject lifetime
- actions and snapshots need versioned contracts
- build and test pipelines must verify the core without a UE installation
- the core cannot assume rendering, audio, input, filesystem layout, or wall-clock time

## Alternatives rejected

- putting biological logic directly in Blueprints
- using Unreal tick as the simulation clock
- sharing mutable actor state as the scientific source of truth
- copying legacy browser formulas into the new engine
