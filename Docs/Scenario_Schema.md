# Scenario schema

Scenario files are JSON documents with schema version `1`.

Required root fields:

| Field | Type | Unit/meaning |
|---|---|---|
| `schema_version` | integer | Schema identity; currently `1` |
| `scenario_id` | string | Stable scenario identifier |
| `simulation_version` | string | Simulation model version captured in output |
| `fixed_timestep_seconds` | number | Fixed simulation step duration in seconds |
| `rooms` | array | Room definitions |
| `plants` | array | Plant identity and location definitions |

Room fields:

- `id`: stable room identifier
- `width_m`: room width in metres
- `depth_m`: room depth in metres
- `cell_size_m`: spatial-cell edge length in metres

Plant fields:

- `id`: stable plant identifier
- `room_id`: referenced room identifier
- `cultivar_id`: genetics reference identifier, not a biological model
- `x_m`, `y_m`, `z_m`: plant position in metres in the room coordinate system

The bootstrap rejects unlabeled alternatives such as `fixed_timestep_hours`, non-positive dimensions, duplicate identifiers, unknown room references, and out-of-bounds plant locations. Biological environmental values are not accepted in this schema yet because no biological model has been approved.

The machine-readable schema is [`scenario.schema.json`](</home/jd/CannaVille-V2/Docs/scenario.schema.json>). The executable smoke scenario is [`smoke_test.json`](</home/jd/CannaVille-V2/Scenarios/smoke_test.json>).
