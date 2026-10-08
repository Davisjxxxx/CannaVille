# Scientific model governance

This document governs future horticultural models. The current bootstrap intentionally implements none of the biological equations.

## Required provenance

Every biological model must identify:

- model identifier and version
- scientific source or sources
- source type and citation record
- modeled variables and units
- assumptions and boundary conditions
- calibration status
- validation status
- valid operating range
- known limitations
- regression scenarios covering the model

## Constants

Every constant must record:

- name
- value
- units
- source
- calibration status
- valid range
- uncertainty, if known
- whether it is scientific, calibrated, or gameplay-only

An unlabeled biological number may not enter a model interface.

## Approximation labels

Every approximation must be explicitly labeled as one of:

- `scientific_model`: supported by an identified source and implemented within its stated range
- `calibrated_model`: fitted or adjusted against identified observations with calibration evidence
- `engineering_approximation`: chosen to preserve stability or tractability, with known error boundaries
- `gameplay_approximation`: intentionally simplified for player comprehension or pacing
- `placeholder`: interface-only behavior pending a model

The label must be visible in the model registry and documentation.

## Scientific truth versus gameplay

Scientific truth parameters and gameplay/time-compression parameters must be separate configuration namespaces. Gameplay balancing must not silently modify scientific constants. A time multiplier may change how simulation time is presented or advanced, but it must not be encoded as an undisclosed biological rate change.

## Randomness

Randomness may represent uncertainty, measurement noise, or biological variation when a source and distribution are documented. Randomness cannot substitute for an absent causal mechanism. Every random stream must have an explicit seed and scope.

## Change control

Model changes require:

1. a versioned registry entry
2. source and calibration updates
3. regression scenarios
4. deterministic output comparison
5. review of unit and valid-range changes
6. explicit migration notes for saved state

## State authority

A single generic `health` scalar is not a biological source of truth. Observable player-facing state, latent biological state, derived UI metrics, configuration, stochastic state, and historical/cumulative state must remain separate. A UI metric may be derived from several latent variables, but it must not replace them.

See [`model_registry.schema.json`](</home/jd/CannaVille-V2/Docs/model_registry.schema.json>) for the metadata contract.
