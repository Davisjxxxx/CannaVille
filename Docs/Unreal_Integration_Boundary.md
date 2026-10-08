# Unreal Engine integration boundary

The simulation core is the scientific source of truth. Unreal Engine is a presentation, input, and orchestration client. Unreal must not own biological equations or silently reimplement simulation state.

## Future adapter responsibilities

The future UE adapter will:

1. load a versioned scenario and simulation configuration
2. initialize a headless simulation instance with an explicit seed
3. translate validated player actions into simulation actions
4. advance simulation time using an explicit duration or fixed-step request
5. query observable state and derived UI metrics
6. receive state changes through snapshots, events, or a documented delta stream
7. serialize and restore simulation state through the core serialization contract
8. map simulation identifiers to UE actors without making actors authoritative

## Prohibited adapter behavior

- no Unreal headers in `Simulation/`
- no UE actor as the source of biological state
- no duplicate plant growth equations in Blueprints or gameplay C++
- no hidden wall-clock time in the simulation call path
- no client-side economy authority
- no unversioned state migration

## Planned call shape

The current `ISimulation` interface exposes the intended boundary:

- `initialize(scenario, seed)`
- `submit_player_action(action)`
- `advance_fixed_step()`
- `reconcile_offline_elapsed(elapsed)`
- `observable_state()` returns a player-facing projection and excludes latent, stochastic, and other internal source-of-truth fields
- `serialize_state()`
- `load_serialized_state(serialized)`

The interface is intentionally inert where models do not yet exist. Future behavior must be introduced through governed model systems and regression scenarios, not by changing the adapter into a second simulation.
